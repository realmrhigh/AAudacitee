package com.example.audioapp.avst;

public class Plugin {
    private final long nativeHandle;
    private boolean bypass = false;

    public Plugin(long nativeHandle) {
        this.nativeHandle = nativeHandle;
    }

    public long getNativeHandle() {
        return nativeHandle;
    }

    public void setBypass(boolean bypass) {
        this.bypass = bypass;
        native_setBypass(nativeHandle, bypass);
    }

    public boolean isBypassed() {
        return bypass;
    }

    private static native void native_setBypass(long nativeHandle, boolean bypass);
}
