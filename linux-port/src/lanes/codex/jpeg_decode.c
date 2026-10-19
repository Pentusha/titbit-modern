/* Game JPEG frontend, native 0x49bbb0. IJG 6a structures require /Zp4.
 * The original permits metadata-only calls with output == NULL and keeps
 * invalid layout combinations as a no-conversion scanline pass. */
#define fill_memory_input FillMemoryInput
#include "../../native/jpeg_frontend.c"
#include <setjmp.h>

extern int g_005ef6a4;

typedef struct {
    struct jpeg_error_mgr pub;
    jmp_buf recovery;
} JpegRecovery;

/* Native callback 0x49b890: report through IJG, then restore saved context. */
static void JpegErrorExit(j_common_ptr info)
{
    JpegRecovery *error = (JpegRecovery *)info->err;
    (*info->err->output_message)(info);
    longjmp(error->recovery, 1);
}

int __cdecl DecodeJpegMemory(const void *data, int length,
                            unsigned char *output,
                            unsigned int *width, unsigned int *height,
                            unsigned int *source_format,
                            int destination_format, unsigned int flags)
{
    struct jpeg_decompress_struct info;
    JpegRecovery error;
    JSAMPARRAY row;
    unsigned int conversion = 0xd431;

    g_005ef6a4 = 1;
    info.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpegErrorExit;
    if (setjmp(error.recovery)) {
        jpeg_destroy_decompress(&info);
        return 0;
    }
    jpeg_create_decompress(&info);
    game_jpeg_memory_source(&info, data, length);
    jpeg_read_header(&info, TRUE);
    switch (flags & 15) {
    case 0:
        info.dct_method = JDCT_ISLOW;
        info.dither_mode = JDITHER_NONE;
        break;
    case 1:
        info.dct_method = JDCT_FLOAT;
        info.dither_mode = JDITHER_NONE;
        break;
    case 8:
        info.dct_method = JDCT_IFAST;
        info.do_fancy_upsampling = FALSE;
        info.do_block_smoothing = FALSE;
        info.dither_mode = JDITHER_NONE;
        info.two_pass_quantize = FALSE;
        break;
    }
    *width = info.image_width;
    *height = info.image_height;
    *source_format = info.num_components > 2 ? 0 : 8;
    if (output == NULL) {
        jpeg_destroy_decompress(&info);
        return g_005ef6a4;
    }
    if (*source_format == 0) {
        if (destination_format == 0) conversion = 5;
        else if (destination_format == 8) conversion = 4;
        else if (destination_format == 0x340) conversion = 6;
    } else if (*source_format == 8) {
        if (destination_format == 0) conversion = 2;
        else if (destination_format == 8) conversion = 1;
        else if (destination_format == 0x340) conversion = 3;
    }
    jpeg_start_decompress(&info);
    row = (*info.mem->alloc_sarray)((j_common_ptr)&info, JPOOL_IMAGE,
                                  info.output_width * info.output_components, 1);
    while (info.output_scanline < info.output_height) {
        jpeg_read_scanlines(&info, row, 1);
        switch (conversion) {
        case 1: gray_to_gray(*row, output + (info.output_scanline - 1) * info.image_width, info.image_width); break;
        case 2: gray_to_bgra(*row, (unsigned int *)(output + (info.output_scanline - 1) * info.image_width * 4), info.image_width); break;
        case 3: gray_to_alpha(*row, output + (info.output_scanline - 1) * info.image_width * 4, info.image_width); break;
        case 4: rgb_to_gray(*row, output + (info.output_scanline - 1) * info.image_width, info.image_width); break;
        case 5: rgb_to_bgra(*row, (unsigned int *)(output + (info.output_scanline - 1) * info.image_width * 4), info.image_width); break;
        case 6: rgb_to_alpha(*row, output + (info.output_scanline - 1) * info.image_width * 4, info.image_width); break;
        }
    }
    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    return g_005ef6a4;
}
