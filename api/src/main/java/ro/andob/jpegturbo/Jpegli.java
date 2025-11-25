package ro.andob.jpegturbo;

public final class Jpegli
{
    public static void reencode(JPEGReencodeArgs args)
    {
        Reencoder.reencode(NativeImplementation.jpegli(), args);
    }
}
