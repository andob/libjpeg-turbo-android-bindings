package ro.andob.jpegturbo;

public final class JPEGTurbo
{
    public static void reencode(JPEGReencodeArgs args)
    {
        Reencoder.reencode(NativeImplementation.jpegTurbo(), args);
    }
}
