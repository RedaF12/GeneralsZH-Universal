/*
** Command & Conquer Generals Zero Hour(tm)
** Copyright 2025 Electronic Arts Inc.
** Licensed under GPL-3.0-or-later. See LICENSE.md.
*/

package com.generalsx.zerohour;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.pm.ActivityInfo;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.widget.SwitchCompat;

import java.io.File;
import java.util.ArrayList;

/** Landscape editor for the hotkey overlay shown above the running game. */
public class TouchControlsActivity extends Activity {
    private TouchControlConfig config;
    private EditorCanvas editor;
    private SwitchCompat enabledSwitch;
    private TextView selectedText;
    private TextView panValue;
    private TextView sizeValue;
    private TextView opacityValue;
    private SeekBar panSeek;
    private SeekBar sizeSeek;
    private SeekBar opacitySeek;
    private int selectedIndex = -1;

    @Override
    protected void attachBaseContext(android.content.Context newBase) {
        super.attachBaseContext(LocaleHelper.wrap(newBase));
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        setTitle(R.string.touch_editor_title);
        config = TouchControlConfig.load(this);
        buildUi();
    }

    private void buildUi() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(8), dp(6), dp(8), dp(6));
        root.setBackgroundColor(Color.rgb(11, 17, 23));
        setContentView(root);
        InsetUtil.applySafeInsets(root);

        LinearLayout toolbar = new LinearLayout(this);
        toolbar.setGravity(android.view.Gravity.CENTER_VERTICAL);
        enabledSwitch = new SwitchCompat(this);
        enabledSwitch.setText(R.string.touch_overlay_enabled);
        enabledSwitch.setChecked(config.enabled);
        toolbar.addView(enabledSwitch);

        selectedText = new TextView(this);
        selectedText.setTextColor(Color.WHITE);
        selectedText.setPadding(dp(12), 0, dp(8), 0);
        toolbar.addView(selectedText, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        addToolbarButton(toolbar, R.string.touch_add_button, this::addNewButton);
        addToolbarButton(toolbar, R.string.touch_edit_button, this::editSelectedButton);
        addToolbarButton(toolbar, R.string.touch_delete_button, this::deleteSelectedButton);
        addToolbarButton(toolbar, R.string.touch_reset_button, this::resetDefaults);
        addToolbarButton(toolbar, R.string.touch_save_button, this::saveAndFinish);
        root.addView(toolbar);

        editor = new EditorCanvas();
        root.addView(editor, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        LinearLayout sliders = new LinearLayout(this);
        sliders.setOrientation(LinearLayout.HORIZONTAL);
        sliders.setPadding(0, dp(4), 0, 0);
        panValue = addSlider(sliders, R.string.touch_pan_speed, 35, 250,
            Math.round(config.panSensitivity * 100f), value -> config.panSensitivity = value / 100f);
        panSeek = (SeekBar)panValue.getTag();
        sizeValue = addSlider(sliders, R.string.touch_button_size, 65, 175,
            Math.round(config.buttonScale * 100f), value -> {
                config.buttonScale = value / 100f;
                editor.invalidate();
            });
        sizeSeek = (SeekBar)sizeValue.getTag();
        opacityValue = addSlider(sliders, R.string.touch_button_opacity, 25, 100,
            Math.round(config.buttonOpacity * 100f), value -> {
                config.buttonOpacity = value / 100f;
                editor.invalidate();
            });
        opacitySeek = (SeekBar)opacityValue.getTag();
        root.addView(sliders);
        refreshLabels();
    }

    private interface SliderValueListener { void onValue(int value); }

    private TextView addSlider(LinearLayout row, int titleRes, int min, int max,
                               int initial, SliderValueListener listener) {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setPadding(dp(8), 0, dp(8), 0);
        TextView label = new TextView(this);
        label.setTextColor(Color.WHITE);
        box.addView(label);
        SeekBar seek = new SeekBar(this);
        seek.setMin(min);
        seek.setMax(max);
        seek.setProgress(initial);
        seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                listener.onValue(value);
                label.setText(getString(titleRes, value));
            }
            @Override public void onStartTrackingTouch(SeekBar bar) {}
            @Override public void onStopTrackingTouch(SeekBar bar) {}
        });
        box.addView(seek);
        row.addView(box, new LinearLayout.LayoutParams(0,
            LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        label.setTag(seek);
        label.setText(getString(titleRes, initial));
        return label;
    }

    private void addToolbarButton(LinearLayout row, int textRes, Runnable action) {
        Button button = new Button(this);
        button.setText(textRes);
        button.setTextSize(12);
        button.setMinWidth(0);
        button.setMinimumWidth(0);
        button.setPadding(dp(9), 0, dp(9), 0);
        button.setOnClickListener(v -> action.run());
        row.addView(button);
    }

    private void addNewButton() {
        if (config.buttons.size() >= 20) {
            Toast.makeText(this, R.string.touch_too_many_buttons, Toast.LENGTH_SHORT).show();
            return;
        }
        config.buttons.add(new TouchControlConfig.ButtonSpec("E", android.view.KeyEvent.KEYCODE_E,
            0, 0.5f, 0.5f));
        selectedIndex = config.buttons.size() - 1;
        editor.invalidate();
        refreshLabels();
        editSelectedButton();
    }

    private void deleteSelectedButton() {
        if (selectedIndex < 0 || selectedIndex >= config.buttons.size()) return;
        config.buttons.remove(selectedIndex);
        selectedIndex = config.buttons.isEmpty() ? -1 : Math.min(selectedIndex, config.buttons.size() - 1);
        editor.invalidate();
        refreshLabels();
    }

    private void editSelectedButton() {
        if (selectedIndex < 0 || selectedIndex >= config.buttons.size()) {
            Toast.makeText(this, R.string.touch_select_button_first, Toast.LENGTH_SHORT).show();
            return;
        }
        TouchControlConfig.ButtonSpec spec = config.buttons.get(selectedIndex);
        LinearLayout form = new LinearLayout(this);
        form.setOrientation(LinearLayout.VERTICAL);
        int pad = dp(20);
        form.setPadding(pad, dp(8), pad, 0);

        EditText label = new EditText(this);
        label.setHint(R.string.touch_button_label_hint);
        label.setText(spec.label);
        label.setSelectAllOnFocus(true);
        form.addView(label);

        Spinner keySpinner = new Spinner(this);
        String[] names = TouchControlConfig.keyNames();
        int[] codes = TouchControlConfig.keyCodes();
        keySpinner.setAdapter(new ArrayAdapter<>(this,
            android.R.layout.simple_spinner_dropdown_item, names));
        int selectedKey = 0;
        for (int i = 0; i < codes.length; ++i) if (codes[i] == spec.keyCode) selectedKey = i;
        keySpinner.setSelection(selectedKey);
        form.addView(keySpinner);

        LinearLayout modifiers = new LinearLayout(this);
        CheckBox ctrl = new CheckBox(this);
        ctrl.setText("Ctrl");
        ctrl.setChecked((spec.modifiers & TouchControlConfig.MOD_CTRL) != 0);
        modifiers.addView(ctrl);
        CheckBox shift = new CheckBox(this);
        shift.setText("Shift");
        shift.setChecked((spec.modifiers & TouchControlConfig.MOD_SHIFT) != 0);
        modifiers.addView(shift);
        CheckBox alt = new CheckBox(this);
        alt.setText("Alt");
        alt.setChecked((spec.modifiers & TouchControlConfig.MOD_ALT) != 0);
        modifiers.addView(alt);
        form.addView(modifiers);

        new AlertDialog.Builder(this)
            .setTitle(R.string.touch_edit_dialog_title)
            .setView(form)
            .setNegativeButton(R.string.common_cancel, null)
            .setPositiveButton(R.string.touch_apply_button, (dialog, which) -> {
                String newLabel = label.getText().toString().trim();
                if (newLabel.isEmpty()) newLabel = names[keySpinner.getSelectedItemPosition()];
                if (newLabel.length() > 8) newLabel = newLabel.substring(0, 8);
                spec.label = newLabel;
                spec.keyCode = codes[keySpinner.getSelectedItemPosition()];
                spec.modifiers = (ctrl.isChecked() ? TouchControlConfig.MOD_CTRL : 0)
                    | (shift.isChecked() ? TouchControlConfig.MOD_SHIFT : 0)
                    | (alt.isChecked() ? TouchControlConfig.MOD_ALT : 0);
                editor.invalidate();
                refreshLabels();
            })
            .show();
    }

    private void resetDefaults() {
        config = TouchControlConfig.defaults();
        selectedIndex = -1;
        enabledSwitch.setChecked(config.enabled);
        panSeek.setProgress(Math.round(config.panSensitivity * 100f));
        sizeSeek.setProgress(Math.round(config.buttonScale * 100f));
        opacitySeek.setProgress(Math.round(config.buttonOpacity * 100f));
        editor.invalidate();
        refreshLabels();
    }

    private void saveAndFinish() {
        config.enabled = enabledSwitch.isChecked();
        config.save(this);
        String gamePath = SetupActivity.getSavedGamePath(this);
        if (gamePath != null) TouchControlConfig.prepareForLaunch(this, new File(gamePath));
        Toast.makeText(this, R.string.touch_saved, Toast.LENGTH_SHORT).show();
        finish();
    }

    private void refreshLabels() {
        if (selectedIndex >= 0 && selectedIndex < config.buttons.size()) {
            TouchControlConfig.ButtonSpec spec = config.buttons.get(selectedIndex);
            selectedText.setText(getString(R.string.touch_selected_button, spec.label));
        } else {
            selectedText.setText(R.string.touch_drag_help_short);
        }
        panValue.setText(getString(R.string.touch_pan_speed, Math.round(config.panSensitivity * 100f)));
        sizeValue.setText(getString(R.string.touch_button_size, Math.round(config.buttonScale * 100f)));
        opacityValue.setText(getString(R.string.touch_button_opacity, Math.round(config.buttonOpacity * 100f)));
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }

    private final class EditorCanvas extends View {
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint text = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final ArrayList<RectF> rects = new ArrayList<>();
        private int dragPointer = -1;

        EditorCanvas() {
            super(TouchControlsActivity.this);
            text.setTextAlign(Paint.Align.CENTER);
            text.setFakeBoldText(true);
            setBackgroundColor(Color.rgb(18, 27, 36));
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(1f);
            paint.setColor(Color.argb(45, 255, 255, 255));
            for (int i = 1; i < 10; ++i) {
                float x = getWidth() * i / 10f;
                canvas.drawLine(x, 0, x, getHeight(), paint);
            }
            for (int i = 1; i < 6; ++i) {
                float y = getHeight() * i / 6f;
                canvas.drawLine(0, y, getWidth(), y, paint);
            }
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(Color.rgb(9, 13, 18));
            canvas.drawRect(0, getHeight() * 0.82f, getWidth(), getHeight(), paint);

            rects.clear();
            float size = dp(52) * config.buttonScale;
            int alpha = Math.round(255f * config.buttonOpacity);
            for (int i = 0; i < config.buttons.size(); ++i) {
                TouchControlConfig.ButtonSpec spec = config.buttons.get(i);
                float width = spec.label.length() > 4 ? size * 1.28f : size;
                RectF rect = new RectF(spec.x * getWidth() - width / 2f,
                    spec.y * getHeight() - size / 2f,
                    spec.x * getWidth() + width / 2f,
                    spec.y * getHeight() + size / 2f);
                rects.add(rect);
                paint.setStyle(Paint.Style.FILL);
                paint.setColor(i == selectedIndex ? Color.rgb(80, 102, 119) : Color.rgb(22, 33, 44));
                paint.setAlpha(i == selectedIndex ? Math.min(255, alpha + 45) : alpha);
                canvas.drawRoundRect(rect, dp(9), dp(9), paint);
                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(i == selectedIndex ? dp(3) : dp(1));
                paint.setColor(Color.rgb(220, 181, 86));
                paint.setAlpha(255);
                canvas.drawRoundRect(rect, dp(9), dp(9), paint);
                text.setColor(Color.WHITE);
                text.setTextSize(dp(spec.label.length() > 5 ? 12 : 15) * config.buttonScale);
                Paint.FontMetrics metrics = text.getFontMetrics();
                canvas.drawText(spec.label, rect.centerX(),
                    rect.centerY() - (metrics.ascent + metrics.descent) / 2f, text);
            }
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    selectedIndex = hit(event.getX(), event.getY());
                    dragPointer = event.getPointerId(0);
                    refreshLabels();
                    invalidate();
                    return true;
                case MotionEvent.ACTION_MOVE:
                    if (selectedIndex >= 0 && selectedIndex < config.buttons.size()) {
                        int index = event.findPointerIndex(dragPointer);
                        if (index >= 0) {
                            TouchControlConfig.ButtonSpec spec = config.buttons.get(selectedIndex);
                            spec.x = TouchControlConfig.clamp(event.getX(index) / getWidth(), 0.025f, 0.975f);
                            spec.y = TouchControlConfig.clamp(event.getY(index) / getHeight(), 0.06f, 0.94f);
                            invalidate();
                        }
                    }
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    dragPointer = -1;
                    return true;
                default:
                    return true;
            }
        }

        private int hit(float x, float y) {
            for (int i = rects.size() - 1; i >= 0; --i) if (rects.get(i).contains(x, y)) return i;
            return -1;
        }
    }
}
