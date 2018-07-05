#pragma once

#include <stdint.h>
#include <string>

#ifdef ANDROID_NDK
struct AAsset;
#else
#include <cstdio>
#endif

namespace g2d {

class file_input_stream
{
public:
    file_input_stream(const char *path);
    ~file_input_stream();

    file_input_stream(const file_input_stream&) = delete;
    file_input_stream& operator=(const file_input_stream&) = delete;

    char *gets(char *str, size_t size);

    uint8_t read_uint8();
    uint16_t read_uint16();
    uint32_t read_uint32();
    std::string read_string();

    size_t read(void *buf, size_t size);

    off_t size() const
    { return size_; }

private:
#ifdef ANDROID_NDK
    AAsset *asset_ = nullptr;
#else
    FILE *stream_ = nullptr;
#endif
    off_t size_ = 0;
};

};
