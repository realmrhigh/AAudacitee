package com.example.audioapp.engine;

public class QualityProfile {
    public enum Quality {
        PREVIEW,
        GOOD,
        BEST
    }

    private Quality quality;

    public QualityProfile(Quality quality) {
        this.quality = quality;
    }

    public Quality getQuality() {
        return quality;
    }

    public void setQuality(Quality quality) {
        this.quality = quality;
    }
}
