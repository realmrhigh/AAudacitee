package com.example.audioapp.audio;

public class AudioEngine {
    public static final int FORMAT_WAV = 0;
    public static final int FORMAT_MP3 = 1;

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
    public static native double native_getLoudness();
    public static native void native_normalizeLoudness(double targetLoudness);
    public static native boolean native_exportFile(String path, int format, double targetLoudness, int bitrate);
}
