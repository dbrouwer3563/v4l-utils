/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <math.h> /* for pow */
#include "v4l2-tracer-common.h"

void trace_mem(int fd, __u32 offset, __u32 type, int index, __u32 bytesused, unsigned long start);
void trace_mem_encoded(int fd, __u32 offset);

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
	int pic_order_cnt_lsb;
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

std::string ver2s(unsigned int version)
{
	char buf[16];
	sprintf(buf, "%d.%d.%d", version >> 16, (version >> 8) & 0xff, version & 0xff);
	return buf;
}

struct trace_options options;

struct trace_context ctx_trace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

bool options_are_set(void)
{
	return options.options_are_set;
}

void set_options(void)
{
	options.verbose = getenv("TRACE_OPTION_VERBOSE") ? true : false;
	options.pretty_print_mem = getenv("TRACE_OPTION_PRETTY_PRINT_MEM") ? true : false;
	options.pretty_print_all = getenv("TRACE_OPTION_PRETTY_PRINT_ALL") ? true : false;
	options.write_decoded_data_to_json_file = getenv("TRACE_OPTION_WRITE_DECODED_TO_JSON_FILE") ? true : false;
	options.write_decoded_data_to_yuv_file = getenv("TRACE_OPTION_WRITE_DECODED_TO_YUV_FILE") ? true : false;
	options.options_are_set = true;
}

bool option_is_set_verbose(void)
{
	return options.verbose;
}

bool option_is_set_pretty_print_mem(void)
{
	return options.pretty_print_mem;
}

bool option_is_set_pretty_print_all(void)
{
	return options.pretty_print_all;
}

bool option_is_set_write_decoded_data_to_json_file(void)
{
	return options.write_decoded_data_to_json_file;
}

bool is_video_or_media_device(const char *path)
{
	std::string dev_path_video = "/dev/video";
	std::string dev_path_media = "/dev/media";
	if (strncmp(path, dev_path_video.c_str(), dev_path_video.length()) &&
	    strncmp(path, dev_path_media.c_str(), dev_path_media.length()))
		return false;
	return true;
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

int count_devices(void)
{
	int count = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	count = ctx_trace.devices.size();
	pthread_mutex_unlock(&ctx_trace.lock);
	return count;
}

void set_pixelformat_trace(__u32 width, __u32 height, __u32 pixelformat)
{
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.width = width;
	ctx_trace.height = height;
	ctx_trace.pixelformat = pixelformat;
	pthread_mutex_unlock(&ctx_trace.lock);
}

__u32 get_pixelformat(void)
{
	__u32 pixelformat = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	pixelformat = ctx_trace.pixelformat;
	pthread_mutex_unlock(&ctx_trace.lock);
	return pixelformat;
}

void set_compression_format(__u32 compression_format)
{
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.compression_format = compression_format;
	pthread_mutex_unlock(&ctx_trace.lock);
}

__u32 get_compression_format(void)
{
	__u32 compression_format = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	compression_format = ctx_trace.compression_format;
	pthread_mutex_unlock(&ctx_trace.lock);
	return compression_format;
}

void set_compressed_frame_count(int count)
{
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.compressed_frame_count = count;
	pthread_mutex_unlock(&ctx_trace.lock);
}

int get_compressed_frame_count(void)
{
	int ret = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	ret = ctx_trace.compressed_frame_count;
	pthread_mutex_unlock(&ctx_trace.lock);
	return ret;
}

void print_decode_order(void)
{
	pthread_mutex_lock(&ctx_trace.lock);
	fprintf(stderr, "Decode order: ");
	for (auto &num : ctx_trace.decode_order)
		fprintf(stderr, "%ld, ",  num);
	fprintf(stderr, ".\n");
	pthread_mutex_unlock(&ctx_trace.lock);
}

void set_decode_order(long decode_order)
{
	if (option_is_set_verbose())
		fprintf(stderr, "%s: %ld\n", __func__, decode_order);

	std::list<long>::iterator it;
	pthread_mutex_lock(&ctx_trace.lock);
	it = find(ctx_trace.decode_order.begin(), ctx_trace.decode_order.end(), decode_order);
	if (it == ctx_trace.decode_order.end())
		ctx_trace.decode_order.push_front(decode_order);
	pthread_mutex_unlock(&ctx_trace.lock);

	if (option_is_set_verbose())
		print_decode_order();
}

long get_decode_order(void)
{
	long decode_order = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	if (!ctx_trace.decode_order.empty())
		decode_order = ctx_trace.decode_order.front();
	pthread_mutex_unlock(&ctx_trace.lock);
	return decode_order;
}

void set_max_pic_order_cnt_lsb(int max_pic_order_cnt_lsb)
{
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.max_pic_order_cnt_lsb = max_pic_order_cnt_lsb;
	pthread_mutex_unlock(&ctx_trace.lock);
}

int get_max_pic_order_cnt_lsb(void)
{
	int max_pic_order_cnt_lsb = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	max_pic_order_cnt_lsb = ctx_trace.max_pic_order_cnt_lsb;
	pthread_mutex_unlock(&ctx_trace.lock);
	return max_pic_order_cnt_lsb;
}

void set_pic_order_cnt_lsb(int pic_order_cnt_lsb)
{
	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.pic_order_cnt_lsb = pic_order_cnt_lsb;
	pthread_mutex_unlock(&ctx_trace.lock);
}

int get_pic_order_cnt_lsb(void)
{
	int pic_order_cnt_lsb = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	pic_order_cnt_lsb = ctx_trace.pic_order_cnt_lsb;
	pthread_mutex_unlock(&ctx_trace.lock);
	return pic_order_cnt_lsb;
}

void add_buffer_trace(int fd, __u32 type, __u32 index, __u32 offset = 0)
{
	struct buffer_trace buf = {};
	buf.fd = fd;
	buf.type = type;
	buf.index = index;
	buf.offset = offset;
	buf.display_order = -1;

	pthread_mutex_lock(&ctx_trace.lock);
	ctx_trace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_trace.lock);
}

void remove_buffer_trace(int fd)
{
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
		if (it->fd == fd) {
			ctx_trace.buffers.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

bool buffer_in_trace_context(int fd, __u32 offset)
{
	bool buffer_in_trace_context = false;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			buffer_in_trace_context = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return buffer_in_trace_context;
}

int get_buffer_fd_trace(__u32 type, __u32 index)
{
	int fd = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.type == type) && (b.index == index)) {
			fd = b.fd;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return fd;
}

__u32 get_buffer_type_trace(int fd, __u32 offset)
{
	__u32 type = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			type = b.type;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return type;
}

int get_buffer_index_trace(int fd, __u32 offset)
{
	int index = -1;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			index = b.index;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return index;
}

__u32 get_buffer_offset_trace(__u32 type, __u32 index)
{
	__u32 offset = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.type == type) && (b.index == index)) {
			offset = b.offset;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return offset;
}

void set_buffer_bytesused_trace(int fd, __u32 offset, __u32 bytesused)
{
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			b.bytesused = bytesused;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

long get_buffer_bytesused_trace(int fd, __u32 offset)
{
	long bytesused = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			bytesused = b.bytesused;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return bytesused;
}

void set_buffer_display_order(int fd, __u32 offset, long display_order)
{
	if (option_is_set_verbose())
		fprintf(stderr, "%s: %ld\n", __func__, display_order);
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			b.display_order = display_order;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

void set_buffer_address_trace(int fd, __u32 offset, unsigned long address)
{
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			b.address = address;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

unsigned long get_buffer_address_trace(int fd, __u32 offset)
{
	unsigned long address = 0;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			address = b.address;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return address;
}

bool buffer_is_mapped(unsigned long buffer_address)
{
	bool ret = false;
	pthread_mutex_lock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		if (b.address == buffer_address) {
			ret = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	return ret;
}

void print_buffers_trace(void)
{
	pthread_mutex_unlock(&ctx_trace.lock);
	for (auto &b : ctx_trace.buffers) {
		fprintf(stderr, "fd: %d, %s, index: %d, display_order: %ld, bytesused: %d, ",
		        b.fd, buftype2s(b.type).c_str(), b.index, b.display_order, b.bytesused);
		fprintf(stderr, "address: %lu, offset: %u \n",  b.address, b.offset);
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}

unsigned get_expected_length_trace()
{
	unsigned width;
	unsigned height;
	unsigned pixelformat;
	unsigned expected_length;

	pthread_mutex_lock(&ctx_trace.lock);
	width = ctx_trace.width;
	height = ctx_trace.height;
	pixelformat = ctx_trace.pixelformat;
	pthread_mutex_unlock(&ctx_trace.lock);

	/*
	 * TODO: this assumes that the stride is equal to the real width and that the
	 * padding follows the end of the chroma plane. It could be improved by
	 * following the model in v4l2-ctl-streaming.cpp read_write_padded_frame()
	 */
	expected_length = width * height;
	if (pixelformat == V4L2_PIX_FMT_NV12 || pixelformat == V4L2_PIX_FMT_YUV420) {
		expected_length *= 3;
		expected_length /= 2;
		expected_length += (expected_length % 2);
	}
	return expected_length;
}

void trace_mem_decoded(void)
{
	int displayed_count = 0;
	unsigned expected_length = get_expected_length_trace();

	pthread_mutex_lock(&ctx_trace.lock);

	while (!ctx_trace.decode_order.empty()) {

		std::list<buffer_trace>::iterator it;
		long next_frame_to_be_displayed = *std::min_element(ctx_trace.decode_order.begin(),
		                                                    ctx_trace.decode_order.end());

		for (it = ctx_trace.buffers.begin(); it != ctx_trace.buffers.end(); ++it) {
			if (it->display_order != next_frame_to_be_displayed)
				continue;
			if (!it->address)
				break;
			if (it->bytesused < expected_length)
				break;
			if (option_is_set_verbose())
				fprintf(stderr, "%s, displaying: %ld, %s, index: %d\n",
				        __func__, it->display_order, buftype2s(it->type).c_str(), it->index);
			displayed_count++;

			if (options.write_decoded_data_to_yuv_file) {
				std::string filename = getenv("TRACE_ID");
				filename +=  ".yuv";
				FILE *fp = fopen(filename.c_str(), "a");
				unsigned char *buffer_pointer = (unsigned char*) it->address;
				for (__u32 i = 0; i < expected_length; i++)
					fwrite(&buffer_pointer[i], sizeof(unsigned char), 1, fp);
				fclose(fp);
			}
			int fd = it->fd;
			__u32 offset = it->offset;
			__u32 type = it->type;
			int index = it->index;
			__u32 bytesused = it->bytesused;
			unsigned long start = it->address;

			pthread_mutex_unlock(&ctx_trace.lock);
			trace_mem(fd, offset, type, index, bytesused, start);
			pthread_mutex_lock(&ctx_trace.lock);

			ctx_trace.decode_order.remove(next_frame_to_be_displayed);
			it->display_order = -1;
			break;
		}
		if (!it->address || it == ctx_trace.buffers.end() || it->bytesused < expected_length)
			break;
	}
	pthread_mutex_unlock(&ctx_trace.lock);
	set_compressed_frame_count(get_compressed_frame_count() - displayed_count);
}

void s_ext_ctrls_setup(struct v4l2_ext_controls *ext_controls)
{
	if (ext_controls->which != V4L2_CTRL_WHICH_REQUEST_VAL)
		return;

	if (option_is_set_verbose())
		fprintf(stderr, "\n%s\n", __func__);

	/*
	 * Since userspace sends H264 frames out of order, get information
	 * about the correct display order of each frame so that v4l2-tracer
	 * can write the decoded frames to a file.
	 */
	for (__u32 i = 0; i < ext_controls->count; i++) {
		struct v4l2_ext_control ctrl = ext_controls->controls[i];

		switch (ctrl.id) {
		case V4L2_CID_STATELESS_H264_SPS: {
			set_max_pic_order_cnt_lsb(pow(2, ctrl.p_h264_sps->log2_max_pic_order_cnt_lsb_minus4 + 4));
			break;
		}
		case V4L2_CID_STATELESS_H264_DECODE_PARAMS: {
			long pic_order_cnt_msb;
			int max = get_max_pic_order_cnt_lsb();
			long prev_pic_order_cnt_msb = get_decode_order();
			int prev_pic_order_cnt_lsb = get_pic_order_cnt_lsb();
			int pic_order_cnt_lsb = ctrl.p_h264_decode_params->pic_order_cnt_lsb;

			if (option_is_set_verbose()) {
				fprintf(stderr, "\tprev_pic_order_cnt_lsb: %d\n", prev_pic_order_cnt_lsb);
				fprintf(stderr, "\tprev_pic_order_cnt_msb: %ld\n", prev_pic_order_cnt_msb);
				fprintf(stderr, "\tpic_order_cnt_lsb: %d\n", pic_order_cnt_lsb);
			}

			/* The first frame. */
			if (prev_pic_order_cnt_msb == 0)
				pic_order_cnt_msb = 0;

			/*
			 * TODO: improve the displaying of decoded frames following H264 specification
			 * 8.2.1.1. For now, dump all the previously decoded frames when an IDR_PIC is
			 * received to avoid losing frames although this will still sometimes result
			 * in frames out of order.
			 */
			if (ctrl.p_h264_decode_params->flags & V4L2_H264_DECODE_PARAM_FLAG_IDR_PIC) {
				if (get_compressed_frame_count())
					trace_mem_decoded();
			}

			/*
			 * When pic_order_cnt_lsb wraps around to zero, adjust the total count using
			 * max to keep the correct display order.
			 */
			if ((pic_order_cnt_lsb < prev_pic_order_cnt_lsb) &&
				((prev_pic_order_cnt_lsb - pic_order_cnt_lsb) >= (max / 2))) {
				pic_order_cnt_msb = prev_pic_order_cnt_msb + max;
			} else if ((pic_order_cnt_lsb > prev_pic_order_cnt_lsb) &&
				((pic_order_cnt_lsb - prev_pic_order_cnt_lsb) > (max / 2))) {
				pic_order_cnt_msb = prev_pic_order_cnt_msb - max;
			} else {
				pic_order_cnt_msb = prev_pic_order_cnt_msb + (pic_order_cnt_lsb - prev_pic_order_cnt_lsb);
			}

			if (option_is_set_verbose())
				fprintf(stderr, "\tpic_order_cnt_msb: %ld\n", pic_order_cnt_msb);

			set_pic_order_cnt_lsb(pic_order_cnt_lsb);
			set_decode_order(pic_order_cnt_msb);
			break;
		}
		default:
			break;
		}
	}
}

void qbuf_setup(struct v4l2_buffer *buf)
{
	if (option_is_set_verbose())
		fprintf(stderr, "\n%s: %s, index: %d\n", __func__, buftype2s(buf->type).c_str(), buf->index);

	int buf_fd = get_buffer_fd_trace(buf->type, buf->index);
	__u32 buf_offset = get_buffer_offset_trace(buf->type, buf->index);

	__u32 bytesused = 0;
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		bytesused = buf->m.planes[0].bytesused;
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT || buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		bytesused = buf->bytesused;
	set_buffer_bytesused_trace(buf_fd, buf_offset, bytesused);

	/* The output buffer should have compressed data just before it is queued, so trace it. */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		trace_mem_encoded(buf_fd, buf_offset);
		set_compressed_frame_count(get_compressed_frame_count() + 1);
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {

		/* If the capture buffer is queued for reuse, trace it before it is reused. */
		if (get_compressed_frame_count())
			trace_mem_decoded();

		/* H264 sets display order in controls, otherwise display just in the order queued. */
		if (get_compression_format() != V4L2_PIX_FMT_H264_SLICE)
			set_decode_order(get_decode_order() + 1);

		set_buffer_display_order(buf_fd, buf_offset, get_decode_order());

		if (option_is_set_verbose()) {
			print_decode_order();
			print_buffers_trace();
		}
	}
}

void streamoff_cleanup(v4l2_buf_type buf_type)
{
	if (option_is_set_verbose()) {
		fprintf(stderr, "\nVIDIOC_STREAMOFF: %s\n", buftype2s(buf_type).c_str());
		fprintf(stderr, "compression: %s, pixelformat: %s %s, width: %d, height: %d\n",
		        pixfmt2s(get_compression_format()).c_str(), pixfmt2s(get_pixelformat()).c_str(),
		        fcc2s(get_pixelformat()).c_str(), ctx_trace.width, ctx_trace.height);
	}

	/*
	 * Before turning off the stream, trace any remaining capture buffers that were missed
	 * because they were not queued for reuse.
	 */
	if (buf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf_type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (get_compressed_frame_count())
			trace_mem_decoded();
	}
}

void g_fmt_setup_trace(struct v4l2_format *format)
{
	if (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		set_pixelformat_trace(format->fmt.pix.width,
		                      format->fmt.pix.height, format->fmt.pix.pixelformat);
	if (format->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		set_pixelformat_trace(format->fmt.pix_mp.width, format->fmt.pix_mp.height,
		                      format->fmt.pix_mp.pixelformat);

	if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		set_compression_format(format->fmt.pix.pixelformat);
	if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		set_compression_format(format->fmt.pix_mp.pixelformat);
}

void s_fmt_setup(struct v4l2_format *format)
{
	if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		set_compression_format(format->fmt.pix.pixelformat);
	if (format->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		set_compression_format(format->fmt.pix_mp.pixelformat);
}

void expbuf_setup(struct v4l2_exportbuffer *export_buffer)
{
	__u32 type = export_buffer->type;
	__u32 index = export_buffer->index;
	int fd_found_in_trace_context = get_buffer_fd_trace(type, index);

	/* If the buffer was already added to the trace context don't add it again. */
	if (fd_found_in_trace_context == export_buffer->fd)
		return;

	/*
	 * If a buffer was previously added to the trace context using the video device
	 * file descriptor, replace the video fd with the more specific buffer fd from EXPBUF.
	 */
	if (fd_found_in_trace_context)
		remove_buffer_trace(fd_found_in_trace_context);

	add_buffer_trace(export_buffer->fd, type, index);
}

void querybuf_setup(int fd, struct v4l2_buffer *buf)
{
	/* If the buffer was already added to the trace context don't add it again. */
	if (get_buffer_fd_trace(buf->type, buf->index))
		return;

	switch (buf->memory) {
	case V4L2_MEMORY_MMAP: {
		__u32 offset = 0;
		if ((buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
		    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT))
			offset = buf->m.offset;
		if ((buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
		    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE))
			offset = buf->m.planes->m.mem_offset;
		add_buffer_trace(fd, buf->type, buf->index, offset);
		break;
	}
	case V4L2_MEMORY_USERPTR:
	case V4L2_MEMORY_DMABUF:
	default:
		break;
	}
}

void write_json_object_to_json_file(json_object *jobj, int flags)
{
	if (options.pretty_print_all)
		flags = JSON_C_TO_STRING_PRETTY;

	std::string json_str = json_object_to_json_string_ext(jobj, flags);
	pthread_mutex_lock(&ctx_trace.lock);

	if (!ctx_trace.trace_file) {
		ctx_trace.trace_filename = getenv("TRACE_ID");
		ctx_trace.trace_filename += ".json";
		ctx_trace.trace_file = fopen(ctx_trace.trace_filename.c_str(), "a");
	}

	fwrite(json_str.c_str(), sizeof(char), json_str.length(), ctx_trace.trace_file);
	fputs(",\n", ctx_trace.trace_file);
	fflush(ctx_trace.trace_file);
	pthread_mutex_unlock(&ctx_trace.lock);
}

void close_json_file(void)
{
	pthread_mutex_lock(&ctx_trace.lock);
	if (ctx_trace.trace_file) {
		fclose(ctx_trace.trace_file);
		ctx_trace.trace_file = 0;
	}
	pthread_mutex_unlock(&ctx_trace.lock);
}
