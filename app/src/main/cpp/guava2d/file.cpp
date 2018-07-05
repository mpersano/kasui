#include <algorithm>

#include <cstring>
#include <cerrno>

#ifdef ANDROID_NDK
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
extern AAssetManager *g_asset_manager;
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h> // for PATH_MAX
#endif

#include "panic.h"
#include "file.h"

namespace g2d {

file_input_stream::file_input_stream(const char *path)
{
#ifndef ANDROID_NDK
    char real_path[PATH_MAX];

    sprintf(real_path, "assets/%s", path);

    struct stat sb;

    if (::stat(real_path, &sb) < 0)
        panic("stat on `%s' failed: %s\n", real_path, strerror(errno));

    if ((sb.st_mode & S_IFMT) != S_IFREG)
        panic("`%s' not a regular file\n");

    size_ = sb.st_size;

    if (!(stream_ = ::fopen(real_path, "rb")))
        panic("failed to open `%s': %s\n", real_path, strerror(errno));
#else
    asset_ = AAssetManager_open(g_asset_manager, path, AASSET_MODE_UNKNOWN);
    // XXX does this return nullptr on error?

    size_ = AAsset_getLength(asset_);
#endif
}

file_input_stream::~file_input_stream()
{
#ifndef ANDROID_NDK
    if (stream_)
        fclose(stream_);
#else
    if (asset_)
        AAsset_close(asset_);
#endif
}

uint8_t
file_input_stream::read_uint8()
{
    uint8_t v;

    if (read(&v, 1) != 1)
        panic("read failed");

    return v;
}

uint16_t
file_input_stream::read_uint16()
{
    uint16_t lo = static_cast<uint16_t>(read_uint8());
    uint16_t hi = static_cast<uint16_t>(read_uint8());
    return lo | (hi << 8);
}

uint32_t
file_input_stream::read_uint32()
{
    uint32_t lo = static_cast<uint32_t>(read_uint16());
    uint32_t hi = static_cast<uint32_t>(read_uint16());
    return lo | (hi << 16);
}

std::string
file_input_stream::read_string()
{
    uint8_t len = read_uint8();

    std::string str;
    for (int i = 0; i < len; i++)
        str.push_back(read_uint8());

    return str;
}

char *
file_input_stream::gets(char *str, size_t size)
{
    char *p;
    for (p = str, size--; size > 0; --size) {
        if (read(p, 1) == 0)
            break;
        if (*p++ == '\n')
            break;
    }
    *p = '\0';
    return p == str ? nullptr : p;
}

size_t
file_input_stream::read(void *buf, size_t size)
{
#ifndef ANDROID_NDK
    return fread(buf, 1, size, stream_);
#else
    return AAsset_read(asset_, buf, size);
#endif
}

}
