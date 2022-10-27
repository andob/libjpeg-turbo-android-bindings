package ro.andob.jpegturbo;

import android.content.Context;
import java.io.File;
import java.util.Objects;
import java.util.function.Consumer;

public class JPEGReencodeArgs
{
    private final Context context;
    private File inputFile;
    private File outputFile;
    private int quality = 85;
    private boolean progressive = false;
    private boolean optimize = false;
    private boolean verbose = false;
    private boolean shouldThrowOnError = true;
    private Consumer<Throwable> errorLogger = Throwable::printStackTrace;
    private Consumer<Throwable> warningLogger = Throwable::printStackTrace;

    private JPEGReencodeArgs(Context context)
    {
        this.context = context;
    }

    public static JPEGReencodeArgs with(Context context)
    {
        return new JPEGReencodeArgs(context);
    }

    public JPEGReencodeArgs inputFile(File inputFile)
    {
        this.inputFile = Objects.requireNonNull(inputFile);
        return this;
    }

    public JPEGReencodeArgs outputFile(File outputFile)
    {
        this.outputFile = Objects.requireNonNull(outputFile);
        return this;
    }

    public JPEGReencodeArgs inputOutputFile(File file)
    {
        this.inputFile = Objects.requireNonNull(file);
        this.outputFile = Objects.requireNonNull(file);
        return this;
    }

    public JPEGReencodeArgs quality(int quality)
    {
        this.quality = quality;
        return this;
    }

    public JPEGReencodeArgs progressive()
    {
        this.progressive = true;
        return this;
    }

    public JPEGReencodeArgs optimize()
    {
        this.optimize = true;
        return this;
    }

    public JPEGReencodeArgs verbose()
    {
        this.verbose = true;
        return this;
    }

    public JPEGReencodeArgs shouldNotThrowOnError()
    {
        this.shouldThrowOnError = false;
        return this;
    }

    public JPEGReencodeArgs errorLogger(Consumer<Throwable> errorLogger)
    {
        this.errorLogger = Objects.requireNonNull(errorLogger);
        return this;
    }

    public JPEGReencodeArgs warningLogger(Consumer<Throwable> warningLogger)
    {
        this.warningLogger = Objects.requireNonNull(warningLogger);
        return this;
    }

    Context getContext()
    {
        return context;
    }

    File getInputFile()
    {
        if (inputFile == null)
            throw new RuntimeException("Please specify inputFile!");
        if (!inputFile.exists())
            throw new RuntimeException("Input file "+inputFile.getAbsolutePath()+" does not exist!");
        return inputFile;
    }

    File getOutputFile()
    {
        if (outputFile == null)
            throw new RuntimeException("Please specify outputFile!");
        return outputFile;
    }

    int getQuality()
    {
        return quality;
    }

    boolean isProgressive()
    {
        return progressive;
    }

    boolean isOptimize()
    {
        return optimize;
    }

    boolean isVerbose()
    {
        return verbose;
    }

    boolean shouldThrowOnError()
    {
        return shouldThrowOnError;
    }

    Consumer<Throwable> getErrorLogger()
    {
        if (errorLogger == null)
            throw new RuntimeException("Please specify errorLogger!");
        return errorLogger;
    }

    Consumer<Throwable> getWarningLogger()
    {
        if (warningLogger == null)
            throw new RuntimeException("Please specify warningLogger!");
        return warningLogger;
    }

    @Override
    public String toString()
    {
        return "JPEGTransformArgs{" +
                ", inputFile=" + inputFile.getAbsolutePath() +
                ", outputFile=" + outputFile.getAbsolutePath() +
                ", quality=" + quality +
                ", progressive=" + progressive +
                ", optimize=" + optimize +
                ", verbose=" + verbose +
                '}';
    }
}
