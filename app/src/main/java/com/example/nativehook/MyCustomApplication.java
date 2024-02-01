package com.example.nativehook;

import android.app.Application;
import android.content.Context;

import com.example.plthook.PLTHook;

/**
 * author: hongming.wei
 * data: 2024/1/31
 */
public class MyCustomApplication extends Application {
    private String TAG = "plt_hook_tag";


    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);

        System.loadLibrary("hookee");

        int r = PLTHook.init(new PLTHook.ConfigBuilder()
                .setMode(PLTHook.Mode.AUTOMATIC)
                .setDebug(true)
                .setRecordable(true)
                .build());

        System.loadLibrary("hacker");
    }
}
