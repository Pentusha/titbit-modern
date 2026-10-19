/* NONMATCHING: authored native JPEG pixel conversions; canonical behavior source remains
 * in src/native/jpeg_frontend.c. Count and channel arithmetic are preserved.
 */
void __cdecl FUN_0049ba40(const unsigned char *src, unsigned char *dst, int count)
{
    int i;
    for (i = 0; i < count; ++i) dst[i] = src[i];
}
void __cdecl FUN_0049ba70(const unsigned char *src, unsigned int *dst, int count)
{
    for (; count > 0; --count) {
        unsigned int v = *src++;
        *dst++ = (((0xffffff00U | v) << 8 | v) << 8) | v;
    }
}
void __cdecl FUN_0049bab0(const unsigned char *src, unsigned char *dst, int count)
{
    dst += 3;
    for (; count > 0; --count) { *dst = *src++; dst += 4; }
}
void __cdecl FUN_0049bae0(const unsigned char *src, unsigned char *dst, int count)
{
    for (; count > 0; --count) {
        unsigned int r = *src++, g = *src++, b = *src++;
        *dst++ = (unsigned char)((10*r + 18*g + 4*b) >> 5);
    }
}
void __cdecl FUN_0049bb20(const unsigned char *src, unsigned int *dst, int count)
{
    for (; count > 0; --count) {
        unsigned int r = *src++, g = *src++, b = *src++;
        *dst++ = (((0xffffff00U | r) << 8 | g) << 8) | b;
    }
}
void __cdecl FUN_0049bb60(const unsigned char *src, unsigned char *dst, int count)
{
    dst += 3;
    for (; count > 0; --count) {
        unsigned int r = *src++, g = *src++, b = *src++;
        *dst = (unsigned char)((10*r + 18*g + 4*b) >> 5);
        dst += 4;
    }
}
