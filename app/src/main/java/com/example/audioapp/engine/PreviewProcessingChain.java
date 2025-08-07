package com.example.audioapp.engine;

import com.example.audioapp.avst.Plugin;
import java.util.ArrayList;
import java.util.List;

public class PreviewProcessingChain {
    private final List<Plugin> plugins = new ArrayList<>();

    public void add(Plugin plugin) {
        plugins.add(plugin);
    }

    public void remove(Plugin plugin) {
        plugins.remove(plugin);
    }

    public void process(float[] buffer, int sampleRate) {
        long[] nativeHandles = new long[plugins.size()];
        for (int i = 0; i < plugins.size(); i++) {
            nativeHandles[i] = plugins.get(i).getNativeHandle();
        }
        native_process(nativeHandles, buffer, sampleRate);
    }

    private static native void native_process(long[] nativeHandles, float[] buffer, int sampleRate);
}
