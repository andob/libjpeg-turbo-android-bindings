package ro.andob.jpegturbo.sample;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.provider.MediaStore;
import android.widget.Toast;
import androidx.core.content.FileProvider;
import java.io.File;
import java.io.FileOutputStream;
import java.util.function.Supplier;
import ro.andob.jpegturbo.JPEGTurbo;

public class MainActivity extends Activity
{
    private final Supplier<File> inputFile = () -> new File(getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), "input.jpg");
    private final Supplier<File> outputFile = () -> new File(getExternalFilesDir(Environment.DIRECTORY_DOCUMENTS), "output.jpg");

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
                int quality = 85;

                long startTime1 = System.currentTimeMillis();
                JPEGTurbo.jpegtran2(this, 85,
                    "-progressive",
                    "-outfile", outputFile.get().getAbsolutePath(),
                    inputFile.get().getAbsolutePath()
                );
                long stopTime1 = System.currentTimeMillis();
                long deltaTime1 = stopTime1 - startTime1;

                long startTime2 = System.currentTimeMillis();
                JPEGTurbo.jpegtran2(this, 85,
                    "-outfile", outputFile.get().getAbsolutePath(),
                    inputFile.get().getAbsolutePath()
                );
                long stopTime2 = System.currentTimeMillis();
                long deltaTime2 = stopTime2 - startTime2;

                runOnUiThread(() ->
                {
                    String message = "JPEGTRAN: "+deltaTime1+"ms"+", System: "+deltaTime2+"ms";
                    Toast.makeText(this, message, Toast.LENGTH_LONG).show();
                });
            }).start();
        }
    }
}
