#include <cstring>

#include <guava2d/xwchar.h>

#include "panic.h"
#include "utf8.h"

enum
{
    MAX_UTF8_BYTES_PER_WCHAR = 3,
};

wchar_t *utf8_to_wchar(const char *utf8_data, int utf8_data_size)
{
    wchar_t *result = new wchar_t[utf8_data_size + 1];

    wchar_t *q = result;
    wchar_t cur_wchar = 0;
    int extra_bytes = 0;

    for (const char *p = utf8_data; p != &utf8_data[utf8_data_size]; p++) {
        unsigned char ch = *p;

        if (extra_bytes == 0) {
            if ((ch & 0x80) == 0) {
                *q++ = ch;
            } else if ((ch & ~0x1f) == 0xc0) {
                cur_wchar = ch & 0x1f;
                extra_bytes = 1;
            } else if ((ch & ~0x0f) == 0xe0) {
                cur_wchar = ch & 0x0f;
                extra_bytes = 2;
            } else if ((ch & ~0x07) == 0xf0) {
                cur_wchar = ch & 0x07;
                extra_bytes = 3;
            } else if ((ch & ~0x03) == 0xf8) {
                cur_wchar = ch & 0x03;
                extra_bytes = 4;
            } else if ((ch & ~0x01) == 0xfc) {
                cur_wchar = ch & 0x01;
                extra_bytes = 5;
            } else {
                goto failed;
            }
        } else {
            if ((ch & ~0x3f) != 0x80)
                goto failed;

            cur_wchar = (cur_wchar << 6) | (ch & 0x3f);

            if (--extra_bytes == 0) {
                *q++ = cur_wchar;
                cur_wchar = 0;
            }
        }
    }

    if (extra_bytes != 0)
        goto failed;

    *q = L'\0';

    return result;

failed:
    panic("invalid UTF-8 character sequence");
    return nullptr;
}

wchar_t *utf8_to_wchar(const char *utf8_data)
{
    return utf8_to_wchar(utf8_data, strlen(utf8_data));
}

unsigned char *wchar_to_utf8(const wchar_t *wchar_data, int wchar_data_size)
{
    unsigned char *result = new unsigned char[MAX_UTF8_BYTES_PER_WCHAR * wchar_data_size + 1];

    unsigned char *q = result;

    for (const wchar_t *p = wchar_data; p != &wchar_data[wchar_data_size]; p++) {
        wchar_t ch = *p;

        if (ch < 0x80) {
            *q++ = ch;
        } else if (ch < 0x800) {
            *q++ = static_cast<unsigned char>(0xc0u + (ch >> 6));
            *q++ = static_cast<unsigned char>(0x80u + (ch & 0x3fu));
        } else {
            *q++ = static_cast<unsigned char>(0xe0u + (ch >> 12));
            *q++ = static_cast<unsigned char>(0x80u + ((ch >> 6) & 0x3fu));
            *q++ = static_cast<unsigned char>(0x80u + (ch & 0x3fu));
        }
    }

    *q = '\0';

    return result;
}

unsigned char *wchar_to_utf8(const wchar_t *wchar_data)
{
    return wchar_to_utf8(wchar_data, wcslen(wchar_data));
}
