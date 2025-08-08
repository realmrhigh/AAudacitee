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

public class CompressorView extends View {

    private static final int GAIN_REDUCTION_PARAM_INDEX = 6;
    private Plugin plugin;
    private Paint grPaint;
    private float gainReductionDb = 0.0f;
    private Handler handler = new Handler();
    private Runnable updater;

    public CompressorView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        grPaint = new Paint();
        grPaint.setColor(Color.RED);
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
                handler.postDelayed(this, 50); // Faster update for meter
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

        if (plugin != null) {
            gainReductionDb = plugin.getParameter(GAIN_REDUCTION_PARAM_INDEX);
        }

        // Draw gain reduction meter
        float width = getWidth() * (Math.abs(gainReductionDb) / 60.0f); // Assume max 60dB reduction
        canvas.drawRect(0, 0, width, getHeight(), grPaint);
    }
}
