package com.codetend.myvideo;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;

import com.codetend.myvideo.video.CameraThread;

public class MainActivity extends AppCompatActivity {

    private SurfaceView mPreviewView;
    private SurfaceView mPlayView;
    private CameraThread cameraThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        mPreviewView = findViewById(R.id.surfaceview1);
        mPlayView = findViewById(R.id.surfaceview2);


    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (cameraThread != null) {
            cameraThread.finish();
        }
    }

    public void startPreview(View view) {
        if (cameraThread == null) {
            cameraThread = new CameraThread();
        } else {
            cameraThread.finish();
            cameraThread = new CameraThread();
        }
        cameraThread.setSurfaceView(mPreviewView);
        cameraThread.start();
    }

    public void stopPreview(View view) {
        if (cameraThread != null) {
            cameraThread.finish();
            cameraThread = null;
        }
    }


    public void test(View view) {
        FFmpegManager.getInstance().endEncode();
    }
}
