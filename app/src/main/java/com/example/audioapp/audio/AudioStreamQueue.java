package com.example.audioapp.audio;

public class AudioStreamQueue {
    private long nativeHandle;

    public AudioStreamQueue(int size) {
        nativeHandle = native_create(size);
    }

    public void destroy() {
        native_destroy(nativeHandle);
        nativeHandle = 0;
    }

    public boolean push(byte[] data, int size) {
        return native_push(nativeHandle, data, size);
    }

    public int pop(byte[] data) {
        return native_pop(nativeHandle, data);
    }

    private native long native_create(int size);
    private native void native_destroy(long handle);
    private native boolean native_push(long handle, byte[] data, int size);
    private native int native_pop(long handle, byte[] data);

    public long getNativeHandle() {
        return nativeHandle;
    }

    static {
        System.loadLibrary("audioapp");
    }
}
