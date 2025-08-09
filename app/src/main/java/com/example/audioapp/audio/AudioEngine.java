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
    
    // Parametric EQ controls
    public static native void native_setEQBandEnabled(int bandIndex, boolean enabled);
    public static native void native_setEQBandFrequency(int bandIndex, float frequency);
    public static native void native_setEQBandGain(int bandIndex, float gainDb);
    public static native void native_setEQBandQ(int bandIndex, float q);
    public static native void native_setMasterVolume(float volume);
    
    // Export and loudness analysis
    public static native double native_getLoudness();
    public static native void native_normalizeLoudness(double targetLoudness);
    public static native boolean native_exportFile(String path, int format, double targetLoudness, int bitrate);
}
