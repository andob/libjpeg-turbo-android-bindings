project(jpeg_turbo)
cmake_minimum_required(VERSION 3.18.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

set(LIBJPEG_TURBO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libjpeg-turbo-2.1.4")
include_directories(${LIBJPEG_TURBO_DIR})

set(JPEG_LIB_VERSION 80)
set(VERSION 2.1.4)

set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(WITH_SIMD 1)
set(REQUIRE_SIMD 1)
set(NEON_INTRINSICS 1)

add_compile_definitions(JPEG_INTERNALS)

include(CheckCSourceCompiles)
include(CheckIncludeFiles)
include(CheckTypeSize)

macro(boolean_number var)
    if(${var})
        set(${var} 1 ${ARGN})
    else()
        set(${var} 0 ${ARGN})
    endif()
endmacro()

function(generate_jconfig_h)
    set(COPYRIGHT_YEAR "1991-2022")
    string(REPLACE "." ";" VERSION_TRIPLET ${VERSION})
    list(GET VERSION_TRIPLET 0 VERSION_MAJOR)
    list(GET VERSION_TRIPLET 1 VERSION_MINOR)
    list(GET VERSION_TRIPLET 2 VERSION_REVISION)
    function(pad_number NUMBER OUTPUT_LEN)
        string(LENGTH "${${NUMBER}}" INPUT_LEN)
        if(INPUT_LEN LESS OUTPUT_LEN)
            math(EXPR ZEROES "${OUTPUT_LEN} - ${INPUT_LEN} - 1")
            set(NUM ${${NUMBER}})
            foreach(C RANGE ${ZEROES})
                set(NUM "0${NUM}")
            endforeach()
            set(${NUMBER} ${NUM} PARENT_SCOPE)
        endif()
    endfunction()
    pad_number(VERSION_MINOR 3)
    pad_number(VERSION_REVISION 3)
    set(LIBJPEG_TURBO_VERSION_NUMBER ${VERSION_MAJOR}${VERSION_MINOR}${VERSION_REVISION})
    set(BITS_IN_JSAMPLE 8)
    configure_file(${LIBJPEG_TURBO_DIR}/jconfig.h.in ${LIBJPEG_TURBO_DIR}/jconfig.h)
endfunction()

function(generate_jconfigint_h)
    string(TIMESTAMP DEFAULT_BUILD "%Y%m%d")
    set(BUILD ${DEFAULT_BUILD} CACHE STRING "Build string (default: ${DEFAULT_BUILD})")

    if(MSVC)
        set(INLINE_OPTIONS "__inline;inline")
    else()
        set(INLINE_OPTIONS "__inline__;inline")
    endif()
    option(FORCE_INLINE "Force function inlining" TRUE)
    boolean_number(FORCE_INLINE)
    if(FORCE_INLINE)
        if(MSVC)
            list(INSERT INLINE_OPTIONS 0 "__forceinline")
        else()
            list(INSERT INLINE_OPTIONS 0 "inline __attribute__((always_inline))")
            list(INSERT INLINE_OPTIONS 0 "__inline__ __attribute__((always_inline))")
        endif()
    endif()
    foreach(inline ${INLINE_OPTIONS})
        check_c_source_compiles("${inline} static int foo(void) { return 0; } int main(void) { return foo(); }"
                INLINE_WORKS)
        if(INLINE_WORKS)
            set(INLINE ${inline})
            break()
        endif()
    endforeach()
    if(NOT INLINE_WORKS)
        message(FATAL_ERROR "Could not determine how to inline functions.")
    endif()
    message(STATUS "INLINE = ${INLINE} (FORCE_INLINE = ${FORCE_INLINE})")

    if(MSVC)
        set(THREAD_LOCAL "__declspec(thread)")
    else()
        set(THREAD_LOCAL "__thread")
    endif()
    check_c_source_compiles("${THREAD_LOCAL} int i;  int main(void) { i = 0;  return i; }" HAVE_THREAD_LOCAL)
    if(HAVE_THREAD_LOCAL)
        message(STATUS "THREAD_LOCAL = ${THREAD_LOCAL}")
    else()
        message(WARNING "Thread-local storage is not available.  The TurboJPEG API library's global error handler will not be thread-safe.")
        unset(THREAD_LOCAL)
    endif()

    check_type_size("size_t" SIZE_T)
    check_type_size("unsigned long" UNSIGNED_LONG)
    if(SIZE_T EQUAL UNSIGNED_LONG)
        check_c_source_compiles("int main(int argc, char **argv) { unsigned long a = argc;  return __builtin_ctzl(a); }"
                HAVE_BUILTIN_CTZL)
    endif()
    if(MSVC)
        check_include_files("intrin.h" HAVE_INTRIN_H)
    endif()

    if(UNIX)
        if(CMAKE_CROSSCOMPILING)
            set(RIGHT_SHIFT_IS_UNSIGNED 0)
        else()
            include(CheckCSourceRuns)
            check_c_source_runs("
      #include <stdio.h>
      #include <stdlib.h>
      int is_shifting_signed (long arg) {
        long res = arg >> 4;
        if (res == -0x7F7E80CL)
          return 1; /* right shift is signed */
        /* see if unsigned-shift hack will fix it. */
        /* we can't just test exact value since it depends on width of long... */
        res |= (~0L) << (32-4);
        if (res == -0x7F7E80CL)
          return 0; /* right shift is unsigned */
        printf(\"Right shift isn't acting as I expect it to.\\\\n\");
        printf(\"I fear the JPEG software will not work at all.\\\\n\\\\n\");
        return 0; /* try it with unsigned anyway */
      }
      int main (void) {
        exit(is_shifting_signed(-0x7F7E80B1L));
      }" RIGHT_SHIFT_IS_UNSIGNED)
        endif()
    endif()

    configure_file(${LIBJPEG_TURBO_DIR}/jconfigint.h.in ${LIBJPEG_TURBO_DIR}/jconfigint.h)
endfunction()

function(generate_jversion_h)
    configure_file(${LIBJPEG_TURBO_DIR}/jversion.h.in ${LIBJPEG_TURBO_DIR}/jversion.h)
endfunction()

function(configure_simd)
    # Detect CPU type and whether we're building 64-bit or 32-bit code
    math(EXPR BITS "${CMAKE_SIZEOF_VOID_P} * 8")
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} CMAKE_SYSTEM_PROCESSOR_LC)
    set(COUNT 1)
    foreach(ARCH ${CMAKE_OSX_ARCHITECTURES})
        if(COUNT GREATER 1)
            message(FATAL_ERROR "The libjpeg-turbo build system does not support multiple values in CMAKE_OSX_ARCHITECTURES.")
        endif()
        math(EXPR COUNT "${COUNT}+1")
    endforeach()
    if(CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86_64" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "amd64" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "i[0-9]86" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "x86" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "ia32")
        if(BITS EQUAL 64 OR CMAKE_C_COMPILER_ABI MATCHES "ELF X32")
            set(CPU_TYPE x86_64)
        else()
            set(CPU_TYPE i386)
        endif()
        if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL ${CPU_TYPE})
            set(CMAKE_SYSTEM_PROCESSOR ${CPU_TYPE})
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR_LC STREQUAL "aarch64" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "^arm")
        if(BITS EQUAL 64)
            set(CPU_TYPE arm64)
        else()
            set(CPU_TYPE arm)
        endif()
    elseif(CMAKE_SYSTEM_PROCESSOR_LC MATCHES "^ppc" OR
            CMAKE_SYSTEM_PROCESSOR_LC MATCHES "^powerpc")
        set(CPU_TYPE powerpc)
    else()
        set(CPU_TYPE ${CMAKE_SYSTEM_PROCESSOR_LC})
    endif()
    if(CMAKE_OSX_ARCHITECTURES MATCHES "x86_64" OR
            CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR
            CMAKE_OSX_ARCHITECTURES MATCHES "i386")
        set(CPU_TYPE ${CMAKE_OSX_ARCHITECTURES})
    endif()
    if(CMAKE_OSX_ARCHITECTURES MATCHES "ppc")
        set(CPU_TYPE powerpc)
    endif()
    if(MSVC_IDE AND CMAKE_GENERATOR_PLATFORM MATCHES "arm64")
        set(CPU_TYPE arm64)
    endif()

    if(WITH_SIMD)
        add_subdirectory(${LIBJPEG_TURBO_DIR}/simd)
        if(NEON_INTRINSICS)
            add_definitions(-DNEON_INTRINSICS)
        endif()
    elseif(NOT WITH_12BIT)
        message(STATUS "SIMD extensions: None (WITH_SIMD = ${WITH_SIMD})")
    endif()
    if(WITH_SIMD)
        message(STATUS "SIMD extensions: ${CPU_TYPE} (WITH_SIMD = ${WITH_SIMD})")
        if(MSVC_IDE OR XCODE)
            set_source_files_properties(${SIMD_OBJS} PROPERTIES GENERATED 1)
        endif()
    else()
        add_library(simd OBJECT ${LIBJPEG_TURBO_DIR}/jsimd_none.c)
        if(NOT WIN32 AND (CMAKE_POSITION_INDEPENDENT_CODE OR ENABLE_SHARED))
            set_target_properties(simd PROPERTIES POSITION_INDEPENDENT_CODE 1)
        endif()
    endif()
endfunction()

function(preinclude_common_headers)
    add_definitions(-include stdlib.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jconfig.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jconfigint.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jmorecfg.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jversion.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jpeglib.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jpegint.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jerror.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jdmerge.h)
    add_definitions(-include ${LIBJPEG_TURBO_DIR}/jinclude.h)
endfunction()

generate_jconfig_h()
generate_jconfigint_h()
generate_jversion_h()

preinclude_common_headers()

configure_simd()

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/jpegtranmod.c
    ${LIBJPEG_TURBO_DIR}/cdjpeg.c
    ${LIBJPEG_TURBO_DIR}/jaricom.c
    ${LIBJPEG_TURBO_DIR}/jcarith.c
    ${LIBJPEG_TURBO_DIR}/jcapimin.c
    ${LIBJPEG_TURBO_DIR}/jcapistd.c
    ${LIBJPEG_TURBO_DIR}/jccoefct.c
    ${LIBJPEG_TURBO_DIR}/jccolor.c
    ${LIBJPEG_TURBO_DIR}/jcdctmgr.c
    ${LIBJPEG_TURBO_DIR}/jchuff.c
    ${LIBJPEG_TURBO_DIR}/jcicc.c
    ${LIBJPEG_TURBO_DIR}/jcinit.c
    ${LIBJPEG_TURBO_DIR}/jcmainct.c
    ${LIBJPEG_TURBO_DIR}/jcmarker.c
    ${LIBJPEG_TURBO_DIR}/jcmaster.c
    ${LIBJPEG_TURBO_DIR}/jcomapi.c
    ${LIBJPEG_TURBO_DIR}/jcparam.c
    ${LIBJPEG_TURBO_DIR}/jcphuff.c
    ${LIBJPEG_TURBO_DIR}/jcprepct.c
    ${LIBJPEG_TURBO_DIR}/jcsample.c
    ${LIBJPEG_TURBO_DIR}/jctrans.c
    ${LIBJPEG_TURBO_DIR}/jdapimin.c
    ${LIBJPEG_TURBO_DIR}/jdapistd.c
    ${LIBJPEG_TURBO_DIR}/jdarith.c
    ${LIBJPEG_TURBO_DIR}/jdatadst.c
    ${LIBJPEG_TURBO_DIR}/jdatasrc.c
    ${LIBJPEG_TURBO_DIR}/jdcoefct.c
    ${LIBJPEG_TURBO_DIR}/jdcolor.c
    ${LIBJPEG_TURBO_DIR}/jddctmgr.c
    ${LIBJPEG_TURBO_DIR}/jdhuff.c
    ${LIBJPEG_TURBO_DIR}/jdicc.c
    ${LIBJPEG_TURBO_DIR}/jdinput.c
    ${LIBJPEG_TURBO_DIR}/jdmainct.c
    ${LIBJPEG_TURBO_DIR}/jdmarker.c
    ${LIBJPEG_TURBO_DIR}/jdmaster.c
    ${LIBJPEG_TURBO_DIR}/jdmerge.c
    ${LIBJPEG_TURBO_DIR}/jdphuff.c
    ${LIBJPEG_TURBO_DIR}/jdpostct.c
    ${LIBJPEG_TURBO_DIR}/jdsample.c
    ${LIBJPEG_TURBO_DIR}/jdtrans.c
    ${LIBJPEG_TURBO_DIR}/jerror.c
    ${LIBJPEG_TURBO_DIR}/jfdctflt.c
    ${LIBJPEG_TURBO_DIR}/jfdctfst.c
    ${LIBJPEG_TURBO_DIR}/jfdctint.c
    ${LIBJPEG_TURBO_DIR}/jidctflt.c
    ${LIBJPEG_TURBO_DIR}/jidctfst.c
    ${LIBJPEG_TURBO_DIR}/jidctint.c
    ${LIBJPEG_TURBO_DIR}/jidctred.c
    ${LIBJPEG_TURBO_DIR}/jmemmgr.c
    ${LIBJPEG_TURBO_DIR}/jmemnobs.c
    ${LIBJPEG_TURBO_DIR}/jquant1.c
    ${LIBJPEG_TURBO_DIR}/jquant2.c
    ${LIBJPEG_TURBO_DIR}/jutils.c
    ${LIBJPEG_TURBO_DIR}/rdswitch.c
    ${LIBJPEG_TURBO_DIR}/transupp.c
)

add_library(jpeg_turbo SHARED ${SOURCES})
target_link_libraries(jpeg_turbo simd)
