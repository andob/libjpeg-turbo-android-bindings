package ro.andob.jpegturbo;

import ro.andob.jpegturbo.jpegturbo_native.BuildConfig;

final class JPEGTurboNative
{
    static { System.loadLibrary(BuildConfig.NATIVE_LIBRARY_NAME); }

    static native int reencode(String inputFilePath, String outputFilePath, String errorFilePath,
                               boolean progressive, boolean optimize, boolean verbose);

    static native int mergeExifAndJpeg(String inputExifFilePath, String inputImageFilePath, String outputFilePath);

    static native int createPipeFile(String pipeFilePath);
}
