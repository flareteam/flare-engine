/*
 * Copyright (C) 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.downloader;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.content.DialogInterface;
import android.content.Intent;
import org.apache.http.impl.client.DefaultHttpClient;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import java.security.MessageDigest;
import android.util.Log;
import android.util.Xml;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpHead;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.net.MalformedURLException;
import java.net.URL;
import java.security.NoSuchAlgorithmException;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class DownloaderActivity extends Activity {

    /**
     * Checks if data has been downloaded. If so, returns true. If not,
     * starts an activity to download the data and returns false. If this
     * function returns false the caller should immediately return from its
     * onCreate method. The calling activity will later be restarted
     * (using a copy of its original intent) once the data download completes.
     * @param activity The calling activity.
     * @param customText A text string that is displayed in the downloader UI.
     * @param fileConfigUrl The URL of the download configuration URL.
     * @param configVersion The version of the configuration file.
     * @param dataPath The directory on the device where we want to store the
     * data.
     * @param userAgent The user agent string to use when fetching URLs.
     * @return true if the data has already been downloaded successfully, or
     * false if the data needs to be downloaded.
     */
    public static boolean ensureDownloaded(Activity activity,
            String customText, String fileConfigUrl,
            String configVersion, String dataPath,
            String userAgent) {
        File dest = new File(dataPath);
        if (dest.exists()) {
            // Check version
            if (versionMatches(dest, configVersion)) {
                Log.i(LOG_TAG, "Versions match, no need to download.");
                return true;
            }
        }
        Intent intent = PreconditionActivityHelper.createPreconditionIntent(
                activity, DownloaderActivity.class);
        intent.putExtra(EXTRA_CUSTOM_TEXT, customText);
        intent.putExtra(EXTRA_FILE_CONFIG_URL, fileConfigUrl);
        intent.putExtra(EXTRA_CONFIG_VERSION, configVersion);
        intent.putExtra(EXTRA_DATA_PATH, dataPath);
        intent.putExtra(EXTRA_USER_AGENT, userAgent);
        PreconditionActivityHelper.startPreconditionActivityAndFinish(
                activity, intent);
        return false;
    }

    /**
     * Delete a directory and all its descendants.
     * @param directory The directory to delete
     * @return true if the directory was deleted successfully.
     */
    public static boolean deleteData(String directory) {
        return deleteTree(new File(directory), true);
    }

    private static boolean deleteTree(File base, boolean deleteBase) {
        boolean result = true;
        if (base.isDirectory()) {
            for (File child : base.listFiles()) {
                result &= deleteTree(child, true);
            }
        }
        if (deleteBase) {
            result &= base.delete();
        }
        return result;
    }

    private static boolean versionMatches(File dest, String expectedVersion) {
        Config config = getLocalConfig(dest, LOCAL_CONFIG_FILE);
        if (config != null) {
            return config.version.equals(expectedVersion);
        }
        return false;
    }

    private static Config getLocalConfig(File destPath, String configFilename) {
        File configPath = new File(destPath, configFilename);
        FileInputStream is;
        try {
            is = new FileInputStream(configPath);
        } catch (FileNotFoundException e) {
            return null;
        }
        try {
            Config config = ConfigHandler.parse(is);
            return config;
        } catch (Exception e) {
            Log.e(LOG_TAG, "Unable to read local config file", e);
            return null;
        } finally {
            quietClose(is);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        requestWindowFeature(Window.FEATURE_CUSTOM_TITLE);
        setContentView(R.layout.downloader);
        getWindow().setFeatureInt(Window.FEATURE_CUSTOM_TITLE,
                R.layout.downloader_title);
        ((TextView) findViewById(R.id.customText)).setText(
                intent.getStringExtra(EXTRA_CUSTOM_TEXT));
        mProgress = (TextView) findViewById(R.id.progress);
        mTimeRemaining = (TextView) findViewById(R.id.time_remaining);
        Button button = (Button) findViewById(R.id.cancel);
        button.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                if (mDownloadThread != null) {
                    mSuppressErrorMessages = true;
                    mDownloadThread.interrupt();
                }
            }
        });
        startDownloadThread();
    }

    private void startDownloadThread() {
        mSuppressErrorMessages = false;
        mProgress.setText("");
        mTimeRemaining.setText("");
        mDownloadThread = new Thread(new Downloader(), "Downloader");
        mDownloadThread.setPriority(Thread.NORM_PRIORITY - 1);
        mDownloadThread.start();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mSuppressErrorMessages = true;
        mDownloadThread.interrupt();
        try {
            mDownloadThread.join();
        } catch (InterruptedException e) {
            // Don't care.
        }
    }

    private void onDownloadSucceeded() {
        Log.i(LOG_TAG, "Download succeeded");
        PreconditionActivityHelper.startOriginalActivityAndFinish(this);
    }

    private void onDownloadFailed(String reason) {
        Log.e(LOG_TAG, "Download stopped: " + reason);
        String shortReason;
        int index = reason.indexOf('\n');
        if (index >= 0) {
            shortReason = reason.substring(0, index);
        } else {
            shortReason = reason;
        }
        AlertDialog alert = new Builder(this).create();
        alert.setTitle(R.string.download_activity_download_stopped);

        if (!mSuppressErrorMessages) {
            alert.setMessage(shortReason);
        }

        alert.setButton(getString(R.string.download_activity_retry),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                startDownloadThread();
            }

        });
        alert.setButton2(getString(R.string.download_activity_quit),
                new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                finish();
            }

        });
        try {
            alert.show();
        } catch (WindowManager.BadTokenException e) {
            // Happens when the Back button is used to exit the activity.
            // ignore.
        }
    }

    private void onReportProgress(int progress) {
        mProgress.setText(mPercentFormat.format(progress / 10000.0));
        long now = SystemClock.elapsedRealtime();
        if (mStartTime == 0) {
            mStartTime = now;
        }
        long delta = now - mStartTime;
        String timeRemaining = getString(R.string.download_activity_time_remaining_unknown);
        if ((delta > 3 * MS_PER_SECOND) && (progress > 100)) {
            long totalTime = 10000 * delta / progress;
            long timeLeft = Math.max(0L, totalTime - delta);
            if (timeLeft > MS_PER_DAY) {
                timeRemaining = Long.toString(
                    (timeLeft + MS_PER_DAY - 1) / MS_PER_DAY)
                    + " "
                    + getString(R.string.download_activity_time_remaining_days);
            } else if (timeLeft > MS_PER_HOUR) {
                timeRemaining = Long.toString(
                        (timeLeft + MS_PER_HOUR - 1) / MS_PER_HOUR)
                        + " "
                        + getString(R.string.download_activity_time_remaining_hours);
            } else if (timeLeft > MS_PER_MINUTE) {
                timeRemaining = Long.toString(
                        (timeLeft + MS_PER_MINUTE - 1) / MS_PER_MINUTE)
                        + " "
                        + getString(R.string.download_activity_time_remaining_minutes);
            } else {
                timeRemaining = Long.toString(
                        (timeLeft + MS_PER_SECOND - 1) / MS_PER_SECOND)
                        + " "
                        + getString(R.string.download_activity_time_remaining_seconds);
            }
        }
        mTimeRemaining.setText(timeRemaining);
    }

    private void onReportVerifying() {
        mProgress.setText(getString(R.string.download_activity_verifying));
        mTimeRemaining.setText("");
    }

    private static void quietClose(InputStream is) {
        try {
            if (is != null) {
                is.close();
            }
        } catch (IOException e) {
            // Don't care.
        }
    }

    private static void quietClose(OutputStream os) {
        try {
            if (os != null) {
                os.close();
            }
        } catch (IOException e) {
            // Don't care.
        }
    }

    private static class Config {
        long getSize() {
            long result = 0;
            for(File file : mFiles) {
                result += file.getSize();
            }
            return result;
        }
        static class File {
            public File(String src, String dest, String md5, long size) {
                if (src != null) {
                    this.mParts.add(new Part(src, md5, size));
                }
                this.dest = dest;
            }
            static class Part {
                Part(String src, String md5, long size) {
                    this.src = src;
                    this.md5 = md5;
                    this.size = size;
                }
                String src;
                String md5;
                long size;
            }
            ArrayList<Part> mParts = new ArrayList<Part>();
            String dest;
            long getSize() {
                long result = 0;
                for(Part part : mParts) {
                    if (part.size > 0) {
                        result += part.size;
                    }
                }
                return result;
            }
        }
        String version;
        ArrayList<File> mFiles = new ArrayList<File>();
    }

    /**
     * <config version="">
     *   <file src="http:..." dest ="b.x" />
     *   <file dest="b.x">
     *     <part src="http:..." />
     *     ...
     *   ...
     * </config>
     *
     */
    private static class ConfigHandler extends DefaultHandler {

        public static Config parse(InputStream is) throws SAXException,
            UnsupportedEncodingException, IOException {
            ConfigHandler handler = new ConfigHandler();
            Xml.parse(is, Xml.findEncodingByName("UTF-8"), handler);
            return handler.mConfig;
        }

        private ConfigHandler() {
            mConfig = new Config();
        }

        @Override
        public void startElement(String uri, String localName, String qName,
                Attributes attributes) throws SAXException {
            if (localName.equals("config")) {
                mConfig.version = getRequiredString(attributes, "version");
            } else if (localName.equals("file")) {
                String src = attributes.getValue("", "src");
                String dest = getRequiredString(attributes, "dest");
                String md5 = attributes.getValue("", "md5");
                long size = getLong(attributes, "size", -1);
                mConfig.mFiles.add(new Config.File(src, dest, md5, size));
            } else if (localName.equals("part")) {
                String src = getRequiredString(attributes, "src");
                String md5 = attributes.getValue("", "md5");
                long size = getLong(attributes, "size", -1);
                int length = mConfig.mFiles.size();
                if (length > 0) {
                    mConfig.mFiles.get(length-1).mParts.add(
                            new Config.File.Part(src, md5, size));
                }
            }
        }

        private static String getRequiredString(Attributes attributes,
                String localName) throws SAXException {
            String result = attributes.getValue("", localName);
            if (result == null) {
                throw new SAXException("Expected attribute " + localName);
            }
            return result;
        }

        private static long getLong(Attributes attributes, String localName,
                long defaultValue) {
            String value = attributes.getValue("", localName);
            if (value == null) {
                return defaultValue;
            } else {
                return Long.parseLong(value);
            }
        }

        public Config mConfig;
    }

    private class DownloaderException extends Exception {
        public DownloaderException(String reason) {
            super(reason);
        }
    }

    private class Downloader implements Runnable {
        public void run() {
            Intent intent = getIntent();
            mFileConfigUrl = intent.getStringExtra(EXTRA_FILE_CONFIG_URL);
            mConfigVersion = intent.getStringExtra(EXTRA_CONFIG_VERSION);
            mDataPath = intent.getStringExtra(EXTRA_DATA_PATH);
            mUserAgent = intent.getStringExtra(EXTRA_USER_AGENT);

            mDataDir = new File(mDataPath);

            try {
                // Download files.
                mHttpClient = new DefaultHttpClient();
                Config config = getConfig();
                filter(config);
                persistantDownload(config);
                verify(config);
                cleanup();
                reportSuccess();
            } catch (Exception e) {
                reportFailure(e.toString() + "\n" + Log.getStackTraceString(e));
            }
        }

        private void persistantDownload(Config config)
        throws ClientProtocolException, DownloaderException, IOException {
            while(true) {
                try {
                    download(config);
                    break;
                } catch(java.net.SocketException e) {
                    if (mSuppressErrorMessages) {
                        throw e;
                    }
                } catch(java.net.SocketTimeoutException e) {
                    if (mSuppressErrorMessages) {
                        throw e;
                    }
                }
                Log.i(LOG_TAG, "Network connectivity issue, retrying.");
            }
        }

        private void filter(Config config)
        throws IOException, DownloaderException {
            File filteredFile = new File(mDataDir, LOCAL_FILTERED_FILE);
            if (filteredFile.exists()) {
                return;
            }

            File localConfigFile = new File(mDataDir, LOCAL_CONFIG_FILE_TEMP);
            HashSet<String> keepSet = new HashSet<String>();
            keepSet.add(localConfigFile.getCanonicalPath());

            HashMap<String, Config.File> fileMap =
                new HashMap<String, Config.File>();
            for(Config.File file : config.mFiles) {
                String canonicalPath =
                    new File(mDataDir, file.dest).getCanonicalPath();
                fileMap.put(canonicalPath, file);
            }
            recursiveFilter(mDataDir, fileMap, keepSet, false);
            touch(filteredFile);
        }

        private void touch(File file) throws FileNotFoundException {
            FileOutputStream os = new FileOutputStream(file);
            quietClose(os);
        }

        private boolean recursiveFilter(File base,
                HashMap<String, Config.File> fileMap,
                HashSet<String> keepSet, boolean filterBase)
        throws IOException, DownloaderException {
            boolean result = true;
            if (base.isDirectory()) {
                for (File child : base.listFiles()) {
                    result &= recursiveFilter(child, fileMap, keepSet, true);
                }
            }
            if (filterBase) {
                if (base.isDirectory()) {
                    if (base.listFiles().length == 0) {
                        result &= base.delete();
                    }
                } else {
                    if (!shouldKeepFile(base, fileMap, keepSet)) {
                        result &= base.delete();
                    }
                }
            }
            return result;
        }

        private boolean shouldKeepFile(File file,
                HashMap<String, Config.File> fileMap,
                HashSet<String> keepSet)
        throws IOException, DownloaderException {
            String canonicalPath = file.getCanonicalPath();
            if (keepSet.contains(canonicalPath)) {
                return true;
            }
            Config.File configFile = fileMap.get(canonicalPath);
            if (configFile == null) {
                return false;
            }
            return verifyFile(configFile, false);
        }

        private void reportSuccess() {
            mHandler.sendMessage(
                    Message.obtain(mHandler, MSG_DOWNLOAD_SUCCEEDED));
        }

        private void reportFailure(String reason) {
            mHandler.sendMessage(
                    Message.obtain(mHandler, MSG_DOWNLOAD_FAILED, reason));
        }

        private void reportProgress(int progress) {
            mHandler.sendMessage(
                    Message.obtain(mHandler, MSG_REPORT_PROGRESS, progress, 0));
        }

        private void reportVerifying() {
            mHandler.sendMessage(
                    Message.obtain(mHandler, MSG_REPORT_VERIFYING));
        }

        private Config getConfig() throws DownloaderException,
            ClientProtocolException, IOException, SAXException {
            Config config = null;
            if (mDataDir.exists()) {
                config = getLocalConfig(mDataDir, LOCAL_CONFIG_FILE_TEMP);
                if ((config == null)
                        || !mConfigVersion.equals(config.version)) {
                    if (config == null) {
                        Log.i(LOG_TAG, "Couldn't find local config.");
                    } else {
                        Log.i(LOG_TAG, "Local version out of sync. Wanted " +
                                mConfigVersion + " but have " + config.version);
                    }
                    config = null;
                }
            } else {
                Log.i(LOG_TAG, "Creating directory " + mDataPath);
                mDataDir.mkdirs();
                mDataDir.mkdir();
                if (!mDataDir.exists()) {
                    throw new DownloaderException(
                            "Could not create the directory " + mDataPath);
                }
            }
            if (config == null) {
                File localConfig = download(mFileConfigUrl,
                        LOCAL_CONFIG_FILE_TEMP);
                InputStream is = new FileInputStream(localConfig);
                try {
                    config = ConfigHandler.parse(is);
                } finally {
                    quietClose(is);
                }
                if (! config.version.equals(mConfigVersion)) {
                    throw new DownloaderException(
                            "Configuration file version mismatch. Expected " +
                            mConfigVersion + " received " +
                            config.version);
                }
            }
            return config;
        }

        private void noisyDelete(File file) throws IOException {
            if (! file.delete() ) {
                throw new IOException("could not delete " + file);
            }
        }

        private void download(Config config) throws DownloaderException,
            ClientProtocolException, IOException {
            mDownloadedSize = 0;
            getSizes(config);
            Log.i(LOG_TAG, "Total bytes to download: "
                    + mTotalExpectedSize);
            for(Config.File file : config.mFiles) {
                downloadFile(file);
            }
        }

        private void downloadFile(Config.File file) throws DownloaderException,
                FileNotFoundException, IOException, ClientProtocolException {
            boolean append = false;
            File dest = new File(mDataDir, file.dest);
            long bytesToSkip = 0;
            if (dest.exists() && dest.isFile()) {
                append = true;
                bytesToSkip = dest.length();
                mDownloadedSize += bytesToSkip;
            }
            FileOutputStream os = null;
            long offsetOfCurrentPart = 0;
            try {
                for(Config.File.Part part : file.mParts) {
                    // The part.size==0 check below allows us to download
                    // zero-length files.
                    if ((part.size > bytesToSkip) || (part.size == 0)) {
                        MessageDigest digest = null;
                        if (part.md5 != null) {
                            digest = createDigest();
                            if (bytesToSkip > 0) {
                                FileInputStream is = openInput(file.dest);
                                try {
                                    is.skip(offsetOfCurrentPart);
                                    readIntoDigest(is, bytesToSkip, digest);
                                } finally {
                                    quietClose(is);
                                }
                            }
                        }
                        if (os == null) {
                            os = openOutput(file.dest, append);
                        }
                        downloadPart(part.src, os, bytesToSkip,
                                part.size, digest);
                        if (digest != null) {
                            String hash = getHash(digest);
                            if (!hash.equalsIgnoreCase(part.md5)) {
                                Log.e(LOG_TAG, "web MD5 checksums don't match. "
                                        + part.src + "\nExpected "
                                        + part.md5 + "\n     got " + hash);
                                quietClose(os);
                                dest.delete();
                                throw new DownloaderException(
                                      "Received bad data from web server");
                            } else {
                               Log.i(LOG_TAG, "web MD5 checksum matches.");
                            }
                        }
                    }
                    bytesToSkip -= Math.min(bytesToSkip, part.size);
                    offsetOfCurrentPart += part.size;
                }
            } finally {
                quietClose(os);
            }
        }

        private void cleanup() throws IOException {
            File filtered = new File(mDataDir, LOCAL_FILTERED_FILE);
            noisyDelete(filtered);
            File tempConfig = new File(mDataDir, LOCAL_CONFIG_FILE_TEMP);
            File realConfig = new File(mDataDir, LOCAL_CONFIG_FILE);
            tempConfig.renameTo(realConfig);
        }

        private void verify(Config config) throws DownloaderException,
        ClientProtocolException, IOException {
            Log.i(LOG_TAG, "Verifying...");
            String failFiles = null;
            for(Config.File file : config.mFiles) {
                if (! verifyFile(file, true) ) {
                    if (failFiles == null) {
                        failFiles = file.dest;
                    } else {
                        failFiles += " " + file.dest;
                    }
                }
            }
            if (failFiles != null) {
                throw new DownloaderException(
                        "Possible bad SD-Card. MD5 sum incorrect for file(s) "
                        + failFiles);
            }
        }

        private boolean verifyFile(Config.File file, boolean deleteInvalid)
                throws FileNotFoundException, DownloaderException, IOException {
            Log.i(LOG_TAG, "verifying " + file.dest);
            reportVerifying();
            File dest = new File(mDataDir, file.dest);
            if (! dest.exists()) {
                Log.e(LOG_TAG, "File does not exist: " + dest.toString());
                return false;
            }
            long fileSize = file.getSize();
            long destLength = dest.length();
            if (fileSize != destLength) {
                Log.e(LOG_TAG, "Length doesn't match. Expected " + fileSize
                        + " got " + destLength);
                if (deleteInvalid) {
                    dest.delete();
                    return false;
                }
            }
            FileInputStream is = new FileInputStream(dest);
            try {
                for(Config.File.Part part : file.mParts) {
                    if (part.md5 == null) {
                        continue;
                    }
                    MessageDigest digest = createDigest();
                    readIntoDigest(is, part.size, digest);
                    String hash = getHash(digest);
                    if (!hash.equalsIgnoreCase(part.md5)) {
                        Log.e(LOG_TAG, "MD5 checksums don't match. " +
                                part.src + " Expected "
                                + part.md5 + " got " + hash);
                        if (deleteInvalid) {
                            quietClose(is);
                            dest.delete();
                        }
                        return false;
                    }
                }
            } finally {
                quietClose(is);
            }
            return true;
        }

        private void readIntoDigest(FileInputStream is, long bytesToRead,
                MessageDigest digest) throws IOException {
            while(bytesToRead > 0) {
                int chunkSize = (int) Math.min(mFileIOBuffer.length,
                        bytesToRead);
                int bytesRead = is.read(mFileIOBuffer, 0, chunkSize);
                if (bytesRead < 0) {
                    break;
                }
                updateDigest(digest, bytesRead);
                bytesToRead -= bytesRead;
            }
        }

        private MessageDigest createDigest() throws DownloaderException {
            MessageDigest digest;
            try {
                digest = MessageDigest.getInstance("MD5");
            } catch (NoSuchAlgorithmException e) {
                throw new DownloaderException("Couldn't create MD5 digest");
            }
            return digest;
        }

        private void updateDigest(MessageDigest digest, int bytesRead) {
            if (bytesRead == mFileIOBuffer.length) {
                digest.update(mFileIOBuffer);
            } else {
                // Work around an awkward API: Create a
                // new buffer with just the valid bytes
                byte[] temp = new byte[bytesRead];
                System.arraycopy(mFileIOBuffer, 0,
                        temp, 0, bytesRead);
                digest.update(temp);
            }
        }

        private String getHash(MessageDigest digest) {
            StringBuilder builder = new StringBuilder();
            for(byte b : digest.digest()) {
                builder.append(Integer.toHexString((b >> 4) & 0xf));
                builder.append(Integer.toHexString(b & 0xf));
            }
            return builder.toString();
        }


        /**
         * Ensure we have sizes for all the items.
         * @param config
         * @throws ClientProtocolException
         * @throws IOException
         * @throws DownloaderException
         */
        private void getSizes(Config config)
            throws ClientProtocolException, IOException, DownloaderException {
            for (Config.File file : config.mFiles) {
                for(Config.File.Part part : file.mParts) {
                    if (part.size < 0) {
                        part.size = getSize(part.src);
                    }
                }
            }
            mTotalExpectedSize = config.getSize();
        }

        private long getSize(String url) throws ClientProtocolException,
            IOException {
            url = normalizeUrl(url);
            Log.i(LOG_TAG, "Head " + url);
            HttpHead httpGet = new HttpHead(url);
            HttpResponse response = mHttpClient.execute(httpGet);
            if (response.getStatusLine().getStatusCode() != HttpStatus.SC_OK) {
                throw new IOException("Unexpected Http status code "
                    + response.getStatusLine().getStatusCode());
            }
            Header[] clHeaders = response.getHeaders("Content-Length");
            if (clHeaders.length > 0) {
                Header header = clHeaders[0];
                return Long.parseLong(header.getValue());
            }
            return -1;
        }

        private String normalizeUrl(String url) throws MalformedURLException {
            return (new URL(new URL(mFileConfigUrl), url)).toString();
        }

        private InputStream get(String url, long startOffset,
                long expectedLength)
            throws ClientProtocolException, IOException {
            url = normalizeUrl(url);
            Log.i(LOG_TAG, "Get " + url);

            mHttpGet = new HttpGet(url);
            int expectedStatusCode = HttpStatus.SC_OK;
            if (startOffset > 0) {
                String range = "bytes=" + startOffset + "-";
                if (expectedLength >= 0) {
                    range += expectedLength-1;
                }
                Log.i(LOG_TAG, "requesting byte range " + range);
                mHttpGet.addHeader("Range", range);
                expectedStatusCode = HttpStatus.SC_PARTIAL_CONTENT;
            }
            HttpResponse response = mHttpClient.execute(mHttpGet);
            long bytesToSkip = 0;
            int statusCode = response.getStatusLine().getStatusCode();
            if (statusCode != expectedStatusCode) {
                if ((statusCode == HttpStatus.SC_OK)
                        && (expectedStatusCode
                                == HttpStatus.SC_PARTIAL_CONTENT)) {
                    Log.i(LOG_TAG, "Byte range request ignored");
                    bytesToSkip = startOffset;
                } else {
                    throw new IOException("Unexpected Http status code "
                            + statusCode + " expected "
                            + expectedStatusCode);
                }
            }
            HttpEntity entity = response.getEntity();
            InputStream is = entity.getContent();
            if (bytesToSkip > 0) {
                is.skip(bytesToSkip);
            }
            return is;
        }

        private File download(String src, String dest)
            throws DownloaderException, ClientProtocolException, IOException {
            File destFile = new File(mDataDir, dest);
            FileOutputStream os = openOutput(dest, false);
            try {
                downloadPart(src, os, 0, -1, null);
            } finally {
                os.close();
            }
            return destFile;
        }

        private void downloadPart(String src, FileOutputStream os,
                long startOffset, long expectedLength, MessageDigest digest)
            throws ClientProtocolException, IOException, DownloaderException {
            boolean lengthIsKnown = expectedLength >= 0;
            if (startOffset < 0) {
                throw new IllegalArgumentException("Negative startOffset:"
                        + startOffset);
            }
            if (lengthIsKnown && (startOffset > expectedLength)) {
                throw new IllegalArgumentException(
                        "startOffset > expectedLength" + startOffset + " "
                        + expectedLength);
            }
            InputStream is = get(src, startOffset, expectedLength);
            try {
                long bytesRead = downloadStream(is, os, digest);
                if (lengthIsKnown) {
                    long expectedBytesRead = expectedLength - startOffset;
                    if (expectedBytesRead != bytesRead) {
                        Log.e(LOG_TAG, "Bad file transfer from server: " + src
                                + " Expected " + expectedBytesRead
                                + " Received " + bytesRead);
                        throw new DownloaderException(
                                "Incorrect number of bytes received from server");
                    }
                }
            } finally {
                is.close();
                mHttpGet = null;
            }
        }

        private FileOutputStream openOutput(String dest, boolean append)
            throws FileNotFoundException, DownloaderException {
            File destFile = new File(mDataDir, dest);
            File parent = destFile.getParentFile();
            if (! parent.exists()) {
                parent.mkdirs();
            }
            if (! parent.exists()) {
                throw new DownloaderException("Could not create directory "
                        + parent.toString());
            }
            FileOutputStream os = new FileOutputStream(destFile, append);
            return os;
        }

        private FileInputStream openInput(String src)
            throws FileNotFoundException, DownloaderException {
            File srcFile = new File(mDataDir, src);
            File parent = srcFile.getParentFile();
            if (! parent.exists()) {
                parent.mkdirs();
            }
            if (! parent.exists()) {
                throw new DownloaderException("Could not create directory "
                        + parent.toString());
            }
            return new FileInputStream(srcFile);
        }

        private long downloadStream(InputStream is, FileOutputStream os,
                MessageDigest digest)
                throws DownloaderException, IOException {
            long totalBytesRead = 0;
            while(true){
                if (Thread.interrupted()) {
                    Log.i(LOG_TAG, "downloader thread interrupted.");
                    mHttpGet.abort();
                    throw new DownloaderException("Thread interrupted");
                }
                int bytesRead = is.read(mFileIOBuffer);
                if (bytesRead < 0) {
                    break;
                }
                if (digest != null) {
                    updateDigest(digest, bytesRead);
                }
                totalBytesRead += bytesRead;
                os.write(mFileIOBuffer, 0, bytesRead);
                mDownloadedSize += bytesRead;
                int progress = (int) (Math.min(mTotalExpectedSize,
                        mDownloadedSize * 10000 /
                        Math.max(1, mTotalExpectedSize)));
                if (progress != mReportedProgress) {
                    mReportedProgress = progress;
                    reportProgress(progress);
                }
            }
            return totalBytesRead;
        }

        private DefaultHttpClient mHttpClient;
        private HttpGet mHttpGet;
        private String mFileConfigUrl;
        private String mConfigVersion;
        private String mDataPath;
        private File mDataDir;
        private String mUserAgent;
        private long mTotalExpectedSize;
        private long mDownloadedSize;
        private int mReportedProgress;
        private final static int CHUNK_SIZE = 32 * 1024;
        byte[] mFileIOBuffer = new byte[CHUNK_SIZE];
    }

    private final static String LOG_TAG = "Downloader";
    private TextView mProgress;
    private TextView mTimeRemaining;
    private final DecimalFormat mPercentFormat = new DecimalFormat("0.00 %");
    private long mStartTime;
    private Thread mDownloadThread;
    private boolean mSuppressErrorMessages;

    private final static long MS_PER_SECOND = 1000;
    private final static long MS_PER_MINUTE = 60 * 1000;
    private final static long MS_PER_HOUR = 60 * 60 * 1000;
    private final static long MS_PER_DAY = 24 * 60 * 60 * 1000;

    private final static String LOCAL_CONFIG_FILE = ".downloadConfig";
    private final static String LOCAL_CONFIG_FILE_TEMP = ".downloadConfig_temp";
    private final static String LOCAL_FILTERED_FILE = ".downloadConfig_filtered";
    private final static String EXTRA_CUSTOM_TEXT = "DownloaderActivity_custom_text";
    private final static String EXTRA_FILE_CONFIG_URL = "DownloaderActivity_config_url";
    private final static String EXTRA_CONFIG_VERSION = "DownloaderActivity_config_version";
    private final static String EXTRA_DATA_PATH = "DownloaderActivity_data_path";
    private final static String EXTRA_USER_AGENT = "DownloaderActivity_user_agent";

    private final static int MSG_DOWNLOAD_SUCCEEDED = 0;
    private final static int MSG_DOWNLOAD_FAILED = 1;
    private final static int MSG_REPORT_PROGRESS = 2;
    private final static int MSG_REPORT_VERIFYING = 3;

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MSG_DOWNLOAD_SUCCEEDED:
                onDownloadSucceeded();
                break;
            case MSG_DOWNLOAD_FAILED:
                onDownloadFailed((String) msg.obj);
                break;
            case MSG_REPORT_PROGRESS:
                onReportProgress(msg.arg1);
                break;
            case MSG_REPORT_VERIFYING:
                onReportVerifying();
                break;
            default:
                throw new IllegalArgumentException("Unknown message id "
                        + msg.what);
            }
        }

    };

}

