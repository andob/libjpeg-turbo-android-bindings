package ro.andob.jpegturbo;

import android.content.Context;
import androidx.exifinterface.media.ExifInterface;
import java.io.File;
import java.io.IOException;
import java.util.LinkedList;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;

public class JPEGTranArgs
{
    public final Context context;
    private File inputFile;
    private File outputFile;
    private int quality = 85;
    private boolean progressive = false;
    private boolean optimize = false;
    private boolean rotateAccordingToExif = false;
    private boolean verbose = false;
    private Consumer<Throwable> errorLogger = Throwable::printStackTrace;
    private Consumer<Throwable> warningLogger = Throwable::printStackTrace;

    private JPEGTranArgs(Context context)
    {
        this.context = context;
    }

    public static JPEGTranArgs with(Context context)
    {
        return new JPEGTranArgs(context);
    }

    Context getContext()
    {
        return context;
    }

    public JPEGTranArgs inputFile(File inputFile)
    {
        this.inputFile = Objects.requireNonNull(inputFile);
        return this;
    }

    public JPEGTranArgs outputFile(File outputFile)
    {
        this.outputFile = Objects.requireNonNull(outputFile);
        return this;
    }

    public JPEGTranArgs quality(int quality)
    {
        this.quality = quality;
        return this;
    }

    int getQuality()
    {
        return Math.max(0, Math.min(100, quality));
    }

    public JPEGTranArgs progressive()
    {
        this.progressive = true;
        return this;
    }

    public JPEGTranArgs optimize()
    {
        this.optimize = true;
        return this;
    }

    public JPEGTranArgs rotateAccordingToExif()
    {
        this.rotateAccordingToExif = true;
        return this;
    }

    public JPEGTranArgs verbose()
    {
        this.verbose = true;
        return this;
    }

    boolean isVerbose()
    {
        return this.verbose;
    }

    public JPEGTranArgs errorLogger(Consumer<Throwable> errorLogger)
    {
        this.errorLogger = Objects.requireNonNull(errorLogger);
        return this;
    }

    Consumer<Throwable> getErrorLogger()
    {
        return errorLogger;
    }

    public JPEGTranArgs warningLogger(Consumer<Throwable> warningLogger)
    {
        this.warningLogger = Objects.requireNonNull(warningLogger);
        return this;
    }

    Consumer<Throwable> getWarningLogger()
    {
        return warningLogger;
    }

    List<String> getCommandLineArgs()
    {
        List<String> args = new LinkedList<>();

        if (progressive)
            args.add("-progressive");

        if (optimize)
            args.add("-optimize");

        if (rotateAccordingToExif)
        {
            int rotation = 0;
            try
            {
                int orientationAttribute = new ExifInterface(inputFile.getAbsolutePath())
                    .getAttributeInt(ExifInterface.TAG_ORIENTATION, ExifInterface.ORIENTATION_NORMAL);
                if (orientationAttribute == ExifInterface.ORIENTATION_ROTATE_90) rotation=90;
                else if (orientationAttribute == ExifInterface.ORIENTATION_ROTATE_180) rotation=180;
                else if (orientationAttribute == ExifInterface.ORIENTATION_ROTATE_270) rotation=270;
            }
            catch (IOException ex) { warningLogger.accept(ex); }
            if (rotation != 0)  { args.add("-rotate"); args.add(rotation+""); }
        }

        if (verbose)
            args.add("-verbose");

        if (outputFile == null)
            throw new RuntimeException("Please specify output file!");
        if (outputFile.getAbsolutePath().equals(inputFile.getAbsolutePath()))
            throw new RuntimeException("Please specify an output file different than the input file!");
        args.add("-outfile");
        args.add(outputFile.getAbsolutePath());

        if (inputFile == null)
            throw new RuntimeException("Please specify input file!");
        if (!inputFile.exists())
            throw new RuntimeException("Input file does not exist!");
        args.add(inputFile.getAbsolutePath());

        return args;
    }
}
