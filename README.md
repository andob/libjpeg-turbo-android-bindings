#### How to upgrade libjpeg-turbo version?

1. Go to JPEG-Turbo [releases](https://github.com/libjpeg-turbo/libjpeg-turbo/releases) and download latest release's source zip.
2. Delete ``src/main/cpp/libjpeg-turbo-<old_version>`` and extract the archive into a new directory ``src/main/cpp/libjpeg-turbo-<new_version>``.
3. Edit ``src/main/cpp/CMakeLists.txt``. Set ``LIBJPEG_TURBO_DIR`` variable to corresponding path. Set ``VERSION`` variable to the new version. Sync project.
4. Add header guards where needed: ``jmorecfg.h``, ``jpegint.h``, ``jdmerge.h``. For instance, at the beginning of ``jpegint.h`` file: ``#ifndef _JPEG_INT_H_`` (newline) ``#define _JPEG_INT_H_`` then at the end of the file: ``#endif``.
5. To remove ``JPEG_INTERNALS`` macro redefined warning, find ``#define JPEG_INTERNALS`` everywhere. Replace with ``#ifndef JPEG_INTERNALS`` (newline) ``#define JPEG_INTERNALS`` (newline) ``#endif``.
6. Build and fix compilation errors.
7. Test and fix runtime errors.
