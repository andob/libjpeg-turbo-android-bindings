package ro.andob.jpegturbo;

public final class Mozjpeg
{
    public static void reencode(JPEGReencodeArgs args)
    {
        Reencoder.reencode(NativeImplementation.mozjpeg(), args);
    }
}
