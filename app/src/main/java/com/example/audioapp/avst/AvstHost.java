package com.example.audioapp.avst;

import java.util.ArrayList;
import java.util.List;

public class AvstHost {
    private final List<Plugin> plugins = new ArrayList<>();

    public Plugin loadPlugin(String path) {
        long nativeHandle = native_loadPlugin(path);
        if (nativeHandle != 0) {
            Plugin plugin = new Plugin(nativeHandle);
            plugins.add(plugin);
            return plugin;
        }
        return null;
    }

    public void unloadPlugin(Plugin plugin) {
        native_unloadPlugin(plugin.getNativeHandle());
        plugins.remove(plugin);
    }

    public int getParameterCount(Plugin plugin) {
        return native_getParameterCount(plugin.getNativeHandle());
    }

    public String getParameterName(Plugin plugin, int index) {
        return native_getParameterName(plugin.getNativeHandle(), index);
    }

    public float getParameter(Plugin plugin, int index) {
        return native_getParameter(plugin.getNativeHandle(), index);
    }

    public void setParameter(Plugin plugin, int index, float value) {
        native_setParameter(plugin.getNativeHandle(), index, value);
    }

    public void process(float[] buffer, int sampleRate) {
        long[] nativeHandles = new long[plugins.size()];
        for (int i = 0; i < plugins.size(); i++) {
            nativeHandles[i] = plugins.get(i).getNativeHandle();
        }
        native_process(nativeHandles, buffer, sampleRate);
    }

    private static native long native_loadPlugin(String path);
    private static native void native_unloadPlugin(long nativeHandle);
    private static native int native_getParameterCount(long nativeHandle);
    private static native String native_getParameterName(long nativeHandle, int index);
    private static native float native_getParameter(long nativeHandle, int index);
    private static native void native_setParameter(long nativeHandle, int index, float value);
    private static native void native_process(long[] nativeHandles, float[] buffer, int sampleRate);
}
