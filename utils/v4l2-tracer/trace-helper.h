/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef TRACE_HELPER_H
#define TRACE_HELPER_H

bool is_video_or_media_device(const char *path);
void add_device(int fd, std::string path);
std::string get_device(int fd);
int remove_device(int fd);
int count_devices(void);
void print_devices(void);

void set_media_device(std::string media_device);
std::string get_media_device(void);

bool buffer_in_trace_context(int fd, __u32 offset = 0);
__u32 get_buffer_type_trace(int fd, __u32 offset = 0);
int get_buffer_index_trace(int fd, __u32 offset);
long get_buffer_bytesused_trace(int fd, __u32 offset);
void set_buffer_address_trace(int fd, __u32 offset, unsigned long address);
unsigned long get_buffer_address_trace(int fd, __u32 offset);
bool buffer_is_mapped(unsigned long buffer_address);

void s_ext_ctrls_setup(struct v4l2_ext_controls *ext_controls);
void qbuf_setup(struct v4l2_buffer *buf);
void streamoff_cleanup(v4l2_buf_type buf_type);
void g_fmt_setup_trace(struct v4l2_format *format);
void s_fmt_setup(struct v4l2_format *format);
void expbuf_setup(struct v4l2_exportbuffer *export_buffer);
void querybuf_setup(int fd, struct v4l2_buffer *buf);

void write_json_object_to_json_file(json_object *jobj, int flags = JSON_C_TO_STRING_PLAIN);
void close_json_file(void);

#endif
