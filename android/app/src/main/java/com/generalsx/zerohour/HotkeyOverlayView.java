/*
** Command & Conquer Generals Zero Hour(tm)
** Copyright 2025 Electronic Arts Inc.
** Licensed under GPL-3.0-or-later. See LICENSE.md.
*/

package com.generalsx.zerohour;

import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.os.Handler;
import android.os.Looper;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;

import org.libsdl.app.SDLActivity;

import java.util.ArrayList;

/** Transparent, touch-through hotkey layer drawn above SDL's game surface. */
final class HotkeyOverlayView extends View {
    private static final long KEY_HOLD_MS = 42L;

    private final ArrayList<TouchControlConfig.ButtonSpec> buttons = new ArrayList<>();
    private final ArrayList<RectF> hitRects = new ArrayList<>();
    private final Paint fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint borderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final float scale;
    private final float opacity;

    private int activeButton = -1;
    private int activePointer = -1;

    private HotkeyOverlayView(GeneralsZHActivity activity, TouchControlConfig config) {
        super(activity);
        buttons.addAll(TouchControlConfig.copyButtons(config.buttons));
        scale = config.buttonScale;
        opacity = config.buttonOpacity;
        setClickable(false);
        setFocusable(false);
        setWillNotDraw(false);

        borderPaint.setStyle(Paint.Style.STROKE);
        borderPaint.setStrokeWidth(dp(1.5f));
        borderPaint.setColor(Color.rgb(220, 181, 86));
        textPaint.setColor(Color.WHITE);
        textPaint.setTextAlign(Paint.Align.CENTER);
        textPaint.setFakeBoldText(true);
        textPaint.setShadowLayer(dp(2), 0, dp(1), Color.BLACK);
    }

    static void attach(GeneralsZHActivity activity) {
        TouchControlConfig config = TouchControlConfig.load(activity);
        if (!config.enabled || config.buttons.isEmpty()) return;
        HotkeyOverlayView overlay = new HotkeyOverlayView(activity, config);
        activity.addContentView(overlay, new ViewGroup.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        overlay.bringToFront();
        overlay.setElevation(1000f);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        rebuildHitRects();
        int baseAlpha = Math.round(255f * opacity);
        for (int i = 0; i < buttons.size(); ++i) {
            RectF rect = hitRects.get(i);
            boolean pressed = i == activeButton;
            fillPaint.setColor(pressed ? Color.rgb(71, 91, 106) : Color.rgb(22, 33, 44));
            fillPaint.setAlpha(pressed ? Math.min(255, baseAlpha + 45) : baseAlpha);
            borderPaint.setAlpha(pressed ? 255 : Math.max(120, baseAlpha));
            float radius = dp(9) * scale;
            canvas.drawRoundRect(rect, radius, radius, fillPaint);
            canvas.drawRoundRect(rect, radius, radius, borderPaint);

            TouchControlConfig.ButtonSpec spec = buttons.get(i);
            float textSize = dp(spec.label.length() > 5 ? 12 : 15) * scale;
            textPaint.setTextSize(textSize);
            Paint.FontMetrics metrics = textPaint.getFontMetrics();
            float baseline = rect.centerY() - (metrics.ascent + metrics.descent) * 0.5f;
            canvas.drawText(spec.label, rect.centerX(), baseline, textPaint);
        }
    }

    private void rebuildHitRects() {
        hitRects.clear();
        float normalSize = dp(52) * scale;
        for (TouchControlConfig.ButtonSpec spec : buttons) {
            float width = normalSize;
            if (spec.label.length() > 4) width *= 1.28f;
            float cx = spec.x * getWidth();
            float cy = spec.y * getHeight();
            float halfW = width * 0.5f;
            float halfH = normalSize * 0.5f;
            hitRects.add(new RectF(cx - halfW, cy - halfH, cx + halfW, cy + halfH));
        }
    }

    private int findButton(float x, float y) {
        if (hitRects.size() != buttons.size()) rebuildHitRects();
        for (int i = hitRects.size() - 1; i >= 0; --i) {
            if (hitRects.get(i).contains(x, y)) return i;
        }
        return -1;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                int hit = findButton(event.getX(), event.getY());
                if (hit < 0) return false; // Let SDL receive normal battlefield gestures.
                activeButton = hit;
                activePointer = event.getPointerId(0);
                invalidate();
                return true;
            }
            case MotionEvent.ACTION_MOVE: {
                if (activeButton < 0) return false;
                int index = event.findPointerIndex(activePointer);
                if (index >= 0 && !hitRects.get(activeButton).contains(event.getX(index), event.getY(index))) {
                    activeButton = -1;
                    invalidate();
                }
                return true;
            }
            case MotionEvent.ACTION_UP: {
                if (activeButton >= 0) {
                    int hit = activeButton;
                    activeButton = -1;
                    activePointer = -1;
                    invalidate();
                    performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                    sendKey(buttons.get(hit));
                    performClick();
                }
                return true;
            }
            case MotionEvent.ACTION_CANCEL:
                activeButton = -1;
                activePointer = -1;
                invalidate();
                return true;
            default:
                return activeButton >= 0;
        }
    }

    private void sendKey(TouchControlConfig.ButtonSpec spec) {
        final int[] modifiers = modifierKeyCodes(spec.modifiers);
        for (int modifier : modifiers) SDLActivity.onNativeKeyDown(modifier);
        SDLActivity.onNativeKeyDown(spec.keyCode);
        handler.postDelayed(() -> {
            SDLActivity.onNativeKeyUp(spec.keyCode);
            for (int i = modifiers.length - 1; i >= 0; --i) SDLActivity.onNativeKeyUp(modifiers[i]);
        }, KEY_HOLD_MS);
    }

    private int[] modifierKeyCodes(int mask) {
        int count = Integer.bitCount(mask & (TouchControlConfig.MOD_CTRL
            | TouchControlConfig.MOD_SHIFT | TouchControlConfig.MOD_ALT));
        int[] result = new int[count];
        int i = 0;
        if ((mask & TouchControlConfig.MOD_CTRL) != 0) result[i++] = KeyEvent.KEYCODE_CTRL_LEFT;
        if ((mask & TouchControlConfig.MOD_SHIFT) != 0) result[i++] = KeyEvent.KEYCODE_SHIFT_LEFT;
        if ((mask & TouchControlConfig.MOD_ALT) != 0) result[i] = KeyEvent.KEYCODE_ALT_LEFT;
        return result;
    }

    @Override
    public boolean performClick() {
        super.performClick();
        return true;
    }

    private float dp(float value) {
        return value * getResources().getDisplayMetrics().density;
    }
}
