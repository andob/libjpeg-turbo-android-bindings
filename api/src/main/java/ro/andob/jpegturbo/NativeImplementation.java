package ro.andob.jpegturbo;

interface NativeImplementation
{
    int reencode(String inputFilePath, String outputFilePath, String errorFilePath, boolean progressive, boolean optimize, boolean verbose);

    void mergeExifAndJpeg(String inputExifFilePath, String inputImageFilePath, String outputFilePath);

    int createPipeFile(String pipeFilePath);

    static NativeImplementation jpegTurbo()
    {
        try { Class.forName(JPEGTurboNative.class.getName()); }
        catch (ClassNotFoundException ex) { throw new RuntimeException(ex); }

        return new NativeImplementation()
        {
            @Override
            public int reencode(String inputFilePath, String outputFilePath, String errorFilePath, boolean progressive, boolean optimize, boolean verbose)
            {
                return JPEGTurboNative.reencode(inputFilePath, outputFilePath, errorFilePath, progressive, optimize, verbose);
            }

            @Override
            public void mergeExifAndJpeg(String inputExifFilePath, String inputImageFilePath, String outputFilePath)
            {
                JPEGTurboNative.mergeExifAndJpeg(inputExifFilePath, inputImageFilePath, outputFilePath);
            }

            @Override
            public int createPipeFile(String pipeFilePath)
            {
                return JPEGTurboNative.createPipeFile(pipeFilePath);
            }
        };
    }

    static NativeImplementation mozjpeg()
    {
        try { Class.forName(MozjpegNative.class.getName()); }
        catch (ClassNotFoundException ex) { throw new RuntimeException(ex); }

        return new NativeImplementation()
        {
            @Override
            public int reencode(String inputFilePath, String outputFilePath, String errorFilePath, boolean progressive, boolean optimize, boolean verbose)
            {
                return MozjpegNative.reencode(inputFilePath, outputFilePath, errorFilePath, progressive, optimize, verbose);
            }

            @Override
            public void mergeExifAndJpeg(String inputExifFilePath, String inputImageFilePath, String outputFilePath)
            {
                MozjpegNative.mergeExifAndJpeg(inputExifFilePath, inputImageFilePath, outputFilePath);
            }

            @Override
            public int createPipeFile(String pipeFilePath)
            {
                return MozjpegNative.createPipeFile(pipeFilePath);
            }
        };
    }
}
