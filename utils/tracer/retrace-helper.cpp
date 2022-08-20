/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "retracer.h"

struct buffer_retrace {
	int fd;
	long int address_trace;
	long int address_retrace;
};

struct retrace_context {
	pthread_mutex_t lock;

	/* Key is a file descriptor from the trace, value is the corresponding fd in the retrace context. */
	std::unordered_map<int, int> retrace_fds;

	/* List of output and capture buffers being retraced. */
	std::list<struct buffer_retrace> buffers;
};

static struct retrace_context ctx_retrace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

/* Take a buffer's file descriptor and create a new buffer entry in retrace context. */
void add_buffer_retrace(int fd)
{
	struct buffer_retrace buf;
	memset(&buf, 0, sizeof(buffer_retrace));
	buf.fd = fd;
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

/*
 * Use the buffer file descriptor to get a buffer entry from the retrace context.
 * Add the buffer's memory address from both the trace and retrace context to the buffer entry.
 */
void set_buffer_address_retrace(int fd, long address_trace, long address_retrace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.begin(); it != ctx_retrace.buffers.end(); ++it) {
		if (it->fd == fd) {
			it->address_trace = address_trace;
			it->address_retrace = address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

/* Take an address from the trace and return the corresponding address in the retrace context. */
long int get_buffer_address_retrace(long address_trace)
{
	long int address_retrace = 0;

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
		fprintf(stderr, "fd: %d, address_trace:%ld, address_retrace:%ld\n",
				it->fd, it->address_trace, it->address_retrace);
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

/* Take a file descriptor from the trace and return the corresponding fd in the retrace context. */
int get_fd(int fd_trace)
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

void print_context(void)
{
	print_fds();
	print_buffers_retrace();
	fprintf(stderr, "\n");
}

int retrace_v4l2_ext_control_value(json_object *ctrl_obj)
{
	json_object *value_obj;
	json_object_object_get_ex(ctrl_obj, "value", &value_obj);
	return json_object_get_int(value_obj);
}
