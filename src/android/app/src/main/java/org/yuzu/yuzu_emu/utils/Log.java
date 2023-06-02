package org.yuzu.yuzu_emu.utils;

import org.yuzu.yuzu_emu.BuildConfig;

/**
 * Contains methods that call through to {@link android.util.Log}, but
 * with the same TAG automatically provided. Also no-ops VERBOSE and DEBUG log
 * levels in release builds.
 */
public final class Log {
    private static final String TAG = "Yuzu Frontend";

    private Log() {
    }

    public static void verbose(String message) {
        if (BuildConfig.DEBUG) {
            android.util.Log.v(TAG, message);
        }
    }

    public static void debug(String message) {
        if (BuildConfig.DEBUG) {
            android.util.Log.d(TAG, message);
        }
    }

    public static void info(String message) {
        android.util.Log.i(TAG, message);
    }

    public static void warning(String message) {
        android.util.Log.w(TAG, message);
    }

    public static void error(String message) {
        android.util.Log.e(TAG, message);
    }
}
