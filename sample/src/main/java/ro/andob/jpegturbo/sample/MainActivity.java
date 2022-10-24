package ro.andob.jpegturbo.sample;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.widget.Toast;

import androidx.core.content.FileProvider;

import com.gemalto.jp2.JP2Encoder;

import java.io.File;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.OpenOption;
import java.nio.file.StandardOpenOption;
import java.util.function.Supplier;

import ro.andob.jpegturbo.JPEGTurbo;

public class MainActivity extends Activity
{
    private final Supplier<File> inputFile = () -> getFileStreamPath("input.jpg");
    private final Supplier<File> inputRawFile = () -> getFileStreamPath("inputRaw.dat");
    private final Supplier<File> outputFile = () -> getFileStreamPath("output.jpg");

    @Override
    @SuppressWarnings("Convert2MethodRef")
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        JPEGTurbo.setErrorLogger(ex -> ex.printStackTrace());
        JPEGTurbo.setWarningLogger(ex -> ex.printStackTrace());

        new Handler(Looper.getMainLooper()).postDelayed(() ->
        {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            intent.putExtra(MediaStore.EXTRA_OUTPUT, FileProvider.getUriForFile(this, BuildConfig.APPLICATION_ID+".provider", inputFile.get()));
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            startActivityForResult(intent, 0);
        }, 3000);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == RESULT_OK)
        {
            new Thread(() ->
            {
                try
                {
//                    JPEGTurbo.jpegtran(this,
//                        "-optimize", "-progressive",
//                        "-outfile", outputFile.get().getAbsolutePath(),
//                        inputFile.get().getAbsolutePath()
//                    );

//                    Bitmap bitmap = BitmapFactory.decodeFile(inputFile.get().getAbsolutePath());
//                    long startTime = System.currentTimeMillis();
//                    int statusCode = JPEGTurbo.cjpeg(bitmap, 85, outputFile.get().getAbsolutePath(), true, true);
//                    long stopTime = System.currentTimeMillis();
//                    long deltaTime = stopTime-startTime;
//                    runOnUiThread(() -> Toast.makeText(this, "SC = "+statusCode+", Time = "+deltaTime, Toast.LENGTH_LONG).show());
//                    bitmap.recycle();

                    Bitmap bitmap = BitmapFactory.decodeFile(inputFile.get().getAbsolutePath());
                    long startTime = System.currentTimeMillis();
                    byte[] jp2data = new JP2Encoder(bitmap)
                            .setVisualQuality(85)
                            .encode();
                    long stopTime = System.currentTimeMillis();
                    long deltaTime = stopTime-startTime;
                    runOnUiThread(() -> Toast.makeText(this, "Time = "+deltaTime, Toast.LENGTH_LONG).show());
                    bitmap.recycle();
                }
                catch (Throwable ex)
                {
                    runOnUiThread(() -> Toast.makeText(this, ex.getMessage(), Toast.LENGTH_LONG).show());
                }
            }).start();
        }
    }
}
