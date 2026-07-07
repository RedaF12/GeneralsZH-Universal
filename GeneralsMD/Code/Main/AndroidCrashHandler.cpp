/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// GeneralsX @build Android port 07/07/2026
//
// A crash during dlopen()'s ELF constructors or early in SDL_main runs
// before SDL3Main.cpp's own stderr-redirect logic ever executes — on
// stock Android that class of crash is invisible without adb (only a
// system tombstone under /data/tombstones, unreadable without root, and
// a logcat FATAL SIGNAL line that scrolls away in seconds). Users
// testing this port sideloaded, without a PC handy, had no way to tell
// us what broke.
//
// Fix: install a signal handler as an ELF constructor — it runs the
// instant the dynamic linker loads this library, before any engine code,
// before JNI_OnLoad, before main(). On a fatal signal it appends a
// one-line summary to a fixed path using only raw POSIX syscalls (no
// libc buffered I/O, no malloc) so it stays usable even with a corrupted
// heap, then chains to whatever handler was previously installed so the
// OS's own tombstone/logcat capture still happens too.
//
// The log path is the app's internal storage (getFilesDir() on the Java
// side) — always private + writable to this app's UID on every Android
// version, no permission needed, unlike external/shared storage. Android's
// SDL3/LogViewerActivity reads the same file to show it in-app without adb.

#if defined(__ANDROID__)

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#if defined(__aarch64__)
#include <ucontext.h>
#endif

namespace {

// GeneralsX @bugfix Android port 07/07/2026 A bare signal+fault_addr line
// wasn't enough to diagnose a crash reported as happening "on pressing a
// button in-game": two separate sessions hit the exact same fault_addr
// (0x2000000001, a fixed non-null garbage-looking value — not a plain
// null-deref), which pointed at a deterministic bug but gave no way to
// find WHERE. Resolve the crashing PC to "library+file-offset" by walking
// /proc/self/maps with raw syscalls only (open/read/close are POSIX
// async-signal-safe; no malloc, no strtok/libc line-buffering). The
// resulting offset is directly usable with `addr2line -e libmain.so
// <offset>` against the same CI build's unstripped .so.
int hexDigitValue(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static char s_mapsBuf[65536];

bool findLibraryForAddress(uintptr_t pc, char *outName, size_t outNameLen, uintptr_t *outOffset) {
	int fd = open("/proc/self/maps", O_RDONLY);
	if (fd < 0) {
		return false;
	}
	ssize_t total = 0;
	ssize_t n;
	while (total < (ssize_t)sizeof(s_mapsBuf) - 1 &&
	       (n = read(fd, s_mapsBuf + total, sizeof(s_mapsBuf) - 1 - (size_t)total)) > 0) {
		total += n;
	}
	close(fd);
	if (total <= 0) {
		return false;
	}
	s_mapsBuf[total] = '\0';

	char *line = s_mapsBuf;
	char *bufEnd = s_mapsBuf + total;
	while (line < bufEnd) {
		char *lineEnd = line;
		while (lineEnd < bufEnd && *lineEnd != '\n') {
			lineEnd++;
		}

		uintptr_t start = 0, end = 0, fileOffset = 0;
		char *p = line;
		while (p < lineEnd && *p != '-') { start = (start << 4) | (uintptr_t)hexDigitValue(*p); p++; }
		if (p < lineEnd) p++; // skip '-'
		while (p < lineEnd && *p != ' ') { end = (end << 4) | (uintptr_t)hexDigitValue(*p); p++; }
		while (p < lineEnd && *p == ' ') p++;
		while (p < lineEnd && *p != ' ') p++; // skip perms field
		while (p < lineEnd && *p == ' ') p++;
		while (p < lineEnd && *p != ' ') { fileOffset = (fileOffset << 4) | (uintptr_t)hexDigitValue(*p); p++; }

		if (pc >= start && pc < end) {
			char *pathStart = lineEnd;
			while (pathStart > line && *(pathStart - 1) != ' ') {
				pathStart--;
			}
			size_t pathLen = (size_t)(lineEnd - pathStart);
			if (pathLen > 0 && pathLen < outNameLen) {
				memcpy(outName, pathStart, pathLen);
				outName[pathLen] = '\0';
			} else {
				outName[0] = '\0';
			}
			*outOffset = (pc - start) + fileOffset;
			return true;
		}

		line = lineEnd + 1;
	}
	return false;
}

// GeneralsX @bugfix Android port 07/07/2026 Context.getFilesDir() (what
// SDL_GetAndroidInternalStoragePath() and this path both need to agree with)
// resolves under /data/user/<userId>/<pkg>/files on modern Android; the
// /data/data/<pkg> shortcut is only a symlink to user 0's data and silently
// resolves to a WRONG, nonexistent-for-this-run location on a work profile,
// a secondary user, or guest mode. Android multi-user UIDs are always
// userId*100000 + appId (a stable, documented convention), so derive the
// real user id from getuid() instead of assuming user 0.
char s_crashLogPath[256];

void computeCrashLogPath() {
	int userId = (int)(getuid() / 100000);
	snprintf(s_crashLogPath, sizeof(s_crashLogPath),
		"/data/user/%d/com.generalsx.zerohour/files/crash.log", userId);
}

struct sigaction s_prevHandlers[NSIG];
bool s_haveHandlerFor[NSIG] = {};

// Appends a message using only async-signal-safe primitives (open/write/close
// are POSIX async-signal-safe; snprintf into a stack buffer with no malloc
// path is the one pragmatic exception here — bionic's implementation does
// not lock or allocate for a plain format string like this one, and every
// widely-shipped Android crash handler makes the same trade-off).
void appendCrashLog(const char *message, size_t length) {
	if (s_crashLogPath[0] == '\0') {
		return;
	}
	int fd = open(s_crashLogPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd < 0) {
		return;
	}
	ssize_t written = 0;
	while (written < (ssize_t)length) {
		ssize_t n = write(fd, message + written, length - (size_t)written);
		if (n <= 0) {
			break;
		}
		written += n;
	}
	close(fd);
}

void androidCrashHandler(int sig, siginfo_t *info, void *ucontext) {
	char buf[512];
	const void *faultAddr = (info != nullptr) ? info->si_addr : nullptr;
	int len = snprintf(buf, sizeof(buf),
		"\n=== NATIVE CRASH === signal=%d (%s) fault_addr=%p pid=%d tid=%d ===\n"
		"If you're reading this: open the GeneralsZH Setup app -> View Logs,\n"
		"and share this file's contents.\n",
		sig, strsignal(sig), faultAddr, (int)getpid(), (int)gettid());
	if (len > 0) {
		appendCrashLog(buf, (size_t)len < sizeof(buf) ? (size_t)len : sizeof(buf) - 1);
	}

#if defined(__aarch64__)
	uintptr_t pc = (ucontext != nullptr) ? (uintptr_t)((ucontext_t *)ucontext)->uc_mcontext.pc : 0;
	if (pc != 0) {
		char libName[192];
		uintptr_t libOffset = 0;
		if (findLibraryForAddress(pc, libName, sizeof(libName), &libOffset)) {
			int pcLen = snprintf(buf, sizeof(buf),
				"crash PC=%p is in %s+0x%lx (symbolize with: addr2line -f -C -e <that .so> 0x%lx)\n",
				(void *)pc, libName, (unsigned long)libOffset, (unsigned long)libOffset);
			if (pcLen > 0) {
				appendCrashLog(buf, (size_t)pcLen < sizeof(buf) ? (size_t)pcLen : sizeof(buf) - 1);
			}
		} else {
			int pcLen = snprintf(buf, sizeof(buf), "crash PC=%p (no matching /proc/self/maps entry)\n", (void *)pc);
			if (pcLen > 0) {
				appendCrashLog(buf, (size_t)pcLen < sizeof(buf) ? (size_t)pcLen : sizeof(buf) - 1);
			}
		}
	}
#endif

	// Chain to whatever handler was previously installed (Android's own
	// debuggerd hook in the common case) so the system tombstone and the
	// logcat "FATAL SIGNAL" block still get generated too — this handler
	// only ADDS a no-adb-needed diagnostic, it doesn't replace the OS one.
	if (sig >= 0 && sig < NSIG && s_haveHandlerFor[sig]) {
		struct sigaction &prev = s_prevHandlers[sig];
		if ((prev.sa_flags & SA_SIGINFO) != 0 && prev.sa_sigaction != nullptr) {
			prev.sa_sigaction(sig, info, ucontext);
			return;
		}
		if (prev.sa_handler != SIG_DFL && prev.sa_handler != SIG_IGN && prev.sa_handler != nullptr) {
			prev.sa_handler(sig);
			return;
		}
	}
	signal(sig, SIG_DFL);
	raise(sig);
}

__attribute__((constructor))
void installAndroidCrashHandler() {
	computeCrashLogPath();

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = androidCrashHandler;
	sa.sa_flags = SA_SIGINFO | SA_RESTART;
	sigemptyset(&sa.sa_mask);

	const int signals[] = { SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE };
	for (int s : signals) {
		if (sigaction(s, &sa, &s_prevHandlers[s]) == 0) {
			s_haveHandlerFor[s] = true;
		}
	}

	// A signpost written unconditionally at load time: if crash.log exists
	// but ends right after this line, the crash happened somewhere between
	// here and the next log line the engine itself writes (SDL3Main.cpp) —
	// i.e. during the rest of dlopen's constructors or very early SDL_main,
	// still before the regular stderr log redirect is set up.
	time_t now = time(nullptr);
	char stamp[128];
	int len = snprintf(stamp, sizeof(stamp), "\n=== libmain.so loaded, crash handler installed (t=%ld) ===\n", (long)now);
	if (len > 0) {
		appendCrashLog(stamp, (size_t)len < sizeof(stamp) ? (size_t)len : sizeof(stamp) - 1);
	}
}

} // namespace

#endif // __ANDROID__
