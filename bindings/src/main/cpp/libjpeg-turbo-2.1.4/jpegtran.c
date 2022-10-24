/*
 * jpegtran.c
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1995-2019, Thomas G. Lane, Guido Vollbeding.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2010, 2014, 2017, 2019-2022, D. R. Commander.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file contains a command-line user interface for JPEG transcoding.
 * It is very similar to cjpeg.c, and partly to djpeg.c, but provides
 * lossless transcoding between different JPEG file formats.  It also
 * provides some lossless and sort-of-lossless transformations of JPEG data.
 */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ConstantConditionsOC"
#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "cdjpeg.h"             /* Common decls for cjpeg/djpeg applications */
#include "transupp.h"           /* Support routines for jpegtran */
#include "jversion.h"           /* for version message */
#include "jconfigint.h"


/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static const char *progname;    /* program name for error messages */
static char *icc_filename;      /* for -icc switch */
JDIMENSION max_scans;           /* for -maxscans switch */
static char *outfilename;       /* for -outfile switch */
static char *dropfilename;      /* for -drop switch */
boolean report;                 /* for -report switch */
boolean strict;                 /* for -strict switch */
static JCOPY_OPTION copyoption; /* -copy switch */
static jpeg_transform_info transformoption; /* image transformation options */


LOCAL(void)
usage(FILE* errorFile)
/* complain about bad command line */
{
  fprintf(errorFile, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(errorFile, "inputfile outputfile\n");
#else
  fprintf(errorFile, "[inputfile]\n");
#endif

  fprintf(errorFile, "Switches (names may be abbreviated):\n");
  fprintf(errorFile, "  -copy none     Copy no extra markers from source file\n");
  fprintf(errorFile, "  -copy comments Copy only comment markers (default)\n");
  fprintf(errorFile, "  -copy icc      Copy only ICC profile markers\n");
  fprintf(errorFile, "  -copy all      Copy all extra markers\n");
#ifdef ENTROPY_OPT_SUPPORTED
  fprintf(errorFile, "  -optimize      Optimize Huffman table (smaller file, but slow compression)\n");
#endif
#ifdef C_PROGRESSIVE_SUPPORTED
  fprintf(errorFile, "  -progressive   Create progressive JPEG file\n");
#endif
  fprintf(errorFile, "Switches for modifying the image:\n");
#if TRANSFORMS_SUPPORTED
  fprintf(errorFile, "  -crop WxH+X+Y  Crop to a rectangular region\n");
  fprintf(errorFile, "  -drop +X+Y filename          Drop (insert) another image\n");
  fprintf(errorFile, "  -flip [horizontal|vertical]  Mirror image (left-right or top-bottom)\n");
  fprintf(errorFile, "  -grayscale     Reduce to grayscale (omit color data)\n");
  fprintf(errorFile, "  -perfect       Fail if there is non-transformable edge blocks\n");
  fprintf(errorFile, "  -rotate [90|180|270]         Rotate image (degrees clockwise)\n");
#endif
#if TRANSFORMS_SUPPORTED
  fprintf(errorFile, "  -transpose     Transpose image\n");
  fprintf(errorFile, "  -transverse    Transverse transpose image\n");
  fprintf(errorFile, "  -trim          Drop non-transformable edge blocks\n");
  fprintf(errorFile, "                 with -drop: Requantize drop file to match source file\n");
  fprintf(errorFile, "  -wipe WxH+X+Y  Wipe (gray out) a rectangular region\n");
#endif
  fprintf(errorFile, "Switches for advanced users:\n");
#ifdef C_ARITH_CODING_SUPPORTED
  fprintf(errorFile, "  -arithmetic    Use arithmetic coding\n");
#endif
  fprintf(errorFile, "  -icc FILE      Embed ICC profile contained in FILE\n");
  fprintf(errorFile, "  -restart N     Set restart interval in rows, or in blocks with B\n");
  fprintf(errorFile, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
  fprintf(errorFile, "  -maxscans N    Maximum number of scans to allow in input file\n");
  fprintf(errorFile, "  -outfile name  Specify name for output file\n");
  fprintf(errorFile, "  -report        Report transformation progress\n");
  fprintf(errorFile, "  -strict        Treat all warnings as fatal\n");
  fprintf(errorFile, "  -verbose  or  -debug   Emit debug output\n");
  fprintf(errorFile, "  -version       Print version information and exit\n");
  fprintf(errorFile, "Switches for wizards:\n");
#ifdef C_MULTISCAN_FILES_SUPPORTED
  fprintf(errorFile, "  -scans FILE    Create multi-scan JPEG per script FILE\n");
#endif
}


LOCAL(int)
select_transform(JXFORM_CODE transform, FILE* errorFile)
/* Silly little routine to detect multiple transform options,
 * which we can't handle.
 */
{
#if TRANSFORMS_SUPPORTED
  if (transformoption.transform == JXFORM_NONE ||
      transformoption.transform == transform) {
    transformoption.transform = transform;
  } else {
    fprintf(errorFile, "%s: can only do one image transformation at a time\n",
            progname);
    usage(errorFile);
    return EXIT_FAILURE;
  }
#else
  fprintf(errorFile, "%s: sorry, image transformation was not compiled\n",
          progname);
  return EXIT_FAILURE;
#endif
  return EXIT_SUCCESS;
}


LOCAL(int)
parse_switches(j_compress_ptr cinfo, int argc, char **argv,
               int last_file_arg_seen, boolean for_real,
               int* parse_switches_result, FILE* errorFile)
/* Parse optional switches.
 * Returns argv[] index of first file-name argument (== argc if none).
 * Any file names with indexes <= last_file_arg_seen are ignored;
 * they have presumably been processed in a previous iteration.
 * (Pass 0 for last_file_arg_seen on the first or only iteration.)
 * for_real is FALSE on the first (dummy) pass; we may skip any expensive
 * processing.
 */
{
  int argn;
  char *arg;
  boolean simple_progressive;
  char *scansarg = NULL;        /* saves -scans parm if any */

  /* Set up default JPEG parameters. */
  simple_progressive = FALSE;
  icc_filename = NULL;
  max_scans = 0;
  outfilename = NULL;
  report = FALSE;
  strict = FALSE;
  copyoption = JCOPYOPT_DEFAULT;
  transformoption.transform = JXFORM_NONE;
  transformoption.perfect = FALSE;
  transformoption.trim = FALSE;
  transformoption.force_grayscale = FALSE;
  transformoption.crop = FALSE;
  transformoption.slow_hflip = FALSE;
  cinfo->err->trace_level = 0;

  /* Scan command line options, adjust parameters */

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') {
      /* Not a switch, must be a file name argument */
      if (argn <= last_file_arg_seen) {
        outfilename = NULL;     /* -outfile applies to just one input file */
        continue;               /* ignore this name if previously processed */
      }
      break;                    /* else done parsing switches */
    }
    arg++;                      /* advance past switch marker character */

    if (keymatch(arg, "arithmetic", 1)) {
      /* Use arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
      cinfo->arith_code = TRUE;
#else
      fprintf(errorFile, "%s: sorry, arithmetic coding not supported\n",
              progname);
      return EXIT_FAILURE;
#endif

    } else if (keymatch(arg, "copy", 2)) {
      /* Select which extra markers to copy. */
      if (++argn >= argc) { /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (keymatch(argv[argn], "none", 1)) {
        copyoption = JCOPYOPT_NONE;
      } else if (keymatch(argv[argn], "comments", 1)) {
        copyoption = JCOPYOPT_COMMENTS;
      } else if (keymatch(argv[argn], "icc", 1)) {
        copyoption = JCOPYOPT_ICC;
      } else if (keymatch(argv[argn], "all", 1)) {
        copyoption = JCOPYOPT_ALL;
      } else {
        usage(errorFile);
        return EXIT_FAILURE;
      }

    } else if (keymatch(arg, "crop", 2)) {
      /* Perform lossless cropping. */
#if TRANSFORMS_SUPPORTED
      if (++argn >= argc) {      /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (transformoption.crop /* reject multiple crop/drop/wipe requests */ ||
          !jtransform_parse_crop_spec(&transformoption, argv[argn])) {
        fprintf(errorFile, "%s: bogus -crop argument '%s'\n",
                progname, argv[argn]);
        return EXIT_FAILURE;
      }
#else
      select_transform(JXFORM_NONE);    /* force an error */
#endif

    } else if (keymatch(arg, "drop", 2)) {
#if TRANSFORMS_SUPPORTED
      if (++argn >= argc) {      /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (transformoption.crop /* reject multiple crop/drop/wipe requests */ ||
          !jtransform_parse_crop_spec(&transformoption, argv[argn]) ||
          transformoption.crop_width_set != JCROP_UNSET ||
          transformoption.crop_height_set != JCROP_UNSET) {
        fprintf(errorFile, "%s: bogus -drop argument '%s'\n",
                progname, argv[argn]);
        return EXIT_FAILURE;
      }
      if (++argn >= argc) {      /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      dropfilename = argv[argn];
      int select_transform_result = select_transform(JXFORM_DROP, errorFile);
      if (select_transform_result != EXIT_SUCCESS)
        return select_transform_result;
#else
      int select_transform_result = select_transform(JXFORM_NONE); /* force an error */
      if (select_transform_result != EXIT_SUCCESS)
        return select_transform_result;
#endif

    } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      /* Enable debug printouts. */
      /* On first -d, print version identification */
      static boolean printed_version = FALSE;

      if (!printed_version) {
        fprintf(errorFile, "%s version %s (build %s)\n",
                PACKAGE_NAME, VERSION, BUILD);
        fprintf(errorFile, "%s\n\n", JCOPYRIGHT);
        fprintf(errorFile, "Emulating The Independent JPEG Group's software, version %s\n\n",
                JVERSION);
        printed_version = TRUE;
      }
      cinfo->err->trace_level++;

    } else if (keymatch(arg, "version", 4)) {
      fprintf(errorFile, "%s version %s (build %s)\n",
              PACKAGE_NAME, VERSION, BUILD);
      return EXIT_FAILURE;

    } else if (keymatch(arg, "flip", 1)) {
      /* Mirror left-right or top-bottom. */
      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (keymatch(argv[argn], "horizontal", 1))
      {
        int select_transform_result = select_transform(JXFORM_FLIP_H, errorFile);
        if (select_transform_result != EXIT_SUCCESS)
          return select_transform_result;
      }
      else if (keymatch(argv[argn], "vertical", 1))
      {
        int select_transform_result = select_transform(JXFORM_FLIP_V, errorFile);
        if (select_transform_result != EXIT_SUCCESS)
          return select_transform_result;
      }
      else {
        usage(errorFile);
        return EXIT_FAILURE;
      }

    } else if (keymatch(arg, "grayscale", 1) ||
               keymatch(arg, "greyscale", 1)) {
      /* Force to grayscale. */
#if TRANSFORMS_SUPPORTED
      transformoption.force_grayscale = TRUE;
#else
      select_transform(JXFORM_NONE);    /* force an error */
#endif

    } else if (keymatch(arg, "icc", 1)) {
      /* Set ICC filename. */
      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      icc_filename = argv[argn];

    } else if (keymatch(arg, "maxmemory", 3)) {
      /* Maximum memory in Kb (or Mb with 'm'). */
      long lval;
      char ch = 'x';

      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1) {
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (ch == 'm' || ch == 'M')
        lval *= 1000L;
      cinfo->mem->max_memory_to_use = lval * 1000L;

    } else if (keymatch(arg, "maxscans", 4)) {
      if (++argn >= argc) {      /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (sscanf(argv[argn], "%u", &max_scans) != 1) {
        usage(errorFile);
        return EXIT_FAILURE;
      }

    } else if (keymatch(arg, "optimize", 1) || keymatch(arg, "optimise", 1)) {
      /* Enable entropy parm optimization. */
#ifdef ENTROPY_OPT_SUPPORTED
      cinfo->optimize_coding = TRUE;
#else
      fprintf(errorFile, "%s: sorry, entropy optimization was not compiled\n",
              progname);
      return EXIT_FAILURE;
#endif

    } else if (keymatch(arg, "outfile", 4)) {
      /* Set output file name. */
      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      outfilename = argv[argn]; /* save it away for later use */

    } else if (keymatch(arg, "perfect", 2)) {
      /* Fail if there is any partial edge MCUs that the transform can't
       * handle. */
      transformoption.perfect = TRUE;

    } else if (keymatch(arg, "progressive", 2)) {
      /* Select simple progressive mode. */
#ifdef C_PROGRESSIVE_SUPPORTED
      simple_progressive = TRUE;
      /* We must postpone execution until num_components is known. */
#else
      fprintf(errorFile, "%s: sorry, progressive output was not compiled\n",
              progname);
      return EXIT_FAILURE;
#endif

    } else if (keymatch(arg, "report", 3)) {
      report = TRUE;

    } else if (keymatch(arg, "restart", 1)) {
      /* Restart interval in MCU rows (or in MCUs with 'b'). */
      long lval;
      char ch = 'x';

      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1) {
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (lval < 0 || lval > 65535L) {
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (ch == 'b' || ch == 'B') {
        cinfo->restart_interval = (unsigned int)lval;
        cinfo->restart_in_rows = 0; /* else prior '-restart n' overrides me */
      } else {
        cinfo->restart_in_rows = (int)lval;
        /* restart_interval will be computed during startup */
      }

    } else if (keymatch(arg, "rotate", 2)) {
      /* Rotate 90, 180, or 270 degrees (measured clockwise). */
      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (keymatch(argv[argn], "90", 2))
      {
        int select_transform_result = select_transform(JXFORM_ROT_90, errorFile);
        if (select_transform_result != EXIT_SUCCESS)
          return select_transform_result;
      }
      else if (keymatch(argv[argn], "180", 3))
      {
        int select_transform_result = select_transform(JXFORM_ROT_180, errorFile);
        if (select_transform_result != EXIT_SUCCESS)
          return select_transform_result;
      }
      else if (keymatch(argv[argn], "270", 3))
      {
        int select_transform_result = select_transform(JXFORM_ROT_270, errorFile);
        if (select_transform_result != EXIT_SUCCESS)
          return select_transform_result;
      }
      else {
        usage(errorFile);
        return EXIT_FAILURE;
      }

    } else if (keymatch(arg, "scans", 1)) {
      /* Set scan script. */
#ifdef C_MULTISCAN_FILES_SUPPORTED
      if (++argn >= argc) {     /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      scansarg = argv[argn];
      /* We must postpone reading the file in case -progressive appears. */
#else
      fprintf(errorFile, "%s: sorry, multi-scan output was not compiled\n",
              progname);
      return EXIT_FAILURE;
#endif

    } else if (keymatch(arg, "strict", 2)) {
      strict = TRUE;

    } else if (keymatch(arg, "transpose", 1)) {
      /* Transpose (across UL-to-LR axis). */
      int select_transform_result = select_transform(JXFORM_TRANSPOSE, errorFile);
      if (select_transform_result != EXIT_SUCCESS)
        return select_transform_result;

    } else if (keymatch(arg, "transverse", 6)) {
      /* Transverse transpose (across UR-to-LL axis). */
      int select_transform_result = select_transform(JXFORM_TRANSVERSE, errorFile);
      if (select_transform_result != EXIT_SUCCESS)
        return select_transform_result;

    } else if (keymatch(arg, "trim", 3)) {
      /* Trim off any partial edge MCUs that the transform can't handle. */
      transformoption.trim = TRUE;

    } else if (keymatch(arg, "wipe", 1)) {
#if TRANSFORMS_SUPPORTED
      if (++argn >= argc) {      /* advance to next argument */
        usage(errorFile);
        return EXIT_FAILURE;
      }
      if (transformoption.crop /* reject multiple crop/drop/wipe requests */ ||
          !jtransform_parse_crop_spec(&transformoption, argv[argn])) {
        fprintf(errorFile, "%s: bogus -wipe argument '%s'\n",
                progname, argv[argn]);
        return EXIT_FAILURE;
      }
      int select_transform_result = select_transform(JXFORM_WIPE, errorFile);
      if (select_transform_result != EXIT_SUCCESS)
        return select_transform_result;
#else
      select_transform(JXFORM_NONE);    /* force an error */
#endif

    } else {
      usage(errorFile);                  /* bogus switch */
      return EXIT_FAILURE;
    }
  }

  /* Post-switch-scanning cleanup */

  if (for_real) {

#ifdef C_PROGRESSIVE_SUPPORTED
    if (simple_progressive)     /* process -progressive; -scans can override */
      jpeg_simple_progression(cinfo);
#endif

#ifdef C_MULTISCAN_FILES_SUPPORTED
    if (scansarg != NULL)       /* process -scans if it was present */
      if (!read_scan_script(cinfo, scansarg)) {
        usage(errorFile);
        return EXIT_FAILURE;
      }
#endif
  }

  (*parse_switches_result) = argn;
  return EXIT_SUCCESS;
}

/*
 * The main program.
 */

int
jpegtran_main(int argc, char **argv, FILE* errorFile, int quality)
{
  struct jpeg_decompress_struct srcinfo;
#if TRANSFORMS_SUPPORTED
  struct jpeg_decompress_struct dropinfo;
  struct jpeg_error_mgr jdroperr;
  FILE *drop_file;
#endif
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jsrcerr, jdsterr;
  struct cdjpeg_progress_mgr src_progress, dst_progress;
  jvirt_barray_ptr *src_coef_arrays;
  jvirt_barray_ptr *dst_coef_arrays;
  int file_index;
  /* We assume all-in-memory processing and can therefore use only a
   * single file pointer for sequential input and output operation.
   */
  FILE *fp;
  FILE *icc_file;
  JOCTET *icc_profile = NULL;
  long icc_len = 0;

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "jpegtran";      /* in case C library doesn't provide it */

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
  set_quality_ratings_int(&dstinfo, quality, FALSE);

  /* Scan command line to find file names.
   * It is convenient to use just one switch-parsing routine, but the switch
   * values read here are mostly ignored; we will rescan the switches after
   * opening the input file.  Also note that most of the switches affect the
   * destination JPEG object, so we parse into that and then copy over what
   * needs to affect the source too.
   */

  int parse_switches_result = parse_switches(&dstinfo, argc, argv, 0, FALSE, &file_index, errorFile);
  if (parse_switches_result != EXIT_SUCCESS)
    return parse_switches_result;
  jsrcerr.trace_level = jdsterr.trace_level;
  srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

#ifdef TWO_FILE_COMMANDLINE
  /* Must have either -outfile switch or explicit output file name */
  if (outfilename == NULL) {
    if (file_index != argc - 2) {
      fprintf(errorFile, "%s: must name one input and one output file\n",
              progname);
      usage(errorFile);
    }
    outfilename = argv[file_index + 1];
  } else {
    if (file_index != argc - 1) {
      fprintf(errorFile, "%s: must name one input and one output file\n",
              progname);
      usage(errorFile);
    }
  }
#else
  /* Unix style: expect zero or one file name */
  if (file_index < argc - 1) {
    fprintf(errorFile, "%s: only one input file\n", progname);
    usage(errorFile);
    return EXIT_FAILURE;
  }
#endif /* TWO_FILE_COMMANDLINE */

  /* Open the input file. */
  if (file_index < argc) {
    if ((fp = fopen(argv[file_index], READ_BINARY)) == NULL) {
      fprintf(errorFile, "%s: can't open %s for reading\n", progname,
              argv[file_index]);
      return EXIT_FAILURE;
    }
  } else {
    usage(errorFile);
    return EXIT_FAILURE;
  }

  if (icc_filename != NULL) {
    if ((icc_file = fopen(icc_filename, READ_BINARY)) == NULL) {
      fprintf(errorFile, "%s: can't open %s\n", progname, icc_filename);
      return EXIT_FAILURE;
    }
    if (fseek(icc_file, 0, SEEK_END) < 0 ||
        (icc_len = ftell(icc_file)) < 1 ||
        fseek(icc_file, 0, SEEK_SET) < 0) {
      fprintf(errorFile, "%s: can't determine size of %s\n", progname,
              icc_filename);
      return EXIT_FAILURE;
    }
    if ((icc_profile = (JOCTET *)malloc(icc_len)) == NULL) {
      fprintf(errorFile, "%s: can't allocate memory for ICC profile\n", progname);
      fclose(icc_file);
      return EXIT_FAILURE;
    }
    if (fread(icc_profile, icc_len, 1, icc_file) < 1) {
      fprintf(errorFile, "%s: can't read ICC profile from %s\n", progname,
              icc_filename);
      free(icc_profile);
      fclose(icc_file);
      return EXIT_FAILURE;
    }
    fclose(icc_file);
    if (copyoption == JCOPYOPT_ALL)
      copyoption = JCOPYOPT_ALL_EXCEPT_ICC;
    if (copyoption == JCOPYOPT_ICC)
      copyoption = JCOPYOPT_NONE;
  }

  if (report) {
    start_progress_monitor((j_common_ptr)&dstinfo, &dst_progress);
    dst_progress.report = report;
  }
  if (report || max_scans != 0) {
    start_progress_monitor((j_common_ptr)&srcinfo, &src_progress);
    src_progress.report = report;
    src_progress.max_scans = max_scans;
  }
#if TRANSFORMS_SUPPORTED
  /* Open the drop file. */
  if (dropfilename != NULL) {
    if ((drop_file = fopen(dropfilename, READ_BINARY)) == NULL) {
      fprintf(errorFile, "%s: can't open %s for reading\n", progname,
              dropfilename);
      return EXIT_FAILURE;
    }
    dropinfo.err = jpeg_std_error(&jdroperr);
    jdroperr.error_file = errorFile;
    if (setjmp(jdroperr.jump_buffer) != EXIT_SUCCESS) {
      return EXIT_FAILURE;
    }
    jpeg_create_decompress(&dropinfo);
    jpeg_stdio_src(&dropinfo, drop_file);
  } else {
    drop_file = NULL;
  }
#endif

  /* Specify data source for decompression */
  jpeg_stdio_src(&srcinfo, fp);

  /* Enable saving of extra markers that we want to copy */
  jcopy_markers_setup(&srcinfo, copyoption);

  /* Read file header */
  (void)jpeg_read_header(&srcinfo, TRUE);

#if TRANSFORMS_SUPPORTED
  if (dropfilename != NULL) {
    (void)jpeg_read_header(&dropinfo, TRUE);
    transformoption.crop_width = dropinfo.image_width;
    transformoption.crop_width_set = JCROP_POS;
    transformoption.crop_height = dropinfo.image_height;
    transformoption.crop_height_set = JCROP_POS;
    transformoption.drop_ptr = &dropinfo;
  }
#endif

  /* Any space needed by a transform option must be requested before
   * jpeg_read_coefficients so that memory allocation will be done right.
   */
#if TRANSFORMS_SUPPORTED
  /* Fail right away if -perfect is given and transformation is not perfect.
   */
  if (!jtransform_request_workspace(&srcinfo, &transformoption)) {
    fprintf(errorFile, "%s: transformation is not perfect\n", progname);
    return EXIT_FAILURE;
  }
#endif

  /* Read source file as DCT coefficients */
  src_coef_arrays = jpeg_read_coefficients(&srcinfo);

#if TRANSFORMS_SUPPORTED
  if (dropfilename != NULL) {
    transformoption.drop_coef_arrays = jpeg_read_coefficients(&dropinfo);
  }
#endif

  /* Initialize destination compression parameters from source values */
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  /* Adjust destination parameters if required by transform options;
   * also find out which set of coefficient arrays will hold the output.
   */
#if TRANSFORMS_SUPPORTED
  dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
                                                 src_coef_arrays,
                                                 &transformoption);
#else
  dst_coef_arrays = src_coef_arrays;
#endif

  /* Close input file, if we opened it.
   * Note: we assume that jpeg_read_coefficients consumed all input
   * until JPEG_REACHED_EOI, and that jpeg_finish_decompress will
   * only consume more while (!cinfo->inputctl->eoi_reached).
   * We cannot call jpeg_finish_decompress here since we still need the
   * virtual arrays allocated from the source object for processing.
   */
  fclose(fp);

  /* Open the output file. */
  if (outfilename != NULL) {
    if ((fp = fopen(outfilename, WRITE_BINARY)) == NULL) {
      fprintf(errorFile, "%s: can't open %s for writing\n", progname,
              outfilename);
      return EXIT_FAILURE;
    }
  } else {
    usage(errorFile);
    return EXIT_FAILURE;
  }

  /* Adjust default compression parameters by re-parsing the options */
  parse_switches_result = parse_switches(&dstinfo, argc, argv, 0, TRUE, &file_index, errorFile);
  if (parse_switches_result != EXIT_SUCCESS)
    return parse_switches_result;

  /* Specify data destination for compression */
  jpeg_stdio_dest(&dstinfo, fp);

  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients(&dstinfo, dst_coef_arrays);

  /* Copy to the output file any extra markers that we want to preserve */
  jcopy_markers_execute(&srcinfo, &dstinfo, copyoption);

  if (icc_profile != NULL)
    jpeg_write_icc_profile(&dstinfo, icc_profile, (unsigned int)icc_len);

  /* Execute image transformation, if any */
#if TRANSFORMS_SUPPORTED
  jtransform_execute_transformation(&srcinfo, &dstinfo, src_coef_arrays,
                                    &transformoption);
#endif

  /* Finish compression and release memory */
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
#if TRANSFORMS_SUPPORTED
  if (dropfilename != NULL) {
    (void)jpeg_finish_decompress(&dropinfo);
    jpeg_destroy_decompress(&dropinfo);
  }
#endif
  (void)jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  /* Close output file, if we opened it */
  fclose(fp);
#if TRANSFORMS_SUPPORTED
  if (drop_file != NULL)
    fclose(drop_file);
#endif

  if (report)
    end_progress_monitor((j_common_ptr)&dstinfo);
  if (report || max_scans != 0)
    end_progress_monitor((j_common_ptr)&srcinfo);

  free(icc_profile);

  /* All done. */
#if TRANSFORMS_SUPPORTED
  if (dropfilename != NULL)
    return jsrcerr.num_warnings + jdroperr.num_warnings +
           jdsterr.num_warnings ? EXIT_WARNING : EXIT_SUCCESS;
#endif
  return jsrcerr.num_warnings + jdsterr.num_warnings ?
         EXIT_WARNING : EXIT_SUCCESS;
}

#include <jni.h>
#include <unistd.h>

JNIEXPORT int JNICALL
Java_ro_andob_jpegturbo_JPEGTurbo_jpegtran(JNIEnv* env, jclass clazz, jobjectArray argsFromJava, int quality)
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
  int resultCode = jpegtran_main(argc, argv, errorFile, quality);
  
  for (int i=0; i<argc; i++)
    free(argv[i]);
  free(argv);
  fclose(errorFile);
  return resultCode;
}

#pragma clang diagnostic pop