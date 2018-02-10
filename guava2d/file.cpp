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

	bytes_remaining_ = sb.st_size;

	if (!(stream_ = ::fopen(real_path, "rb")))
		panic("failed to open `%s': %s\n", real_path, strerror(errno));
#else
	AAsset *asset = AAssetManager_open(g_asset_manager, path, AASSET_MODE_UNKNOWN);

	off_t offset;

        int fd = AAsset_openFileDescriptor(asset, &offset, &bytes_remaining_);

	stream_ = fdopen(fd, "r");
	fseek(stream_, offset, SEEK_SET);

	AAsset_close(asset);
#endif
}

file_input_stream::~file_input_stream()
{
	if (stream_)
		fclose(stream_);
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
	if (bytes_remaining_ <= 0) {
		return nullptr;
	} else {
		if (size > bytes_remaining_ + 1)
			size = bytes_remaining_ + 1;

		::fgets(str, size, stream_);

		bytes_remaining_ -= strlen(str);

		return str;
	}
}

size_t
file_input_stream::read(void *buf, size_t size)
{
	if (bytes_remaining_ <= 0)
		return 0;

	size_t bytes_read = ::fread(buf, 1, std::min(size, static_cast<size_t>(bytes_remaining_)), stream_);
	bytes_remaining_ -= bytes_read;

	return bytes_read;
}

}
