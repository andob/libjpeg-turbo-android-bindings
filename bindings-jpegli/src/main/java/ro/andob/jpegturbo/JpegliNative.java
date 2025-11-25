package ro.andob.jpegturbo;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import io.github.awxkee.jpegli.coder.IccStrategy;
import io.github.awxkee.jpegli.coder.JpegliCoder;
import io.github.awxkee.jpegli.coder.Scalar;
import ro.andob.jpegturbo.jpegli_native.BuildConfig;

final class JpegliNative
{
    static { System.loadLibrary(BuildConfig.NATIVE_LIBRARY_NAME); }

    static final int EXIT_SUCCESS = 0, EXIT_FAILURE = 1;

    static int reencode(String inputFilePath, String outputFilePath, String errorFilePath,
                               boolean progressive, boolean optimize, boolean verbose)
    {
        try (FileOutputStream outputStream = new FileOutputStream(outputFilePath))
        {
            Bitmap inputBitmap = BitmapFactory.decodeFile(inputFilePath);
            JpegliCoder.Companion.compress(inputBitmap, 90, IccStrategy.DEFAULT,
                Scalar.Companion.getZERO(), progressive, outputStream);
        }
        catch (Throwable ex)
        {
            String exceptionAsString = String.format("%s\n%s", ex.getMessage(), Log.getStackTraceString(ex));
            try { Files.write(new File(errorFilePath).toPath(), exceptionAsString.getBytes()); }
            catch (IOException ignored) {}
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    static native int mergeExifAndJpeg(String inputExifFilePath, String inputImageFilePath, String outputFilePath);

    static native int createPipeFile(String pipeFilePath);
}
