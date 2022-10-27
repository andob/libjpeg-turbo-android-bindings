#include <jni.h>
#include <setjmp.h>
#include <transupp.h>
#include <cdjpeg.h>

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

static void parse_options(struct transform_args* args,
    struct jpeg_compress_struct* compress_info,
    struct jpeg_decompress_struct* decompress_info,
    FILE* error_file)
{
    jpeg_set_quality(compress_info, args->quality, /*forceBaseline*/ FALSE);

    if (args->progressive)
    {
        #ifdef C_PROGRESSIVE_SUPPORTED
            jpeg_simple_progression(compress_info);
        #else
            #error Please compile with progressive JPEG support!
        #endif
    }

    if (args->optimize)
    {
        #ifdef ENTROPY_OPT_SUPPORTED
            compress_info->optimize_coding = TRUE;
        #else
            #error Please compile with JPEG optimisation support!
        #endif
    }

    if (args->verbose)
    {
        fprintf(error_file, "%s version %s (build %s)\n", PACKAGE_NAME, VERSION, BUILD);
        fprintf(error_file, "%s\n\n", JCOPYRIGHT);
        fprintf(error_file, "Emulating The Independent JPEG Group's software, version %s\n\n", JVERSION);

        compress_info->err->trace_level++;
        decompress_info->err->trace_level++;
    }
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
    parse_options(&args, &compress_info, &decompress_info, error_file);
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
