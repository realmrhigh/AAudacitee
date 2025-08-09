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

    public int getParameterCount() {
        return AvstHost.native_getParameterCount(nativeHandle);
    }

    public String getParameterName(int index) {
        return AvstHost.native_getParameterName(nativeHandle, index);
    }

    public float getParameter(int index) {
        return AvstHost.native_getParameter(nativeHandle, index);
    }

    public void setParameter(int index, float value) {
        AvstHost.native_setParameter(nativeHandle, index, value);
    }

    public float[] getFrequencyResponse() {
        return AvstHost.native_getFrequencyResponse(nativeHandle);
    }

    private static native void native_setBypass(long nativeHandle, boolean bypass);
}
