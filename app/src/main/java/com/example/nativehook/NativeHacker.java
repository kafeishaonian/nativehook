package com.example.nativehook;

/**
 * author: hongming.wei
 * data: 2024/1/31
 */
public class NativeHacker {

    public static void hook(int type) {
        nativeHook(type);
    }

    public static void unhook() {
        nativeUnhook();
    }

    public static void dumpRecords(String pathname) {
        nativeDumpRecords(pathname);
    }



    private static native int nativeHook(int type);
    private static native int nativeUnhook();
    private static native void nativeDumpRecords(String pathname);

}
