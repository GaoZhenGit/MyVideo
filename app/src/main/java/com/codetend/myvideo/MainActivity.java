package com.codetend.myvideo;

import android.hardware.Camera;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

import com.codetend.myvideo.video.CameraThread;

import java.util.List;

public class MainActivity extends AppCompatActivity {

    private SurfaceView mPreviewView;
    private SurfaceView mPlayView;
    private CameraThread cameraThread;
    private List<Camera.Size> mCameraSizeList;
    private Camera.Size mSelectSize;

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
        cameraThread.setPreviewSize(mSelectSize);
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

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        mCameraSizeList = CameraThread.getSupportPreviewSize();
        for (int i = 0; i < mCameraSizeList.size(); i++) {
            Camera.Size size = mCameraSizeList.get(i);
            String widthHeightRate = size.width + ":" + size.height;
            menu.add(0, i, i, widthHeightRate);
        }
        return super.onCreateOptionsMenu(menu);
    }


    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (mCameraSizeList != null) {
            mSelectSize = mCameraSizeList.get(item.getItemId());
            Toast.makeText(this, item.getTitle() + " selected", Toast.LENGTH_SHORT).show();
        }
        return true;
    }
}
