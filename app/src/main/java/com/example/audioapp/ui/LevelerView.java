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

public class LevelerView extends View {

    private static final int GAIN_PARAM_INDEX = 2;
    private Plugin plugin;
    private Paint gainPaint;
    private float gainDb = 0.0f;
    private Handler handler = new Handler();
    private Runnable updater;

    public LevelerView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        gainPaint = new Paint();
        gainPaint.setColor(Color.BLUE);
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
                handler.postDelayed(this, 50);
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
            gainDb = plugin.getParameter(GAIN_PARAM_INDEX);
        }

        // Draw gain meter
        float height = getHeight() * (gainDb + 20) / 40.0f; // Assume gain range is -20dB to +20dB
        canvas.drawRect(0, getHeight() - height, getWidth(), getHeight(), gainPaint);
    }
}
