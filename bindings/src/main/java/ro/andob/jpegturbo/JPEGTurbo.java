package ro.andob.jpegturbo;

import java.io.File;
import java.nio.file.Files;
import java.util.UUID;

public class JPEGTurbo
{
    //from cdjpeg.h
    private static final int EXIT_FAILURE = 1;
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_WARNING = 2;

    private static final String ERROR_FILE_NAME = "jpeg_turbo_error.txt";

    static { System.loadLibrary("jpeg_turbo"); }

    public static native int reencodeNative(String inputFilePath, String outputFilePath, String errorFilePath,
        int quality, boolean progressive, boolean optimize, boolean verbose);

    public static native void mergeExifAndJpegNative(String inputExifFilePath, String inputImageFilePath, String outputFilePath);

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static void reencode(JPEGReencodeArgs args)
    {
        try
        {
            File errorFile = args.getContext().getFileStreamPath(ERROR_FILE_NAME);
            if (!errorFile.exists())
                errorFile.delete();

            String intermediateFileName = UUID.randomUUID().toString().replace("-", "");
            File intermediateFile = args.getContext().getFileStreamPath(intermediateFileName);
            if (!intermediateFile.exists())
                intermediateFile.delete();

            int resultCode = reencodeNative(
                /*inputFilePath*/ args.getInputFile().getAbsolutePath(),
                /*outputFilePath*/ intermediateFile.getAbsolutePath(),
                /*errorFilePath*/ errorFile.getAbsolutePath(),
                /*quality*/ args.getQuality(),
                /*progressive*/ args.isProgressive(),
                /*optimize*/ args.isOptimize(),
                /*verbose*/ args.isVerbose()
            );

            mergeExifAndJpegNative(
                /*inputExifFilePath*/ args.getInputFile().getAbsolutePath(),
                /*inputImageFilePath*/ intermediateFile.getAbsolutePath(),
                /*outputFilePath*/ args.getOutputFile().getAbsolutePath()
            );

            intermediateFile.delete();

            if (!args.getOutputFile().exists())
                throw new RuntimeException("Output file "+args.getOutputFile().getAbsolutePath()+" does not exist!");

            if (resultCode != EXIT_SUCCESS || args.isVerbose())
            {
                StringBuilder errorMessage = new StringBuilder("JPEGTurbo! ").append(args);

                errorMessage.append("\n\nResult code: ");
                if (resultCode == EXIT_SUCCESS) errorMessage.append("Success");
                else if (resultCode == EXIT_WARNING) errorMessage.append("Warning");
                else if (resultCode == EXIT_FAILURE) errorMessage.append("Failure");
                else errorMessage.append(resultCode);

                if (errorFile.exists())
                {
                    try { errorMessage.append("\n\n").append(new String(Files.readAllBytes(errorFile.toPath()))); }
                    catch (Throwable ex) { args.getWarningLogger().accept(ex); }
                }

                if (resultCode == EXIT_FAILURE)
                    throw new RuntimeException(errorMessage.toString());

                if (resultCode == EXIT_WARNING || args.isVerbose())
                    args.getWarningLogger().accept(new RuntimeException(errorMessage.toString()));
            }
        }
        catch (Throwable ex)
        {
            args.getErrorLogger().accept(ex);
            if (args.shouldThrowOnError())
                throw ex;
        }
    }
}
