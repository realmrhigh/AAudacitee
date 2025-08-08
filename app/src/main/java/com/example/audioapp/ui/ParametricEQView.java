package com.example.audioapp.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.os.Handler;
import android.util.AttributeSet;
import android.view.View;

import androidx.annotation.Nullable;

import com.example.audioapp.avst.Plugin;

public class ParametricEQView extends View {

    private Plugin plugin;
    private Paint gridPaint;
    private Paint curvePaint;
    private float[] magnitudes;
    private Handler handler = new Handler();
    private Runnable updater;

    public ParametricEQView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        gridPaint = new Paint();
        gridPaint.setColor(Color.DKGRAY);
        gridPaint.setStrokeWidth(1);

        curvePaint = new Paint();
        curvePaint.setColor(Color.GREEN);
        curvePaint.setStrokeWidth(3);
        curvePaint.setStyle(Paint.Style.STROKE);
    }

    public void setPlugin(Plugin plugin) {
        this.plugin = plugin;
        if (updater != null) {
            handler.removeCallbacks(updater);
        }
        updater = new Runnable() {
            @Override
            public void run() {
                invalidate();
                handler.postDelayed(this, 100);
            }
        };
        handler.post(updater);
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        if (updater != null) {
            handler.post(updater);
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        if (updater != null) {
            handler.removeCallbacks(updater);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        drawGrid(canvas);
        drawCurve(canvas);
    }

    private void drawGrid(Canvas canvas) {
        // Draw vertical grid lines (frequency)
        for (int i = 0; i < 10; i++) {
            float x = getWidth() * i / 10.0f;
            canvas.drawLine(x, 0, x, getHeight(), gridPaint);
        }

        // Draw horizontal grid lines (gain)
        for (int i = 0; i < 6; i++) {
            float y = getHeight() * i / 6.0f;
            canvas.drawLine(0, y, getWidth(), y, gridPaint);
        }
    }

    private void drawCurve(Canvas canvas) {
        if (plugin == null) {
            return;
        }

        magnitudes = plugin.getFrequencyResponse();

        if (magnitudes == null) {
            return;
        }

        for (int i = 0; i < magnitudes.length - 1; i++) {
            float x1 = getWidth() * i / (float) (magnitudes.length - 1);
            float y1 = getHeight() / 2.0f - magnitudes[i] * 50; // Scale for visibility
            float x2 = getWidth() * (i + 1) / (float) (magnitudes.length - 1);
            float y2 = getHeight() / 2.0f - magnitudes[i+1] * 50;
            canvas.drawLine(x1, y1, x2, y2, curvePaint);
        }
    }
}
