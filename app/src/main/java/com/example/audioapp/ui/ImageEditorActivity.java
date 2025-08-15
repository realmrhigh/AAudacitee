package com.example.audioapp.ui;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Button;
import android.widget.ImageView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.appcompat.app.AppCompatActivity;
import com.canhub.cropper.CropImageContract;
import com.canhub.cropper.CropImageContractOptions;
import com.canhub.cropper.CropImageOptions;
import com.example.audioapp.R;

public class ImageEditorActivity extends AppCompatActivity {

    private ImageView imageView;
    private ActivityResultLauncher<Intent> imagePickerLauncher;
    private ActivityResultLauncher<CropImageContractOptions> cropImageLauncher;
    private Uri currentImageUri;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_image_editor);

        imageView = findViewById(R.id.image_view);
        Button loadImageButton = findViewById(R.id.button_load_image);

        imagePickerLauncher = registerForActivityResult(
                new ActivityResultContracts.StartActivityForResult(),
                result -> {
                    if (result.getResultCode() == RESULT_OK && result.getData() != null) {
                        Uri imageUri = result.getData().getData();
                        currentImageUri = imageUri;
                        imageView.setImageURI(imageUri);
                    }
                });

        cropImageLauncher = registerForActivityResult(new CropImageContract(), result -> {
            if (result.isSuccessful()) {
                currentImageUri = result.getUriContent();
                imageView.setImageURI(currentImageUri);
            }
        });

        loadImageButton.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("image/*");
            imagePickerLauncher.launch(intent);
        });

        Button resizeButton = findViewById(R.id.button_resize);
        resizeButton.setOnClickListener(v -> showResizeDialog());

        Button cropButton = findViewById(R.id.button_crop);
        cropButton.setOnClickListener(v -> {
            if (currentImageUri != null) {
                cropImageLauncher.launch(new CropImageContractOptions(currentImageUri, new CropImageOptions()));
            }
        });

        Button chopButton = findViewById(R.id.button_chop);
        chopButton.setOnClickListener(v -> {
            if (currentImageUri != null) {
                Intent intent = new Intent(ImageEditorActivity.this, ChopEditorActivity.class);
                intent.setData(currentImageUri);
                startActivity(intent);
            }
        });
    }

    private void chopImage() {
        android.graphics.drawable.Drawable drawable = imageView.getDrawable();
        if (drawable instanceof android.graphics.drawable.BitmapDrawable) {
            android.graphics.Bitmap bitmap = ((android.graphics.drawable.BitmapDrawable) drawable).getBitmap();
            int width = bitmap.getWidth();
            int height = bitmap.getHeight();
            int x = (int) (width * 0.1);
            int y = (int) (height * 0.1);
            int newWidth = (int) (width * 0.8);
            int newHeight = (int) (height * 0.8);
            android.graphics.Bitmap choppedBitmap = android.graphics.Bitmap.createBitmap(bitmap, x, y, newWidth, newHeight);
            imageView.setImageBitmap(choppedBitmap);
        }
    }

    private void showResizeDialog() {
        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Resize Image");

        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);

        final android.widget.EditText widthInput = new android.widget.EditText(this);
        widthInput.setHint("Width");
        widthInput.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        layout.addView(widthInput);

        final android.widget.EditText heightInput = new android.widget.EditText(this);
        heightInput.setHint("Height");
        heightInput.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        layout.addView(heightInput);

        builder.setView(layout);

        builder.setPositiveButton("Resize", (dialog, which) -> {
            android.graphics.drawable.Drawable drawable = imageView.getDrawable();
            if (drawable instanceof android.graphics.drawable.BitmapDrawable) {
                android.graphics.Bitmap bitmap = ((android.graphics.drawable.BitmapDrawable) drawable).getBitmap();
                int width = Integer.parseInt(widthInput.getText().toString());
                int height = Integer.parseInt(heightInput.getText().toString());
                android.graphics.Bitmap resizedBitmap = android.graphics.Bitmap.createScaledBitmap(bitmap, width, height, true);
                imageView.setImageBitmap(resizedBitmap);
            }
        });
        builder.setNegativeButton("Cancel", (dialog, which) -> dialog.cancel());

        builder.show();
    }
}
