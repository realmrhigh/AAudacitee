package com.example.audioapp.audio;

public class AudioEngine {
    static {
        System.loadLibrary("audioapp");
    }

    public static native void native_create();
    public static native void native_start();
    public static native void native_stop();
    public static native void native_setPlaying(boolean isPlaying);
    public static native void native_setPlaybackPosition(int position);
    public static native int native_getPlaybackPosition();
    public static native void native_setAudioBuffer(AudioBuffer buffer);
    public static native void native_setBufferSize(int bufferSize);
}
