## Android bindings for [jpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo) and [mozjpeg](https://github.com/mozilla/mozjpeg)

"libjpeg-turbo is a JPEG image codec that uses SIMD instructions to accelerate baseline JPEG compression and decompression on x86, x86-64, Arm, PowerPC, and MIPS systems, as well as progressive JPEG compression on x86, x86-64, and Arm systems. On such systems, libjpeg-turbo is generally 2-6x as fast as libjpeg, all else being equal."

"MozJPEG improves JPEG compression efficiency achieving higher visual quality and smaller file sizes at the same time. It is compatible with the JPEG standard, and the vast majority of the world's deployed JPEG decoders."

This library provides Android bindings on jpeg-turbo and mozjpeg. You can use it as an alternative to the standard Bitmap class, to decode and re-encode JPEG files.

### Usage

Import it with:

```
repositories {
    maven { url "https://andob.io/repository/open_source" }
}
```

```
dependencies {
    implementation 'ro.andob.jpegturbo:bindings-api:1.1.2'
    implementation 'ro.andob.jpegturbo:bindings-jpegturbo:1.1.2'
    implementation 'ro.andob.jpegturbo:bindings-mozjpeg:1.1.2'
}
```

To import only jpegturbo, import only api and jpegturbo modules. To import only mozjpeg, import only api and mozjpeg modules. To import both jpegturbo and mozjpeg, import all 3 modules, api, jpegturbo and mozjpeg.

### Usage

Basic usage:

```java
File inputFile = ...;
File outputFile = ...; //outputFile may be same as inputFile
JPEGTurbo.reencode(JPEGReencodeArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile));
```

Or:

```java
File inputFile = ...;
File outputFile = ...; //outputFile may be same as inputFile
Mozjpeg.reencode(JPEGReencodeArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile));
```

Usage with optional features:

```java
JPEGTurbo.reencode(JPEGReencodeArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .progressive() //encode to progressive JPEG instead of baseline JPEG
    .optimize() //optimize encoded JPEG
    .errorLogger(Throwable::printStackTrace) //provide an error logger
    .warningLogger(Throwable::printStackTrace)); //provide a warning logger
```

One can use optional featurs in any combination, for instance:

```java
JPEGTurbo.reencode(JPEGReencodeArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .quality(90)
    .progressive());
```

```java
JPEGTurbo.reencode(JPEGReencodeArgs.with(context)
    .inputFile(inputFile)
    .outputFile(outputFile)
    .quality(100));
```

It is recommended to run this method on a background thread.

The library splits exceptional cases into errors and warnings:

- If there is an error, the ``errorLogger`` callback will be called and the ``reencode`` method will throw an exception. You can modify this behavior by calling ``shouldNotThrowOnError()`` on ``JPEGReencodeArgs``.
- If there is a warning, the ``reencode`` method will not throw exception. Also, the ``warningLogger`` callback will be called.

Note that debug builds are much slower and do not benefit from SIMD optimisations. Only use RELEASE builds to test performance (only on release builds SIMD instructions gets properly compiled into the APK).

### Rationale

1. While standard Android SDK Bitmap class is fine, jpeg-turbo is faster, because it uses SIMD (ARM NEON) instructions.
2. While mozjpeg is bit slower than jpeg-turbo and the Bitmap class, it outputs smaller JPEG images.
3. Android SDK's Bitmap class is not able to compress into a progressive JPEG, only baseline JPEG. This library can encode into baseline JPEGs, as well as progressive JPEGs. A progressive JPEG file is a kind of image which can be rendered while it gets downloaded.
4. Edge computing / client side processing. Instead of transforming images on the server, after they are uploaded to the server, one can transform the files on device, before uploading them (saves both bandwidth and server disk space).

#### (Note for myself) How to upgrade libjpeg-turbo version?

1. Go to JPEG-Turbo [releases](https://github.com/libjpeg-turbo/libjpeg-turbo/releases) and download latest release's source zip.
2. Delete ``native/libjpeg-turbo-<old_version>`` and extract the archive into a new directory ``native/libjpeg-turbo-<new_version>``.
3. Edit ``native/CMakeLists.txt``. Set ``LIBJPEG_TURBO_DIR`` variable to corresponding path. Set ``VERSION`` variable to the new version. Sync project.
4. Add header guards where needed: ``jmorecfg.h``, ``jpegint.h``, ``jdmerge.h``. For instance, at the beginning of ``jpegint.h`` file: ``#ifndef _JPEG_INT_H_`` (newline) ``#define _JPEG_INT_H_`` then at the end of the file: ``#endif``.
5. To remove ``JPEG_INTERNALS`` macro redefined warning, find ``#define JPEG_INTERNALS`` everywhere. Replace with ``#ifndef JPEG_INTERNALS`` (newline) ``#define JPEG_INTERNALS`` (newline) ``#endif``.
6. Build and fix compilation errors.
7. Test and fix runtime errors.

#### (Note for myself) How to upgrade mozjpeg version?

Similar to jpeg-turbo upgrade.

### License

```
Copyright 2022-2023 Andrei Dobrescu

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
