package com.example.audioapp.engine;

import android.util.Log;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

public class BufferPool {
    private BlockingQueue<float[]> pool;
    private int bufferSize;
    private final int poolSize;

    public BufferPool(int poolSize, int bufferSize) {
        this.poolSize = poolSize;
        this.bufferSize = bufferSize;
        this.pool = new ArrayBlockingQueue<>(poolSize);
        for (int i = 0; i < poolSize; i++) {
            pool.offer(new float[bufferSize]);
        }
    }

    public synchronized void setBufferSize(int newSize) {
        if (newSize == bufferSize) {
            return;
        }
        this.bufferSize = newSize;
        this.pool = new ArrayBlockingQueue<>(poolSize);
        for (int i = 0; i < poolSize; i++) {
            pool.offer(new float[bufferSize]);
        }
    }

    public float[] getBuffer() {
        float[] buffer = pool.poll();
        if (buffer == null) {
            Log.w("BufferPool", "Buffer pool underflow");
            buffer = new float[bufferSize];
        }
        return buffer;
    }

    public void releaseBuffer(float[] buffer) {
        if (!pool.offer(buffer)) {
            Log.w("BufferPool", "Buffer pool overflow");
        }
    }
}
