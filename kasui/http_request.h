#pragma once

#include <functional>

struct http_response
{
	bool success;
	int status;
	const char *content;
	size_t content_len;
};

class http_request_impl;

class http_request
{
public:
	using completion_delegate = std::function<void(const http_response&)>;

	http_request();
	~http_request();

	http_request(const http_request&) = delete;
	http_request& operator=(const http_request&) = delete;

	bool get(const char *url, const completion_delegate& delegate);
	bool poll();

private:
	http_request_impl *impl_;
};
