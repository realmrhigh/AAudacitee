package com.example.audioapp.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;
import com.example.audioapp.audio.AudioBuffer;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TimelineView extends View {

    private AudioBuffer audioBuffer;
    private Paint paint;
    private int playbackPosition;

    // Downsampled waveform data (per-pixel min/max)
    private float[] peaksMin;
    private float[] peaksMax;
    private boolean peaksReady = false;
    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    public TimelineView(Context context, AttributeSet attrs) {
        super(context, attrs);
        paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        paint.setColor(Color.BLUE);
        paint.setStrokeWidth(2f);
    }

    public void setAudioBuffer(AudioBuffer audioBuffer) {
        this.audioBuffer = audioBuffer;
        peaksReady = false;
        peaksMin = null;
        peaksMax = null;
        requestLayout();
        // If we already know our size, prepare peaks now; otherwise wait for onSizeChanged
        if (getWidth() > 0 && getHeight() > 0 && audioBuffer != null) {
            preparePeaksAsync(getWidth());
        } else {
            invalidate();
        }
    }

    public void setPlaybackPosition(int playbackPosition) {
        if (playbackPosition >= 0) {
            this.playbackPosition = playbackPosition;
            invalidate();
        }
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        if (audioBuffer != null && w > 0 && h > 0) {
            preparePeaksAsync(w);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        // Keep the provided width; do not scale to frame count (prevents huge layouts)
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        setMeasuredDimension(width, height);
    }

    private void preparePeaksAsync(int targetWidth) {
        peaksReady = false;
        final AudioBuffer buf = audioBuffer;
        if (buf == null) return;
        final int width = Math.max(1, targetWidth);
        executor.submit(() -> {
            try {
                float[] data = buf.getData();
                if (data == null || data.length == 0) return;
                int channels = Math.max(1, buf.getChannelCount());
                int frames = Math.max(1, buf.getFrameCount());

                float[] minArr = new float[width];
                float[] maxArr = new float[width];

                // Samples per pixel bucket
                int spp = Math.max(1, frames / width);
                for (int x = 0; x < width; x++) {
                    int start = x * spp;
                    int end = (x == width - 1) ? frames : Math.min(frames, start + spp);
                    float minV = 1f, maxV = -1f;
                    for (int i = start; i < end; i++) {
                        int base = i * channels;
                        // Use first channel if stereo; can be extended to max across channels
                        float sample = data[base];
                        if (channels >= 2) {
                            // average L/R to visualize
                            sample = (data[base] + data[base + 1]) * 0.5f;
                        }
                        if (sample < minV) minV = sample;
                        if (sample > maxV) maxV = sample;
                    }
                    // Handle empty buckets gracefully
                    if (start >= end) { minV = 0f; maxV = 0f; }
                    minArr[x] = minV;
                    maxArr[x] = maxV;
                }
                peaksMin = minArr;
                peaksMax = maxArr;
                peaksReady = true;
                postInvalidateOnAnimation();
            } catch (Throwable t) {
                // Ignore visualization errors; do not crash UI
            }
        });
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        int w = getWidth();
        int h = getHeight();
        float centerY = h / 2f;
        float scaleY = h / 2f;

        // Draw baseline
        paint.setColor(Color.GRAY);
        canvas.drawLine(0, centerY, w, centerY, paint);

        // Draw waveform
        paint.setColor(Color.BLUE);
        if (peaksReady && peaksMin != null && peaksMax != null) {
            int count = Math.min(w, Math.min(peaksMin.length, peaksMax.length));
            for (int x = 0; x < count; x++) {
                float yMin = centerY - clamp(peaksMin[x]) * scaleY;
                float yMax = centerY - clamp(peaksMax[x]) * scaleY;
                if (yMin > yMax) { float tmp = yMin; yMin = yMax; yMax = tmp; }
                canvas.drawLine(x, yMin, x, yMax, paint);
            }
        }

        // Draw playback cursor
        if (audioBuffer != null) {
            int frames = Math.max(1, audioBuffer.getFrameCount());
            float x = (frames > 0) ? (playbackPosition / (float) frames) * w : 0f;
            paint.setColor(Color.RED);
            canvas.drawLine(x, 0, x, h, paint);
        }
    }

    private static float clamp(float v) {
        if (v < -1f) return -1f;
        if (v > 1f) return 1f;
        return v;
    }
}
