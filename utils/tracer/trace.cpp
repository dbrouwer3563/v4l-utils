/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

json_object *trace_open(const char *path, int oflag, mode_t mode)
{
	json_object *open_args = json_object_new_object();

	json_object_object_add(open_args, "path", json_object_new_string(path));
	json_object_object_add(open_args, "oflag", json_object_new_int(oflag));
	json_object_object_add(open_args, "mode", json_object_new_uint64(mode));

	return open_args;
}

json_object *trace_mmap64(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	json_object *mmap_args = json_object_new_object();

	json_object_object_add(mmap_args, "addr", json_object_new_int64((int64_t)addr));
	json_object_object_add(mmap_args, "len", json_object_new_uint64(len));
	json_object_object_add(mmap_args, "prot", json_object_new_int(prot));
	json_object_object_add(mmap_args, "flags", json_object_new_int(flags));
	json_object_object_add(mmap_args, "fildes", json_object_new_int(fildes));
	json_object_object_add(mmap_args, "off", json_object_new_int64(off));

	return mmap_args;
}

/*
 * Get a buffer from memory and convert it to a json string array.
 * The bytes are displayed with the most-significant bits first.
 */
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

		/*  Add a space every two bytes e.g. "012A 4001" and a newline every 16 bytes. */
		if (byte_count_per_line == 16) {
			byte_count_per_line = 0;
			json_object_array_add(mem_array_obj, json_object_new_string(s.c_str()));
			s.clear();
		} else if (i % 2) {
			s += " ";
		}
	}

	/* Trace the last line if it was less than a full 16 bytes. */
	if (byte_count_per_line)
		json_object_array_add(mem_array_obj, json_object_new_string(s.c_str()));

	return mem_array_obj;
}

/* Get the decoded capture buffer from memory and write it to a binary yuv file. */
void write_decoded_frames_to_yuv_file(unsigned char *buffer_pointer, __u32 bytesused, std::string filename)
{
	FILE *fp = fopen(filename.c_str(), "a");
	for (__u32 i = 0; i < bytesused; i++)
		fwrite(&buffer_pointer[i], sizeof(unsigned char), 1, fp);
	fflush(fp);
	fclose(fp);
}

/* Trace an output or capture buffer. */
void trace_mem(int fd)
{
	int index;
	__u32 type;
	__u32 bytesused;
	unsigned char *buffer_pointer;
	long int start = get_buffer_address_trace(fd);

	/* Don't trace unmapped memory. */
	if (!start)
		return;

	buffer_pointer = (unsigned char*) start;
	type = get_buffer_type_trace(fd);
	index = get_buffer_index_trace(fd);
	bytesused = get_buffer_bytesused_trace(fd);

	json_object *mem_obj = json_object_new_object();
	json_object_object_add(mem_obj, "mem_dump", json_object_new_string(buftype2s(type).c_str()));
	json_object_object_add(mem_obj, "fd", json_object_new_int(fd));
	json_object_object_add(mem_obj, "type", json_object_new_uint64(type));
	json_object_object_add(mem_obj, "index", json_object_new_int(index));
	json_object_object_add(mem_obj, "bytesused", json_object_new_uint64(bytesused));
	json_object_object_add(mem_obj, "address", json_object_new_int64(start));

	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		json_object *mem_array_obj = trace_buffer(buffer_pointer, bytesused);
		json_object_object_add(mem_obj, "mem_array", mem_array_obj);
	}

	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {

		if (create_yuv_file()) {
			std::string filename = getenv("TRACE_ID");
			filename +=  ".yuv";
			write_decoded_frames_to_yuv_file(buffer_pointer, bytesused, filename);
			json_object_object_add(mem_obj, "filename", json_object_new_string(filename.c_str()));
		}

		if (write_decoded_data_to_json()) {
			json_object *mem_array_obj = trace_buffer(buffer_pointer, bytesused);
			json_object_object_add(mem_obj, "mem_array_capture", mem_array_obj);
		}

	}

	if (pretty_print_mem())
		write_json_object_to_json_file(mem_obj, JSON_C_TO_STRING_PRETTY);
	else
		write_json_object_to_json_file(mem_obj);

	json_object_put(mem_obj);
}

void trace_vidioc_querycap(void *arg, json_object *ioctl_args)
{
	json_object *cap_obj = json_object_new_object();
	struct v4l2_capability *cap = static_cast<struct v4l2_capability*>(arg);

	json_object_object_add(cap_obj, "driver", json_object_new_string((const char *)cap->driver));
	json_object_object_add(cap_obj, "card", json_object_new_string((const char *)cap->card));
	json_object_object_add(cap_obj, "bus_info", json_object_new_string((const char *)cap->bus_info));
	json_object_object_add(cap_obj, "version", json_object_new_string(ver2s(cap->version).c_str()));
	json_object_object_add(cap_obj, "capabilities", json_object_new_int64(cap->capabilities));
	json_object_object_add(cap_obj, "capabilities_str",
		                       json_object_new_string(capflag2s(cap->capabilities).c_str()));
	json_object_object_add(cap_obj, "device_caps", json_object_new_int64(cap->device_caps));
	json_object_object_add(ioctl_args, "v4l2_capability", cap_obj);
}

void trace_vidioc_enum_fmt(void *arg, json_object *ioctl_args)
{
	json_object *fmtdesc_obj = json_object_new_object();
	struct v4l2_fmtdesc *fmtdesc = static_cast<struct v4l2_fmtdesc*>(arg);

	json_object_object_add(fmtdesc_obj, "index", json_object_new_uint64(fmtdesc->index));
	json_object_object_add(fmtdesc_obj, "type_str", json_object_new_string(buftype2s(fmtdesc->type).c_str()));
	json_object_object_add(fmtdesc_obj, "type", json_object_new_uint64(fmtdesc->type));
	json_object_object_add(fmtdesc_obj, "flags", json_object_new_uint64(fmtdesc->flags));
	json_object_object_add(fmtdesc_obj, "description", json_object_new_string((const char *)fmtdesc->description));
	json_object_object_add(fmtdesc_obj, "pixelformat", json_object_new_uint64(fmtdesc->pixelformat));
	json_object_object_add(fmtdesc_obj, "mbus_code", json_object_new_uint64(fmtdesc->mbus_code));
	json_object_object_add(ioctl_args, "v4l2_fmtdesc", fmtdesc_obj);
}

void trace_v4l2_plane_pix_format(json_object *pix_mp_obj, struct v4l2_plane_pix_format plane_fmt, int plane)
{
	json_object *plane_fmt_obj = json_object_new_object();

	json_object_object_add(plane_fmt_obj, "sizeimage", json_object_new_uint64(plane_fmt.sizeimage));
	json_object_object_add(plane_fmt_obj, "bytesperline", json_object_new_uint64(plane_fmt.bytesperline));

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
	json_object_object_add(pix_mp_obj, "pixelformat", json_object_new_uint64(pix_mp.pixelformat));
	json_object_object_add(pix_mp_obj, "pixelformat_str", json_object_new_string(fcc2s(pix_mp.pixelformat).c_str()));
	json_object_object_add(pix_mp_obj, "field", json_object_new_uint64(pix_mp.field));
	json_object_object_add(pix_mp_obj, "field_str", json_object_new_string(field2s(pix_mp.field).c_str()));
	json_object_object_add(pix_mp_obj, "colorspace", json_object_new_uint64(pix_mp.colorspace));
	json_object_object_add(pix_mp_obj, "colorspace_str", json_object_new_string(colorspace2s(pix_mp.colorspace).c_str()));
	json_object_object_add(pix_mp_obj, "num_planes", json_object_new_int(pix_mp.num_planes));
	for (int i = 0; i < pix_mp.num_planes; i++)
		trace_v4l2_plane_pix_format(pix_mp_obj, pix_mp.plane_fmt[i], i);

	json_object_object_add(pix_mp_obj, "flags", json_object_new_int(pix_mp.flags));
	json_object_object_add(pix_mp_obj, "flags_str", json_object_new_string(pixflags2s(pix_mp.flags).c_str()));
	json_object_object_add(pix_mp_obj, "ycbcr_enc", json_object_new_int(pix_mp.ycbcr_enc));
	json_object_object_add(pix_mp_obj, "ycbcr_enc_str", json_object_new_string(ycbcr_enc2s(pix_mp.ycbcr_enc).c_str()));
	json_object_object_add(pix_mp_obj, "quantization", json_object_new_int(pix_mp.quantization));
	json_object_object_add(pix_mp_obj, "quantization_str", json_object_new_string(quantization2s(pix_mp.quantization).c_str()));
	json_object_object_add(pix_mp_obj, "xfer_func", json_object_new_int(pix_mp.xfer_func));
	json_object_object_add(pix_mp_obj, "xfer_func_str", json_object_new_string(xfer_func2s(pix_mp.xfer_func).c_str()));

	json_object_object_add(format_obj, "v4l2_pix_format_mplane", pix_mp_obj);
}

void trace_v4l2_format(void *arg, json_object *ioctl_args)
{
	json_object *format_obj = json_object_new_object();
	struct v4l2_format *format = static_cast<struct v4l2_format*>(arg);

	json_object_object_add(format_obj, "type", json_object_new_uint64(format->type));
	json_object_object_add(format_obj, "type_str", json_object_new_string(buftype2s(format->type).c_str()));

	switch (format->type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
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

	json_object_object_add(request_buffers_obj, "count", json_object_new_uint64(request_buffers->count));
	json_object_object_add(request_buffers_obj, "type", json_object_new_uint64(request_buffers->type));
	json_object_object_add(request_buffers_obj, "type_str",
	                       json_object_new_string(buftype2s(request_buffers->type).c_str()));
	json_object_object_add(request_buffers_obj, "memory", json_object_new_uint64(request_buffers->memory));
	json_object_object_add(request_buffers_obj, "memory_str",
	                       json_object_new_string(v4l2_memory2s(request_buffers->memory).c_str()));
	json_object_object_add(request_buffers_obj, "capabilities", json_object_new_uint64(request_buffers->capabilities));
	json_object_object_add(request_buffers_obj, "capabilities_str",
	                       json_object_new_string(bufcap2s(request_buffers->capabilities).c_str()));
	json_object_object_add(request_buffers_obj, "flags", json_object_new_int(request_buffers->flags));
	json_object_object_add(request_buffers_obj, "flags_str",
						   json_object_new_string(request_buffers_flag2s(request_buffers->flags).c_str()));

	json_object_object_add(ioctl_args, "v4l2_requestbuffers", request_buffers_obj);
}

json_object *trace_v4l2_plane(struct v4l2_plane *p, __u32 memory)
{
	json_object *plane_obj = json_object_new_object();

	json_object_object_add(plane_obj, "bytesused", json_object_new_int64(p->bytesused));
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
	json_object_object_add(buf_obj, "type", json_object_new_uint64(buf->type));
	json_object_object_add(buf_obj, "type_str",
	                       json_object_new_string(buftype2s(buf->type).c_str()));
	json_object_object_add(buf_obj, "bytesused", json_object_new_uint64(buf->bytesused));
	json_object_object_add(buf_obj, "flags", json_object_new_uint64(buf->flags));
	json_object_object_add(buf_obj, "flags_str",
	                       json_object_new_string(bufferflags2s(buf->flags).c_str()));
	json_object_object_add(buf_obj, "field", json_object_new_uint64(buf->field));

	json_object *timestamp_obj = json_object_new_object();
	json_object_object_add(timestamp_obj, "tv_sec", json_object_new_int64(buf->timestamp.tv_sec));
	json_object_object_add(timestamp_obj, "tv_usec", json_object_new_int64(buf->timestamp.tv_usec));
	json_object_object_add(buf_obj, "timestamp", timestamp_obj);
	json_object_object_add(buf_obj, "sequence", json_object_new_uint64(buf->sequence));
	json_object_object_add(buf_obj, "memory", json_object_new_uint64(buf->memory));
	json_object_object_add(buf_obj, "memory_str",
	                       json_object_new_string(v4l2_memory2s(buf->memory).c_str()));

	json_object *m_obj = json_object_new_object();
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {

		json_object *planes_obj = json_object_new_array();
		/* TODO tracer only works for decoded formats with one plane e.g. V4L2_PIX_FMT_NV12 */
		json_object_array_add(planes_obj, trace_v4l2_plane(buf->m.planes, buf->memory));
		json_object_object_add(m_obj, "planes", planes_obj);
	}
	json_object_object_add(buf_obj, "m", m_obj);
	json_object_object_add(buf_obj, "length", json_object_new_uint64(buf->length));

	/* For memory-to-memory devices you can use requests only for output buffers. */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		json_object_object_add(buf_obj, "request_fd", json_object_new_int(buf->request_fd));

	json_object_object_add(ioctl_args, "v4l2_buffer", buf_obj);
}

void trace_v4l2_exportbuffer(void *arg, json_object *ioctl_args)
{
	json_object *exportbuffer_obj = json_object_new_object();
	struct v4l2_exportbuffer *export_buffer = static_cast<struct v4l2_exportbuffer*>(arg);

	json_object_object_add(exportbuffer_obj, "type", json_object_new_uint64(export_buffer->type));
	json_object_object_add(exportbuffer_obj, "type_str",
	                       json_object_new_string(buftype2s(export_buffer->type).c_str()));

	json_object_object_add(exportbuffer_obj, "index", json_object_new_uint64(export_buffer->index));
	json_object_object_add(exportbuffer_obj, "plane", json_object_new_uint64(export_buffer->plane));
	json_object_object_add(exportbuffer_obj, "flags", json_object_new_uint64(export_buffer->flags));
	json_object_object_add(exportbuffer_obj, "fd", json_object_new_int(export_buffer->fd));

	json_object_object_add(ioctl_args, "v4l2_exportbuffer", exportbuffer_obj);
}

void trace_vidioc_stream(void *arg, json_object *ioctl_args)
{
	v4l2_buf_type buf_type = *(static_cast<v4l2_buf_type*>(arg));
	json_object_object_add(ioctl_args, "buf_type", json_object_new_int(buf_type));
	json_object_object_add(ioctl_args, "buf_type_str", json_object_new_string(buftype2s(buf_type).c_str()));
}

void trace_v4l2_ext_control(json_object *ext_controls_obj, struct v4l2_ext_control ctrl, __u32 control_idx)
{
	std::string unique_key_for_control;

	json_object *ctrl_obj = json_object_new_object();
	json_object_object_add(ctrl_obj, "id", json_object_new_uint64(ctrl.id));
	json_object_object_add(ctrl_obj, "control_class_str", json_object_new_string(ctrlclass2s(ctrl.id).c_str()));
	json_object_object_add(ctrl_obj, "size", json_object_new_uint64(ctrl.size));

	if ((ctrl.id & V4L2_CID_CODEC_STATELESS_BASE) == V4L2_CID_CODEC_STATELESS_BASE) {
		switch (ctrl.id) {
		case V4L2_CID_STATELESS_VP8_FRAME:
			trace_v4l2_ctrl_vp8_frame(ctrl.p_vp8_frame, ctrl_obj);
			break;
		default:
			break;
		}
	}

	unique_key_for_control = "v4l2_ext_control_";
	unique_key_for_control += std::to_string(control_idx);

	json_object_object_add(ext_controls_obj, unique_key_for_control.c_str(), ctrl_obj);
}

void trace_v4l2_ext_controls(void *arg, json_object *ioctl_args)
{
	json_object *ext_controls_obj = json_object_new_object();
	struct v4l2_ext_controls *ext_controls = static_cast<struct v4l2_ext_controls*>(arg);

	json_object_object_add(ext_controls_obj, "which", json_object_new_int64(ext_controls->which));
	json_object_object_add(ext_controls_obj, "which_str", json_object_new_string(which2s(ext_controls->which).c_str()));
	json_object_object_add(ext_controls_obj, "count", json_object_new_uint64(ext_controls->count));

	/* error_idx is defined only if the ioctl returned an error  */
	if (errno)
		json_object_object_add(ext_controls_obj, "error_idx", json_object_new_uint64(ext_controls->error_idx));

	/* request_fd is only valid when "which" == V4L2_CTRL_WHICH_REQUEST_VAL */
	if (ext_controls->which == V4L2_CTRL_WHICH_REQUEST_VAL)
		json_object_object_add(ext_controls_obj, "request_fd", json_object_new_int(ext_controls->request_fd));

	for (__u32 i = 0; i < ext_controls->count; i++)
		trace_v4l2_ext_control(ext_controls_obj, ext_controls->controls[i], i);

	json_object_object_add(ioctl_args, "v4l2_ext_controls", ext_controls_obj);
}

void trace_vidioc_query_ext_ctrl(void *arg, json_object *ioctl_args)
{
	json_object *query_ext_ctrl_obj = json_object_new_object();
	struct v4l2_query_ext_ctrl *queryextctrl = static_cast<struct v4l2_query_ext_ctrl*>(arg);

	json_object_object_add(query_ext_ctrl_obj, "id", json_object_new_uint64(queryextctrl->id));
	json_object_object_add(query_ext_ctrl_obj, "control_class_str", json_object_new_string(ctrlclass2s(queryextctrl->id).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "type", json_object_new_uint64(queryextctrl->type));
	json_object_object_add(query_ext_ctrl_obj, "type_str", json_object_new_string(ctrltype2s(queryextctrl->type).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "name", json_object_new_string(queryextctrl->name));
	json_object_object_add(query_ext_ctrl_obj, "minimum", json_object_new_int64(queryextctrl->minimum));
	json_object_object_add(query_ext_ctrl_obj, "maximum", json_object_new_int64(queryextctrl->maximum));
	json_object_object_add(query_ext_ctrl_obj, "step", json_object_new_uint64(queryextctrl->step));
	json_object_object_add(query_ext_ctrl_obj, "default_value", json_object_new_int64(queryextctrl->default_value));
	json_object_object_add(query_ext_ctrl_obj, "flags", json_object_new_uint64(queryextctrl->flags));
	json_object_object_add(query_ext_ctrl_obj, "flags_str", json_object_new_string(ctrlflags2s(queryextctrl->flags).c_str()));
	json_object_object_add(query_ext_ctrl_obj, "elem_size", json_object_new_uint64(queryextctrl->elem_size));
	json_object_object_add(query_ext_ctrl_obj, "elems", json_object_new_uint64(queryextctrl->elems));
	json_object_object_add(query_ext_ctrl_obj, "nr_of_dims", json_object_new_uint64(queryextctrl->nr_of_dims));

	/* 	__u32   dims[V4L2_CTRL_MAX_DIMS] */
	json_object *dim_obj = json_object_new_array();

	for (unsigned int i = 0; i < queryextctrl->nr_of_dims; i++)
		json_object_array_add(dim_obj, json_object_new_uint64(queryextctrl->dims[i]));

	json_object_object_add(query_ext_ctrl_obj, "dims", dim_obj);
	json_object_object_add(ioctl_args, "v4l2_query_ext_ctrl", query_ext_ctrl_obj);
}

void trace_ioctl_media(unsigned long request, void *arg, json_object *ioctl_args)
{
	if (request ==  MEDIA_IOC_REQUEST_ALLOC) {
		__s32 *request_fd = static_cast<__s32*>(arg);
		json_object_object_add(ioctl_args, "request_fd", json_object_new_int(*request_fd));
	}
}

void trace_ioctl_video(unsigned long int request, void *arg, json_object *ioctl_args, bool from_userspace)
{
	switch (request) {
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

void trace_dma_buf_ioctl_sync(void *arg, json_object *ioctl_args)
{
	struct dma_buf_sync *sync = static_cast<struct dma_buf_sync*>(arg);
	json_object *sync_obj = json_object_new_object();
	json_object_object_add(sync_obj, "flags", json_object_new_uint64(sync->flags));
	json_object_object_add(sync_obj, "flags_str", json_object_new_string(bufsyncflag2s(sync->flags).c_str()));
	json_object_object_add(ioctl_args, "dma_buf_sync", sync_obj);
}

std::string get_ioctl_request_str(unsigned long request)
{
	__u8 ioctl_type = _IOC_TYPE(request);
	switch (ioctl_type) {
		case 'V':
			return ioctl2s_video(request);
		case '|':
			return ioctl2s_media(request);
		case 'b':
			if (request == DMA_BUF_IOCTL_SYNC)
				return "DMA_BUF_IOCTL_SYNC";
			break;
		default:
			break;
	}
	return "unknown ioctl";
}

json_object *trace_ioctl_args(int fd, unsigned long request, void *arg, bool from_userspace)
{
	json_object *ioctl_args = json_object_new_object();
	__u8 ioctl_type = _IOC_TYPE(request);
	switch (ioctl_type) {
		case 'V':
			trace_ioctl_video(request, arg, ioctl_args, from_userspace);
			break;
		case '|':
			trace_ioctl_media(request, arg, ioctl_args);
			break;
		case 'b':
			if (request == DMA_BUF_IOCTL_SYNC && from_userspace) {
				trace_dma_buf_ioctl_sync(arg, ioctl_args);
			}
			break;
		default:
			break;
	}

	return ioctl_args;
}
