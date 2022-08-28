#ifndef STBI_INCLUDE_STB_IMAGE_HPP
#define STBI_INCLUDE_STB_IMAGE_HPP

#ifndef STBI_NO_STDIO
#  include <cstdio>
#endif // STBI_NO_STDIO
#include <cstdlib>

#ifndef STBIDEF
#  ifdef STB_IMAGE_STATIC
#    define STBIDEF static
#  else
#    define STBIDEF extern
#  endif
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define STBI_VERSION 1

enum {
  STBI_default = 0, // only used for desired_channels

  STBI_grey = 1,
  STBI_grey_alpha = 2,
  STBI_rgb = 3,
  STBI_rgb_alpha = 4
};

using stbi_uc = unsigned char;
using stbi_us = unsigned short;

//////////////////////////////////////////////////////////////////////////////
//
// PRIMARY API - works on images of any type
//

//
// load image by filename, open file, or memory buffer
//

struct stbi_io_callbacks {
  int (*read)(void *user, char *data,
              int size); // fill 'data' with 'size' bytes.  return number of bytes actually read
  void (*skip)(void *user,
               int n);    // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
  int (*eof)(void *user); // returns nonzero if we are at end of file/data
};

////////////////////////////////////
//
// 8-bits-per-channel interface
//

STBIDEF auto stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y,
                                   int *channels_in_file, int desired_channels) noexcept
    -> stbi_uc *;
STBIDEF auto stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y,
                                      int *channels_in_file, int desired_channels) noexcept
    -> stbi_uc *;

#ifndef STBI_NO_STDIO
STBIDEF auto stbi_load(char const *filename, int *x, int *y, int *channels_in_file,
                       int desired_channels) noexcept -> stbi_uc *;
STBIDEF auto stbi_load_from_file(FILE *f, int *x, int *y, int *channels_in_file,
                                 int desired_channels) noexcept -> stbi_uc *;
// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_GIF
STBIDEF auto stbi_load_gif_from_memory(stbi_uc const *buffer, int len, int **delays, int *x, int *y,
                                       int *z, int *comp, int req_comp) noexcept -> stbi_uc *;
#endif

#ifdef STBI_WINDOWS_UTF8
STBIDEF auto stbi_convert_wchar_to_utf8(char *buffer, size_t bufferlen,
                                        const wchar_t *input) noexcept -> int;
#endif

////////////////////////////////////
//
// 16-bits-per-channel interface
//

STBIDEF auto stbi_load_16_from_memory(stbi_uc const *buffer, int len, int *x, int *y,
                                      int *channels_in_file, int desired_channels) noexcept
    -> stbi_us *;
STBIDEF auto stbi_load_16_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y,
                                         int *channels_in_file, int desired_channels) noexcept
    -> stbi_us *;

#ifndef STBI_NO_STDIO
STBIDEF auto stbi_load_16(char const *filename, int *x, int *y, int *channels_in_file,
                          int desired_channels) noexcept -> stbi_us *;
STBIDEF auto stbi_load_from_file_16(FILE *f, int *x, int *y, int *channels_in_file,
                                    int desired_channels) noexcept -> stbi_us *;
#endif

////////////////////////////////////
//
// float-per-channel interface
//
#ifndef STBI_NO_LINEAR
STBIDEF auto stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y,
                                    int *channels_in_file, int desired_channels) noexcept
    -> float *;
STBIDEF auto stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y,
                                       int *channels_in_file, int desired_channels) noexcept
    -> float *;

#  ifndef STBI_NO_STDIO
STBIDEF auto stbi_loadf(char const *filename, int *x, int *y, int *channels_in_file,
                        int desired_channels) noexcept -> float *;
STBIDEF auto stbi_loadf_from_file(FILE *f, int *x, int *y, int *channels_in_file,
                                  int desired_channels) noexcept -> float *;
#  endif
#endif

#ifndef STBI_NO_HDR
STBIDEF void stbi_hdr_to_ldr_gamma(float gamma) noexcept;
STBIDEF void stbi_hdr_to_ldr_scale(float scale) noexcept;
#endif // STBI_NO_HDR

#ifndef STBI_NO_LINEAR
STBIDEF void stbi_ldr_to_hdr_gamma(float gamma) noexcept;
STBIDEF void stbi_ldr_to_hdr_scale(float scale) noexcept;
#endif // STBI_NO_LINEAR

// stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR
STBIDEF auto stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user) noexcept -> int;
STBIDEF auto stbi_is_hdr_from_memory(stbi_uc const *buffer, int len) noexcept -> int;
#ifndef STBI_NO_STDIO
STBIDEF auto stbi_is_hdr(char const *filename) noexcept -> int;
STBIDEF auto stbi_is_hdr_from_file(FILE *f) noexcept -> int;
#endif // STBI_NO_STDIO

// get a VERY brief reason for failure
// on most compilers (and ALL modern mainstream compilers) this is threadsafe
STBIDEF auto stbi_failure_reason() noexcept -> const char *;

// free the loaded image -- this is just free()
STBIDEF void stbi_image_free(void *retval_from_stbi_load);

// get image dimensions & components without fully decoding
STBIDEF auto stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y,
                                   int *comp) noexcept -> int;
STBIDEF auto stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y,
                                      int *comp) noexcept -> int;
STBIDEF auto stbi_is_16_bit_from_memory(stbi_uc const *buffer, int len) noexcept -> int;
STBIDEF auto stbi_is_16_bit_from_callbacks(stbi_io_callbacks const *clbk, void *user) noexcept
    -> int;

#ifndef STBI_NO_STDIO
STBIDEF auto stbi_info(char const *filename, int *x, int *y, int *comp) noexcept -> int;
STBIDEF auto stbi_info_from_file(FILE *f, int *x, int *y, int *comp) noexcept -> int;
STBIDEF auto stbi_is_16_bit(char const *filename) noexcept -> int;
STBIDEF auto stbi_is_16_bit_from_file(FILE *f) noexcept -> int;
#endif

// for image formats that explicitly notate that they have premultiplied alpha,
// we just return the colors as stored in the file. set this flag to force
// unpremultiplication. results are undefined if the unpremultiply overflow.
STBIDEF void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply) noexcept;

// indicate whether we should process iphone images back to canonical format,
// or just pass them through "as-is"
STBIDEF void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert) noexcept;

// flip the image vertically, so the first pixel in the output array is the bottom left
STBIDEF void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip) noexcept;

// as above, but only applies to images loaded on the thread that calls the function
// this function is only available if your compiler supports thread-local variables;
// calling it will fail to link if your compiler doesn't
STBIDEF void stbi_set_unpremultiply_on_load_thread(int flag_true_if_should_unpremultiply) noexcept;
STBIDEF void stbi_convert_iphone_png_to_rgb_thread(int flag_true_if_should_convert) noexcept;
STBIDEF void stbi_set_flip_vertically_on_load_thread(int flag_true_if_should_flip) noexcept;

// ZLIB client - used by PNG, available for other purposes

STBIDEF auto stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size,
                                               int *outlen) noexcept -> char *;
STBIDEF auto stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len,
                                                          int initial_size, int *outlen,
                                                          int parse_header) noexcept -> char *;
STBIDEF auto stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen) noexcept -> char *;
STBIDEF auto stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer,
                                     int ilen) noexcept -> int;

STBIDEF auto stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen) noexcept
    -> char *;
STBIDEF auto stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer,
                                              int ilen) noexcept -> int;

#endif // STBI_INCLUDE_STB_IMAGE_HPP