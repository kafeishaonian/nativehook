package com.example.shadowhook;

public class NativeLib {

    // Used to load the 'shadowhook' library on application startup.
    static {
        System.loadLibrary("shadowhook");
    }

    /**
     * A native method that is implemented by the 'shadowhook' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}