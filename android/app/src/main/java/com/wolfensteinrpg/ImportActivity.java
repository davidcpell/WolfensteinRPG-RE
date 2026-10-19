package com.wolfensteinrpg;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Locale;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public final class ImportActivity extends Activity {
    private static final int PICK_IPA_REQUEST = 1;
    private static final String PRIVATE_IPA_NAME = "Wolfenstein RPG.ipa";
    private static final String[] REQUIRED_IPA_ENTRIES = {
            "Payload/Wolf...RPG.app/Info.plist",
            "Payload/Wolf...RPG.app/Packages/logo.bmp",
            "Payload/Wolf...RPG.app/Packages/strings.idx",
            "Payload/Wolf...RPG.app/Packages/strings00.bin",
            "Payload/Wolf...RPG.app/Packages/tables.bin",
            "Payload/Wolf...RPG.app/Packages/entities.bin",
            "Payload/Wolf...RPG.app/Packages/menus.bin",
            "Payload/Wolf...RPG.app/Packages/newMappings.bin",
            "Payload/Wolf...RPG.app/Packages/newPalettes.bin",
            "Payload/Wolf...RPG.app/Packages/model_0000.bin",
            "Payload/Wolf...RPG.app/Packages/newTexels000.bin",
            "Payload/Wolf...RPG.app/Packages/sounds/Alarm.wav"
    };

    private TextView statusView;
    private Button importButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        showImportScreen();
    }

    private File getPrivateIpaFile() {
        return new File(new File(getFilesDir(), "game"), PRIVATE_IPA_NAME);
    }

    private void showImportScreen() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        int padding = dp(32);
        layout.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText("Wolfenstein RPG Android port");
        title.setTextSize(24.0f);
        title.setGravity(Gravity.CENTER);
        layout.addView(title, matchWidthWrapHeight());

        statusView = new TextView(this);
        statusView.setTextSize(16.0f);
        statusView.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams statusParams = matchWidthWrapHeight();
        statusParams.setMargins(0, dp(20), 0, dp(20));
        layout.addView(statusView, statusParams);

        importButton = new Button(this);
        importButton.setOnClickListener(view -> openDocumentPicker());
        layout.addView(importButton, wrapContent());

        File privateIpa = getPrivateIpaFile();
        if (privateIpa.isFile()) {
            importButton.setText("Replace imported IPA");
            String validationError = validateKnownGoodIpa(privateIpa);
            if (validationError == null) {
                statusView.setText(String.format(
                        Locale.US,
                        "Known-good IPA layout validated\n%,d bytes",
                        privateIpa.length()));

                Button runHarnessButton = new Button(this);
                runHarnessButton.setText("Run graphics harness");
                runHarnessButton.setOnClickListener(view ->
                        startActivity(new Intent(this, WolfensteinSdlActivity.class)));
                LinearLayout.LayoutParams runParams = wrapContent();
                runParams.setMargins(0, dp(12), 0, 0);
                layout.addView(runHarnessButton, runParams);

                Button runAudioButton = new Button(this);
                runAudioButton.setText("Run audio harness");
                runAudioButton.setOnClickListener(view ->
                        startActivity(new Intent(this, WolfensteinAudioActivity.class)));
                LinearLayout.LayoutParams audioParams = wrapContent();
                audioParams.setMargins(0, dp(12), 0, 0);
                layout.addView(runAudioButton, audioParams);

                Button runImportedAudioButton = new Button(this);
                runImportedAudioButton.setText("Test imported music + effect");
                runImportedAudioButton.setOnClickListener(view ->
                        startActivity(new Intent(this, WolfensteinImportedAudioActivity.class)));
                LinearLayout.LayoutParams importedAudioParams = wrapContent();
                importedAudioParams.setMargins(0, dp(12), 0, 0);
                layout.addView(runImportedAudioButton, importedAudioParams);

                Button runGameButton = new Button(this);
                runGameButton.setText("Run game");
                runGameButton.setOnClickListener(view ->
                        startActivity(new Intent(this, WolfensteinGameActivity.class)));
                LinearLayout.LayoutParams gameParams = wrapContent();
                gameParams.setMargins(0, dp(12), 0, 0);
                layout.addView(runGameButton, gameParams);
            } else {
                statusView.setText("Private copy is not supported: " + validationError);
            }
        } else {
            importButton.setText("Select IPA");
            statusView.setText(
                    "Choose your Wolfenstein RPG IPA.\n" +
                    "The app will keep a private copy for later tests.");
        }

        setContentView(layout);
    }

    private void openDocumentPicker() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_IPA_REQUEST);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_IPA_REQUEST) {
            return;
        }
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            statusView.setText("Import cancelled. No files were changed.");
            return;
        }

        Uri selectedIpa = data.getData();
        importButton.setEnabled(false);
        statusView.setText("Copying " + getDisplayName(selectedIpa) + "…");
        new Thread(() -> copyIntoPrivateStorage(selectedIpa), "IPA import").start();
    }

    private void copyIntoPrivateStorage(Uri source) {
        File target = getPrivateIpaFile();
        File directory = target.getParentFile();
        File temporary = new File(directory, PRIVATE_IPA_NAME + ".tmp");

        try {
            if (!directory.isDirectory() && !directory.mkdirs()) {
                throw new IllegalStateException("Could not create the private game directory");
            }

            try (InputStream input = getContentResolver().openInputStream(source);
                 FileOutputStream output = new FileOutputStream(temporary)) {
                if (input == null) {
                    throw new IllegalStateException("The selected document could not be opened");
                }
                byte[] buffer = new byte[64 * 1024];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                }
                output.getFD().sync();
            }

            String validationError = validateKnownGoodIpa(temporary);
            if (validationError != null) {
                throw new IllegalArgumentException(validationError);
            }

            Files.move(
                    temporary.toPath(),
                    target.toPath(),
                    StandardCopyOption.ATOMIC_MOVE,
                    StandardCopyOption.REPLACE_EXISTING);

            runOnUiThread(this::showImportScreen);
        } catch (Exception exception) {
            temporary.delete();
            runOnUiThread(() -> {
                importButton.setEnabled(true);
                statusView.setText("Import failed: " + exception.getMessage());
            });
        }
    }

    private String validateKnownGoodIpa(File ipa) {
        try (ZipFile archive = new ZipFile(ipa)) {
            for (String requiredName : REQUIRED_IPA_ENTRIES) {
                ZipEntry entry = archive.getEntry(requiredName);
                if (entry == null || entry.isDirectory() || entry.getSize() == 0) {
                    return "missing required entry " + requiredName;
                }
            }
            return null;
        } catch (Exception exception) {
            return "not a readable IPA/ZIP archive (" + exception.getMessage() + ")";
        }
    }

    private String getDisplayName(Uri uri) {
        try (android.database.Cursor cursor = getContentResolver().query(
                uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0) {
                    return cursor.getString(column);
                }
            }
        }
        return "selected IPA";
    }

    private LinearLayout.LayoutParams matchWidthWrapHeight() {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    private LinearLayout.LayoutParams wrapContent() {
        return new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
