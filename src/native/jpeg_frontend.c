/* Reconstructed game JPEG memory input and pixel conversion routines.
 * The source-manager behavior derives from IJG 6a jdatasrc.c; see
 * ../jpeg/README for the Independent JPEG Group license and attribution.
 * Native evidence and strict compiled comparisons are recorded separately.
 */
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "../jpeg/jpeglib.h"
#include "../jpeg/jerror.h"

typedef struct {
  struct jpeg_source_mgr pub;
  JOCTET *buffer;
  boolean start_of_file;
  int position;
  int length;
  const JOCTET *data;
} memory_source;
#define INPUT_BUF_SIZE 16384

static void init_memory_source(j_decompress_ptr cinfo)
{
  memory_source *src = (memory_source *)cinfo->src;
  src->start_of_file = TRUE;
}

static boolean fill_memory_input(j_decompress_ptr cinfo)
{
  memory_source *src = (memory_source *)cinfo->src;
  size_t nbytes = INPUT_BUF_SIZE;
  if (src->position + INPUT_BUF_SIZE > src->length)
    nbytes = src->length - src->position;
  memcpy(src->buffer, src->data + src->position, nbytes);
  src->position += nbytes;
  if (nbytes <= 0) {
    if (src->start_of_file) ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    src->buffer[0] = (JOCTET)0xff;
    src->buffer[1] = (JOCTET)JPEG_EOI;
    nbytes = 2;
  }
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;
  return TRUE;
}

static void skip_memory_input(j_decompress_ptr cinfo, long num_bytes)
{
  memory_source *src = (memory_source *)cinfo->src;
  if (num_bytes > 0) {
    while (num_bytes > (long)src->pub.bytes_in_buffer) {
      num_bytes -= (long)src->pub.bytes_in_buffer;
      (void)fill_memory_input(cinfo);
    }
    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
  }
}

static void term_memory_source(j_decompress_ptr cinfo) {}

void game_jpeg_memory_source(j_decompress_ptr cinfo, const void *data, int length)
{
  memory_source *src;
  if (cinfo->src == NULL) {
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
                                sizeof(memory_source));
    src = (memory_source *)cinfo->src;
    src->buffer = (JOCTET *)(*cinfo->mem->alloc_small)
      ((j_common_ptr)cinfo, JPOOL_PERMANENT, INPUT_BUF_SIZE);
  }
  src = (memory_source *)cinfo->src;
  src->pub.init_source = init_memory_source;
  src->pub.fill_input_buffer = fill_memory_input;
  src->pub.skip_input_data = skip_memory_input;
  src->pub.resync_to_restart = jpeg_resync_to_restart;
  src->pub.term_source = term_memory_source;
  src->data = (const JOCTET *)data;
  src->position = 0;
  src->length = length;
  src->pub.bytes_in_buffer = 0;
  src->pub.next_input_byte = NULL;
}

void gray_to_gray(const unsigned char *src, unsigned char *dst, int count)
{
  int i;
  for (i = 0; i < count; ++i) *dst++ = *src++;
}

void gray_to_bgra(const unsigned char *src, unsigned int *dst, int count)
{
  int i;
  for (i = 0; i < count; ++i) {
    unsigned int v = *src++;
    *dst++ = (((0xffffff00U | v) << 8 | v) << 8) | v;
  }
}

void gray_to_alpha(const unsigned char *src, unsigned char *dst, int count)
{
  int i;
  dst += 3;
  for (i = 0; i < count; ++i) { *dst = *src++; dst += 4; }
}

void rgb_to_gray(const unsigned char *src, unsigned char *dst, int count)
{
  int i;
  for (i = 0; i < count; ++i) {
    unsigned int r = *src++, g = *src++, b = *src++;
    *dst++ = (unsigned char)((10*r + 18*g + 4*b) >> 5);
  }
}

void rgb_to_bgra(const unsigned char *src, unsigned int *dst, int count)
{
  int i;
  for (i = 0; i < count; ++i) {
    unsigned int r = *src++, g = *src++, b = *src++;
    *dst++ = (((0xffffff00U | r) << 8 | g) << 8) | b;
  }
}

void rgb_to_alpha(const unsigned char *src, unsigned char *dst, int count)
{
  int i;
  dst += 3;
  for (i = 0; i < count; ++i) {
    unsigned int r = *src++, g = *src++, b = *src++;
    *dst = (unsigned char)((10*r + 18*g + 4*b) >> 5);
    dst += 4;
  }
}
#ifdef __cplusplus
}
#endif
