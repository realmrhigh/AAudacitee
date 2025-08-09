package com.example.audioapp.audio;

import android.content.ContentResolver;
import android.net.Uri;

public class AudioFileLoader {

    public static AudioBuffer load(ContentResolver contentResolver, Uri uri) {
        try {
            return AudioDecoder.decode(contentResolver, uri);
        } catch (Exception e) {
            // Log the error and return null to maintain compatibility
            System.err.println("AudioFileLoader error: " + e.getMessage());
            e.printStackTrace();
            return null;
        }
    }
}
