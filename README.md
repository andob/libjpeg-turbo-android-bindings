## Android binding library on jpegtran command from [jpeg-turbo C library](https://github.com/libjpeg-turbo/libjpeg-turbo)

"libjpeg-turbo is a JPEG image codec that uses SIMD instructions to accelerate baseline JPEG compression and decompression on x86, x86-64, Arm, PowerPC, and MIPS systems, as well as progressive JPEG compression on x86, x86-64, and Arm systems. On such systems, libjpeg-turbo is generally 2-6x as fast as libjpeg, all else being equal."

This library provides Android bindings on a jpegtran-like program. You can use it as an alternative to the standard Android SDK Bitmap class, to transform JPEG files.

### Usage

Import it with:

```
repositories {
    maven { url "https://maven.andob.info/repository/open_source" }
}
```

```
android {
    defaultConfig {
        externalNativeBuild {
            cmake {
                arguments "-DANDROID_ARM_NEON=ON"
            }
        }
    }
}

dependencies {
    implementation 'ro.andob.jpegturbo:jpegturbo-bindings:1.0.0'
}
```

### Usage

Basic usage:

```java
File inputFile = ...;
File outputFile = ...; //outputFile must be different than inputFile
JPEGTurbo.jpegtran(JPEGTranArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile));
```

Usage with optional features:

```java
JPEGTurbo.jpegtran(JPEGTranArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .quality(85) //image quality, from 0 to 100
    .progressive() //encode to progressive JPEG instead of normal JPEG
    .optimize() //optimize encoded JPEG
    .errorLogger(Throwable::printStackTrace) //provide an error logger
    .warningLogger(Throwable::printStackTrace)); //provide a warning logger
```

One can use optional featurs in any combination, for instance:

```java
JPEGTurbo.jpegtran(JPEGTranArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .quality(90)
    .progressive());
```

```java
JPEGTurbo.jpegtran(JPEGTranArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .quality(100)
    .rotateAccordingToExif());
```

It is recommended to run this method on a background thread.

If there is an error, the errorLogger will be called, an exception will be thrown. If there was a warning, the warningLogger will be called. Only errors are turned into exceptions.

Note that debug builds are much slower and do not benefit from SIMD optimisations. Only use RELEASE builds to test performance (only on release builds SIMD instructions gets properly compiled into the APK).

### Rationale

1. While standard Android SDK Bitmap class is fine, jpeg-turbo is faster, because it uses SIMD (ARM NEON) instructions.
2. Bitmap class is not able to compress into a progressive JPEG, only standard JPEG. This library can encode into standard JPEGs, as well as progressive JPEGs. A JPEG file is a kind of image which can be rendered while it gets downloaded.  On Android ecosystem, the only image loader that can display JPEGs progressively is Facebook's [Fresco](https://github.com/facebook/fresco). All other libs, Glide, Coil, Picasso will download the JPEG file and then will render it. On slow internet connections (or when loading lots of images), loading progressive JPEGs with Fresco will make seem that they are loading faster (they are displayed faster, as they are downloaded - see [this demo](https://frescolib.org/docs/progressive-jpegs.html)).
3. Edge computing / client side processing. Instead of transforming images on the server, after they are uploaded to the server, one can transform the files on device, before uploading them. My plan is to use this library to on-device transform images from phone's camera into progressive JPEGs, and then upload them to the server. Later on, when images will be downloaded from the server, Fresco will be used to render them progressively. Also most browsers render progressive JPEGs as they are downloaded.

#### How to upgrade libjpeg-turbo version?

1. Go to JPEG-Turbo [releases](https://github.com/libjpeg-turbo/libjpeg-turbo/releases) and download latest release's source zip.
2. Delete ``src/main/cpp/libjpeg-turbo-<old_version>`` and extract the archive into a new directory ``src/main/cpp/libjpeg-turbo-<new_version>``.
3. Edit ``src/main/cpp/CMakeLists.txt``. Set ``LIBJPEG_TURBO_DIR`` variable to corresponding path. Set ``VERSION`` variable to the new version. Sync project.
4. Add header guards where needed: ``jmorecfg.h``, ``jpegint.h``, ``jdmerge.h``. For instance, at the beginning of ``jpegint.h`` file: ``#ifndef _JPEG_INT_H_`` (newline) ``#define _JPEG_INT_H_`` then at the end of the file: ``#endif``.
5. To remove ``JPEG_INTERNALS`` macro redefined warning, find ``#define JPEG_INTERNALS`` everywhere. Replace with ``#ifndef JPEG_INTERNALS`` (newline) ``#define JPEG_INTERNALS`` (newline) ``#endif``.
6. Build and fix compilation errors.
7. Test and fix runtime errors.

### License

```
Copyright 2022 Andrei Dobrescu

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
