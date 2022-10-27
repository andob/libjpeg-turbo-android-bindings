package ro.andob.jpegturbo;

import java.io.File;
import java.nio.file.Files;

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

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static void reencode(JPEGReencodeArgs args)
    {
        File errorFile = args.getContext().getFileStreamPath(ERROR_FILE_NAME);
        if (!errorFile.exists())
            errorFile.delete();

        //todo neither android's ExifInterface nor apache commons imaging won't save orientation properly
//        List<? extends ImageMetadata.ImageMetadataItem> exifProperties = Collections.emptyList();
//        try { exifProperties = Imaging.getMetadata(args.getInputFile()).getItems(); }
//        catch (ImageReadException | IOException ex) { args.getWarningLogger().accept(ex); }

        int resultCode = reencodeNative(
            /*inputFilePath*/ args.getInputFile().getAbsolutePath(),
            /*outputFilePath*/ args.getOutputFile().getAbsolutePath(),
            /*errorFilePath*/ errorFile.getAbsolutePath(),
            /*quality*/ args.getQuality(),
            /*progressive*/ args.isProgressive(),
            /*optimize*/ args.isOptimize(),
            /*verbose*/ args.isVerbose());

//        try (ByteArrayOutputStream baos = new ByteArrayOutputStream())
//        {
//            TiffOutputSet outputSet = new TiffOutputSet();
//            for (ImageMetadata.ImageMetadataItem item : exifProperties)
//            {
//                TiffOutputDirectory tiffOutputDirectory = outputSet.getOrCreateExifDirectory();
////                tiffOutputDirectory.add(((TiffField)item).getTagInfo(), ((TiffField)item).getValue());
//                TiffField inputField = ((TiffImageMetadata.TiffMetadataItem) item).getTiffField();
//                TiffOutputField outputField = new TiffOutputField(inputField.getTagInfo(), inputField.getFieldType(), (int)inputField.getCount(), inputField.getByteArrayValue());
//                tiffOutputDirectory.add(outputField);
//            }
//
//            new ExifRewriter().updateExifMetadataLossy(args.getOutputFile(), baos, outputSet);
//
//            Files.write(args.getOutputFile().toPath(), baos.toByteArray(), StandardOpenOption.WRITE);
//        }
//        catch (ImageReadException | ImageWriteException | IOException ex)  { args.getWarningLogger().accept(ex); }

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
            {
                RuntimeException ex = new RuntimeException(errorMessage.toString());
                args.getErrorLogger().accept(ex);
                if (args.shouldThrowOnError())
                    throw ex;
            }
            else if (resultCode == EXIT_WARNING || args.isVerbose())
            {
                args.getWarningLogger().accept(new RuntimeException(errorMessage.toString()));
            }
        }
    }
}
