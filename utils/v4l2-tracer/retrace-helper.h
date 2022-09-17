/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef RETRACE_HELPER_H
#define RETRACE_HELPER_H

struct buffer_retrace {
	int fd;
	__u32 offset;
	long address_trace;
	long address_retrace;
};

struct retrace_context {
	__u32 width;
	__u32 height;
	__u32 pixelformat;
	pthread_mutex_t lock;

	/* Key is a file descriptor from the trace, value is the corresponding fd in the retrace context. */
	std::unordered_map<int, int> retrace_fds;

	/* List of output and capture buffers being retraced. */
	std::list<struct buffer_retrace> buffers;
};

bool buffer_in_retrace_context(int fd, __u32 offset = 0);
void add_buffer_retrace(int fd, __u32 offset = 0);

void set_buffer_address_retrace(int fd, __u32 offset, long address_trace, long address_retrace);
long get_retrace_address_from_trace_address(long address_trace);

void add_fd(int fd_trace, int fd_retrace);
int get_fd_retrace_from_fd_trace(int fd_trace);
void remove_fd(int fd_trace);

void set_pixelformat_retrace(__u32 width, __u32 height, __u32 pixelformat);
unsigned get_expected_length_retrace(void);

void print_context(void);

#endif
