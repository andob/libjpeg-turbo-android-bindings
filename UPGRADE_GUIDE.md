### How to upgrade JPEG-Turbo version?

1. Go to JPEG-Turbo [releases](https://github.com/libjpeg-turbo/libjpeg-turbo/releases) and download latest release's source zip.
2. Delete ``src/main/cpp/libjpeg-turbo-<old_version>`` and extract the archive into a new directory ``src/main/cpp/libjpeg-turbo-<new_version>``.
3. Edit ``src/main/cpp/CMakeLists.txt``. Set ``LIBJPEG_TURBO_DIR`` variable to corresponding path. Set ``VERSION`` variable to the new version. Sync project.
4. Add header guards where needed: ``jmorecfg.h``, ``jpegint.h``, ``jdmerge.h``, ``jerror.h``, ``jdmerge.h``. For instance, at the beginning of ``jpegint.h`` file: ``#ifndef _JPEG_INT_H_`` (newline) ``#define _JPEG_INT_H_`` then at the end of the file: ``#endif``.
5. To remove ``JPEG_INTERNALS`` macro redefined warning, find ``#ifndef JPEG_INTERNALS`` everywhere. Replace with ``#ifndef JPEG_INTERNALS`` (newline) ``#define JPEG_INTERNALS`` (newline) ``#endif``.
6. In ``jpegtran.c``, rename ``main`` to ``jpegtran_main``.
7. Add binding code at the end of ``jpegtran.c`` file (Annex 1).
8. Modify ``jpegtran.c`` file. Remove all ``stdin`` and ``stdout`` calls. Program should err when they are used.
9. Modify ``jpegtran.c`` file. Remove all ``exit(int)`` function calls. All methods calling ``exit`` should return a status code instead, one of ``EXIT_SUCCESS``, ``EXIT_WARNING`` or ``EXIT_FAILURE``.
10. Modify ``jpegtran.c`` file. Remove all ``stderr`` calls. All errors should be redirected to another ``FILE*``.
11. Modify ``jpegtran.c`` file. Remove ``my_emit_message`` function. Remove its call.
12. Modify ``jpeg_error_mgr`` struct and its usages. Error manager must not call ``exit`` and messages must be logged to the error file, not to ``stderr``. See Annex 2.
13. Modify ``jpegtran.c`` to accept compression quality argument. See Annex 3.
14. Resolve any other build error.
15. Test the project.

##### Annex 1:

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

##### Annex 2:

In ``jpeglib.h`` file, add ``error_file`` and ``jump_buffer`` fields to the ``jpeg_error_mgr`` structure.

```c
struct jpeg_error_mgr {
  FILE* error_file;
  jmp_buf jump_buffer;

  /* Error exit handler: does not return to caller */
  void (*error_exit) (j_common_ptr cinfo);
  /* Conditionally emit a trace or warning message */
  void (*emit_message) (j_common_ptr cinfo, int msg_level);
  /* Routine that actually outputs a trace or error message */
  void (*output_message) (j_common_ptr cinfo);
  ....
```

In ``jerror.c``, modify the ``output_message`` function to output the message to the error file, not to ``stderr``.

```c
METHODDEF(void)
output_message(j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  /* Create the message */
  (*cinfo->err->format_message) (cinfo, buffer);

#ifdef USE_WINDOWS_MESSAGEBOX
  /* Display it in a message dialog box */
  MessageBox(GetActiveWindow(), buffer, "JPEG Library Error",
             MB_OK | MB_ICONERROR);
#else
  if (cinfo->err->error_file != NULL) {
      /* Send it to file, adding a newline */
      fprintf(cinfo->err->error_file, "%s\n", buffer);
  } else {
      /* Send it to stderr, adding a newline */
      fprintf(stderr, "%s\n", buffer);
  }
#endif
}
```

In ``jerror.c``, modify the ``error_exit`` function to not exit the app.

```c
METHODDEF(void)
error_exit(j_common_ptr cinfo)
{
  /* Always display the message */
  (*cinfo->err->output_message) (cinfo);

  /* Let the memory manager delete any temp files before we die */
  jpeg_destroy(cinfo);

  //NO: exit(EXIT_FAILURE);
  longjmp(cinfo->err->jump_buffer, EXIT_FAILURE);
}
```

In ``jpegtran.c``, set the ``error_file`` and ``format_message`` fields, where needed.

```c
/* Initialize the JPEG decompression object with default error handling. */
srcinfo.err = jpeg_std_error(&jsrcerr);
jsrcerr.error_file = errorFile;
if (setjmp(jsrcerr.jump_buffer) != EXIT_SUCCESS) {
  return EXIT_FAILURE;
}
jpeg_create_decompress(&srcinfo);
/* Initialize the JPEG compression object with default error handling. */
dstinfo.err = jpeg_std_error(&jdsterr);
jdsterr.error_file = errorFile;
if (setjmp(jdsterr.jump_buffer) != EXIT_SUCCESS) {
  return EXIT_FAILURE;
}
jpeg_create_compress(&dstinfo);
```

```c
dropinfo.err = jpeg_std_error(&jdroperr);
jdroperr.error_file = errorFile;
if (setjmp(jdroperr.jump_buffer) != EXIT_SUCCESS) {
  return EXIT_FAILURE;
}
jpeg_create_decompress(&dropinfo);
```

#### Annex 3:

Add quality argument to the binding function:

```c
JNIEXPORT int JNICALL
Java_ro_andob_jpegturbo_JPEGTurbo_jpegtran(JNIEnv* env, jclass clazz, jobjectArray argsFromJava, int quality)
```

Then add it to main function:

```c
int
jpegtran_main(int argc, char **argv, FILE* errorFile, int quality)
```

After ``jpeg_create_compress`` call, do call ``set_quality_ratings_int``:

```c
jpeg_create_compress(&dstinfo);
set_quality_ratings_int(&dstinfo, quality, FALSE);
```

In ``rdswitch.c``, create a function ``set_quality_ratings_int``, after ``set_quality_ratings``:

```c
GLOBAL(boolean)
set_quality_ratings_int(j_compress_ptr cinfo, int quality, boolean force_baseline)
{
  for (int tblno = 0; tblno < NUM_QUANT_TBLS; tblno++) {
    cinfo->q_scale_factor[tblno] = jpeg_quality_scaling(MAX(0, MIN(100, quality)));
  }
  jpeg_default_qtables(cinfo, force_baseline);
  return TRUE;
}
```

In ``cdjpeg.h``, declare the function ``set_quality_ratings_int``, after ``set_quality_ratings``:

```c
EXTERN(boolean) set_quality_ratings_int(j_compress_ptr cinfo, int quality, boolean force_baseline);
```
