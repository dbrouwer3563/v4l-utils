/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <sstream> //   ?
#include <vector> ///?
#include "v4l2-tracer-common.h"

struct buffer_retrace {
	int fd;
	__u32 type; /* enum v4l2_buf_type */
	__u32 index;
	__u32 offset;
	long address_trace;
	long address_retrace;
};

struct retrace_context {
	__u32 width;
	__u32 height;
	__u32 pixelformat;
	pthread_mutex_t lock;
	/* Key is a file descriptor from the trace, value is the corresponding fd in the retrace. */
	std::unordered_map<int, int> retrace_fds;
	/* List of output and capture buffers being retraced. */
	std::list<struct buffer_retrace> buffers;
};

static struct retrace_context ctx_retrace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

unsigned long s2flags(std::string s, const flag_def *def)
{
	size_t idx;

	unsigned long val = 0;

	while (def->flag) {
		idx = s.find(def->str);
		if (idx != std::string::npos) {

			/* Special treatment for flag values that have an offset. */
			std::string temp = def->str;
			if (temp == "V4L2_FWHT_FL_COMPONENTS_NUM_MSK") {
				val += (def->flag & (2 << V4L2_FWHT_FL_COMPONENTS_NUM_OFFSET));
			} else if (temp == "V4L2_FWHT_FL_PIXENC_MSK") {
				val += (def->flag & (1 << V4L2_FWHT_FL_PIXENC_OFFSET));
			} else {
				val += def->flag;
			}

			/* erase the portion of the string that has been found including the separators */
			s.erase(idx, strlen(def->str));
			idx = s.find(", ");
			if (idx != std::string::npos)
				s.erase(idx, 2);
		}
		def++;
	}

	if (s.empty())
		return val;

	fprintf(stderr, "Warning: unrecognized flags: \'%s\'\n", s.c_str());

	/* Handle unrecognized flags. */
	std::stringstream ss(s);
	std::vector<std::string> unknown_flags;
	std::string flag_str;
	while (ss >> flag_str)
		unknown_flags.push_back(flag_str);

	for (unsigned long i = 0; i < unknown_flags.size(); i++) {

		/* remove any separators */
		idx = unknown_flags[i].find(",");
		if (idx != std::string::npos) {
			unknown_flags[i].erase(idx, 1);
		}

		/* unknown flags in hex */
		idx = unknown_flags[i].find("0x");
		if (idx != std::string::npos) {
			try {
				val += std::stoul(unknown_flags[i], nullptr, 16);
			} catch (std::invalid_argument& ia) {
				fprintf(stderr, "Warning: \'%s\' is invalid.\n", unknown_flags[i].c_str());
			} catch (std::out_of_range& oor) {
				fprintf(stderr, "Warning: \'%s\' is out of range for a value.\n", unknown_flags[i].c_str());
			}
			continue;
		}

		/* test if unknown flag is a decimal */
		try {
			val += std::stoul(unknown_flags[i], nullptr, 10);
		} catch (std::invalid_argument& ia) {
			fprintf(stderr, "Warning: \'%s\' can't be converted to a flag value.\n", unknown_flags[i].c_str());
			continue;
		} catch (std::out_of_range& oor) {
			fprintf(stderr, "Warning: \'%s\' is out of range for a flag value.\n", unknown_flags[i].c_str());
			continue;
		}
	}

	return val;
}

unsigned long s2val(std::string s, const val_def *def, bool ioctl)
{
	unsigned long val = 0;

	if (s.empty())
		return val;

	while (def->val != 0) {
		if (def->str == s) {
			val = def->val;
			return val;
		}
		def++;
	}

	/* Handle unrecognized value. */
	if (!ioctl)
		fprintf(stderr, "Warning: \'%s\' unrecognized.\n", s.c_str());

	size_t idx;
	idx = s.find("0x");
	if (idx != std::string::npos) {
		try {
			val = std::stoul(s, nullptr, 16);
		} catch (std::invalid_argument& ia) {
			fprintf(stderr, "Warning: \'%s\' is invalid.\n", s.c_str());
		} catch (std::out_of_range& oor) {
			fprintf(stderr, "Warning: \'%s\' is out of range for a value.\n", s.c_str());
		}
		return val;
	}

	try {
		val = std::stoul(s, nullptr, 10);
	} catch (std::invalid_argument& ia) {
		if (!ioctl)
			fprintf(stderr, "Warning: \'%s\' is invalid.\n", s.c_str());
	} catch (std::out_of_range& oor) {
		fprintf(stderr, "Warning: \'%s\' is out of range for a value.\n", s.c_str());
	}
	return val;
}

unsigned long s2ioctl(std::string s)
{
	unsigned long val = 0;

	val = s2val(s, defs_ioctl_video, true);
	if (!val)
		val = s2val(s, defs_ioctl_media, true);

	if (!val || ioctl2s(val).empty())
		fprintf(stderr, "Warning: unrecognized ioctl value: \'%s\'\n", s.c_str());

	return val;
}

bool buffer_in_retrace_context(int fd, __u32 offset)
{
	bool buffer_in_retrace_context = false;
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			buffer_in_retrace_context = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return buffer_in_retrace_context;
}

int get_buffer_fd_retrace(__u32 type, __u32 index)
{
	int fd = 0;  ///can i change to -1?    why is this 0?
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.type == type) && (b.index == index)) {
			fd = b.fd;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return fd;
}

void add_buffer_retrace(int fd, __u32 type, __u32 index, __u32 offset)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	struct buffer_retrace buf = {};
	buf.fd = fd;
	buf.type = type;
	buf.index = index;
	buf.offset = offset;
	ctx_retrace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void remove_buffer_retrace(int fd)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.begin(); it != ctx_retrace.buffers.end(); ++it) {
		if (it->fd == fd) {
			ctx_retrace.buffers.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void set_buffer_address_retrace(int fd, __u32 offset, long address_trace, long address_retrace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			b.address_trace = address_trace;
			b.address_retrace = address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

long get_retrace_address_from_trace_address(long address_trace)
{
	long address_retrace = 0;
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if (b.address_trace == address_trace) {
			address_retrace = b.address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return address_retrace;
}

void print_buffers_retrace(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		fprintf(stderr, "fd: %d, offset: %d, address_trace:%ld, address_retrace:%ld\n",
		        b.fd, b.offset, b.address_trace, b.address_retrace);
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

/*
 * Create a new file descriptor entry in retrace context.
 * Add both the fd from the trace context and the corresponding fd from the retrace context.
 */
void add_fd(int fd_trace, int fd_retrace)
{
	std::pair<int, int> new_pair;
	new_pair = std::make_pair(fd_trace, fd_retrace);
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.retrace_fds.insert(new_pair);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

int get_fd_retrace_from_fd_trace(int fd_trace)
{
	int fd_retrace = 0;  //can be -1?   
	std::unordered_map<int, int>::const_iterator it;

	pthread_mutex_lock(&ctx_retrace.lock);
	it = ctx_retrace.retrace_fds.find(fd_trace);
	if (it != ctx_retrace.retrace_fds.end())
		fd_retrace = it->second;
	pthread_mutex_unlock(&ctx_retrace.lock);

	return fd_retrace;
}

void remove_fd(int fd_trace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.retrace_fds.erase(fd_trace);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void print_fds(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	if (ctx_retrace.retrace_fds.empty())
		fprintf(stderr, "all devices closed\n");
	for (auto it = ctx_retrace.retrace_fds.cbegin(); it != ctx_retrace.retrace_fds.cend(); ++it)
		fprintf(stderr, "fd_trace: %d, fd_retrace: %d\n", it->first, it->second);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void set_pixelformat_retrace(__u32 width, __u32 height, __u32 pixelformat)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.width = width;
	ctx_retrace.height = height;
	ctx_retrace.pixelformat = pixelformat;
	pthread_mutex_unlock(&ctx_retrace.lock);
}

unsigned get_expected_length_retrace(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	unsigned width = ctx_retrace.width;
	unsigned height = ctx_retrace.height;
	unsigned pixelformat = ctx_retrace.pixelformat;
	pthread_mutex_unlock(&ctx_retrace.lock);

	unsigned expected_length = width * height;
	if (pixelformat == V4L2_PIX_FMT_NV12 || pixelformat == V4L2_PIX_FMT_YUV420) {
		expected_length *= 3;
		expected_length /= 2;
		expected_length += (expected_length % 2);
	}
	return expected_length;
}

void print_context(void)
{
	print_fds();
	print_buffers_retrace();
	fprintf(stderr, "\n");
}
