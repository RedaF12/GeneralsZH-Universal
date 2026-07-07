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
// Shows the two log sources without adb:
//  - crash.log (getFilesDir(), internal storage) — written by
//    AndroidCrashHandler.cpp's signal handler, which installs before any
//    engine code runs, so it captures crashes the regular engine log never
//    gets a chance to record.
//  - generals-stderr.log / -prev.log (getExternalFilesDir(), external
//    storage) — the regular engine log, written once main() starts
//    (SDL3Main.cpp).
// A Share button hands the combined text to any app the user has (Files,
// email, a messaging app) via the standard Android share sheet — the
// no-computer path to getting a log out of the phone.

package com.generalsx.zerohour;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.nio.CharBuffer;

public class LogViewerActivity extends Activity {

    // Cap how much we read/display: crash-looping sessions can produce
    // megabytes, and this is a plain TextView, not a virtualized log viewer.
    private static final int MAX_CHARS_PER_FILE = 200_000;

    private String combinedLog = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("Logs");

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);

        LinearLayout buttonRow = new LinearLayout(this);
        buttonRow.setOrientation(LinearLayout.HORIZONTAL);
        buttonRow.setPadding(dp(8), dp(8), dp(8), dp(8));

        Button refreshButton = new Button(this);
        refreshButton.setText("Refresh");
        refreshButton.setOnClickListener(v -> loadLogs());
        buttonRow.addView(refreshButton, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        Button copyButton = new Button(this);
        copyButton.setText("Copy");
        copyButton.setOnClickListener(v -> {
            ClipboardManager cm = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
            cm.setPrimaryClip(ClipData.newPlainText("GeneralsZH log", combinedLog));
            Toast.makeText(this, "Copied to clipboard.", Toast.LENGTH_SHORT).show();
        });
        buttonRow.addView(copyButton, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        Button shareButton = new Button(this);
        shareButton.setText("Share");
        shareButton.setOnClickListener(v -> {
            Intent share = new Intent(Intent.ACTION_SEND);
            share.setType("text/plain");
            share.putExtra(Intent.EXTRA_SUBJECT, "GeneralsZH Android log");
            share.putExtra(Intent.EXTRA_TEXT, combinedLog);
            startActivity(Intent.createChooser(share, "Share log"));
        });
        buttonRow.addView(shareButton, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        root.addView(buttonRow);

        ScrollView scroll = new ScrollView(this);
        TextView logText = new TextView(this);
        logText.setId(android.R.id.text1);
        logText.setTextIsSelectable(true);
        logText.setPadding(dp(12), dp(12), dp(12), dp(12));
        logText.setTypeface(android.graphics.Typeface.MONOSPACE);
        logText.setTextSize(11);
        scroll.addView(logText);
        root.addView(scroll, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        setContentView(root);
        loadLogs();
    }

    private TextView logText() {
        return findViewById(android.R.id.text1);
    }

    private void loadLogs() {
        StringBuilder sb = new StringBuilder();

        File crashLog = new File(getFilesDir(), "crash.log");
        sb.append("=== crash.log (native crash handler, internal storage) ===\n");
        sb.append(crashLog.exists() ? readTail(crashLog) : "(not present — no native crash recorded since install, or the app has never launched past library load)");
        sb.append("\n\n");

        File extDir = getExternalFilesDir(null);
        File stderrLog = extDir != null ? new File(extDir, "generals-stderr.log") : null;
        sb.append("=== generals-stderr.log (engine log, external storage) ===\n");
        sb.append(stderrLog != null && stderrLog.exists() ? readTail(stderrLog) : "(not present — the engine's main() has not started yet, or external storage is unavailable)");
        sb.append("\n\n");

        File prevLog = extDir != null ? new File(extDir, "generals-stderr-prev.log") : null;
        sb.append("=== generals-stderr-prev.log (previous session) ===\n");
        sb.append(prevLog != null && prevLog.exists() ? readTail(prevLog) : "(not present)");

        combinedLog = sb.toString();
        logText().setText(combinedLog);
    }

    private String readTail(File file) {
        try (Reader r = new FileReader(file)) {
            long size = file.length();
            if (size > MAX_CHARS_PER_FILE) {
                r.skip(size - MAX_CHARS_PER_FILE);
            }
            CharBuffer buf = CharBuffer.allocate(MAX_CHARS_PER_FILE);
            r.read(buf);
            buf.flip();
            return (size > MAX_CHARS_PER_FILE ? "...(truncated, showing the tail)...\n" : "") + buf.toString();
        } catch (IOException e) {
            return "(could not read " + file.getAbsolutePath() + ": " + e.getMessage() + ")";
        }
    }

    private int dp(int value) {
        float density = getResources().getDisplayMetrics().density;
        return (int) (value * density + 0.5f);
    }
}
