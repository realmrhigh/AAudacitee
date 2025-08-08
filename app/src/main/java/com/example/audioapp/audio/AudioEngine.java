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
    
    // Parametric EQ controls
    public static native void native_setEQBandEnabled(int bandIndex, boolean enabled);
    public static native void native_setEQBandFrequency(int bandIndex, float frequency);
    public static native void native_setEQBandGain(int bandIndex, float gainDb);
    public static native void native_setEQBandQ(int bandIndex, float q);
    public static native void native_setMasterVolume(float volume);
}
