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
        // wait until we have storage permission to start the game
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            super.requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
            while (super.checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                android.os.SystemClock.sleep(1000);
            }
        }
        super.onCreate(savedInstanceState);
    }
    
    protected void onDestroy() { 
        super.onDestroy(); 
    }

    protected String[] getLibraries() {
        return new String[] {
                "hidapi",
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
