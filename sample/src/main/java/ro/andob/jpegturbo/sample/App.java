package ro.andob.jpegturbo.sample;

import android.annotation.SuppressLint;
import android.app.Application;
import android.content.Context;

public class App extends Application
{
    @SuppressLint("StaticFieldLeak")
    public static Context context;

    @Override
    public void onCreate()
    {
        super.onCreate();

        App.context = this;
    }
}
