package com.example.audioapp.avst;

import java.util.ArrayList;
import java.util.List;

public class PluginChain {
    private final List<Plugin> plugins = new ArrayList<>();

    public void add(Plugin plugin) {
        plugins.add(plugin);
    }

    public void remove(Plugin plugin) {
        plugins.remove(plugin);
    }

    public List<Plugin> getPlugins() {
        return plugins;
    }
}
