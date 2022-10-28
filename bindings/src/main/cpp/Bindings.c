#include <jni.h>
#include <setjmp.h>
#include <transupp.h>
#include <cdjpeg.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef unsigned char byte;

struct transform_args
{
    const char* input_file_path;
    const char* output_file_path;
    const char* error_file_path;
    int quality;
    boolean progressive;
    boolean optimize;
    boolean verbose;
};

struct jpeg_error_mgr_mod
{
    struct jpeg_error_mgr pub;
    FILE* error_file;
    jmp_buf jump_buffer;
};

static void output_message_mod(j_common_ptr cinfo)
{
    #define JMSG_LENGTH_MAX  200
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);

    struct jpeg_error_mgr_mod* err_mod = (struct jpeg_error_mgr_mod*) cinfo->err;
    if (err_mod->error_file != NULL)
        fprintf(err_mod->error_file, "%s\n", buffer);
}

static void error_exit_mod(j_common_ptr cinfo)
{
    (*cinfo->err->output_message) (cinfo);

    struct jpeg_error_mgr_mod* err_mod = (struct jpeg_error_mgr_mod*) cinfo->err;
    longjmp(err_mod->jump_buffer, EXIT_FAILURE);
}

struct memory_to_release
{
    FILE* error_file;
    struct jpeg_decompress_struct* decompress_info;
    struct jpeg_compress_struct* compress_info;
    FILE* input_file;
    FILE* output_file;

    boolean was_error_file_closed;
    boolean was_decompress_info_destroyed;
    boolean was_compress_info_destroyed;
    boolean was_input_file_closed;
    boolean was_output_file_closed;
};

static void close_input_file(struct memory_to_release* memory_to_release)
{
    if (!memory_to_release->was_input_file_closed)
        fclose(memory_to_release->input_file);
}

static void release_memory(struct memory_to_release* memory_to_release)
{
    if (!memory_to_release->was_error_file_closed && memory_to_release->error_file != NULL)
        fclose(memory_to_release->error_file);
    if (!memory_to_release->was_input_file_closed && memory_to_release->input_file != NULL)
        fclose(memory_to_release->input_file);
    if (!memory_to_release->was_output_file_closed && memory_to_release->output_file != NULL)
        fclose(memory_to_release->output_file);
    if (!memory_to_release->was_decompress_info_destroyed && memory_to_release->decompress_info != NULL)
        jpeg_destroy_decompress(memory_to_release->decompress_info);
    if (!memory_to_release->was_compress_info_destroyed && memory_to_release->compress_info != NULL)
        jpeg_destroy_compress(memory_to_release->compress_info);
}

static int reencode(struct transform_args args)
{
    struct memory_to_release defer_memory_to_release;
    defer_memory_to_release.was_error_file_closed = FALSE;
    defer_memory_to_release.was_decompress_info_destroyed = FALSE;
    defer_memory_to_release.was_compress_info_destroyed = FALSE;
    defer_memory_to_release.was_input_file_closed = FALSE;
    defer_memory_to_release.was_output_file_closed = FALSE;

    #pragma region Create error file
    FILE* error_file = fopen(args.error_file_path, "w");
    if (error_file == NULL)
        return EXIT_FAILURE;
    defer_memory_to_release.error_file = error_file;
    #pragma endregion

    #pragma region Create Decompress info
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wincompatible-pointer-types"
    struct jpeg_decompress_struct decompress_info;
    struct jpeg_error_mgr_mod decompress_error_manager;
    jpeg_std_error(&decompress_error_manager.pub);
    decompress_info.err = &decompress_error_manager;
    decompress_error_manager.error_file = error_file;
    decompress_error_manager.pub.trace_level = 0;
    decompress_error_manager.pub.output_message = output_message_mod;
    decompress_error_manager.pub.error_exit = error_exit_mod;
    defer_memory_to_release.decompress_info = &decompress_info;
    if (setjmp(decompress_error_manager.jump_buffer) != EXIT_SUCCESS)
    {
        release_memory(&defer_memory_to_release);
        return EXIT_FAILURE;
    }
    jpeg_create_decompress(&decompress_info);
    #pragma endregion

    #pragma region Create Compress info
    struct jpeg_compress_struct compress_info;
    struct jpeg_error_mgr_mod compress_error_manager;
    jpeg_std_error(&compress_error_manager.pub);
    compress_info.err = &compress_error_manager;
    compress_error_manager.error_file = error_file;
    compress_error_manager.pub.trace_level = 0;
    compress_error_manager.pub.output_message = output_message_mod;
    compress_error_manager.pub.error_exit = error_exit_mod;
    defer_memory_to_release.compress_info = &compress_info;
    if (setjmp(compress_error_manager.jump_buffer) != EXIT_SUCCESS)
    {
        release_memory(&defer_memory_to_release);
        return EXIT_FAILURE;
    }
    jpeg_create_compress(&compress_info);
    #pragma clang diagnostic pop
    #pragma endregion

    #pragma region Parse input options
    jpeg_set_quality(&compress_info, args.quality, /*forceBaseline*/ FALSE);

    if (args.optimize)
    {
        #ifdef ENTROPY_OPT_SUPPORTED
            compress_info.optimize_coding = TRUE;
        #else
            #error Please compile with JPEG optimisation support!
        #endif
    }

    if (args.verbose)
    {
        fprintf(error_file, "%s version %s (build %s)\n", PACKAGE_NAME, VERSION, BUILD);
        fprintf(error_file, "%s\n\n", JCOPYRIGHT);
        fprintf(error_file, "Emulating The Independent JPEG Group's software, version %s\n\n", JVERSION);

        compress_info.err->trace_level++;
        decompress_info.err->trace_level++;
    }
    #pragma endregion

    #pragma region Read input file
    FILE* input_file = fopen(args.input_file_path, READ_BINARY);
    if (input_file == NULL)
    {
        release_memory(&defer_memory_to_release);
        fprintf(error_file, "Cannot open %s for reading!", args.input_file_path);
        return EXIT_FAILURE;
    }
    defer_memory_to_release.input_file = input_file;

    jpeg_stdio_src(&decompress_info, input_file);
    jcopy_markers_setup(&decompress_info, JCOPYOPT_ALL);
    jpeg_read_header(&decompress_info, TRUE);
    jvirt_barray_ptr* coefficients = jpeg_read_coefficients(&decompress_info);
    jpeg_copy_critical_parameters(&decompress_info, &compress_info);
    close_input_file(&defer_memory_to_release);
    #pragma endregion

    #pragma region Write output file
    FILE* output_file = fopen(args.output_file_path, WRITE_BINARY);
    if (output_file == NULL)
    {
        release_memory(&defer_memory_to_release);
        fprintf(error_file, "Cannot open %s for writing!", args.output_file_path);
        return EXIT_FAILURE;
    }
    defer_memory_to_release.output_file = output_file;

    if (args.progressive)
    {
        #ifdef C_PROGRESSIVE_SUPPORTED
            jpeg_simple_progression(&compress_info);
        #else
            #error Please compile with progressive JPEG support!
        #endif
    }

    jpeg_stdio_dest(&compress_info, output_file);
    jpeg_write_coefficients(&compress_info, coefficients);
    jcopy_markers_execute(&decompress_info, &compress_info, JCOPYOPT_ALL);
    #pragma endregion

    jpeg_finish_compress(&compress_info);
    jpeg_finish_decompress(&decompress_info);

    release_memory(&defer_memory_to_release);

    return decompress_error_manager.pub.num_warnings + compress_error_manager.pub.num_warnings ?
           EXIT_WARNING : EXIT_SUCCESS;
}

JNIEXPORT jint JNICALL Java_ro_andob_jpegturbo_JPEGTurbo_reencodeNative(
    JNIEnv* env, jclass clazz,
    jstring input_file_path_from_java, jstring output_file_path_from_java, jstring error_file_path_from_java,
    int quality, jboolean progressive, jboolean optimize, jboolean verbose)
{
    const char* input_file_path = (*env)->GetStringUTFChars(env, input_file_path_from_java, 0);
    const char* output_file_path = (*env)->GetStringUTFChars(env, output_file_path_from_java, 0);
    const char* error_file_path = (*env)->GetStringUTFChars(env, error_file_path_from_java, 0);

    struct transform_args args;
    args.input_file_path = input_file_path;
    args.output_file_path = output_file_path;
    args.error_file_path = error_file_path;
    args.quality = MAX(0, MIN(100, quality));
    args.progressive = progressive;
    args.optimize = optimize;
    args.verbose = verbose;
    int result = reencode(args);

    (*env)->ReleaseStringUTFChars(env, input_file_path_from_java, input_file_path);
    (*env)->ReleaseStringUTFChars(env, output_file_path_from_java, output_file_path);
    (*env)->ReleaseStringUTFChars(env, error_file_path_from_java, error_file_path);

    return result;
}

JNIEXPORT void JNICALL
Java_ro_andob_jpegturbo_JPEGTurbo_mergeExifAndJpegNative(
    JNIEnv *env, jclass clazz,
    jstring input_exif_file_path_from_java,
    jstring input_image_file_path_from_java,
    jstring output_file_path_from_java)
{
    const char* input_exif_file_path = (*env)->GetStringUTFChars(env, input_exif_file_path_from_java, 0);
    const char* input_image_file_path = (*env)->GetStringUTFChars(env, input_image_file_path_from_java, 0);
    const char* output_file_path = (*env)->GetStringUTFChars(env, output_file_path_from_java, 0);

    //JPEG specification - see https://stackoverflow.com/a/48814876
    byte jpeg_marker[] = { 0xFF, 0xD8 };
    byte exif_marker[] = { 0xFF, 0xE1 };

    int exif_size = 0, image_size = 0;
    byte *exif_data = NULL, *image_data = NULL;

    FILE* input_exif_file = fopen(input_exif_file_path, "rb");
    byte two_bytes[2];
    fread(two_bytes, sizeof(byte), 2, input_exif_file);
    if (two_bytes[0] == jpeg_marker[0] && two_bytes[1] == jpeg_marker[1])
    {
        fread(two_bytes, sizeof(byte), 2, input_exif_file);
        if (two_bytes[0] == exif_marker[0] && two_bytes[1] == exif_marker[1])
        {
            fread(two_bytes, sizeof(byte), 2, input_exif_file);
            exif_size = ((two_bytes[0] << 8) | two_bytes[1]) - 2;
            if (exif_size > 0)
            {
                exif_data = malloc(sizeof(byte) * exif_size);
                fread(exif_data, sizeof(byte), exif_size, input_exif_file);
            }
        }
    }
    fclose(input_exif_file);

    FILE* input_image_file = fopen(input_image_file_path, "rb");
    fseek(input_image_file, 0L, SEEK_END);
    image_size = ftell(input_image_file);
    rewind(input_image_file);
    image_data = malloc(sizeof(byte)*image_size);
    fread(image_data, sizeof(byte), image_size, input_image_file);
    fclose(input_image_file);

    if (exif_data != NULL && exif_size > 0 && image_data != NULL && image_size > 2)
    {
        FILE* output_file = fopen(output_file_path, "wb");
        byte encoded_exif_size[] = { ((exif_size+2)>>8)&0xFF, (exif_size+2)&0xFF };
        fwrite(jpeg_marker, sizeof(byte), 2, output_file);
        fwrite(exif_marker, sizeof(byte), 2, output_file);
        fwrite(encoded_exif_size, sizeof(byte), 2, output_file);
        fwrite(exif_data, sizeof(byte), exif_size, output_file);
        fwrite(image_data+2, sizeof(byte), image_size-2, output_file);
        fclose(output_file);
    }
    else
    {
        rename(input_image_file_path, output_file_path);
    }

    if (image_data != NULL)
        free(image_data);
    if (exif_data != NULL)
        free(exif_data);

    (*env)->ReleaseStringUTFChars(env, input_exif_file_path_from_java, input_exif_file_path);
    (*env)->ReleaseStringUTFChars(env, input_image_file_path_from_java, input_image_file_path);
    (*env)->ReleaseStringUTFChars(env, output_file_path_from_java, output_file_path);
}

JNIEXPORT jint JNICALL
Java_ro_andob_jpegturbo_FileIOUtils_createPipeFileNative(
    JNIEnv *env, jclass clazz,
    jstring pipe_file_path_from_java)
{
    const char* pipe_file_path = (*env)->GetStringUTFChars(env, pipe_file_path_from_java, 0);
    int result = mkfifo(pipe_file_path, S_IRWXU | S_IRWXG | S_IROTH);
    (*env)->ReleaseStringUTFChars(env, pipe_file_path_from_java, pipe_file_path);
    return result;
}
