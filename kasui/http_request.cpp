#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <unistd.h>
#include <fcntl.h>

#include <cassert>

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> // gettimeofday

#include <netdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "http_request.h"

#define ASYNC_DNS

class http_request_impl
{
public:
	using completion_delegate = http_request::completion_delegate;

	http_request_impl();
	~http_request_impl();

	http_request_impl(const http_request_impl&) = delete;
	http_request_impl& operator=(const http_request_impl&) = delete;

	bool get(const char *url, const completion_delegate& delegate);
	bool poll();

	class state {
	public:
		virtual ~state() { }
		virtual void initialize(http_request_impl& req) = 0;
		virtual void poll(http_request_impl& req) = 0;
	};

	void set_state(state *s);

	void on_error();
	void on_completed(int status, const char *content, size_t content_len);

	const std::string& get_host() const
	{ return host_; }

	const int get_port() const
	{ return port_; }

	const std::string& get_path() const
	{ return path_; }

	void cache_addr(const std::string& host, const std::vector<in_addr_t>& addr);

private:
	void init_request();

	completion_delegate delegate_;
	state *state_;
	std::string host_;
	int port_;
	std::string path_;

	// adios, thread safety
	static std::map<std::string, std::vector<in_addr_t>> cache_;
};

std::map<std::string, std::vector<in_addr_t>> http_request_impl::cache_;

namespace {

const int DEFAULT_PORT = 80;

const int CONNECT_TIMEOUT = 5000;

#ifdef ASYNC_DNS
const int DNS_PORT = 53;
const unsigned DNS_RESOLVE_TIMEOUT = 3000;
const in_addr_t NAMESERVER_ADDR = 0x08080808;

struct dns_header
{
	uint16_t id;		// identification number
	uint8_t rd:1;		// recursion desired
	uint8_t tc:1;		// truncated message
	uint8_t aa:1;		// authoritive answer
	uint8_t opcode:4;	// purpose of message
	uint8_t qr:1;		// query/response flag
	uint8_t rcode:4;	// response code
	uint8_t cd:1;		// checking disabled
	uint8_t ad:1;		// authenticated data
	uint8_t z:1;		// its z! reserved
	uint8_t ra:1;		// recursion available
	uint16_t q_count;	// number of question enties
	uint16_t ans_count;	// number of answer entries
	uint16_t auth_count;	// number of authority entries
	uint16_t add_count;	// number of resource entries
};

struct dns_question
{
	uint16_t qtype;
	uint16_t qclass;
};
#endif

uint64_t now()
{
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	return tv.tv_sec*1000ull + tv.tv_usec/1000ull;
}

bool
set_nonblocking(int fd)
{
	int status = fcntl(fd, F_GETFL, 0);
	if (status != -1)
		status = fcntl(fd, F_SETFL, status|O_NONBLOCK);

	return status != -1;
}

int
is_readable(int fd)
{
	assert(fd != -1);

	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval tv = { 0, 0 };

	switch (select(fd + 1, &fds, nullptr, nullptr, &tv)) {
		case -1:
			// wtf?
			log_err("select: %s", strerror(errno));
			return -1;

		case 0:
			return 0;

		default:
			return FD_ISSET(fd, &fds);
	}
}

int
is_writeable(int fd)
{
	assert(fd != -1);

	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval tv = { 0, 0 };

	switch (select(fd + 1, nullptr, &fds, nullptr, &tv)) {
		case -1:
			// wtf?
			log_err("select: %s", strerror(errno));
			return -1;

		case 0:
			return 0;

		default:
			return FD_ISSET(fd, &fds);
	}
}

#ifdef ASYNC_DNS
class resolving_state : public http_request_impl::state
{
public:
	resolving_state();
	~resolving_state() override;

	void initialize(http_request_impl& req) override;
	void poll(http_request_impl& req) override;

private:
	void send_request(http_request_impl& req);

	struct sockaddr_in dns_addr_;
	int dns_fd_;
	uint64_t start_time_;
	int retry_count_;
};
#endif

class resolved_state : public http_request_impl::state
{
public:
	resolved_state(int fd);

	void initialize(http_request_impl&) override { }
	void poll(http_request_impl&) override;

	virtual int is_ready() const = 0;
	virtual void on_ready(http_request_impl&) = 0;

	virtual unsigned timeout() const = 0;

	void close();

protected:
	int fd_;

private:
	uint64_t last_update_;
};

class connecting_state : public resolved_state
{
public:
	connecting_state(const std::vector<in_addr_t>& addr);

	void initialize(http_request_impl& req) override;

	int is_ready() const override
	{ return is_writeable(fd_); }

	void on_ready(http_request_impl& req) override;

	unsigned timeout() const override
	{ return CONNECT_TIMEOUT; }

private:
	std::vector<in_addr_t> addr_;
};

class writing_request_state : public resolved_state
{
public:
	writing_request_state(int fd, const std::string& host, int port, const std::string& path);

	int is_ready() const override
	{ return is_writeable(fd_); }

	void on_ready(http_request_impl& req) override;

	unsigned timeout() const override
	{ return 0; }

private:
	std::string request_;
	size_t bytes_written_;
};

class reading_response_state : public resolved_state
{
public:
	reading_response_state(int fd);

	int is_ready() const override
	{ return is_readable(fd_); }

	void on_ready(http_request_impl& req) override;

	unsigned timeout() const override
	{ return 0; }

private:
	bool on_read(const char *buf, ssize_t size);

	int parse_state_;

	int status_;
	std::vector<char> content_;
};

#ifdef ASYNC_DNS
resolving_state::resolving_state()
: dns_fd_(-1)
, start_time_(0)
, retry_count_(3)
{
	memset(&dns_addr_, 0, sizeof(dns_addr_));
	dns_addr_.sin_family = AF_INET;
	dns_addr_.sin_port = htons(DNS_PORT);
	dns_addr_.sin_addr.s_addr = htonl(NAMESERVER_ADDR);
}

resolving_state::~resolving_state()
{ }

void
resolving_state::initialize(http_request_impl& req)
{
	dns_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (dns_fd_ == -1) {
		log_err("socket: %s", strerror(errno));
		req.on_error();
		return;
	}

	send_request(req);
}

void
resolving_state::send_request(http_request_impl& req)
{
	uint8_t request_buf[65536];

	dns_header *header = reinterpret_cast<dns_header *>(request_buf);
	header->id = htons(rand());
	header->qr = 0;			// query
	header->opcode = 0;		// standard query or reverse lookup?
	header->aa = 0;			// not authoritative
	header->tc = 0;			// not truncated
	header->rd = 1;			// recursion desired
	header->ra = 0;			// recursion not available
	header->z = 0;
	header->ad = 0;
	header->cd = 0;
	header->rcode = 0;
	header->q_count = htons(1);	// # of questions
	header->ans_count = 0;
	header->auth_count = 0;
	header->add_count = 0;

	uint8_t *dest = &request_buf[sizeof(dns_header)];
	const char *src = req.get_host().c_str();

	while (*src) {
		const char *sep = strchr(src, '.');
		if (sep == nullptr)
			sep = src + strlen(src);

		size_t len = sep - src;

		*dest++ = len;
		memcpy(dest, src, len);
		dest += len;

		src = sep;
		if (*src == '.')
			++src;
	}

	++dest;

	dns_question *question = reinterpret_cast<dns_question *>(dest);
	question->qtype = htons(1); // (A) record
	question->qclass = htons(1); // internet

	size_t request_len =
		sizeof(dns_header) +
		req.get_host().size() + 2 +
		sizeof(dns_question);

	ssize_t n = sendto(dns_fd_, request_buf, request_len, 0, reinterpret_cast<struct sockaddr *>(&dns_addr_), sizeof(dns_addr_));
	if (n == -1) {
		log_err("sendto: %s", strerror(errno));
		req.on_error();
		return;
	}

	log_debug("dns query sent");

	set_nonblocking(dns_fd_);

	start_time_ = now();
}

void
resolving_state::poll(http_request_impl& req)
{
	assert(dns_fd_ != -1);

	uint8_t response_buf[65536];

	socklen_t addr_len = sizeof(dns_addr_);
	ssize_t n = recvfrom(
			dns_fd_,
			response_buf, sizeof(response_buf),
			0,
			reinterpret_cast<struct sockaddr *>(&dns_addr_), &addr_len);

	if (n < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			log_err("recvfrom: %s", strerror(errno));
			req.on_error();
		} else {
			if (now() - start_time_ > DNS_RESOLVE_TIMEOUT) {
				log_err("timed out waiting for DNS query response");

				if (!retry_count_) {
					req.on_error();
				} else {
					--retry_count_;
					send_request(req);
				}
			}
		}

		return;
	}

#if 0
45 67 81 80 00 01 00 01 00 00 00 00 06 67 6F 6F         .g...........goo
67 6C 65 03 63 6F 6D 00 00 01 00 01 C0 0C 00 01         gle.com.........
00 01 00 00 00 90 00 04 D8 3A DB EE                     ............

#endif
	log_debug("dns query response received");

	const unsigned char *p =
		response_buf
		+ sizeof(dns_header)
		+ req.get_host().size() + 2
		+ sizeof(dns_question); 

	std::vector<in_addr_t> addr_list;

	while (p - response_buf < n) {
		if (p[0] != 0xc0 || p[2] != 0x00 || p[3] != 0x01 || p[4] != 0x00 || p[5] != 0x01)
			break;

		log_debug("addr: %d.%d.%d.%d", p[12], p[13], p[14], p[15]);

		in_addr_t addr = htonl(p[15] | (p[14] << 8) | (p[13] << 16) | (p[12] << 24));
		addr_list.push_back(addr);

		p += 16;
	}

	if (addr_list.empty()) {
		log_err("invalid dns query response");
		req.on_error();
	} else {
		req.cache_addr(req.get_host(), addr_list);
		req.set_state(new connecting_state(addr_list));
	}
}
#endif

resolved_state::resolved_state(int fd)
: fd_(fd)
, last_update_(now())
{ }

void
resolved_state::close()
{
	if (fd_ != -1) {
		::close(fd_);
		fd_ = -1;
	}
}

void
resolved_state::poll(http_request_impl& req)
{
	switch (is_ready()) {
		case -1:
			req.on_error();
			return;

		case 0:
			// timed out
			if (timeout() != 0) {
				if (now() - last_update_ > timeout()) {
					log_err("timed out");
					req.on_error();
					return;
				}
			}
			break;

		default:
			last_update_ = now();
			on_ready(req);
			break;
	}
}

connecting_state::connecting_state(const std::vector<in_addr_t>& addr)
: resolved_state(-1)
, addr_(addr)
{ }

void
connecting_state::initialize(http_request_impl& req)
{
	fd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd_ == -1) {
		log_err("socket: %s", strerror(errno));
		req.on_error();
		return;
	}

	if (!set_nonblocking(fd_)) {
		log_err("set_nonblocking: %s", strerror(errno));
		close();
		req.on_error();
		return;
	}

	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = addr_.front();
	sin.sin_port = htons(req.get_port());

	log_debug("connecting to %s", inet_ntoa(sin.sin_addr));

	if (::connect(fd_, reinterpret_cast<struct sockaddr *>(&sin), sizeof(sin)) == -1) {
		if (errno != EINPROGRESS) {
			log_err("connect: %s", strerror(errno));
			close();
			req.on_error();
		}
	}
}

void
connecting_state::on_ready(http_request_impl& req)
{
	int status;
	socklen_t len = sizeof(status);

	if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &status, &len) < 0) {
		log_err("getsockopt: %s", strerror(errno));
		req.on_error();
		return;
	}

	if (status) {
		log_err("failed to connect");
		req.on_error();
		return;
	}

	log_debug("connected!");

	req.set_state(new writing_request_state(fd_, req.get_host(), req.get_port(), req.get_path()));
}

writing_request_state::writing_request_state(int fd, const std::string& host, int port, const std::string& path)
: resolved_state(fd)
, bytes_written_(0)
{
	// GET /foo/bar HTTP/1.0
	// Host: localhost:4321
	// Accept: text/html, text/plain, text/css, text/sgml, */*;q=0.01
	// Accept-Encoding: gzip, compress, bzip2
	// Accept-Language: en
	// User-Agent: Lynx/2.8.8dev.12 libwww-FM/2.14 SSL-MM/1.4.1 GNUTLS/2.12.18

	std::stringstream ss;

	ss << "GET " << path << " HTTP/1.0\r\n";

	ss << "Host: " << host;
	if (port != DEFAULT_PORT)
		ss << ":" << port;
	ss << "\r\n";

	ss << "User-Agent: kasui/0.1\r\n";

	ss << "\r\n";

	request_ = ss.str();
}

void
writing_request_state::on_ready(http_request_impl& req)
{
	while (bytes_written_ < request_.size()) {
		ssize_t n = send(fd_, &request_[bytes_written_], request_.size() - bytes_written_, 0);
		if (n < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				log_err("send: %s", strerror(errno));
				req.on_error();
				return;
			}

			break;
		}

		log_debug("wrote %zd bytes", n);

		bytes_written_ += n;
	}

	if (bytes_written_ == request_.size()) {
		log_debug("request written");
		req.set_state(new reading_response_state(fd_));
	}
}

reading_response_state::reading_response_state(int fd)
: resolved_state(fd)
, parse_state_(0)
, status_(0)
{ }

void
reading_response_state::on_ready(http_request_impl& req)
{
	for (;;) {
		char buf[512];

		ssize_t n = recv(fd_, buf, sizeof(buf), 0);
		if (n < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				if (errno == ECONNRESET) {
					req.on_completed(status_, &content_[0], content_.size());
				} else {
					log_err("recv: %s", strerror(errno));
					req.on_error();
				}
			}

			return;
		}

		if (n == 0) {
			req.on_completed(status_, &content_[0], content_.size());
			return;
		}

		log_debug("read %zd bytes", n);

		if (!on_read(buf, n)) {
			req.on_error();
			return;
		}
	}
}

bool
reading_response_state::on_read(const char *buf, ssize_t size)
{
	for (const char *p = buf; p != &buf[size]; p++) {
		char ch = *p;

		switch (parse_state_) {
			case 0:
				// before status
				if (ch == ' ')
					parse_state_ = 1;
				break;

			case 1:
				// status
				if (ch == ' ') {
					parse_state_ = 2;
				} else {
					if (ch < '0' || ch > '9')
						return false;
					status_ = status_*10 + ch - '0';
				}
				break;

			case 2:
				// after status, before \r\n\r\n
				if (ch == '\r')
					parse_state_ = 3;
				break;

			case 3:
				switch (ch) {
					case '\n':
						parse_state_ = 4;
						break;

					case '\r':
						break;

					default:
						parse_state_ = 2;
						break;
				}
				break;

			case 4:
				switch (ch) {
					case '\r':
						parse_state_ = 5;
						break;

					default:
						parse_state_ = 2;
						break;
				}
				break;

			case 5:
				switch (ch) {
					case '\n':
						parse_state_ = 6;
						break;

					case '\r':
						parse_state_ = 3;
						break;

					default:
						parse_state_ = 2;
						break;
				}
				break;

			case 6:
				content_.push_back(ch);
				break;
		}
	}

	return true;
}

} // namespace

http_request_impl::http_request_impl()
: state_(nullptr)
, port_(0)
{ }

http_request_impl::~http_request_impl()
{ }

void
http_request_impl::on_error()
{
	http_response resp;

	resp.success = false;
	delegate_(resp);

	set_state(nullptr);
}

void
http_request_impl::on_completed(int status, const char *content, size_t content_len)
{
	http_response resp;

	resp.success = true;
	resp.status = status;
	resp.content = content;
	resp.content_len = content_len;

	delegate_(resp);

	set_state(nullptr);
}

void
http_request_impl::set_state(state *next_state)
{
	if (state_)
		delete state_;

	if ((state_ = next_state) != nullptr)
		state_->initialize(*this);
}

bool
http_request_impl::get(const char *url, const completion_delegate& delegate)
{
	delegate_ = delegate;

	port_ = 0;
	host_.clear();

	// parse URL

	if (strncmp(url, "http://", 7) != 0)
		return false;

	url += 7;
	bool colon = false;

	while (*url && *url != '/') {
		char ch = *url++;

		if (ch == ':') {
			if (colon) {
				// double colon!
				return false;
			}

			colon = true;
		} else {
			if (!colon) {
				host_.push_back(ch);
			} else {
				if (ch < '0' || ch > '9') {
					// invalid character after colon
					return false;
				}

				port_ = port_*10 + ch - '0';
			}
		}
	}

	path_ = *url ? url : "/";

	if (!colon)
		port_ = DEFAULT_PORT;

	auto it = cache_.find(host_);
	if (it != cache_.end()) {
		set_state(new connecting_state(it->second));
		return true;
	}

#ifdef ASYNC_DNS
	set_state(new resolving_state);
#else
	struct hostent *he = gethostbyname(host_.c_str());
	if (he == 0) {
		log_err("failed to resolve `%s'", host_.c_str());
		return false;
	}

	std::vector<in_addr_t> addr_list;

	for (int i = 0; he->h_addr_list[i]; i++) {
		in_addr_t addr;
		memcpy(&addr, he->h_addr_list[i], he->h_length);

		log_debug("addr: %s", inet_ntoa(*reinterpret_cast<struct in_addr *>(&addr)));

		addr_list.push_back(addr);
	}

	cache_addr(host_, addr_list);

	set_state(new connecting_state(addr_list));
#endif

	return true;
}

bool
http_request_impl::poll()
{
	if (state_)
		state_->poll(*this);

	return state_ != nullptr;
}

void
http_request_impl::cache_addr(const std::string& host, const std::vector<in_addr_t>& addr)
{
	cache_.insert(std::make_pair(host, addr));
}

http_request::http_request()
: impl_(new http_request_impl)
{ }

http_request::~http_request()
{
	delete impl_;
}

bool
http_request::get(const char *url, const completion_delegate& delegate)
{
	return impl_->get(url, delegate);
}

bool
http_request::poll()
{
	return impl_->poll();
}
