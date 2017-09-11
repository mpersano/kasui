#ifndef FILE_H_
#define FILE_H_

#include <cstdio>
#include <stdint.h>
#include <string>

namespace g2d {

class file_input_stream {
public:
	file_input_stream(const char *path);
	~file_input_stream();

	char *gets(char *str, size_t size);

	uint8_t read_uint8();
	uint16_t read_uint16();
	uint32_t read_uint32();
	std::string read_string();

	size_t read(void *buf, size_t size);

	uint64_t get_bytes_remaining() const
	{ return bytes_remaining_; }

	FILE *get_raw_stream()
	{ return stream_; }

private:
	file_input_stream(const file_input_stream&); // disable copy ctor
	file_input_stream& operator=(const file_input_stream&); // disable assignment

	FILE *stream_;
	off_t bytes_remaining_;
};

};

#endif // FILE_H_
