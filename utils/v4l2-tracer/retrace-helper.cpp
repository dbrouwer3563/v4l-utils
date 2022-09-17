/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer.h"

static struct retrace_context ctx_retrace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

bool buffer_in_retrace_context(int fd, __u32 offset)
{
	bool buffer_in_retrace_context = false;
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.begin(); it != ctx_retrace.buffers.end(); ++it) {
		if ((it->fd == fd) && (it->offset == offset)) {
			buffer_in_retrace_context = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return buffer_in_retrace_context;
}

void add_buffer_retrace(int fd, __u32 offset)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	struct buffer_retrace buf;
	memset(&buf, 0, sizeof(buffer_retrace));
	buf.fd = fd;
	buf.offset = offset;
	ctx_retrace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void set_buffer_address_retrace(int fd, __u32 offset, long address_trace, long address_retrace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.begin(); it != ctx_retrace.buffers.end(); ++it) {
		if ((it->fd == fd) && (it->offset == offset)) {
			it->address_trace = address_trace;
			it->address_retrace = address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

long get_retrace_address_from_trace_address(long address_trace)
{
	long address_retrace = 0;

	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.cbegin(); it != ctx_retrace.buffers.cend(); ++it) {
		if (it->address_trace == address_trace) {
			address_retrace = it->address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);

	return address_retrace;
}

void print_buffers_retrace(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.cbegin(); it != ctx_retrace.buffers.cend(); ++it) {
		fprintf(stderr, "fd: %d, offset: %d, address_trace:%ld, address_retrace:%ld\n",
		        it->fd, it->offset, it->address_trace, it->address_retrace);
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
	int fd_retrace = 0;
	std::unordered_map<int, int>::const_iterator it;

	pthread_mutex_lock(&ctx_retrace.lock);
	it = ctx_retrace.retrace_fds.find(fd_trace);
	if (it != ctx_retrace.retrace_fds.end())
		fd_retrace = it->second;
	pthread_mutex_unlock(&ctx_retrace.lock);

	return fd_retrace;
}

/* Using a file descriptor from the trace, find and remove an fd entry from the retrace context.*/
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
	for ( auto it = ctx_retrace.retrace_fds.cbegin(); it != ctx_retrace.retrace_fds.cend(); ++it )
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
	unsigned width;
	unsigned height;
	unsigned pixelformat;
	unsigned expected_length;

	pthread_mutex_lock(&ctx_retrace.lock);
	width = ctx_retrace.width;
	height = ctx_retrace.height;
	pixelformat = ctx_retrace.pixelformat;
	pthread_mutex_unlock(&ctx_retrace.lock);

	expected_length = width * height;
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
