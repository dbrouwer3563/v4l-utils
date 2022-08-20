/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

struct trace_options options;

struct trace_context ctx_trace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

bool options_are_set(void)
{
	return options.set;
}

void set_options(void)
{
	options.pretty_print_mem = getenv("TRACE_OPTION_PRETTY_PRINT_MEM") ? true : false;
	options.pretty_print_all = getenv("TRACE_OPTION_PRETTY_PRINT_ALL") ? true : false;
	options.decoded_data_to_json = getenv("TRACE_OPTION_DECODED_TO_JSON") ? true : false;
	options.create_yuv_file = getenv("TRACE_OPTION_CREATE_YUV_FILE") ? true : false;
	options.set = true;
}

bool pretty_print_mem(void)
{
	return options.pretty_print_mem;
}

bool pretty_print_all(void)
{
	return options.pretty_print_all;
}

bool write_decoded_data_to_json(void)
{
	return options.decoded_data_to_json;
}

bool create_yuv_file(void)
{
	return options.create_yuv_file;
}

void add_device(int fd, std::string path)
{
	std::pair<int, std::string> new_pair = std::make_pair(fd, path);
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.devices.insert(new_pair);
	pthread_mutex_unlock(&ctx_trace.lock);
}

std::string get_device(int fd)
{
	std::string path;
	std::unordered_map<int, std::string>::const_iterator it;
	pthread_mutex_lock(&ctx_trace.lock);
	it = ctx_trace.devices.find(fd);
	if (it != ctx_trace.devices.end())
		path = it->second;
	pthread_mutex_unlock(&ctx_trace.lock);
	return path;
}

int remove_device(int fd)
{
	int ret = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	ret = ctx_trace.devices.erase(fd);
	pthread_mutex_unlock(&ctx_trace.lock);
	return ret;
}

void add_buffer_trace(int fd, __u32 type, __u32 index)
{
	struct buffer_trace buf;
	memset(&buf, 0, sizeof(buffer_trace));
	buf.fd = fd;
	buf.type = type;
	buf.index = index;
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_trace.lock);
}

int get_buffer_fd_trace(__u32 type, __u32 index)
{
	int fd = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if ((it->type == type) && (it->index == index)) {
			fd = it->fd;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return fd;
}

__u32 get_buffer_type_trace(int fd)
{
	__u32 ret = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			ret = it->type;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return ret;
}

int get_buffer_index_trace(int fd)
{
	int index = -1;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			index = it->index;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return index;
}

void set_buffer_address_trace(int fd, long address)
{
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			it->address = address;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

long get_buffer_address_trace(int fd)
{
	long int address = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			address = it->address;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return address;
}

void set_buffer_bytesused_trace(int fd, __u32 bytesused)
{
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			it->bytesused = bytesused;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

long get_buffer_bytesused_trace(int fd)
{
	long int bytesused = -1;

	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			bytesused = it->bytesused;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);

	return bytesused;
}

/* Returns true if the memory address is a mapped buffer address, false otherwise. */
bool buffer_is_mapped(long buffer_address)
{
	bool ret = false;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.cbegin(); it != ctx_trace.buffers.cend(); ++it) {
		if (it->address == buffer_address) {
			ret = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return ret;
}

void write_json_object_to_json_file(json_object *jobj, int flags)
{
	if (pretty_print_all())
		flags = JSON_C_TO_STRING_PRETTY;

	std::string json_str = json_object_to_json_string_ext(jobj, flags);
	pthread_mutex_lock(&ctx_trace.lock);

	if (ctx_trace.trace_filename.empty()) {
		ctx_trace.trace_filename = getenv("TRACE_ID");
		ctx_trace.trace_filename += ".json";
		ctx_trace.trace_file = fopen(ctx_trace.trace_filename.c_str(), "a");
	}

	fwrite(json_str.c_str(), sizeof(char), json_str.length(), ctx_trace.trace_file);
	fputs(",\n", ctx_trace.trace_file);
	fflush(ctx_trace.trace_file);
	pthread_mutex_unlock(&ctx_trace.lock);
}
