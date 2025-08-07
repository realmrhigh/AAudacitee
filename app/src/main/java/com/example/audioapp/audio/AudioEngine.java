package com.example.audioapp.audio;

public class AudioEngine {
    static {
        System.loadLibrary("audioapp");
    }

    public static native void native_create();
    public static native void native_start();
    public static native void native_stop();
}
