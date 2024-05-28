package org.flare.app; 

import org.libsdl.app.SDLActivity;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.*;
/* 
 * A sample wrapper class that just calls SDLActivity 
 */ 

public class FLARE extends SDLActivity { 
    protected void onCreate(Bundle savedInstanceState) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (super.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                super.requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            }
        }
        super.onCreate(savedInstanceState);
    }
    
    protected void onDestroy() { 
        super.onDestroy(); 
    }

    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // read/write permission granted, so we need to restart the app
            // the dialog we pop up when unable to find mods leaks during the restart, so it ends up visible for a brief moment
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                super.recreate();
                super.initialize();
                super.onCreate(null);
            } else {
                super.finishAndRemoveTask();
                super.initialize(); // need a clean state if the app is switched to again
            }
        }
    }

    protected String[] getLibraries() {
        return new String[] {
                "SDL2",
                "SDL2_image",
                "mpg123",
                "SDL2_mixer",
                //"SDL2_net",
                "SDL2_ttf",
                "main"
        };
    }
}
