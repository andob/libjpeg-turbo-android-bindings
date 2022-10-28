package ro.andob.jpegturbo.sample;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.widget.ImageView;
import android.widget.Toast;
import androidx.core.content.FileProvider;
import com.bumptech.glide.Glide;
import com.bumptech.glide.load.engine.DiskCacheStrategy;
import java.io.File;
import java.io.FileOutputStream;
import ro.andob.jpegturbo.JPEGReencodeArgs;
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

    private static long benchmark(Runnable toRun)
    {
        long startTime = System.currentTimeMillis();
        toRun.run();
        long stopTime = System.currentTimeMillis();
        return stopTime-startTime;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == RESULT_OK)
        {
            new Thread(() ->
            {
                long systemTime = benchmark(() ->
                {
                    Bitmap bitmap=BitmapFactory.decodeFile(inputFile.getAbsolutePath());
                    try (FileOutputStream outputStream=new FileOutputStream(outputFile)) {
                        bitmap.compress(Bitmap.CompressFormat.JPEG, 85, outputStream);
                    } catch (Exception ignored) {}
                });

                long turboBaselineTime = benchmark(() ->
                {
                    JPEGTurbo.reencode(JPEGReencodeArgs.with(this)
                        .inputFile(inputFile)
                        .outputFile(outputFile)
                        .quality(85)
                        .optimize()
                        .verbose()
                        .errorLogger(Throwable::printStackTrace)
                        .warningLogger(Throwable::printStackTrace));
                });

                long turboProgressiveTime = benchmark(() ->
                {
                    JPEGTurbo.reencode(JPEGReencodeArgs.with(this)
                        .inputFile(inputFile)
                        .outputFile(outputFile)
                        .quality(85)
                        .optimize()
                        .progressive()
                        .verbose()
                        .errorLogger(Throwable::printStackTrace)
                        .warningLogger(Throwable::printStackTrace));
                });

                StringBuilder benchmarkResults = new StringBuilder();
                benchmarkResults.append("Sys:").append(systemTime).append("ms");
                benchmarkResults.append(" TJB:").append(turboBaselineTime).append("ms");
                benchmarkResults.append(" TJP:").append(turboProgressiveTime).append("ms");
                benchmarkResults.append(BuildConfig.DEBUG?" (DEBUG MODE! Release builds are much faster!)":"");

                runOnUiThread(() ->
                {
                    Toast.makeText(App.context, benchmarkResults.toString(), Toast.LENGTH_LONG).show();

                    Glide.with(imageView)
                        .load(outputFile)
                        .diskCacheStrategy(DiskCacheStrategy.NONE)
                        .skipMemoryCache(true)
                        .into(imageView);
                });
            }).start();
        }
    }
}
