/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef LIBTRACER_H
#define LIBTRACER_H

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <config.h>
#include <pthread.h>
#include <unistd.h> /* needed for close */
#include <linux/dma-buf.h>
#include <json-c/json.h>
#include <v4l2-info.h>
#include <unordered_map>
#include <list>
#include "trace-info.h"

struct trace_options {
	bool set;
	bool pretty_print_all;
	bool pretty_print_mem;
	bool decoded_data_to_json;
	bool create_yuv_file;
};

struct buffer_trace {
	int fd;
	__u32 type; /* enum v4l2_buf_type */
	__u32 index;
	long address;
	__u32 bytesused;
};

struct trace_context {
	FILE *trace_file;
	std::string trace_filename;
	pthread_mutex_t lock;
	/* Key is the file descriptor, value is the path of the video or media device. */
	std::unordered_map<int, std::string> devices;
	/* List of output and capture buffers being traced. */
	std::list<struct buffer_trace> buffers;
};

bool options_are_set(void);
void set_options(void);
bool pretty_print_mem(void);
bool pretty_print_all(void);
bool write_decoded_data_to_json(void);
bool create_yuv_file(void);

void add_device(int fd, std::string path);
std::string get_device(int fd);
int remove_device(int fd);
void add_buffer_trace(int fd, __u32 type, __u32 index);
int get_buffer_fd_trace(__u32 type, __u32 index);
__u32 get_buffer_type_trace(int fd);
int get_buffer_index_trace(int fd);
void set_buffer_address_trace(int fd, long address);
long get_buffer_address_trace(int fd);
void set_buffer_bytesused_trace(int fd, __u32 bytesused);
long get_buffer_bytesused_trace(int fd);
bool buffer_is_mapped(long buffer_address);
void write_json_object_to_json_file(json_object *jobj, int flags = JSON_C_TO_STRING_PLAIN);

json_object *trace_open(const char *path, int oflag, mode_t mode);
json_object *trace_mmap64(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void trace_mem(int fd);
std::string get_ioctl_request_str(unsigned long request);
json_object *trace_ioctl_args(int fd, unsigned long request, void *arg, bool from_userspace = true);

void trace_v4l2_ctrl_h264_decode_mode(struct v4l2_ext_control ctrl, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_start_code(struct v4l2_ext_control ctrl, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_sps(void *p_h264_sps, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_pps(void *p_h264_pps, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_scaling_matrix(void *p_h264_scaling_matrix, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_pred_weights(void *p_h264_pred_weights, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_slice_params(void *p_h264_slice_params, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_decode_params(void *p_h264_decode_params, json_object *ctrl_obj);

void trace_v4l2_ctrl_vp8_frame(void *p_vp8_frame, json_object *ctrl_obj);

void trace_v4l2_ctrl_fwht_params(void *p_fwht_params, json_object *ctrl_obj);

#endif
