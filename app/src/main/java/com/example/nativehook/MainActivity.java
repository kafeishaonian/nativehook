package com.example.nativehook;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.example.nativehook.databinding.ActivityMainBinding;
import com.example.plthook.PLTHook;

import java.io.BufferedReader;
import java.io.FileReader;

public class MainActivity extends AppCompatActivity {

    private String TAG = "plt_hook_tag";
    private int curType = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }


    private void hookOrUnhook(int newType) {
        if (newType == curType) {
            return;
        }

        if(curType != -1) {
            NativeHacker.unhook();
            curType = -1;
        }

        if(newType != -1) {
            NativeHacker.hook(newType);
            curType = newType;
        }
    }

    public void onRadioButtonClicked(View view) {
        if (view.getId() == R.id.radio_hook_single) {
            hookOrUnhook(0);
        } else if (view.getId() == R.id.radio_hook_partial) {
            hookOrUnhook(1);
        } else if (view.getId() == R.id.radio_hook_all) {
            hookOrUnhook(2);
        } else if (view.getId() == R.id.radio_unhook) {
            hookOrUnhook(-1);
        }
    }

    public void onTestClick(View view) {
        NativeHookee.test();
    }

    public void onGetRecordsClick(View view) {
        String records = PLTHook.getRecords();
        if (records != null) {
            for (String line : records.split("\n")) {
                Log.i(TAG, line);
            }
        }
    }

    public void onDumpRecordsClick(View view) {
        String pathname = getApplicationContext().getFilesDir() + "/bytehook_records.txt";
        NativeHacker.dumpRecords(pathname);

        BufferedReader br = null;
        try {
            br = new BufferedReader(new FileReader(pathname));
            String line;
            while ((line = br.readLine()) != null) {
                Log.i(TAG, line);
            }
        } catch (Throwable ignored) {
        } finally {
            if (br != null) {
                try {
                    br.close();
                } catch (Exception ignored) {
                }
            }
        }
    }

}