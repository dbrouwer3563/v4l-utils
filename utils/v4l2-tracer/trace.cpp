/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer-common.h"
#include "trace-helper.h"
#include "trace-gen.h"


void trace_open(int fd, const char *path, int oflag, mode_t mode, bool is_open64)
{
	json_object *open_obj = json_object_new_object();
	json_object_object_add(open_obj, "fd", json_object_new_int(fd));

	json_object *open_args = json_object_new_object();
	json_object_object_add(open_args, "path", json_object_new_string(path));
	json_object_object_add(open_args, "oflag", json_object_new_string(val2s(oflag, open_val_def).c_str()));
	json_object_object_add(open_args, "mode", json_object_new_string(number2s_oct(mode).c_str()));
	if (is_open64)
		json_object_object_add(open_obj, "open64", open_args);
	else
		json_object_object_add(open_obj, "open", open_args);

	/* Add additional topology information about device. */
	std::string path_str = path;
	bool is_media = path_str.find("media") != path_str.npos;
	bool is_video = path_str.find("video") != path_str.npos;

	int media_fd = -1;
	if (is_media)
		media_fd = fd;

	std::string driver;
	if (is_video) {
		struct v4l2_capability cap = {};
		setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
		ioctl(fd, VIDIOC_QUERYCAP, &cap);
		unsetenv("V4L2_TRACER_PAUSE_TRACE");

		std::string path_media = get_path_media(reinterpret_cast<const char *>(cap.driver));

		setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
		media_fd = open(path_media.c_str(), O_RDONLY);
		unsetenv("V4L2_TRACER_PAUSE_TRACE");
	}

	struct media_device_info info = {};
	setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
	ioctl(media_fd, MEDIA_IOC_DEVICE_INFO, &info);
	unsetenv("V4L2_TRACER_PAUSE_TRACE");

	json_object_object_add(open_obj, "driver", json_object_new_string(info.driver));
	json_object_object_add(open_obj, "bus_info", json_object_new_string(info.bus_info));

	if (is_video) {
		std::list<std::string> linked_entities = get_linked_entities(media_fd, path_str);
		json_object *linked_entities_obj = json_object_new_array();
		for (auto &name : linked_entities)
			json_object_array_add(linked_entities_obj, json_object_new_string(name.c_str()));
		json_object_object_add(open_obj, "linked_entities", linked_entities_obj);
		close(media_fd);
	}

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
		write_json_object_to_json_file(mem_obj, JSON_C_TO_STRING_PRETTY_TAB|JSON_C_TO_STRING_PRETTY);
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

json_object *trace_v4l2_plane(struct v4l2_plane *p, __u32 memory)
{
	json_object *plane_obj = json_object_new_object();

	json_object_object_add(plane_obj, "bytesused", json_object_new_int64(p->bytesused));
	json_object_object_add(plane_obj, "length", json_object_new_int64(p->length));

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

void trace_vidioc_stream(void *arg, json_object *ioctl_args)
{
	v4l2_buf_type buf_type = *(static_cast<v4l2_buf_type*>(arg));
	json_object_object_add(ioctl_args, "type", json_object_new_string(val2s(buf_type, v4l2_buf_type_val_def).c_str()));
}

void trace_v4l2_streamparm(void *arg, json_object *ioctl_args)
{
	json_object *v4l2_streamparm_obj = json_object_new_object();
	struct v4l2_streamparm *streamparm = static_cast<struct v4l2_streamparm*>(arg);

	json_object_object_add(v4l2_streamparm_obj, "type", json_object_new_string(val2s(streamparm->type, v4l2_buf_type_val_def).c_str()));

	if ((streamparm->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) || (streamparm->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE))
		trace_v4l2_captureparm_gen(&streamparm->parm, v4l2_streamparm_obj);

	if ((streamparm->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) || (streamparm->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE))
		trace_v4l2_outputparm_gen(&streamparm->parm, v4l2_streamparm_obj);

	json_object_object_add(ioctl_args, "v4l2_streamparm", v4l2_streamparm_obj);
}

void trace_v4l2_ext_control(void *arg, json_object *parent_obj, std::string key_name = "")
{
	json_object *v4l2_ext_control_obj = json_object_new_object();
	struct v4l2_ext_control *p = static_cast<struct v4l2_ext_control*>(arg);

	json_object_object_add(v4l2_ext_control_obj, "id", json_object_new_string(val2s(p->id, control_val_def).c_str()));
	json_object_object_add(v4l2_ext_control_obj, "size", json_object_new_uint64(p->size));

	if (p->ptr == nullptr) {
		json_object_array_add(parent_obj, v4l2_ext_control_obj);
		return;
	}

	switch (p->id) {
	case V4L2_CID_STATELESS_VP8_FRAME:
		trace_v4l2_ctrl_vp8_frame_gen(p->p_vp8_frame, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_MODE: {
		json_object *h264_decode_mode_obj = json_object_new_object();
		json_object_object_add(h264_decode_mode_obj, "value", json_object_new_int(p->value));
		json_object_object_add(v4l2_ext_control_obj, "V4L2_CID_STATELESS_H264_DECODE_MODE", h264_decode_mode_obj);
		break;
	}
	case V4L2_CID_STATELESS_H264_START_CODE: {
		json_object *h264_start_code_obj = json_object_new_object();
		json_object_object_add(h264_start_code_obj, "value", json_object_new_int(p->value));
		json_object_object_add(v4l2_ext_control_obj, "V4L2_CID_STATELESS_H264_START_CODE", h264_start_code_obj);
		break;
	}
	case V4L2_CID_STATELESS_H264_SPS:
		trace_v4l2_ctrl_h264_sps_gen(p->p_h264_sps, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_PPS:
		trace_v4l2_ctrl_h264_pps_gen(p->p_h264_pps, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_SCALING_MATRIX:
		trace_v4l2_ctrl_h264_scaling_matrix_gen(p->p_h264_scaling_matrix, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_PRED_WEIGHTS:
		trace_v4l2_ctrl_h264_pred_weights_gen(p->p_h264_pred_weights, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_SLICE_PARAMS:
		trace_v4l2_ctrl_h264_slice_params_gen(p->p_h264_slice_params, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_PARAMS:
		trace_v4l2_ctrl_h264_decode_params_gen(p->p_h264_decode_params, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_FWHT_PARAMS:
		trace_v4l2_ctrl_fwht_params_gen(p->p_fwht_params, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_VP9_FRAME:
		trace_v4l2_ctrl_vp9_frame_gen(p->p_vp9_frame, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_VP9_COMPRESSED_HDR:
		trace_v4l2_ctrl_vp9_compressed_hdr_gen(p->p_vp9_compressed_hdr_probs, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SPS:
		trace_v4l2_ctrl_hevc_sps_gen(p->p_hevc_sps, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_PPS:
		trace_v4l2_ctrl_hevc_pps_gen(p->p_hevc_pps, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SLICE_PARAMS:
		trace_v4l2_ctrl_hevc_slice_params_gen(p->p_hevc_slice_params, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SCALING_MATRIX:
		trace_v4l2_ctrl_hevc_scaling_matrix_gen(p->p_hevc_scaling_matrix, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_DECODE_PARAMS:
		trace_v4l2_ctrl_hevc_decode_params_gen(p->p_hevc_decode_params, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_SEQUENCE:
		trace_v4l2_ctrl_mpeg2_sequence_gen(p->p_mpeg2_sequence, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_PICTURE:
		trace_v4l2_ctrl_mpeg2_picture_gen(p->p_mpeg2_picture, v4l2_ext_control_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_QUANTISATION:
		trace_v4l2_ctrl_mpeg2_quantisation_gen(p->p_mpeg2_quantisation, v4l2_ext_control_obj);
		break;
	default:
		if (is_verbose())
			fprintf(stderr, "v4l2-tracer: %d: cannot trace control: %s\n", __LINE__, val2s(p->id, control_val_def).c_str());
		break;
	}

	json_object_array_add(parent_obj, v4l2_ext_control_obj);
}

void trace_v4l2_ext_controls(void *arg, json_object *ioctl_args)
{
	json_object *ext_controls_obj = json_object_new_object();
	struct v4l2_ext_controls *ext_controls = static_cast<struct v4l2_ext_controls*>(arg);

	json_object_object_add(ext_controls_obj, "which",
	                       json_object_new_string(val2s(ext_controls->which, which_val_def).c_str()));

	json_object_object_add(ext_controls_obj, "count", json_object_new_int64(ext_controls->count));

	/* error_idx is defined only if the ioctl returned an error  */
	if (errno)
		json_object_object_add(ext_controls_obj, "error_idx",
		                       json_object_new_uint64(ext_controls->error_idx));

	/* request_fd is only valid when "which" == V4L2_CTRL_WHICH_REQUEST_VAL */
	if (ext_controls->which == V4L2_CTRL_WHICH_REQUEST_VAL)
		json_object_object_add(ext_controls_obj, "request_fd",
		                       json_object_new_int(ext_controls->request_fd));

	json_object *controls_obj = json_object_new_array();
	for (__u32 i = 0; i < ext_controls->count; i++) {
		if ((void *) ext_controls->controls == nullptr)
			break;
		trace_v4l2_ext_control((void *) &ext_controls->controls[i], controls_obj);
	}
	json_object_object_add(ext_controls_obj, "controls", controls_obj);


	json_object_object_add(ioctl_args, "v4l2_ext_controls", ext_controls_obj);
}

void trace_v4l2_decoder_cmd(void *arg, json_object *ioctl_args)
{
	json_object *v4l2_decoder_cmd_obj = json_object_new_object();
	struct v4l2_decoder_cmd *p = static_cast<struct v4l2_decoder_cmd*>(arg);

	json_object_object_add(v4l2_decoder_cmd_obj, "cmd", json_object_new_string(val2s(p->cmd, decoder_cmd_val_def).c_str()));

	std::string flags;

	switch (p->cmd) {
	case V4L2_DEC_CMD_START: {
		/*  This command has one flag: V4L2_DEC_CMD_START_MUTE_AUDIO. */
		if (p->flags == V4L2_DEC_CMD_START_MUTE_AUDIO)
			flags = "V4L2_DEC_CMD_START_MUTE_AUDIO";

		/* struct start */
		json_object *start_obj = json_object_new_object();
		json_object_object_add(start_obj, "speed", json_object_new_int(p->start.speed));

		std::string format;
		/* possible values V4L2_DEC_START_FMT_NONE, V4L2_DEC_START_FMT_GOP */
		if (p->start.format == V4L2_DEC_START_FMT_GOP)
			format = "V4L2_DEC_START_FMT_GOP";
		else if (p->start.format == V4L2_DEC_START_FMT_NONE)
			format = "V4L2_DEC_START_FMT_NONE";
		json_object_object_add(start_obj, "format", json_object_new_string(format.c_str()));

		json_object_object_add(v4l2_decoder_cmd_obj, "start", start_obj);
		break;
	}
	case V4L2_DEC_CMD_STOP: {
		/*  This command has two flags */
		if (p->flags == V4L2_DEC_CMD_STOP_TO_BLACK)
			flags = "V4L2_DEC_CMD_STOP_TO_BLACK";
		else if (p->flags == V4L2_DEC_CMD_STOP_IMMEDIATELY)
			flags = "V4L2_DEC_CMD_STOP_IMMEDIATELY";

		json_object *stop_obj = json_object_new_object();
		json_object_object_add(stop_obj, "pts", json_object_new_uint64(p->stop.pts));

		json_object_object_add(v4l2_decoder_cmd_obj, "stop", stop_obj);
		break;
	}

	case V4L2_DEC_CMD_PAUSE: {
		if (p->flags == V4L2_DEC_CMD_PAUSE_TO_BLACK)
			flags = "V4L2_DEC_CMD_PAUSE_TO_BLACK";
		break;
	}
	case V4L2_DEC_CMD_RESUME:
	case V4L2_DEC_CMD_FLUSH:
	default:
		break;
	}
	json_object_object_add(v4l2_decoder_cmd_obj, "flags", json_object_new_string(flags.c_str()));

	json_object_object_add(ioctl_args, "v4l2_decoder_cmd", v4l2_decoder_cmd_obj);
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
		trace_v4l2_capability_gen(arg, ioctl_args);
		break;
	case VIDIOC_ENUM_FMT:
		trace_v4l2_fmtdesc_gen(arg, ioctl_args);
		break;
	case VIDIOC_G_FMT:
	case VIDIOC_TRY_FMT:
	case VIDIOC_S_FMT:
		trace_v4l2_format_gen(arg, ioctl_args);
		break;
	case VIDIOC_REQBUFS:
		trace_v4l2_requestbuffers_gen(arg, ioctl_args);
		break;
	case VIDIOC_PREPARE_BUF:
	case VIDIOC_QUERYBUF:
	case VIDIOC_QBUF:
	case VIDIOC_DQBUF:
		trace_v4l2_buffer(arg, ioctl_args);
		break;
	case VIDIOC_EXPBUF:
		trace_v4l2_exportbuffer_gen(arg, ioctl_args);
		break;
	case VIDIOC_STREAMON:
	case VIDIOC_STREAMOFF:
		if (from_userspace)
			trace_vidioc_stream(arg, ioctl_args);
		break;
	case VIDIOC_G_PARM:
	case VIDIOC_S_PARM:
		trace_v4l2_streamparm(arg, ioctl_args);
		break;
	case VIDIOC_G_CTRL:
	case VIDIOC_S_CTRL:
		trace_v4l2_control_gen(arg, ioctl_args);
		break;
	case VIDIOC_QUERYCTRL:
		trace_v4l2_queryctrl_gen(arg, ioctl_args);
		break;
	case VIDIOC_G_CROP:
	case VIDIOC_S_CROP:
		trace_v4l2_crop_gen(arg, ioctl_args);
		break;
	case VIDIOC_G_EXT_CTRLS:
	case VIDIOC_TRY_EXT_CTRLS:
	case VIDIOC_S_EXT_CTRLS:
		trace_v4l2_ext_controls(arg, ioctl_args);
		break;
	case VIDIOC_TRY_ENCODER_CMD:
	case VIDIOC_ENCODER_CMD:
		trace_v4l2_encoder_cmd_gen(arg, ioctl_args);
		break;
	case VIDIOC_CREATE_BUFS:
		trace_v4l2_create_buffers_gen(arg, ioctl_args);
		break;
	case VIDIOC_G_SELECTION:
	case VIDIOC_S_SELECTION:
		trace_v4l2_selection_gen(arg, ioctl_args);
		break;
	case VIDIOC_TRY_DECODER_CMD:
	case VIDIOC_DECODER_CMD:
		trace_v4l2_decoder_cmd(arg, ioctl_args);
		break;
	case VIDIOC_QUERY_EXT_CTRL:
		trace_v4l2_query_ext_ctrl_gen(arg, ioctl_args);
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
