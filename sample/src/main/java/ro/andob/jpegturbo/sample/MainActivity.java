package ro.andob.jpegturbo.sample;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.widget.ImageView;
import android.widget.Toast;
import androidx.core.content.FileProvider;
import java.io.File;
import ro.andob.jpegturbo.JPEGTranArgs;
import ro.andob.jpegturbo.JPEGTurbo;

public class MainActivity extends Activity
{
    private final File inputFile = App.context.getFileStreamPath("input.jpg");
    private final File outputFile = App.context.getFileStreamPath("output.jpg");

    private ImageView imageView;

    @Override
    @SuppressLint("SourceLockedOrientationActivity")
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        imageView = new ImageView(this);
        imageView.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        imageView.setBackgroundColor(Color.BLACK);
        setContentView(imageView);

        new Handler(Looper.getMainLooper()).postDelayed(() ->
        {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT, FileProvider.getUriForFile(this, BuildConfig.APPLICATION_ID+".provider", inputFile));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            startActivityForResult(intent, 0);
        }, 1000);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == RESULT_OK)
        {
            new Thread(() ->
            {
                long startTime = System.currentTimeMillis();
                JPEGTurbo.jpegtran(JPEGTranArgs.with(this)
                    .inputFile(inputFile)
                    .outputFile(outputFile)
                    .quality(85)
                    .progressive()
                    .optimize()
                    .rotateAccordingToExif()
                    .verbose()
                    .errorLogger(Throwable::printStackTrace)
                    .warningLogger(Throwable::printStackTrace));
                long stopTime = System.currentTimeMillis();
                long deltaTime = stopTime-startTime;

                runOnUiThread(() ->
                {
                    imageView.setImageBitmap(BitmapFactory.decodeFile(outputFile.getAbsolutePath()));
                    Toast.makeText(App.context, deltaTime+"ms"+(BuildConfig.DEBUG?" (DEBUG MODE! Release builds are much faster!)":""), Toast.LENGTH_LONG).show();
                });
            }).start();
        }
    }
}
