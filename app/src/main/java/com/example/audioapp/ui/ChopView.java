package com.example.audioapp.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.net.Uri;
import android.provider.MediaStore;
import android.util.AttributeSet;
import android.view.View;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class ChopView extends View {

    private Bitmap bitmap;
    private final Paint paint = new Paint();
    private final List<Float> verticalLines = new ArrayList<>();
    private final List<Float> horizontalLines = new ArrayList<>();

    public ChopView(Context context) {
        super(context);
        init();
    }

    public ChopView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public ChopView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        paint.setColor(Color.RED);
        paint.setStrokeWidth(5);
    }

    public void setImageUri(Uri imageUri) {
        try {
            bitmap = MediaStore.Images.Media.getBitmap(getContext().getContentResolver(), imageUri);
            invalidate();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (bitmap != null) {
            canvas.drawBitmap(bitmap, null, new Rect(0, 0, getWidth(), getHeight()), null);
        }
        for (float x : verticalLines) {
            canvas.drawLine(x, 0, x, getHeight(), paint);
        }
        for (float y : horizontalLines) {
            canvas.drawLine(0, y, getWidth(), y, paint);
        }
    }

    @Override
    public boolean onTouchEvent(android.view.MotionEvent event) {
        if (event.getAction() == android.view.MotionEvent.ACTION_DOWN) {
            if (isDeleteMode) {
                removeLine(event.getX(), event.getY());
            } else {
                if (event.getX() > getWidth() / 2) {
                    verticalLines.add(event.getX());
                } else {
                    horizontalLines.add(event.getY());
                }
            }
            invalidate();
            return true;
        }
        return super.onTouchEvent(event);
    }

    private boolean isDeleteMode = false;

    public void setDeleteMode(boolean isDeleteMode) {
        this.isDeleteMode = isDeleteMode;
    }

    private void removeLine(float x, float y) {
        float closestVertical = -1;
        float minVerticalDist = Float.MAX_VALUE;
        for (float lineX : verticalLines) {
            float dist = Math.abs(x - lineX);
            if (dist < minVerticalDist) {
                minVerticalDist = dist;
                closestVertical = lineX;
            }
        }

        float closestHorizontal = -1;
        float minHorizontalDist = Float.MAX_VALUE;
        for (float lineY : horizontalLines) {
            float dist = Math.abs(y - lineY);
            if (dist < minHorizontalDist) {
                minHorizontalDist = dist;
                closestHorizontal = lineY;
            }
        }

        if (minVerticalDist < minHorizontalDist && closestVertical != -1 && minVerticalDist < 50) {
            verticalLines.remove(closestVertical);
        } else if (closestHorizontal != -1 && minHorizontalDist < 50) {
            horizontalLines.remove(closestHorizontal);
        }
    }

    public List<Bitmap> getChoppedBitmaps() {
        List<Bitmap> bitmaps = new ArrayList<>();
        if (bitmap == null) {
            return bitmaps;
        }

        List<Float> xSlices = new ArrayList<>();
        xSlices.add(0f);
        xSlices.addAll(verticalLines);
        xSlices.add((float) getWidth());
        java.util.Collections.sort(xSlices);

        List<Float> ySlices = new ArrayList<>();
        ySlices.add(0f);
        ySlices.addAll(horizontalLines);
        ySlices.add((float) getHeight());
        java.util.Collections.sort(ySlices);

        for (int i = 0; i < xSlices.size() - 1; i++) {
            for (int j = 0; j < ySlices.size() - 1; j++) {
                float x = xSlices.get(i);
                float y = ySlices.get(j);
                float width = xSlices.get(i + 1) - x;
                float height = ySlices.get(j + 1) - y;

                int bitmapX = (int) (x / getWidth() * bitmap.getWidth());
                int bitmapY = (int) (y / getHeight() * bitmap.getHeight());
                int bitmapWidth = (int) (width / getWidth() * bitmap.getWidth());
                int bitmapHeight = (int) (height / getHeight() * bitmap.getHeight());

                if (bitmapWidth > 0 && bitmapHeight > 0) {
                    Bitmap croppedBitmap = Bitmap.createBitmap(bitmap, bitmapX, bitmapY, bitmapWidth, bitmapHeight);
                    bitmaps.add(croppedBitmap);
                }
            }
        }

        return bitmaps;
    }
}
