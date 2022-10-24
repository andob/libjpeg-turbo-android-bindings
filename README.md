#### Experiments with progressive JPEG encoding on Android.

Benchmark:

- read a JPEG file, encode it to another file using lower quality and the standard Android SDK APIs (``Bitmap`` class - they also use jpeg-turbo, but not in progressive mode).
- read a JPEG file, encode it to another file using lower quality, progressive JPEG format and the binded jpeg-turbo library.

Benchmark results:

| Device            | CPU                  | Progressive JPEG | Standard JPEG |
|-------------------|----------------------|------------------|---------------|
| Pixel 5           | arm64-v8a (64 bit)   | 3.1s             | 200ms         |
| Motorola Moto G22 | arm64-v8a (64 bit)   | 5.8s             | 800ms         |
| Samsung XCover 4  | armeabi-v7a (32 bit) | 12s              | 1.5s          |
