#ifndef UTF8_H_
#define UTF8_H_

wchar_t *
utf8_to_wchar(const char *utf8_data, int utf8_data_size);

wchar_t *
utf8_to_wchar(const char *utf8_data);

unsigned char *
wchar_to_utf8(const wchar_t *wchar_data, int wchar_data_size);

unsigned char *
wchar_to_utf8(const wchar_t *wchar_data);

#endif // UTF8_H_
