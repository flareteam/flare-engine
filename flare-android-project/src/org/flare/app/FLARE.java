package org.flare.app; 

import org.libsdl.app.SDLActivity; 
import android.os.*; 

/* 
 * A sample wrapper class that just calls SDLActivity 
 */ 

public class FLARE extends SDLActivity { 
    protected void onCreate(Bundle savedInstanceState) { 
        super.onCreate(savedInstanceState);
		if (! DownloaderActivity.ensureDownloaded(this,
                getString(R.string.app_name), FILE_CONFIG_URL,
                CONFIG_VERSION, DATA_PATH, USER_AGENT)) {
            return;
        }
    } 
    
    protected void onDestroy() { 
        super.onDestroy(); 
    }
	
    private final static String FILE_CONFIG_URL = "http://flarerpg.org/android/download.xml";
    private final static String CONFIG_VERSION= "0.20";
    private final static String DATA_PATH = "/data/data/org.flare.app/files";
    private final static String USER_AGENT = "FLARE Downloader";
}
