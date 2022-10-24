package ro.andob.jpegturbo;

import android.content.Context;
import java.io.File;
import java.nio.file.Files;
import java.util.Objects;

public class JPEGTurbo
{
    static { System.loadLibrary("jpeg_turbo"); }

    //from cdjpeg.h
    private static final int EXIT_FAILURE = 1;
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_WARNING = 2;

    public interface ExceptionLogger { void log(Throwable ex); }

    private static ExceptionLogger errorLogger = Throwable::printStackTrace;
    public static void setErrorLogger(ExceptionLogger logger) { errorLogger = Objects.requireNonNull(logger); }

    private static ExceptionLogger warningLogger = Throwable::printStackTrace;
    public static void setWarningLogger(ExceptionLogger logger) { warningLogger = Objects.requireNonNull(logger); }

    private static native int jpegtran(String[] args, int quality);

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static void jpegtran2(Context context, int quality, String... args)
    {
        File errorFile = context.getFileStreamPath("jpeg_turbo_last_error.txt");
        if (!errorFile.exists())
            errorFile.delete();

        String[] argsForC = new String[args.length+1];
        argsForC[0] = errorFile.getAbsolutePath();
        System.arraycopy(args, 0, argsForC, 1, args.length);

        int resultCode = jpegtran(argsForC, quality);
        if (resultCode != EXIT_SUCCESS)
        {
            String errorMessage = "";
            if (errorFile.exists())
            {
                try { errorMessage = new String(Files.readAllBytes(errorFile.toPath())); }
                catch (Throwable ignored) {}
            }

            if (resultCode == EXIT_FAILURE)
            {
                RuntimeException ex = new RuntimeException(errorMessage);
                errorLogger.log(ex);
                throw ex;
            }
            else if (resultCode == EXIT_WARNING)
            {
                RuntimeException ex = new RuntimeException(errorMessage);
                warningLogger.log(ex);
            }
        }
    }
}
