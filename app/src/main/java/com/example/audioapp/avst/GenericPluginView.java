package com.example.audioapp.avst;

import android.content.Context;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class GenericPluginView extends LinearLayout {
    private final Plugin plugin;

    public GenericPluginView(Context context, Plugin plugin) {
        super(context);
        this.plugin = plugin;
        setOrientation(VERTICAL);

        int paramCount = plugin.getParameterCount();
        for (int i = 0; i < paramCount; i++) {
            TextView name = new TextView(context);
            name.setText(plugin.getParameterName(i));
            addView(name);

            SeekBar slider = new SeekBar(context);
            slider.setMax(100);
            slider.setProgress((int) (plugin.getParameter(i) * 100));
            final int finalI = i;
            slider.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if (fromUser) {
                        plugin.setParameter(finalI, progress / 100.0f);
                    }
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {}

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {}
            });
            addView(slider);
        }
    }
}
