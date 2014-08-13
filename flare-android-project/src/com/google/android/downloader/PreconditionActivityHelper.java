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
import android.content.Intent;

/**
 * Usage:
 *
 * Intent intent = PreconditionActivityHelper.createPreconditionIntent(
 *     activity, WaitActivity.class);
 * // Optionally add extras to pass arguments to the intent
 * intent.putExtra(Utils.EXTRA_ACCOUNT, account);
 * PreconditionActivityHelper.startPreconditionActivityAndFinish(this, intent);
 *
 * // And in the wait activity:
 * PreconditionActivityHelper.startOriginalActivityAndFinish(this);
 *
 */

public class PreconditionActivityHelper {
    /**
     * Create a precondition activity intent.
     * @param activity the original activity
     * @param preconditionActivityClazz the precondition activity's class
     * @return an intent which will launch the precondition activity.
     */
    public static Intent createPreconditionIntent(Activity activity,
            Class preconditionActivityClazz) {
        Intent newIntent = new Intent();
        newIntent.setClass(activity, preconditionActivityClazz);
        newIntent.putExtra(EXTRA_WRAPPED_INTENT, activity.getIntent());
        return newIntent;
    }

    /**
     * Start the precondition activity using a given intent, which should
     * have been created by calling createPreconditionIntent.
     * @param activity
     * @param intent
     */
    public static void startPreconditionActivityAndFinish(Activity activity,
            Intent intent) {
        activity.startActivity(intent);
        activity.finish();
    }

    /**
     * Start the original activity, and finish the precondition activity.
     * @param preconditionActivity
     */
    public static void startOriginalActivityAndFinish(
            Activity preconditionActivity) {
        preconditionActivity.startActivity(
                (Intent) preconditionActivity.getIntent()
                    .getParcelableExtra(EXTRA_WRAPPED_INTENT));
        preconditionActivity.finish();
    }

    static private final String EXTRA_WRAPPED_INTENT =
        "PreconditionActivityHelper_wrappedIntent";
}
