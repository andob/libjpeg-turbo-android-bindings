package ro.andob.jpegturbo;

import java.io.File;

public class JPEGTurbo
{
    //from cdjpeg.h
    static final int EXIT_FAILURE = 1;
    static final int EXIT_SUCCESS = 0;
    static final int EXIT_WARNING = 2;

    private static final String ERROR_FILE_NAME = "jpeg_turbo_error.txt";

    static { System.loadLibrary("jpeg_turbo"); }

    private static native int reencodeNative(String inputFilePath, String outputFilePath, String errorFilePath,
        boolean progressive, boolean optimize, boolean verbose);

    private static native void mergeExifAndJpegNative(String inputExifFilePath, String inputImageFilePath, String outputFilePath);

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static void reencode(JPEGReencodeArgs args)
    {
        FileIOUtils fileIOUtils = new FileIOUtils(args.getContext(), args.getWarningLogger());

        File errorFile = fileIOUtils.getFileWithUniqueName();
        File intermediateFile = fileIOUtils.getFileWithUniqueName();

        try
        {
            int resultCode = fileIOUtils.useInputPipeFile(args.getInputFile(),
                inputPipeFile -> reencodeNative(
                    /*inputFilePath*/ inputPipeFile.getAbsolutePath(),
                    /*outputFilePath*/ intermediateFile.getAbsolutePath(),
                    /*errorFilePath*/ errorFile.getAbsolutePath(),
                    /*progressive*/ args.isProgressive(),
                    /*optimize*/ args.isOptimize(),
                    /*verbose*/ args.isVerbose()
                )
            );

            mergeExifAndJpegNative(
                /*inputExifFilePath*/ args.getInputFile().getAbsolutePath(),
                /*inputImageFilePath*/ intermediateFile.getAbsolutePath(),
                /*outputFilePath*/ args.getOutputFile().getAbsolutePath()
            );

            if (!args.getOutputFile().exists())
                throw new RuntimeException("Output file "+args.getOutputFile().getAbsolutePath()+" does not exist!");

            if (resultCode != EXIT_SUCCESS || args.isVerbose())
            {
                StringBuilder errorMessageBuilder = new StringBuilder("JPEGTurbo!\n").append(args);

                errorMessageBuilder.append("\n\nResult code: ");
                if (resultCode == EXIT_SUCCESS) errorMessageBuilder.append("Success");
                else if (resultCode == EXIT_WARNING) errorMessageBuilder.append("Warning");
                else if (resultCode == EXIT_FAILURE) errorMessageBuilder.append("Failure");
                else errorMessageBuilder.append(resultCode);

                if (errorFile.exists())
                    errorMessageBuilder.append("\n\n").append(fileIOUtils.readFileToString(errorFile));

                String errorMessage = errorMessageBuilder.toString();
                if (resultCode == EXIT_FAILURE)
                    throw new RuntimeException(errorMessage);

                if (errorMessage.contains("Corrupt JPEG data"))
                    throw new RuntimeException(errorMessage);

                if (resultCode == EXIT_WARNING || args.isVerbose())
                    args.getWarningLogger().accept(new RuntimeException(errorMessage));
            }
        }
        catch (Throwable ex)
        {
            args.getErrorLogger().accept(ex);

            if (args.getOutputFile().exists())
                args.getOutputFile().delete();

            if (args.shouldThrowOnError())
                throw ex;
        }
        finally
        {
            if (intermediateFile.exists())
                intermediateFile.delete();

            if (errorFile.exists())
                errorFile.delete();
        }
    }
}
