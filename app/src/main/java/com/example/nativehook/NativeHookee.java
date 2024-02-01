package com.example.nativehook;

/**
 * author: hongming.wei
 * data: 2024/1/31
 */
public class NativeHookee {
    public static void test() {
        nativeTest();
    }

    private static native void nativeTest();
}
