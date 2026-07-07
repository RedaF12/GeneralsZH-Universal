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
// A standalone launcher icon ("GeneralsZH Setup"), separate from the game
// itself, so configuring the game folder or reading a crash log never
// depends on the game having launched successfully first — and never
// requires adb. This is the practical answer to "there's no launcher": a
// full mod-manager-style launcher (à la GenLauncher) is future scope, but
// picking where the game lives and seeing why it crashed are needed on
// every single install, so they live here now.

package com.generalsx.zerohour;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class SetupActivity extends Activity {

    static final String PREFS_NAME = "generalszh_setup";
    static final String PREF_GAME_PATH = "game_path";

    // Marker files SDL3Main.cpp / GeneralsZHActivity check for on launch —
    // must match GameEngine/CMake's GeneralsMD/Code/Main/SDL3Main.cpp exactly.
    private static final String[] REQUIRED_GAME_FILES = { "INIZH.big", "INI.big" };

    private TextView statusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTitle("GeneralsZH Setup");
        buildUi();
    }

    @Override
    protected void onResume() {
        super.onResume();
        refreshStatus();
    }

    private void buildUi() {
        ScrollView scroll = new ScrollView(this);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(20);
        root.setPadding(pad, pad, pad, pad);
        scroll.addView(root);
        setContentView(scroll);

        TextView title = new TextView(this);
        title.setText("Command & Conquer Generals: Zero Hour — Android Setup");
        title.setTextSize(20);
        title.setPadding(0, 0, 0, dp(16));
        root.addView(title);

        statusText = new TextView(this);
        statusText.setTextIsSelectable(true);
        statusText.setPadding(0, 0, 0, dp(24));
        root.addView(statusText);

        addButton(root, "Select Game Folder", this::onSelectGameFolder);
        addButton(root, "View Logs", this::onViewLogs);
        addButton(root, "Launch Game", this::onLaunchGame);
        addButton(root, "Clear Game Folder Setting", this::onClearGameFolder);

        TextView help = new TextView(this);
        help.setPadding(0, dp(24), 0, 0);
        help.setText(
            "How this works:\n\n"
            + "1. \"Select Game Folder\" opens a folder browser on your phone's "
            + "storage. Navigate to wherever you copied your own Command & Conquer "
            + "Generals Zero Hour install (the folder containing INIZH.big, Data\\, "
            + "ZH_Generals\\, etc.) and tap \"Use This Folder\".\n\n"
            + "2. If browsing looks empty, Android is asking for the \"All files "
            + "access\" permission first — grant it in the screen that opens (this "
            + "is a normal Android permission, no root involved).\n\n"
            + "3. \"View Logs\" shows the engine log and, if the game crashed, a "
            + "native crash log — no computer or adb needed. Use the Share button "
            + "there to send the log to yourself.\n\n"
            + "4. \"Launch Game\" starts the game with the folder you picked."
        );
        root.addView(help);
    }

    private void addButton(LinearLayout root, String label, Runnable action) {
        Button b = new Button(this);
        b.setText(label);
        b.setOnClickListener(v -> action.run());
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.setMargins(0, 0, 0, dp(12));
        root.addView(b, lp);
    }

    private void refreshStatus() {
        String path = getSavedGamePath();
        StringBuilder sb = new StringBuilder();
        if (path == null) {
            sb.append("Game folder: not set.\n");
        } else {
            boolean valid = isValidGameFolder(new File(path));
            sb.append("Game folder: ").append(path).append('\n');
            sb.append(valid ? "Status: looks valid (found game archive files).\n"
                             : "Status: this folder does NOT contain the expected "
                               + "INIZH.big / INI.big — double check you picked the right one.\n");
        }
        sb.append('\n');
        sb.append("Crash/engine logs live in this app's private storage and are only "
            + "reachable through \"View Logs\" above — Android hides them from normal "
            + "file managers by design (this is standard Android sandboxing, not "
            + "specific to this app).");
        statusText.setText(sb.toString());
    }

    private String getSavedGamePath() {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        return prefs.getString(PREF_GAME_PATH, null);
    }

    static boolean isValidGameFolder(File dir) {
        if (dir == null || !dir.isDirectory()) {
            return false;
        }
        for (String name : REQUIRED_GAME_FILES) {
            if (new File(dir, name).exists()) {
                return true;
            }
        }
        return false;
    }

    private void onSelectGameFolder() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && !Environment.isExternalStorageManager()) {
            Toast.makeText(this,
                "Grant \"Allow access to manage all files\" on the next screen, then come back and tap Select Game Folder again.",
                Toast.LENGTH_LONG).show();
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                intent.setData(Uri.parse("package:" + getPackageName()));
                startActivity(intent);
            } catch (Exception e) {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            }
            return;
        }
        startActivityForResult(new Intent(this, FolderPickerActivity.class), 1001);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 1001 && resultCode == Activity.RESULT_OK && data != null) {
            String path = data.getStringExtra(FolderPickerActivity.EXTRA_SELECTED_PATH);
            if (path != null) {
                saveGamePath(path);
                refreshStatus();
                boolean valid = isValidGameFolder(new File(path));
                Toast.makeText(this,
                    valid ? "Game folder saved." : "Saved, but no INIZH.big/INI.big found there — check the folder.",
                    Toast.LENGTH_LONG).show();
            }
        }
    }

    private void saveGamePath(String path) {
        getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit()
            .putString(PREF_GAME_PATH, path)
            .apply();
        // GeneralsZHActivity/SDL3Main.cpp read this plain-text marker on the
        // NEXT launch (native code has no Android SharedPreferences access).
        File marker = new File(getFilesDir(), "gamedata_path.txt");
        try (java.io.FileWriter w = new java.io.FileWriter(marker, false)) {
            w.write(path);
            w.write("\n");
        } catch (java.io.IOException e) {
            Toast.makeText(this, "Could not save folder marker: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }
        copyDxvkConfIfMissing(path);
    }

    // GeneralsX @bugfix Android port 07/07/2026 DXVK reads dxvk.conf from the
    // engine's CWD, which is now whatever folder the user picked here — not
    // the external-files-dir path package-android-zh.sh originally extracted
    // the APK's bundled dxvk.conf into. Without this, a custom game folder
    // silently loses the packaged tuning (16x anisotropic filtering) even
    // though the env-var log level override still applies regardless of CWD.
    private void copyDxvkConfIfMissing(String gameFolderPath) {
        File dest = new File(gameFolderPath, "dxvk.conf");
        if (dest.exists()) {
            return;
        }
        File bundled = new File(getExternalFilesDir(null), "dxvk.conf");
        if (!bundled.exists()) {
            return;
        }
        try (java.io.InputStream in = new java.io.FileInputStream(bundled);
             java.io.OutputStream out = new java.io.FileOutputStream(dest)) {
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
        } catch (java.io.IOException e) {
            // Not fatal: DXVK falls back to its own defaults without a config file.
        }
    }

    private void onClearGameFolder() {
        getSharedPreferences(PREFS_NAME, MODE_PRIVATE).edit().remove(PREF_GAME_PATH).apply();
        new File(getFilesDir(), "gamedata_path.txt").delete();
        refreshStatus();
        Toast.makeText(this, "Cleared. The game will fall back to its default folder convention.", Toast.LENGTH_SHORT).show();
    }

    private void onViewLogs() {
        startActivity(new Intent(this, LogViewerActivity.class));
    }

    private void onLaunchGame() {
        startActivity(new Intent(this, GeneralsZHActivity.class));
    }

    private int dp(int value) {
        float density = getResources().getDisplayMetrics().density;
        return (int) (value * density + 0.5f);
    }
}
