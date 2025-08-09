package com.example.audioapp.avst;

import com.example.audioapp.engine.QualityProfile;
import java.util.ArrayList;
import java.util.List;

public class AvstHost {
    private final PluginChain pluginChain = new PluginChain();

    public Plugin loadPlugin(String path) {
        long nativeHandle = native_loadPlugin(path);
        if (nativeHandle != 0) {
            Plugin plugin = new Plugin(nativeHandle);
            pluginChain.add(plugin);
            return plugin;
        }
        return null;
    }

    public void unloadPlugin(Plugin plugin) {
        native_unloadPlugin(plugin.getNativeHandle());
        pluginChain.remove(plugin);
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
        long[] nativeHandles = new long[pluginChain.getPlugins().size()];
        for (int i = 0; i < pluginChain.getPlugins().size(); i++) {
            nativeHandles[i] = pluginChain.getPlugins().get(i).getNativeHandle();
        }
        native_process(nativeHandles, buffer, sampleRate);
    }

    public float getCpuUsage(Plugin plugin) {
        return native_getCpuUsage(plugin.getNativeHandle());
    }

    public byte[] savePreset(Plugin plugin) {
        return native_savePreset(plugin.getNativeHandle());
    }

    public void loadPreset(Plugin plugin, byte[] preset) {
        native_loadPreset(plugin.getNativeHandle(), preset);
    }

    public byte[] saveChain() {
        long[] nativeHandles = new long[pluginChain.getPlugins().size()];
        for (int i = 0; i < pluginChain.getPlugins().size(); i++) {
            nativeHandles[i] = pluginChain.getPlugins().get(i).getNativeHandle();
        }
        return native_saveChain(nativeHandles);
    }

    public void loadChain(byte[] chain) {
        for (Plugin plugin : new ArrayList<>(pluginChain.getPlugins())) {
            unloadPlugin(plugin);
        }
        long[] nativeHandles = native_loadChain(chain);
        for (long nativeHandle : nativeHandles) {
            Plugin plugin = new Plugin(nativeHandle);
            pluginChain.add(plugin);
        }
    }

    public byte[] saveBypassState() {
        byte[] bypassState = new byte[pluginChain.getPlugins().size()];
        for (int i = 0; i < pluginChain.getPlugins().size(); i++) {
            bypassState[i] = (byte) (pluginChain.getPlugins().get(i).isBypassed() ? 1 : 0);
        }
        return bypassState;
    }

    public void loadBypassState(byte[] bypassState) {
        for (int i = 0; i < bypassState.length; i++) {
            if (i < pluginChain.getPlugins().size()) {
                pluginChain.getPlugins().get(i).setBypass(bypassState[i] == 1);
            }
        }
    }

    public void setPluginQuality(Plugin plugin, int quality) {
        native_setPluginQuality(plugin.getNativeHandle(), quality);
    }

    public void setQualityProfile(QualityProfile.Quality quality) {
        for (Plugin plugin : pluginChain.getPlugins()) {
            setPluginQuality(plugin, quality.ordinal());
        }
    }

    private static native long native_loadPlugin(String path);
    private static native void native_unloadPlugin(long nativeHandle);
    static native int native_getParameterCount(long nativeHandle);
    static native String native_getParameterName(long nativeHandle, int index);
    static native float native_getParameter(long nativeHandle, int index);
    static native void native_setParameter(long nativeHandle, int index, float value);
    private static native void native_process(long[] nativeHandles, float[] buffer, int sampleRate);
    private static native float native_getCpuUsage(long nativeHandle);
    private static native byte[] native_savePreset(long nativeHandle);
    private static native void native_loadPreset(long nativeHandle, byte[] preset);
    private static native byte[] native_saveChain(long[] nativeHandles);
    private static native long[] native_loadChain(byte[] chain);
    private static native void native_setPluginQuality(long nativeHandle, int quality);
    public static native float[] native_getFrequencyResponse(long nativeHandle);
}
