package ro.andob.jpegturbo;

import android.content.Context;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.StandardOpenOption;
import java.util.Objects;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.function.Function;

class FileIOUtils
{
    private final Context context;
    private final Consumer<Throwable> exceptionLogger;

    private static native int createPipeFileNative(String pipeFilePath);

    public FileIOUtils(Context context, Consumer<Throwable> exceptionLogger)
    {
        this.context = Objects.requireNonNull(context);
        this.exceptionLogger = Objects.requireNonNull(exceptionLogger);
    }

    File getFileWithUniqueName()
    {
        File file = new File(context.getCacheDir(), UUID.randomUUID().toString().replace("-", ""));
        return file.exists() ? getFileWithUniqueName() : file;
    }

    private File createPipeFile()
    {
        File pipeFile = getFileWithUniqueName();

        int resultCode = createPipeFileNative(pipeFile.getAbsolutePath());
        if (resultCode != JPEGTurbo.EXIT_SUCCESS)
            throw new RuntimeException("Cannot create pipe file!");

        return pipeFile;
    }

    File createOutputPipeFile(File outputFile)
    {
        File outputPipeFile = createPipeFile();

        new Thread(() ->
        {
//            try (BufferedSource source = Okio.buffer(Okio.source(outputPipeFile))) {
//                try (BufferedSink sink = Okio.buffer(Okio.sink(outputFile))) {
//                    sink.writeAll(source);
//                }
//            }
            try
            {
                Files.write(outputFile.toPath(), Files.readAllBytes(outputPipeFile.toPath()), StandardOpenOption.WRITE);
            }
            catch (Exception ex)
            {
                exceptionLogger.accept(ex);
            }
        }).start();

        return outputPipeFile;
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    <T> T useOutputPipeFile(File inputFile, Function<File, T> consumer)
    {
        File outputPipeFile = createOutputPipeFile(inputFile);
        try { return consumer.apply(outputPipeFile); }
        finally { outputPipeFile.delete(); }
    }

    File createInputPipeFile(File inputFile)
    {
        File inputPipeFile = createPipeFile();

        new Thread(() ->
        {
//            try (BufferedSink sink = Okio.buffer(Okio.sink(inputPipeFile))) {
//                try (BufferedSource source = Okio.buffer(Okio.source(inputFile))) {
//                    sink.writeAll(source);
//                }
//            }
            try
            {
                Files.write(inputPipeFile.toPath(), Files.readAllBytes(inputFile.toPath()), StandardOpenOption.WRITE);
            }
            catch (Exception ex)
            {
                exceptionLogger.accept(ex);
            }
        }).start();

        return inputPipeFile;
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    <T> T useInputPipeFile(File inputFile, Function<File, T> consumer)
    {
        File inputPipeFile = createInputPipeFile(inputFile);
        try { return consumer.apply(inputPipeFile); }
        finally { inputPipeFile.delete(); }
    }

    String readFileToString(File file)
    {
//        try (BufferedSink sink = Okio.buffer(Okio.sink(new ByteArrayOutputStream()))) {
//            try (BufferedSource source = Okio.buffer(Okio.source(file))) {
//                sink.writeAll(source);
//            }
//        }
        try
        {
            return new String(Files.readAllBytes(file.toPath()));
        }
        catch (Exception ex)
        {
            exceptionLogger.accept(ex);
            return "";
        }
    }
}
