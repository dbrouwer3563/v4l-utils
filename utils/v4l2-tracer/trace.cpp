/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer-common.h"
#include "trace-helper.h"
#include "trace-ctrl-gen.h"

void trace_open(int fd, const char *path, int oflag, mode_t mode, bool is_open64)
{
	int media_fd = -1;
	std::string path_media;
	std::string path_video;
	std::string path_str = path;
	bool is_video_device = path_str.find("video") != path_str.npos;
	bool is_media_device = path_str.find("media") != path_str.npos;

	if (is_video_device) {
		path_video = path;
		path_media = get_path_media_from_path_video(path_str);
		setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
		media_fd = open(path_media.c_str(), O_RDONLY);
		unsetenv("V4L2_TRACER_PAUSE_TRACE");

	} else if (is_media_device) {
		path_media = path;
		path_video = get_path_video_from_fd_media(fd);
		media_fd = fd;
	}

	struct media_device_info info = {};
	ioctl(media_fd, MEDIA_IOC_DEVICE_INFO, &info);

	json_object *open_obj = json_object_new_object();
	json_object_object_add(open_obj, "fd", json_object_new_int(fd));
	json_object *open_args = json_object_new_object();

	json_object_object_add(open_args, "driver", json_object_new_string(info.driver));
	json_object_object_add(open_args, "bus_info", json_object_new_string(info.bus_info));

	json_object_object_add(open_args, "path", json_object_new_string(path));
	if (is_video_device)
		json_object_object_add(open_args, "path_media", json_object_new_string(path_media.c_str()));
	else if (is_media_device)
		json_object_object_add(open_args, "path_video", json_object_new_string(path_video.c_str()));

	std::list<std::string> linked_entities = get_linked_entities(media_fd, path_video);

	json_object *linked_entities_obj = json_object_new_array();
	for (auto &e : linked_entities)
		json_object_array_add(linked_entities_obj, json_object_new_string(e.c_str()));
	json_object_object_add(open_args, "linked_entities", linked_entities_obj);

	if (is_video_device)
		close(media_fd);

	json_object_object_add(open_args, "oflag", json_object_new_string(val2s(oflag, open_val_def).c_str()));
	json_object_object_add(open_args, "mode", json_object_new_string(number2s_oct(mode).c_str()));

	if (is_open64)
		json_object_object_add(open_obj, "open64", open_args);
	else
		json_object_object_add(open_obj, "open", open_args);

	write_json_object_to_json_file(open_obj);
	json_object_put(open_obj);
}

void trace_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off, unsigned long buf_address, bool is_mmap64)
{
	json_object *mmap_obj = json_object_new_object();

	if (errno)
		json_object_object_add(mmap_obj, "errno", json_object_new_string(strerrorname_np(errno)));

	json_object *mmap_args = json_object_new_object();
	json_object_object_add(mmap_args, "addr", json_object_new_int64((int64_t)addr));
	json_object_object_add(mmap_args, "len", json_object_new_uint64(len));
	json_object_object_add(mmap_args, "prot", json_object_new_int(prot));
	json_object_object_add(mmap_args, "flags", json_object_new_string(number2s(flags).c_str()));
	json_object_object_add(mmap_args, "fildes", json_object_new_int(fildes));
	json_object_object_add(mmap_args, "off", json_object_new_int64(off));

	if (is_mmap64)
		json_object_object_add(mmap_obj, "mmap64", mmap_args);
	else
		json_object_object_add(mmap_obj, "mmap", mmap_args);


	json_object_object_add(mmap_obj, "buffer_address", json_object_new_uint64(buf_address));

	write_json_object_to_json_file(mmap_obj);
	json_object_put(mmap_obj);
}

json_object *trace_buffer(unsigned char *buffer_pointer, __u32 bytesused)
{
	char buf[5];
	std::string s;
	int byte_count_per_line = 0;
	json_object *mem_array_obj = json_object_new_array();

	for (__u32 i = 0; i < bytesused; i++) {
		memset(buf, 0, sizeof(buf));
		/* Each byte e.g. D9 will write a string of two characters "D9". */
		sprintf(buf, "%02x", buffer_pointer[i]);
		s += buf;
		byte_count_per_line++;

		/*  Add a space every two bytes e.g. "012A 4001" and a newline every 32 bytes. */
		if (byte_count_per_line == 32) {
			byte_count_per_line = 0;
			json_object_array_add(mem_array_obj, json_object_new_string(s.c_str()));
			s.clear();
		} else if (getenv("V4L2_TRACER_OPTION_PRETTY_PRINT_MEM") || getenv("V4L2_TRACER_OPTION_PRETTY_PRINT_ALL")) {
			s += " ";
		}
	}

	/* Trace the last line if it was less than a full line. */
	if (byte_count_per_line)
		json_object_array_add(mem_array_obj, json_object_new_string(s.c_str()));

	return mem_array_obj;
}

void trace_mem(int fd, __u32 offset, __u32 type, int index, __u32 bytesused, unsigned long start)
{
	json_object *mem_obj = json_object_new_object();
	json_object_object_add(mem_obj, "mem_dump", json_object_new_string(val2s(type, v4l2_buf_type_val_def).c_str()));
	json_object_object_add(mem_obj, "fd", json_object_new_int(fd));
	json_object_object_add(mem_obj, "offset", json_object_new_uint64(offset));
	json_object_object_add(mem_obj, "index", json_object_new_int(index));
	json_object_object_add(mem_obj, "bytesused", json_object_new_uint64(bytesused));
	json_object_object_add(mem_obj, "address", json_object_new_uint64(start));

	if ((type == V4L2_BUF_TYPE_VIDEO_OUTPUT || type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) ||
	    (getenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_JSON_FILE"))) {
		json_object *mem_array_obj = trace_buffer((unsigned char*) start, bytesused);
		json_object_object_add(mem_obj, "mem_array", mem_array_obj);
	}

	if (getenv("V4L2_TRACER_OPTION_PRETTY_PRINT_MEM"))
		write_json_object_to_json_file(mem_obj, JSON_C_TO_STRING_PRETTY);
	else
		write_json_object_to_json_file(mem_obj);

	json_object_put(mem_obj);
}

void trace_mem_encoded(int fd, __u32 offset)
{
	int index;
	__u32 type;
	__u32 bytesused;
	unsigned long start = get_buffer_address_trace(fd, offset);

	if (!start)
		return;

	bytesused = get_buffer_bytesused_trace(fd, offset);
	type = get_buffer_type_trace(fd, offset);
	index = get_buffer_index_trace(fd, offset);
	trace_mem(fd, offset, type, index, bytesused, start);
}

void trace_vidioc_querycap(void *arg, json_object *ioctl_args)
{
	json_object *cap_obj = json_object_new_object();
	struct v4l2_capability *cap = static_cast<struct v4l2_capability*>(arg);

	json_object_object_add(cap_obj, "driver", json_object_new_string((const char *)cap->driver));
	json_object_object_add(cap_obj, "card", json_object_new_string((const char *)cap->card));
	json_object_object_add(cap_obj, "bus_info",
	                       json_object_new_string((const char *)cap->bus_info));
	json_object_object_add(cap_obj, "version",
	                       json_object_new_string(ver2s(cap->version).c_str()));
	json_object_object_add(cap_obj, "capabilities", json_object_new_string(fl2s(cap->capabilities, capabilities_flag_def).c_str()));
	json_object_object_add(cap_obj, "device_caps", json_object_new_string(fl2s(cap->device_caps, capabilities_flag_def).c_str()));
	json_object_object_add(ioctl_args, "v4l2_capability", cap_obj);
}

void trace_vidioc_enum_fmt(void *arg, json_object *ioctl_args)
{
	json_object *fmtdesc_obj = json_object_new_object();
	struct v4l2_fmtdesc *fmtdesc = static_cast<struct v4l2_fmtdesc*>(arg);

	json_object_object_add(fmtdesc_obj, "index", json_object_new_uint64(fmtdesc->index));
	json_object_object_add(fmtdesc_obj, "type", json_object_new_string(val2s(fmtdesc->type, v4l2_buf_type_val_def).c_str()));
	if (fmtdesc->flags)
		json_object_object_add(fmtdesc_obj, "flags", json_object_new_string(fl2s(fmtdesc->flags, v4l2_fmt_flag_def).c_str()));
	if (fmtdesc->pixelformat)
		json_object_object_add(fmtdesc_obj, "pixelformat", json_object_new_string(val2s(fmtdesc->pixelformat, v4l2_pix_fmt_val_def).c_str()));
	json_object_object_add(fmtdesc_obj, "mbus_code", json_object_new_uint64(fmtdesc->mbus_code));
	json_object_object_add(ioctl_args, "v4l2_fmtdesc", fmtdesc_obj);
}

void trace_v4l2_plane_pix_format(json_object *pix_mp_obj, struct v4l2_plane_pix_format plane_fmt, int plane)
{
	json_object *plane_fmt_obj = json_object_new_object();

	json_object_object_add(plane_fmt_obj, "sizeimage",
	                       json_object_new_uint64(plane_fmt.sizeimage));
	json_object_object_add(plane_fmt_obj, "bytesperline",
	                       json_object_new_uint64(plane_fmt.bytesperline));

	/* Create a unique key name for each plane. */
	std::string unique_key_for_plane = "v4l2_plane_pix_format_";
	unique_key_for_plane += std::to_string(plane);

	json_object_object_add(pix_mp_obj, unique_key_for_plane.c_str(), plane_fmt_obj);
}

void trace_v4l2_pix_format_mplane(json_object *format_obj, struct v4l2_pix_format_mplane pix_mp)
{
	json_object *pix_mp_obj = json_object_new_object();

	json_object_object_add(pix_mp_obj, "width", json_object_new_uint64(pix_mp.width));
	json_object_object_add(pix_mp_obj, "height", json_object_new_uint64(pix_mp.height));

	json_object_object_add(pix_mp_obj, "pixelformat",
	                       json_object_new_string(val2s(pix_mp.pixelformat, v4l2_pix_fmt_val_def).c_str()));

	json_object_object_add(pix_mp_obj, "field", json_object_new_string(val2s(pix_mp.field, v4l2_field_val_def).c_str()));
	json_object_object_add(pix_mp_obj, "colorspace",
	                       json_object_new_string(val2s(pix_mp.colorspace, v4l2_colorspace_val_def).c_str()));
	json_object_object_add(pix_mp_obj, "num_planes", json_object_new_int(pix_mp.num_planes));
	for (int i = 0; i < pix_mp.num_planes; i++)
		trace_v4l2_plane_pix_format(pix_mp_obj, pix_mp.plane_fmt[i], i);

	json_object_object_add(pix_mp_obj, "flags", json_object_new_string(fl2s(pix_mp.flags, v4l2_pix_fmt_flag_def).c_str()));
	json_object_object_add(pix_mp_obj, "ycbcr_enc",
	                       json_object_new_string(val2s(pix_mp.ycbcr_enc, v4l2_ycbcr_encoding_val_def).c_str()));
	json_object_object_add(pix_mp_obj, "quantization",
	                       json_object_new_string(val2s(pix_mp.quantization, v4l2_quantization_val_def).c_str()));
	json_object_object_add(pix_mp_obj, "xfer_func",
	                       json_object_new_string(val2s(pix_mp.xfer_func, v4l2_xfer_func_val_def).c_str()));

	json_object_object_add(format_obj, "v4l2_pix_format_mplane", pix_mp_obj);
}

void trace_v4l2_pix_format(json_object *format_obj, struct v4l2_pix_format pix)
{
	json_object *pix_obj = json_object_new_object();

	json_object_object_add(pix_obj, "width", json_object_new_uint64(pix.width));
	json_object_object_add(pix_obj, "height", json_object_new_uint64(pix.height));
	json_object_object_add(pix_obj, "pixelformat", json_object_new_string(val2s(pix.pixelformat, v4l2_pix_fmt_val_def).c_str()));
	json_object_object_add(pix_obj, "field", json_object_new_string(val2s(pix.field, v4l2_field_val_def).c_str()));
	json_object_object_add(pix_obj, "bytesperline", json_object_new_uint64(pix.bytesperline));
	json_object_object_add(pix_obj, "sizeimage", json_object_new_uint64(pix.sizeimage));
	json_object_object_add(pix_obj, "colorspace",
	                       json_object_new_string(val2s(pix.colorspace, v4l2_colorspace_val_def).c_str()));

	if (pix.priv == V4L2_PIX_FMT_PRIV_MAGIC) {
		json_object_object_add(pix_obj, "priv",
		                       json_object_new_string("V4L2_PIX_FMT_PRIV_MAGIC"));
		json_object_object_add(pix_obj, "flags",
		                       json_object_new_string(fl2s(pix.flags, v4l2_pix_fmt_flag_def).c_str()));
		json_object_object_add(pix_obj, "ycbcr_enc",
		                       json_object_new_string(val2s(pix.ycbcr_enc, v4l2_ycbcr_encoding_val_def).c_str()));
		json_object_object_add(pix_obj, "quantization",
		                       json_object_new_string(val2s(pix.quantization, v4l2_quantization_val_def).c_str()));
		json_object_object_add(pix_obj, "xfer_func",
		                       json_object_new_string(val2s(pix.xfer_func, v4l2_xfer_func_val_def).c_str()));
	}
	json_object_object_add(format_obj, "v4l2_pix_format", pix_obj);
}

void trace_v4l2_format(void *arg, json_object *ioctl_args)
{
	json_object *format_obj = json_object_new_object();
	struct v4l2_format *format = static_cast<struct v4l2_format*>(arg);

	json_object_object_add(format_obj, "type", json_object_new_string(val2s(format->type, v4l2_buf_type_val_def).c_str()));

	switch (format->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		trace_v4l2_pix_format(format_obj, format->fmt.pix);
		break;
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY:
		break;
	case V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		trace_v4l2_pix_format_mplane(format_obj, format->fmt.pix_mp);
		break;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
	case V4L2_BUF_TYPE_SDR_OUTPUT:
	case V4L2_BUF_TYPE_META_CAPTURE:
	case V4L2_BUF_TYPE_META_OUTPUT:
		break;
	}
	json_object_object_add(ioctl_args, "v4l2_format", format_obj);
}

void trace_v4l2_requestbuffers(void *arg, json_object *ioctl_args)
{
	json_object *request_buffers_obj = json_object_new_object();
	struct v4l2_requestbuffers *request_buffers = static_cast<struct v4l2_requestbuffers*>(arg);

	json_object_object_add(request_buffers_obj, "count",
	                       json_object_new_uint64(request_buffers->count));
	json_object_object_add(request_buffers_obj, "type", json_object_new_string(val2s(request_buffers->type, v4l2_buf_type_val_def).c_str()));
	json_object_object_add(request_buffers_obj, "memory",
	                       json_object_new_string(val2s(request_buffers->memory, v4l2_memory_val_def).c_str()));
	json_object_object_add(request_buffers_obj, "capabilities",
	                       json_object_new_string(bufcap2s(request_buffers->capabilities).c_str()));
	json_object_object_add(request_buffers_obj, "flags", json_object_new_string(fl2s(request_buffers->flags, v4l2_memory_flag_def).c_str()));
	json_object_object_add(ioctl_args, "v4l2_requestbuffers", request_buffers_obj);
}

json_object *trace_v4l2_plane(struct v4l2_plane *p, __u32 memory)
{
	json_object *plane_obj = json_object_new_object();

	json_object_object_add(plane_obj, "bytesused", json_object_new_uint64(p->bytesused));
	json_object_object_add(plane_obj, "length", json_object_new_uint64(p->length));

	json_object *m_obj = json_object_new_object();
	if (memory == V4L2_MEMORY_MMAP)
		json_object_object_add(m_obj, "mem_offset", json_object_new_int64(p->m.mem_offset));
	json_object_object_add(plane_obj, "m", m_obj);

	json_object_object_add(plane_obj, "data_offset", json_object_new_int64(p->data_offset));

	return plane_obj;
}

void trace_v4l2_buffer(void *arg, json_object *ioctl_args)
{
	json_object *buf_obj = json_object_new_object();
	struct v4l2_buffer *buf = static_cast<struct v4l2_buffer*>(arg);

	json_object_object_add(buf_obj, "index", json_object_new_uint64(buf->index));
	json_object_object_add(buf_obj, "type", json_object_new_string(val2s(buf->type, v4l2_buf_type_val_def).c_str()));
	json_object_object_add(buf_obj, "bytesused", json_object_new_uint64(buf->bytesused));

	json_object_object_add(buf_obj, "flags", json_object_new_string(fl2s_buffer(buf->flags).c_str()));
	json_object_object_add(buf_obj, "field", json_object_new_string(val2s(buf->field, v4l2_field_val_def).c_str()));

	json_object *timestamp_obj = json_object_new_object();
	json_object_object_add(timestamp_obj, "tv_sec", json_object_new_int64(buf->timestamp.tv_sec));
	json_object_object_add(timestamp_obj, "tv_usec",
	                       json_object_new_int64(buf->timestamp.tv_usec));
	json_object_object_add(buf_obj, "timestamp", timestamp_obj);
	json_object_object_add(buf_obj, "timestamp_ns",
	                       json_object_new_uint64(v4l2_timeval_to_ns(&buf->timestamp)));

	json_object_object_add(buf_obj, "sequence", json_object_new_uint64(buf->sequence));
	json_object_object_add(buf_obj, "memory",
	                       json_object_new_string(val2s(buf->memory, v4l2_memory_val_def).c_str()));

	json_object *m_obj = json_object_new_object();
	/* multi-planar API */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {

		json_object *planes_obj = json_object_new_array();
		/* TODO add planes > 0 */
		json_object_array_add(planes_obj, trace_v4l2_plane(buf->m.planes, buf->memory));
		json_object_object_add(m_obj, "planes", planes_obj);
	}

	/* single-planar API */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {

		switch (buf->memory) {
		case V4L2_MEMORY_MMAP:
			json_object_object_add(m_obj, "offset", json_object_new_uint64(buf->m.offset));
			break;
		case V4L2_MEMORY_USERPTR:
		case V4L2_MEMORY_DMABUF:
			break;
		}
	}
	json_object_object_add(buf_obj, "m", m_obj);
	json_object_object_add(buf_obj, "length", json_object_new_uint64(buf->length));

	if (buf->flags & V4L2_BUF_FLAG_REQUEST_FD)
		json_object_object_add(buf_obj, "request_fd", json_object_new_int(buf->request_fd));

	json_object_object_add(ioctl_args, "v4l2_buffer", buf_obj);
}

void trace_v4l2_exportbuffer(void *arg, json_object *ioctl_args)
{
	json_object *exportbuffer_obj = json_object_new_object();
	struct v4l2_exportbuffer *export_buffer = static_cast<struct v4l2_exportbuffer*>(arg);

	json_object_object_add(exportbuffer_obj, "type", json_object_new_string(val2s(export_buffer->type, v4l2_buf_type_val_def).c_str()));
	json_object_object_add(exportbuffer_obj, "index", json_object_new_uint64(export_buffer->index));
	json_object_object_add(exportbuffer_obj, "plane", json_object_new_uint64(export_buffer->plane));
	json_object_object_add(exportbuffer_obj, "flags", json_object_new_string(number2s(export_buffer->flags).c_str()));
	json_object_object_add(exportbuffer_obj, "fd", json_object_new_int(export_buffer->fd));

	json_object_object_add(ioctl_args, "v4l2_exportbuffer", exportbuffer_obj);
}

void trace_vidioc_stream(void *arg, json_object *ioctl_args)
{
	v4l2_buf_type buf_type = *(static_cast<v4l2_buf_type*>(arg));
	json_object_object_add(ioctl_args, "type", json_object_new_string(val2s(buf_type, v4l2_buf_type_val_def).c_str()));
}

void trace_v4l2_ext_control(json_object *ext_controls_obj, struct v4l2_ext_control ctrl, __u32 control_idx)
{
	std::string unique_key_for_control;

	json_object *ctrl_obj = json_object_new_object();
	json_object_object_add(ctrl_obj, "id", json_object_new_string(val2s(ctrl.id, stateless_controls_val_def).c_str()));
	json_object_object_add(ctrl_obj, "size", json_object_new_uint64(ctrl.size));

	switch (ctrl.id) {
	case V4L2_CID_STATELESS_VP8_FRAME:
		trace_v4l2_ctrl_vp8_frame_gen(ctrl.p_vp8_frame, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_MODE: {
			json_object *h264_decode_mode_obj = json_object_new_object();
			json_object_object_add(h264_decode_mode_obj, "value",
			                       json_object_new_int(ctrl.value));
			json_object_object_add(ctrl_obj, "V4L2_CID_STATELESS_H264_DECODE_MODE",
			                       h264_decode_mode_obj);
		break;
	}
	case V4L2_CID_STATELESS_H264_START_CODE: {
		json_object *h264_start_code_obj = json_object_new_object();
		json_object_object_add(h264_start_code_obj, "value", json_object_new_int(ctrl.value));
		json_object_object_add(ctrl_obj, "V4L2_CID_STATELESS_H264_START_CODE", h264_start_code_obj);
		break;
	}
	case V4L2_CID_STATELESS_H264_SPS:
		trace_v4l2_ctrl_h264_sps_gen(ctrl.p_h264_sps, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_PPS:
		trace_v4l2_ctrl_h264_pps_gen(ctrl.p_h264_pps, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_SCALING_MATRIX:
		trace_v4l2_ctrl_h264_scaling_matrix_gen(ctrl.p_h264_scaling_matrix, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_PRED_WEIGHTS:
		trace_v4l2_ctrl_h264_pred_weights_gen(ctrl.p_h264_pred_weights, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_SLICE_PARAMS:
		trace_v4l2_ctrl_h264_slice_params_gen(ctrl.p_h264_slice_params, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_PARAMS:
		trace_v4l2_ctrl_h264_decode_params_gen(ctrl.p_h264_decode_params, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_FWHT_PARAMS:
		trace_v4l2_ctrl_fwht_params_gen(ctrl.p_fwht_params, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_VP9_FRAME:
		trace_v4l2_ctrl_vp9_frame_gen(ctrl.p_vp9_frame, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_VP9_COMPRESSED_HDR:
		trace_v4l2_ctrl_vp9_compressed_hdr_gen(ctrl.p_vp9_compressed_hdr_probs, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SPS:
		trace_v4l2_ctrl_hevc_sps_gen(ctrl.p_hevc_sps, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_PPS:
		trace_v4l2_ctrl_hevc_pps_gen(ctrl.p_hevc_pps, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SLICE_PARAMS:
		trace_v4l2_ctrl_hevc_slice_params_gen(ctrl.p_hevc_slice_params, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SCALING_MATRIX:
		trace_v4l2_ctrl_hevc_scaling_matrix_gen(ctrl.p_hevc_scaling_matrix, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_DECODE_PARAMS:
		trace_v4l2_ctrl_hevc_decode_params_gen(ctrl.p_hevc_decode_params, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_SEQUENCE:
		trace_v4l2_ctrl_mpeg2_sequence_gen(ctrl.p_mpeg2_sequence, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_PICTURE:
		trace_v4l2_ctrl_mpeg2_picture_gen(ctrl.p_mpeg2_picture, ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_QUANTISATION:
		trace_v4l2_ctrl_mpeg2_quantisation_gen(ctrl.p_mpeg2_quantisation, ctrl_obj);
		break;
	default:
		json_object_object_add(ctrl_obj, "Trace warning",
		                       json_object_new_string("Unknown control id."));
		break;
	}

	unique_key_for_control = "v4l2_ext_control_";
	unique_key_for_control += std::to_string(control_idx);

	json_object_object_add(ext_controls_obj, unique_key_for_control.c_str(), ctrl_obj);
}

void trace_v4l2_ext_controls(void *arg, json_object *ioctl_args)
{
	json_object *ext_controls_obj = json_object_new_object();
	struct v4l2_ext_controls *ext_controls = static_cast<struct v4l2_ext_controls*>(arg);

	json_object_object_add(ext_controls_obj, "which",
	                       json_object_new_string(val2s(ext_controls->which, which_val_def).c_str()));
	json_object_object_add(ext_controls_obj, "count", json_object_new_uint64(ext_controls->count));

	/* error_idx is defined only if the ioctl returned an error  */
	if (errno)
		json_object_object_add(ext_controls_obj, "error_idx",
		                       json_object_new_uint64(ext_controls->error_idx));

	/* request_fd is only valid when "which" == V4L2_CTRL_WHICH_REQUEST_VAL */
	if (ext_controls->which == V4L2_CTRL_WHICH_REQUEST_VAL)
		json_object_object_add(ext_controls_obj, "request_fd",
		                       json_object_new_int(ext_controls->request_fd));

	for (__u32 i = 0; i < ext_controls->count; i++)
		trace_v4l2_ext_control(ext_controls_obj, ext_controls->controls[i], i);

	json_object_object_add(ioctl_args, "v4l2_ext_controls", ext_controls_obj);
}

void trace_vidioc_query_ext_ctrl(void *arg, json_object *ioctl_args)
{
	json_object *query_ext_ctrl_obj = json_object_new_object();
	struct v4l2_query_ext_ctrl *queryextctrl = static_cast<struct v4l2_query_ext_ctrl*>(arg);

	json_object_object_add(query_ext_ctrl_obj, "id", json_object_new_string(number2s(queryextctrl->id).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "control_class",
	                       json_object_new_string(val2s(queryextctrl->id & 0xFFFF0000, ctrlclass_val_def).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "type",
	                       json_object_new_string(val2s(queryextctrl->type, v4l2_ctrl_type_val_def).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "name", json_object_new_string(queryextctrl->name));
	json_object_object_add(query_ext_ctrl_obj, "minimum",
	                       json_object_new_int64(queryextctrl->minimum));
	json_object_object_add(query_ext_ctrl_obj, "maximum",
	                       json_object_new_int64(queryextctrl->maximum));
	json_object_object_add(query_ext_ctrl_obj, "step", json_object_new_uint64(queryextctrl->step));
	json_object_object_add(query_ext_ctrl_obj, "default_value",
	                       json_object_new_int64(queryextctrl->default_value));
	json_object_object_add(query_ext_ctrl_obj, "flags",
	                       json_object_new_string(fl2s(queryextctrl->flags, v4l2_ctrl_flag_def).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "elem_size",
	                       json_object_new_uint64(queryextctrl->elem_size));
	json_object_object_add(query_ext_ctrl_obj, "elems",
	                       json_object_new_uint64(queryextctrl->elems));
	json_object_object_add(query_ext_ctrl_obj, "nr_of_dims",
	                       json_object_new_uint64(queryextctrl->nr_of_dims));

	/* 	__u32   dims[V4L2_CTRL_MAX_DIMS] */
	json_object *dim_obj = json_object_new_array();

	for (unsigned int i = 0; i < queryextctrl->nr_of_dims; i++)
		json_object_array_add(dim_obj, json_object_new_uint64(queryextctrl->dims[i]));

	json_object_object_add(query_ext_ctrl_obj, "dims", dim_obj);
	json_object_object_add(ioctl_args, "v4l2_query_ext_ctrl", query_ext_ctrl_obj);
}

void trace_ioctl_media(unsigned long cmd, void *arg, json_object *ioctl_args)
{
	if (cmd ==  MEDIA_IOC_REQUEST_ALLOC) {
		__s32 *request_fd = static_cast<__s32*>(arg);
		json_object_object_add(ioctl_args, "request_fd", json_object_new_int(*request_fd));
	}
}

void trace_ioctl_video(unsigned long cmd, void *arg, json_object *ioctl_args, bool from_userspace)
{
	switch (cmd) {
	case VIDIOC_QUERYCAP:
		if (!from_userspace)
			trace_vidioc_querycap(arg, ioctl_args);
		break;
	case VIDIOC_ENUM_FMT:
		trace_vidioc_enum_fmt(arg, ioctl_args);
		break;
	case VIDIOC_G_FMT:
	case VIDIOC_S_FMT:
		trace_v4l2_format(arg, ioctl_args);
		break;
	case VIDIOC_REQBUFS:
		trace_v4l2_requestbuffers(arg, ioctl_args);
		break;
	case VIDIOC_QUERYBUF:
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF:
		trace_v4l2_buffer(arg, ioctl_args);
		break;
	case VIDIOC_EXPBUF:
		trace_v4l2_exportbuffer(arg, ioctl_args);
		break;
	case VIDIOC_STREAMON:
	case VIDIOC_STREAMOFF:
		if (from_userspace)
			trace_vidioc_stream(arg, ioctl_args);
		break;
	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS:
		trace_v4l2_ext_controls(arg, ioctl_args);
		break;
	case VIDIOC_QUERY_EXT_CTRL:
		trace_vidioc_query_ext_ctrl(arg, ioctl_args);
		break;
	default:
		break;
	}
}

json_object *trace_ioctl_args(int fd, unsigned long cmd, void *arg, bool from_userspace)
{
	json_object *ioctl_args = json_object_new_object();
	__u8 ioctl_type = _IOC_TYPE(cmd);
	switch (ioctl_type) {
		case 'V':
			trace_ioctl_video(cmd, arg, ioctl_args, from_userspace);
			break;
		case '|':
			trace_ioctl_media(cmd, arg, ioctl_args);
			break;
		default:
			break;
	}

	return ioctl_args;
}
