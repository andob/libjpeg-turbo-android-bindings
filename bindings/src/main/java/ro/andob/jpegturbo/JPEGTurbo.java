package ro.andob.jpegturbo;

import java.io.File;
import java.nio.file.Files;
import java.util.List;

public class JPEGTurbo
{
    //from cdjpeg.h
    private static final int EXIT_FAILURE = 1;
    private static final int EXIT_SUCCESS = 0;
    private static final int EXIT_WARNING = 2;

    private static final String ERROR_FILE_NAME = "jpeg_turbo_error.txt";

    static { System.loadLibrary("jpeg_turbo"); }

    public static native int jpegtran(int quality, String... args);

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static void jpegtran(JPEGTranArgs args)
    {
        File errorFile = args.getContext().getFileStreamPath(ERROR_FILE_NAME);
        if (!errorFile.exists())
            errorFile.delete();

        List<String> rawCommandLineArgs = args.getCommandLineArgs();
        String[] commandLineArgs = new String[rawCommandLineArgs.size()+1];
        commandLineArgs[0] = errorFile.getAbsolutePath();
        for (int i = 0; i < rawCommandLineArgs.size(); i++)
            commandLineArgs[i+1] = rawCommandLineArgs.get(i);

        int resultCode = jpegtran(args.getQuality(), commandLineArgs);

        if (resultCode != EXIT_SUCCESS || args.isVerbose())
        {
            StringBuilder errorMessage = new StringBuilder("JPEGTurbo! ");
            for (String rawCommandLineArg : rawCommandLineArgs)
                errorMessage.append(rawCommandLineArg).append(" ");

            errorMessage.append('\n').append("Result code: ");
            if (resultCode == EXIT_SUCCESS) errorMessage.append("Success");
            else if (resultCode == EXIT_WARNING) errorMessage.append("Warning");
            else if (resultCode == EXIT_FAILURE) errorMessage.append("Failure");
            else errorMessage.append(resultCode);

            if (errorFile.exists())
            {
                try { errorMessage.append('\n').append(new String(Files.readAllBytes(errorFile.toPath()))); }
                catch (Throwable ex) { args.getWarningLogger().accept(ex); }
            }

            if (resultCode == EXIT_FAILURE)
            {
                RuntimeException ex = new RuntimeException(errorMessage.toString());
                args.getErrorLogger().accept(ex);
                throw ex;
            }
            else if (resultCode == EXIT_WARNING || args.isVerbose())
            {
                args.getWarningLogger().accept(new RuntimeException(errorMessage.toString()));
            }
        }
    }
}
