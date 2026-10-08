package com.samp.mobile.launcher.util;

import android.content.Context;
import android.content.res.AssetManager;

import org.ini4j.Wini;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class ConfigValidator {
    public static void validateConfigFiles(Context context) {
        File externalFilesDir = context.getExternalFilesDir(null);
        if (externalFilesDir == null) {
            return;
        }
        File file = new File(externalFilesDir, "SAMP/settings.ini");
        if (!file.exists()) {
            file.getParentFile().mkdirs();
            copyAsset(context.getAssets(), "settings.ini", file.toString());
        }
        if (file.exists()) {
            try {
                Wini settings = new Wini(file);
                settings.put("client", "name", "");
                settings.store();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        /*File file2 = new File(externalFilesDir, "gta_sa.set");
        if (!file2.exists()) {
            file2.getParentFile().mkdirs();
            copyAsset(context.getAssets(), "gta_sa.set", file2.toString());
        }*/
    }

    static boolean copyAsset(AssetManager assetManager, String str, String str2) {
        try {
            InputStream open = assetManager.open(str);
            new File(str2).createNewFile();
            FileOutputStream fileOutputStream = new FileOutputStream(str2);
            copyFile(open, fileOutputStream);
            open.close();
            fileOutputStream.flush();
            fileOutputStream.close();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    static void copyFile(InputStream inputStream, OutputStream outputStream) throws IOException {
        byte[] bArr = new byte[1024];
        while (true) {
            int read = inputStream.read(bArr);
            if (read != -1) {
                outputStream.write(bArr, 0, read);
            } else {
                return;
            }
        }
    }
}
