package ro.andob.jpegturbo.sample;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import ro.andob.jpegturbo.JPEGReencodeArgs;
import ro.andob.jpegturbo.JPEGTurbo;
import ro.andob.jpegturbo.Jpegli;
import ro.andob.jpegturbo.Mozjpeg;

public class MainActivity extends Activity
{
    private final File inputFile = App.context.getFileStreamPath("input.jpg");
    private final File outputFile = App.context.getFileStreamPath("output.jpg");

    private TextView textView;

    @Override
    @SuppressLint("SourceLockedOrientationActivity")
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        textView = new TextView(this);
        textView.setBackgroundColor(Color.BLACK);
        textView.setTextColor(Color.WHITE);
        setContentView(textView);

        new Handler(Looper.getMainLooper()).postDelayed(() ->
        {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT, FileProvider.getUriForFile(this, BuildConfig.APPLICATION_ID+".provider", inputFile));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            startActivityForResult(intent, 0);
        }, 1000);
    }

    private static class BenchmarkResults
    {
        public final long deltaTime;
        public final long oldFileSize;
        public final long newFileSize;

        public BenchmarkResults(long deltaTime, long oldFileSize, long newFileSize)
        {
            this.deltaTime = deltaTime;
            this.oldFileSize = oldFileSize;
            this.newFileSize = newFileSize;
        }

        @Override
        public @NonNull String toString()
        {
            return "Took: "+deltaTime+"ms\n"+
                   "Old file size: "+(((double)oldFileSize)/1024/1024)+"MB\n"+
                   "New file size: "+(((double)newFileSize)/1024/1024)+"MB\n";
        }
    }

    private BenchmarkResults benchmark(Runnable toRun)
    {
        long startTime = System.currentTimeMillis();
        toRun.run();
        long stopTime = System.currentTimeMillis();
        long deltaTime = stopTime-startTime;

        long oldFileSize = 0;
        try (FileInputStream inputStream = new FileInputStream(inputFile)) { oldFileSize = inputStream.available(); }
        catch (Exception ignored) {}

        long newFileSize = 0;
        try (FileInputStream inputStream = new FileInputStream(outputFile)) { newFileSize = inputStream.available(); }
        catch (Exception ignored) {}

        return new BenchmarkResults(deltaTime, oldFileSize, newFileSize);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == RESULT_OK)
        {
            new Thread(() ->
            {
                BenchmarkResults reference = benchmark(() ->
                {
                    Bitmap bitmap = BitmapFactory.decodeFile(inputFile.getAbsolutePath());
                    try (FileOutputStream outputStream = new FileOutputStream(outputFile)) {
                        bitmap.compress(Bitmap.CompressFormat.JPEG, 95, outputStream);
                    } catch (Exception ignored) {}
                });

                BenchmarkResults turbojpeg = benchmark(() ->
                    JPEGTurbo.reencode(JPEGReencodeArgs.with(this)
                        .inputFile(inputFile)
                        .outputFile(outputFile)
                        .optimize().verbose()
                        .errorLogger(Throwable::printStackTrace)
                        .warningLogger(Throwable::printStackTrace)));

                BenchmarkResults mozjpeg = benchmark(() ->
                    Mozjpeg.reencode(JPEGReencodeArgs.with(this)
                        .inputFile(inputFile)
                        .outputFile(outputFile)
                        .optimize().verbose()
                        .errorLogger(Throwable::printStackTrace)
                        .warningLogger(Throwable::printStackTrace)));

                BenchmarkResults jpegli = benchmark(() ->
                    Jpegli.reencode(JPEGReencodeArgs.with(this)
                        .inputFile(inputFile)
                        .outputFile(outputFile)
                        .optimize().verbose()
                        .errorLogger(Throwable::printStackTrace)
                        .warningLogger(Throwable::printStackTrace)));

                StringBuilder benchmarkResults = new StringBuilder();
                benchmarkResults.append("Reference:\n").append(reference).append("\n\n\n");
                benchmarkResults.append("Turbojpeg:\n").append(turbojpeg).append("\n\n\n");
                benchmarkResults.append("Mozjpeg:\n").append(mozjpeg).append("\n\n\n");
                benchmarkResults.append("Jpegli:\n").append(jpegli).append("\n\n\n");
                benchmarkResults.append(BuildConfig.DEBUG?" (DEBUG MODE! Release builds are much faster!)":"");

                runOnUiThread(() -> textView.setText(benchmarkResults));
            }).start();
        }
    }
}
