/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef TRACE_HELPER_H
#define TRACE_HELPER_H

struct buffer_trace {
	int fd;
	__u32 type; /* enum v4l2_buf_type */
	__u32 index;
	__u32 offset;
	__u32 bytesused;
	long display_order;
	unsigned long address;
};

struct trace_context {
	__u32 width;
	__u32 height;
	FILE *trace_file;
	__u32 pixelformat;
	int pic_order_ctn_lsb;
	int max_pic_order_cnt_lsb;
	pthread_mutex_t lock;
	__u32 compression_format;
	std::string trace_filename;
	int compressed_frame_count;
	std::list<long> decode_order;
	std::list<struct buffer_trace> buffers;
	std::unordered_map<int, std::string> devices; /* key:fd, value: path of the device */
};

struct trace_options {
	bool options_are_set;
	bool verbose;
	bool pretty_print_all;
	bool pretty_print_mem;
	bool write_decoded_data_to_yuv_file;
	bool write_decoded_data_to_json_file;
};

bool options_are_set(void);
void set_options(void);
bool option_is_set_verbose(void);
bool option_is_set_pretty_print_mem(void);
bool option_is_set_write_decoded_data_to_json_file(void);

bool is_video_or_media_device(const char *path);
void add_device(int fd, std::string path);
std::string get_device(int fd);
int remove_device(int fd);
int count_devices(void);

void add_buffer_trace(int fd, __u32 type, __u32 index, __u32 offset = 0);
bool buffer_in_trace_context(int fd, __u32 offset = 0);
__u32 get_buffer_type_trace(int fd, __u32 offset = 0);
int get_buffer_index_trace(int fd, __u32 offset);
long get_buffer_bytesused_trace(int fd, __u32 offset);
void set_buffer_address_trace(int fd, __u32 offset, unsigned long address);
unsigned long get_buffer_address_trace(int fd, __u32 offset);
bool buffer_is_mapped(unsigned long buffer_address);

std::string get_ioctl_request_str(unsigned long request);

void s_ext_ctrls_setup(struct v4l2_ext_controls *ext_controls);
void qbuf_setup(struct v4l2_buffer *buf);
void streamoff_cleanup(v4l2_buf_type buf_type);
void g_fmt_setup_trace(struct v4l2_format *format);
void s_fmt_setup(struct v4l2_format *format);
void expbuf_setup(int fd, struct v4l2_exportbuffer *export_buffer);
void querybuf_setup(int fd, struct v4l2_buffer *buf);

void print_decode_order(void);
void print_buffers_trace(void);

void write_json_object_to_json_file(json_object *jobj, int flags = JSON_C_TO_STRING_PLAIN);
void close_json_file(void);

#endif
