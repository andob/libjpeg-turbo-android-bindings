### How to upgrade JPEG-Turbo version?

1. Go to JPEG-Turbo [releases](https://github.com/libjpeg-turbo/libjpeg-turbo/releases) and download latest release's source zip.
2. Delete ``src/main/cpp/libjpeg-turbo-<old_version>`` and extract the archive into a new directory ``src/main/cpp/libjpeg-turbo-<new_version>``.
3. Edit ``src/main/cpp/CMakeLists.txt``. Set ``LIBJPEG_TURBO_DIR`` variable to corresponding path. Set ``VERSION`` variable to the new version. Sync project.
4. Add header guards where needed (``jmorecfg.h``, ``jpegint.h``, ``jdmerge.h``, ``jerror.h``, ``jdmerge.h``). For instance, at the beginning of ``jpegint.h`` file: ``#ifndef _JPEG_INT_H_`` (newline) ``#define _JPEG_INT_H_`` then at the end of the file: ``#endif``.
5. Find ``#ifndef JPEG_INTERNALS`` everywhere. Replace with ``#ifndef JPEG_INTERNALS (newline)`` ``#define JPEG_INTERNALS`` (newline) ``#endif``.
6. In ``jpegtran.c``, rename ``main`` to ``jpegtran_main``.
7. Add binding code at the end of ``jpegtran.c`` file (Annex 1).
8. Modify ``jpegtran.c`` file. Remove all ``exit(int)`` function calls. Remove all ``stdin``, ``stdout`` calls. Redirect ``stderr`` to a file.
9. Build project. Resolve build errors.
10. Test project.

Annex 1:
```c
#include <jni.h>
#include <unistd.h>
JNIEXPORT int JNICALL
Java_ro_andob_jpegturbo_JPEGTurbo_jpegtran(JNIEnv* env, jclass clazz, jobjectArray argsFromJava)
{
    int argc = (*env)->GetArrayLength(env, argsFromJava);
    char** argv = malloc(sizeof(char*)*argc);
    for (int i=0; i<argc; i++)
    {
        jstring argFromJava = (*env)->GetObjectArrayElement(env, argsFromJava, i);
        const char* rawArgFromJava = (*env)->GetStringUTFChars(env, argFromJava, 0);
        char* arg = malloc(sizeof(char)*(strlen(rawArgFromJava)+1));
        strcpy(arg, rawArgFromJava);
        (*env)->ReleaseStringUTFChars(env, argFromJava, rawArgFromJava);
        argv[i] = arg;
    }

    FILE* errorFile = fopen(argv[0], "w");
    dup2(fileno(errorFile), STDERR_FILENO);

    int resultCode = jpegtran_main(argc, argv);
    for (int i=0; i<argc; i++)
        free(argv[i]);
    free(argv);
    fclose(errorFile);
    return resultCode;
}

```
