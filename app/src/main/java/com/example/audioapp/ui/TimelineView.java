package com.example.audioapp.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;
import com.example.audioapp.audio.AudioBuffer;

public class TimelineView extends View {

    private AudioBuffer audioBuffer;
    private Paint paint;
    private int playbackPosition;

    public TimelineView(Context context, AttributeSet attrs) {
        super(context, attrs);
        paint = new Paint();
        paint.setColor(Color.BLUE);
        paint.setStrokeWidth(2);
    }

    public void setAudioBuffer(AudioBuffer audioBuffer) {
        this.audioBuffer = audioBuffer;
        requestLayout();
        invalidate();
    }

    public void setPlaybackPosition(int playbackPosition) {
        this.playbackPosition = playbackPosition;
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        if (audioBuffer != null) {
            int width = audioBuffer.getFrameCount();
            int height = MeasureSpec.getSize(heightMeasureSpec);
            setMeasuredDimension(width, height);
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (audioBuffer != null) {
            float[] data = audioBuffer.getData();
            int channelCount = audioBuffer.getChannelCount();
            int frameCount = audioBuffer.getFrameCount();
            float centerY = getHeight() / 2;
            float scaleY = getHeight() / 2;

            for (int i = 0; i < frameCount - 1; i++) {
                float startX = i;
                float startY = centerY - data[i * channelCount] * scaleY;
                float stopX = i + 1;
                float stopY = centerY - data[(i + 1) * channelCount] * scaleY;
                canvas.drawLine(startX, startY, stopX, stopY, paint);
            }

            paint.setColor(Color.RED);
            canvas.drawLine(playbackPosition, 0, playbackPosition, getHeight(), paint);
            paint.setColor(Color.BLUE);
        }
    }
}
