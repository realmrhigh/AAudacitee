package com.example.audioapp.ui;

import android.net.Uri;
import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import com.example.audioapp.R;

public class ChopEditorActivity extends AppCompatActivity {

    private ChopView chopView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_chop_editor);

        chopView = findViewById(R.id.chop_view);
        Uri imageUri = getIntent().getData();
        if (imageUri != null) {
            chopView.setImageUri(imageUri);
        }

        Button deleteModeButton = findViewById(R.id.delete_mode_button);
        deleteModeButton.setOnClickListener(v -> {
            chopView.setDeleteMode(!chopView.isDeleteMode());
        });

        Button saveButton = findViewById(R.id.save_button);
        saveButton.setOnClickListener(v -> saveChoppedImages());
    }

    private void saveChoppedImages() {
        List<android.graphics.Bitmap> bitmaps = chopView.getChoppedBitmaps();
        String originalFileName = new java.io.File(getIntent().getData().getPath()).getName();
        int i = 1;
        for (android.graphics.Bitmap bitmap : bitmaps) {
            String fileName = originalFileName.replaceFirst("[.][^.]+$", "") + "_chop_" + i + ".png";
            try {
                java.io.OutputStream fos = getContentResolver().openOutputStream(
                        android.provider.MediaStore.Images.Media.getContentUri(android.provider.MediaStore.VOLUME_EXTERNAL_PRIMARY),
                        new android.content.ContentValues() {{
                            put(android.provider.MediaStore.Images.Media.DISPLAY_NAME, fileName);
                            put(android.provider.MediaStore.Images.Media.MIME_TYPE, "image/png");
                        }}
                );
                bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG, 100, fos);
                fos.close();
            } catch (java.io.IOException e) {
                e.printStackTrace();
            }
            i++;
        }
        android.widget.Toast.makeText(this, "Images saved", android.widget.Toast.LENGTH_SHORT).show();
    }
}
