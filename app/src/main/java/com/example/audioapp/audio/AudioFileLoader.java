package com.example.audioapp.audio;

import android.content.ContentResolver;
import android.net.Uri;

public class AudioFileLoader {

    public static AudioBuffer load(ContentResolver contentResolver, Uri uri) {
        return AudioDecoder.decode(contentResolver, uri);
    }
}
