/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer.h"

bool verbose = false;
std::string retrace_filename;
std::string dev_path_video;
std::string dev_path_media;

enum Option_Retracer {
    OptSetDevice_Retracer = 'd',
    OptSetMediaDevice_Retracer = 'm',
    OptVerbose_Retracer = 'v',
};

static struct option long_options_retracer[] = {
    { "device", required_argument, nullptr, OptSetDevice_Retracer },
    { "media", required_argument, nullptr, OptSetMediaDevice_Retracer },
    { "verbose", no_argument, nullptr, OptVerbose_Retracer },
    { nullptr, 0, nullptr, 0 }
};

char short_options_retracer[] = {
    OptSetDevice_Retracer, ':',
    OptSetMediaDevice_Retracer, ':',
    OptVerbose_Retracer
};

void retrace_mmap(json_object *mmap_obj, bool is_mmap64)
{
	json_object *mmap_args_obj;
	json_object_object_get_ex(mmap_obj, "mmap_args", &mmap_args_obj);

	json_object *len_obj;
	json_object_object_get_ex(mmap_args_obj, "len", &len_obj);
	size_t len = (size_t) json_object_get_int(len_obj);

	json_object *prot_obj;
	json_object_object_get_ex(mmap_args_obj, "prot", &prot_obj);
	int prot = json_object_get_int(prot_obj);

	json_object *flags_obj;
	json_object_object_get_ex(mmap_args_obj, "flags", &flags_obj);
	int flags = json_object_get_int(flags_obj);

	json_object *fildes_obj;
	json_object_object_get_ex(mmap_args_obj, "fildes", &fildes_obj);
	int fd_trace = json_object_get_int(fildes_obj);

	json_object *off_obj;
	json_object_object_get_ex(mmap_args_obj, "off", &off_obj);
	off_t off = (off_t) json_object_get_int64(off_obj);

	int fd_retrace = get_fd_retrace_from_fd_trace(fd_trace);

	/* Only retrace mmap calls that map a buffer. */
	if (!buffer_in_retrace_context(fd_retrace, off))
		return;

	void *buf_address_retrace_pointer;
	if (is_mmap64)
		buf_address_retrace_pointer = mmap64(0, len, prot, flags, fd_retrace, off);
	else
		buf_address_retrace_pointer = mmap(0, len, prot, flags, fd_retrace, off);

	if (buf_address_retrace_pointer == MAP_FAILED) {
		if (is_mmap64)
			perror("mmap64");
		else
			perror("mmap");
		print_context();
		exit(EXIT_FAILURE);
	}

	/*
	 * Get and store the original trace address so that it can be matched
	 * with the munmap address later.
	 */
	json_object *buffer_address_obj;
	json_object_object_get_ex(mmap_obj, "buffer_address", &buffer_address_obj);
	long buf_address_trace = json_object_get_int64(buffer_address_obj);
	set_buffer_address_retrace(fd_retrace, off, buf_address_trace, (long) buf_address_retrace_pointer);

	if (verbose || (errno != 0)) {
		if (is_mmap64)
			perror("mmap64");
		else
			perror("mmap");
		fprintf(stderr, "fd: %d, offset: %ld\n", fd_retrace, off);
		print_context();
	}
}

void retrace_munmap(json_object *syscall_obj)
{
	json_object *munmap_args_obj;
	json_object_object_get_ex(syscall_obj, "munmap_args", &munmap_args_obj);

	json_object *start_obj;
	json_object_object_get_ex(munmap_args_obj, "start", &start_obj);
	long start = json_object_get_int64(start_obj);

	json_object *length_obj;
	json_object_object_get_ex(munmap_args_obj, "length", &length_obj);
	size_t length = (size_t) json_object_get_int(length_obj);

	long buffer_address_retrace = get_retrace_address_from_trace_address(start);

	if (buffer_address_retrace < 0)
		return;

	munmap((void *)buffer_address_retrace, length);

	if (verbose || (errno != 0)) {
		perror("munmap");
		fprintf(stderr, "unmapped: %ld\n", buffer_address_retrace);
		fprintf(stderr, "\n");
	}
}

void retrace_open(json_object *jobj, bool is_open64)
{
	json_object *fd_trace_obj;
	json_object_object_get_ex(jobj, "fd", &fd_trace_obj);
	int fd_trace = json_object_get_int(fd_trace_obj);

	json_object *open_args_obj;
	json_object_object_get_ex(jobj, "open_args", &open_args_obj);

	json_object *path_obj;
	json_object_object_get_ex(open_args_obj, "path", &path_obj);
	std::string path = json_object_get_string(path_obj);

	json_object *oflag_obj;
	json_object_object_get_ex(open_args_obj, "oflag", &oflag_obj);
	int oflag = json_object_get_int(oflag_obj);

	json_object *mode_obj;
	json_object_object_get_ex(open_args_obj, "mode", &mode_obj);
	int mode = json_object_get_int(mode_obj);

	/* If a device is provided on the command line, use it instead of the device from the trace file. */
	if ((path.find("video") != path.npos) && !dev_path_video.empty())
		path = dev_path_video;

	if ((path.find("media") != path.npos) && !dev_path_media.empty())
		path = dev_path_media;

	int fd_retrace = 0;
	if (is_open64)
		fd_retrace = open64(path.c_str(), oflag, mode);
	else
		fd_retrace = open(path.c_str(), oflag, mode);

	if (fd_retrace <= 0) {
		fprintf(stderr, "Cannot open: %s\n", path.c_str());
		exit(fd_retrace);
	}

	add_fd(fd_trace, fd_retrace);

	if (verbose || (errno != 0)) {
		if (is_open64)
			perror("open64");
		else
			perror("open");
		fprintf(stderr, "path: %s \n", path.c_str());
		print_context();
	}
}

void retrace_close(json_object *jobj)
{
	json_object *fd_trace_obj;
	json_object_object_get_ex(jobj, "fd", &fd_trace_obj);
	int fd_retrace = get_fd_retrace_from_fd_trace(json_object_get_int(fd_trace_obj));

	/* Only close devices that were opened in the retrace context. */
	if (fd_retrace) {
		close(fd_retrace);
		remove_fd(json_object_get_int(fd_trace_obj));

		if (verbose || (errno != 0)) {
			perror("close");
			fprintf(stderr, "fd: %d\n\n", fd_retrace);
			print_context();
		}
	}
}

struct v4l2_requestbuffers retrace_v4l2_requestbuffers(json_object *ioctl_args)
{
	struct v4l2_requestbuffers request_buffers;
	memset(&request_buffers, 0, sizeof(v4l2_requestbuffers));

	json_object *requestbuffers_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_requestbuffers", &requestbuffers_obj);

	json_object *count_obj;
	json_object_object_get_ex(requestbuffers_obj, "count", &count_obj);
	request_buffers.count = json_object_get_int(count_obj);

	json_object *type_obj;
	json_object_object_get_ex(requestbuffers_obj, "type", &type_obj);
	request_buffers.type = json_object_get_int(type_obj);

	json_object *memory_obj;
	json_object_object_get_ex(requestbuffers_obj, "memory", &memory_obj);
	request_buffers.memory = json_object_get_int(memory_obj);

	json_object *capabilities_obj;
	json_object_object_get_ex(requestbuffers_obj, "capabilities", &capabilities_obj);
	request_buffers.capabilities = json_object_get_int(capabilities_obj);

	json_object *flags_obj;
	json_object_object_get_ex(requestbuffers_obj, "flags", &flags_obj);
	request_buffers.flags = json_object_get_int(flags_obj);

	return request_buffers;
}

void retrace_vidioc_reqbufs(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_requestbuffers request_buffers = retrace_v4l2_requestbuffers(ioctl_args);

	ioctl(fd_retrace, VIDIOC_REQBUFS, &request_buffers);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_REQBUFS");
		fprintf(stderr, "type: %s, request_buffers.count: %d\n",
		        buftype2s(request_buffers.type).c_str(), request_buffers.count);
		print_context();
	}
}

struct v4l2_plane *retrace_v4l2_plane(json_object *plane_obj, __u32 memory)
{
	struct v4l2_plane *pl = (struct v4l2_plane *) calloc(1, sizeof(v4l2_plane));

	json_object *bytesused_obj;
	json_object_object_get_ex(plane_obj, "bytesused", &bytesused_obj);
	pl->bytesused = (__u32) json_object_get_uint64(bytesused_obj);

	json_object *length_obj;
	json_object_object_get_ex(plane_obj, "length", &length_obj);
	pl->length = (__u32) json_object_get_uint64(length_obj);

	json_object *m_obj;
	json_object_object_get_ex(plane_obj, "m", &m_obj);
	if (memory == V4L2_MEMORY_MMAP) {
		json_object *mem_offset_obj;
		json_object_object_get_ex(m_obj, "mem_offset", &mem_offset_obj);
		pl->m.mem_offset = (__u32) json_object_get_int64(mem_offset_obj);
	}

	json_object *data_offset_obj;
	json_object_object_get_ex(plane_obj, "data_offset", &data_offset_obj);
	pl->data_offset = (__u32) json_object_get_int64(data_offset_obj);

	return pl;
}

struct v4l2_buffer *retrace_v4l2_buffer(json_object *ioctl_args)
{
	struct v4l2_buffer *buf = (struct v4l2_buffer *) calloc(1, sizeof(struct v4l2_buffer));

	json_object *buf_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_buffer", &buf_obj);

	json_object *index_obj;
	json_object_object_get_ex(buf_obj, "index", &index_obj);
	buf->index = (__u32) json_object_get_uint64(index_obj);

	json_object *type_obj;
	json_object_object_get_ex(buf_obj, "type", &type_obj);
	buf->type = (__u32) json_object_get_uint64(type_obj);

	json_object *bytesused_obj;
	json_object_object_get_ex(buf_obj, "bytesused", &bytesused_obj);
	buf->bytesused = (__u32) json_object_get_uint64(bytesused_obj);

	json_object *flags_obj;
	json_object_object_get_ex(buf_obj, "flags", &flags_obj);
	buf->flags = (__u32) json_object_get_uint64(flags_obj);

	json_object *field_obj;
	json_object_object_get_ex(buf_obj, "field", &field_obj);
	buf->field = (__u32) json_object_get_uint64(field_obj);

	json_object *timestamp_obj;
	json_object_object_get_ex(buf_obj, "timestamp", &timestamp_obj);

	struct timeval tv;
	memset(&tv, 0, sizeof(timeval));
	json_object *tv_sec_obj;
	json_object_object_get_ex(timestamp_obj, "tv_sec", &tv_sec_obj);
	tv.tv_sec = json_object_get_int64(tv_sec_obj);
	json_object *tv_usec_obj;
	json_object_object_get_ex(timestamp_obj, "tv_usec", &tv_usec_obj);
	tv.tv_usec = json_object_get_int64(tv_usec_obj);
	buf->timestamp = tv;

	json_object *sequence_obj;
	json_object_object_get_ex(buf_obj, "sequence", &sequence_obj);
	buf->sequence = (__u32) json_object_get_uint64(sequence_obj);

	json_object *memory_obj;
	json_object_object_get_ex(buf_obj, "memory", &memory_obj);
	buf->memory = (__u32) json_object_get_uint64(memory_obj);

	json_object *length_obj;
	json_object_object_get_ex(buf_obj, "length", &length_obj);
	buf->length = (__u32) json_object_get_uint64(length_obj);

	json_object *m_obj;
	json_object_object_get_ex(buf_obj, "m", &m_obj);

	/* multi-planar API */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		json_object *planes_obj;
		json_object_object_get_ex(m_obj, "planes", &planes_obj);
		json_object *plane_obj = json_object_array_get_idx(planes_obj, 0); /* TODO add planes > 0 */
		buf->m.planes = retrace_v4l2_plane(plane_obj, buf->memory);
	}

	/* single-planar API */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {

		switch (buf->memory) {
		case V4L2_MEMORY_MMAP: {
			json_object *offset_obj;
			json_object_object_get_ex(m_obj, "offset", &offset_obj);
			buf->m.offset = (__u32) json_object_get_uint64(offset_obj);
			break;
		}
		case V4L2_MEMORY_USERPTR:
		case V4L2_MEMORY_DMABUF:
			break;
		}
	}

	if (buf->flags & V4L2_BUF_FLAG_REQUEST_FD) {
		json_object *request_fd_obj;
		json_object_object_get_ex(buf_obj, "request_fd", &request_fd_obj);
		buf->request_fd = (__s32) get_fd_retrace_from_fd_trace(json_object_get_int(request_fd_obj));
	}

	return buf;
}

void retrace_vidioc_querybuf(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_buffer *buf = retrace_v4l2_buffer(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_QUERYBUF, buf);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE || buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes != nullptr) {
			free(buf->m.planes);
		}
	}

	/*
	 * For single-planar API, use QUERYBUF as an opportunity
	 * to store the buffer in the retrace context for future access.
	 */
	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE || buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		switch (buf->memory) {
		case V4L2_MEMORY_MMAP: {
			if (!buffer_in_retrace_context(fd_retrace, buf->m.offset))
				add_buffer_retrace(fd_retrace, buf->m.offset);
			break;
		}
		case V4L2_MEMORY_USERPTR:
		case V4L2_MEMORY_DMABUF:
		default:
			break;
		}
	}

	if (verbose || (errno != 0)) {
		perror("VIDIOC_QUERYBUF");
		fprintf(stderr, "buf->type: %s, buf->index: %d, fd_retrace: %d\n",
		        buftype2s(buf->type).c_str(), buf->index, fd_retrace);
		print_context();
	}

	free(buf);
}

void retrace_vidioc_qbuf(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_buffer *buf = retrace_v4l2_buffer(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_QBUF, buf);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE || buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes != nullptr) {
			free(buf->m.planes);
		}
	}

	if (verbose || (errno != 0)) {
		perror("VIDIOC_QBUF");
		fprintf(stderr, "buf->type: %s, buf->index: %d, fd_retrace: %d, \n",
		        buftype2s(buf->type).c_str(), buf->index, fd_retrace);
		print_context();
	}

	free(buf);
}

struct v4l2_exportbuffer retrace_v4l2_exportbuffer(json_object *ioctl_args)
{
	struct v4l2_exportbuffer export_buffer;
	memset(&export_buffer, 0, sizeof(v4l2_exportbuffer));

	json_object *exportbuffer_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_exportbuffer", &exportbuffer_obj);

	json_object *type_obj;
	json_object_object_get_ex(exportbuffer_obj, "type", &type_obj);
	export_buffer.type = json_object_get_int(type_obj);

	json_object *index_obj;
	json_object_object_get_ex(exportbuffer_obj, "index", &index_obj);
	export_buffer.index = json_object_get_int(index_obj);

	json_object *plane_obj;
	json_object_object_get_ex(exportbuffer_obj, "plane", &plane_obj);
	export_buffer.plane = json_object_get_int(plane_obj);

	json_object *flags_obj;
	json_object_object_get_ex(exportbuffer_obj, "flags", &flags_obj);
	export_buffer.flags = json_object_get_int(flags_obj);

	json_object *fd_obj;
	json_object_object_get_ex(exportbuffer_obj, "fd", &fd_obj);
	export_buffer.fd = json_object_get_int(fd_obj);

	return export_buffer;
}

void retrace_vidioc_expbuf(int fd_retrace, json_object *ioctl_args_user, json_object *ioctl_args_driver)
{
	struct v4l2_exportbuffer export_buffer;
	memset(&export_buffer, 0, sizeof(v4l2_exportbuffer));

	export_buffer = retrace_v4l2_exportbuffer(ioctl_args_user);
	ioctl(fd_retrace, VIDIOC_EXPBUF, &export_buffer);

	int buf_fd_retrace = export_buffer.fd;

	/*
	 * For multi-planar API use EXPBUF as an opportunity
	 * to store the buffer in the retrace context for future access.
	 */
	if ((export_buffer.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
	    (export_buffer.type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
			if (!buffer_in_retrace_context(buf_fd_retrace))
				add_buffer_retrace(buf_fd_retrace);
	}

	/*
	 * Get the export buffer file descriptor as provided by the driver in the original trace context.
	 * Then associate this original file descriptor with the current file descriptor in the retrace context.
	 */
	memset(&export_buffer, 0, sizeof(v4l2_exportbuffer));
	export_buffer = retrace_v4l2_exportbuffer(ioctl_args_driver);
	int buf_fd_trace = export_buffer.fd;
	add_fd(buf_fd_trace, buf_fd_retrace);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_EXPBUF");
		fprintf(stderr, "type: %s \n", buftype2s(export_buffer.type).c_str());
		fprintf(stderr, "index: %d, fd: %d\n", export_buffer.index, buf_fd_retrace);
		print_context();
	}
}

void retrace_vidioc_dqbuf(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_buffer *buf = retrace_v4l2_buffer(ioctl_args_user);

	struct pollfd *pfds = (struct pollfd *) calloc(1, sizeof(struct pollfd));
	if (pfds == NULL)
		exit(EXIT_FAILURE);
	pfds[0].fd = fd_retrace;
	pfds[0].events = POLLIN;
	poll(pfds, 1, 5000);
	free(pfds);

	ioctl(fd_retrace, VIDIOC_DQBUF, buf);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_DQBUF");
		fprintf(stderr, "fd_retrace: %d\n", fd_retrace);
		fprintf(stderr, "buf->index: %d\n", buf->index);
		fprintf(stderr, "buf->type: %s,\n", buftype2s(buf->type).c_str());
		fprintf(stderr, "buf->bytesused: %u, \n", buf->bytesused);
		fprintf(stderr, "buf->flags: %u\n", buf->flags);
		fprintf(stderr, "buf->field: %u, buf->request_fd: %d\n", buf->field, buf->request_fd);
		fprintf(stderr, "buf->request_fd: %d\n", buf->request_fd);
		print_context();
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE || buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes != nullptr) {
			free(buf->m.planes);
		}
	}

	free(buf);
}

void retrace_vidioc_streamon(int fd_retrace, json_object *ioctl_args)
{
	json_object *buf_type_obj;
	json_object_object_get_ex(ioctl_args, "buf_type", &buf_type_obj);
	v4l2_buf_type buf_type = (v4l2_buf_type) json_object_get_int(buf_type_obj);

	ioctl(fd_retrace, VIDIOC_STREAMON, &buf_type);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_STREAMON");
		fprintf(stderr, "buftype: %s\n\n", buftype2s(buf_type).c_str());
	}
}

void retrace_vidioc_streamoff(int fd_retrace, json_object *ioctl_args)
{
	json_object *buf_type_obj;
	json_object_object_get_ex(ioctl_args, "buf_type", &buf_type_obj);
	v4l2_buf_type buf_type = (v4l2_buf_type) json_object_get_int(buf_type_obj);

	ioctl(fd_retrace, VIDIOC_STREAMOFF, &buf_type);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_STREAMOFF");
		fprintf(stderr, "buftype: %s\n", buftype2s(buf_type).c_str());
		fprintf(stderr, "\n");
	}
}

struct v4l2_plane_pix_format get_v4l2_plane_pix_format(json_object *pix_mp_obj, int plane)
{
	std::string key;
	struct v4l2_plane_pix_format plane_fmt;
	memset(&plane_fmt, 0, sizeof(v4l2_plane_pix_format));

	json_object *plane_fmt_obj, *sizeimage_obj, *bytesperline_obj;

	key = "v4l2_plane_pix_format_";
	key += std::to_string(plane);
	json_object_object_get_ex(pix_mp_obj, key.c_str(), &plane_fmt_obj);

	json_object_object_get_ex(plane_fmt_obj, "sizeimage", &sizeimage_obj);
	plane_fmt.sizeimage = json_object_get_int64(sizeimage_obj);

	json_object_object_get_ex(plane_fmt_obj, "bytesperline", &bytesperline_obj);
	plane_fmt.bytesperline = json_object_get_int64(bytesperline_obj);

	return plane_fmt;
}

struct v4l2_pix_format_mplane retrace_v4l2_pix_format_mplane(json_object *v4l2_format_obj)
{
	struct v4l2_pix_format_mplane pix_mp;
	memset(&pix_mp, 0, sizeof(v4l2_pix_format_mplane));

	json_object *pix_mp_obj;
	json_object_object_get_ex(v4l2_format_obj, "v4l2_pix_format_mplane", &pix_mp_obj);

	json_object *width_obj;
	json_object_object_get_ex(pix_mp_obj, "width", &width_obj);
	pix_mp.width = json_object_get_int(width_obj);

	json_object *height_obj;
	json_object_object_get_ex(pix_mp_obj, "height", &height_obj);
	pix_mp.height = json_object_get_int(height_obj);

	json_object *pixelformat_obj;
	json_object_object_get_ex(pix_mp_obj, "pixelformat", &pixelformat_obj);
	pix_mp.pixelformat = json_object_get_int(pixelformat_obj);

	json_object *field_obj;
	json_object_object_get_ex(pix_mp_obj, "field", &field_obj);
	pix_mp.field = json_object_get_int(field_obj);

	json_object *colorspace_obj;
	json_object_object_get_ex(pix_mp_obj, "colorspace", &colorspace_obj);
	pix_mp.colorspace = json_object_get_int(colorspace_obj);

	json_object *num_planes_obj;
	json_object_object_get_ex(pix_mp_obj, "num_planes", &num_planes_obj);
	pix_mp.num_planes = json_object_get_int(num_planes_obj);

	for (int i = 0; i < pix_mp.num_planes; i++)
		pix_mp.plane_fmt[i] = get_v4l2_plane_pix_format(pix_mp_obj, i);

	json_object *flags_obj;
	json_object_object_get_ex(pix_mp_obj, "flags", &flags_obj);
	pix_mp.flags = json_object_get_int(flags_obj);

	json_object *ycbcr_enc_obj;
	json_object_object_get_ex(pix_mp_obj, "ycbcr_enc", &ycbcr_enc_obj);
	pix_mp.ycbcr_enc = json_object_get_int(ycbcr_enc_obj);

	json_object *quantization_obj;
	json_object_object_get_ex(pix_mp_obj, "quantization", &quantization_obj);
	pix_mp.quantization = json_object_get_int(quantization_obj);

	json_object *xfer_func_obj;
	json_object_object_get_ex(pix_mp_obj, "xfer_func", &xfer_func_obj);
	pix_mp.xfer_func = json_object_get_int(xfer_func_obj);

	return pix_mp;
}

struct v4l2_format retrace_v4l2_format(json_object *ioctl_args)
{
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));

	json_object *v4l2_format_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_format", &v4l2_format_obj);

	json_object *type_obj;
	json_object_object_get_ex(v4l2_format_obj, "type", &type_obj);
	format.type = json_object_get_int(type_obj);

	switch (format.type) {
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
		format.fmt.pix_mp = retrace_v4l2_pix_format_mplane(v4l2_format_obj);
		break;
	case V4L2_BUF_TYPE_SDR_CAPTURE:
	case V4L2_BUF_TYPE_SDR_OUTPUT:
	case V4L2_BUF_TYPE_META_CAPTURE:
	case V4L2_BUF_TYPE_META_OUTPUT:
		break;
	}

	return format;
}

void retrace_vidioc_g_fmt(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));

	format = retrace_v4l2_format(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_G_FMT, &format);

	if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		set_pixelformat_retrace(format.fmt.pix.width, format.fmt.pix.height, format.fmt.pix.pixelformat);
	if (format.type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		set_pixelformat_retrace(format.fmt.pix_mp.width, format.fmt.pix_mp.height, format.fmt.pix_mp.pixelformat);

	if (verbose || (errno != 0))
		perror("VIDIOC_G_FMT");
}

void retrace_vidioc_s_fmt(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));

	format = retrace_v4l2_format(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_S_FMT, &format);

	if (verbose || (errno != 0)) {
		perror("VIDIOC_S_FMT");
		fprintf(stderr, "%s\n", buftype2s(format.type).c_str());
		fprintf(stderr, "format.fmt.pix_mp.pixelformat: %s\n\n",
		        fcc2s(format.fmt.pix_mp.pixelformat).c_str());
	}
}

int retrace_v4l2_ext_control_value(json_object *ctrl_obj)
{
	json_object *value_obj;
	json_object_object_get_ex(ctrl_obj, "value", &value_obj);
	return json_object_get_int(value_obj);
}

struct v4l2_ext_control retrace_v4l2_ext_control(json_object *ext_controls_obj, int ctrl_idx)
{
	struct v4l2_ext_control ctrl;
	memset(&ctrl, 0, sizeof(v4l2_ext_control));

	std::string unique_key_for_control = "v4l2_ext_control_";
	unique_key_for_control += std::to_string(ctrl_idx);

	json_object *ctrl_obj;
	json_object_object_get_ex(ext_controls_obj, unique_key_for_control.c_str(), &ctrl_obj);

	json_object *id_obj;
	json_object_object_get_ex(ctrl_obj, "id", &id_obj);
	ctrl.id = json_object_get_int64(id_obj);

	json_object *size_obj;
	json_object_object_get_ex(ctrl_obj, "size", &size_obj);
	ctrl.size = json_object_get_int64(size_obj);

	if ((ctrl.id & V4L2_CID_CODEC_STATELESS_BASE) == V4L2_CID_CODEC_STATELESS_BASE) {
		switch (ctrl.id) {
		case V4L2_CID_STATELESS_VP8_FRAME:
			ctrl.ptr = retrace_v4l2_ctrl_vp8_frame_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_DECODE_MODE:
		case V4L2_CID_STATELESS_H264_START_CODE:
			ctrl.value = retrace_v4l2_ext_control_value(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_SPS:
			ctrl.ptr = retrace_v4l2_ctrl_h264_sps_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_PPS:
			ctrl.ptr = retrace_v4l2_ctrl_h264_pps_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_SCALING_MATRIX:
			ctrl.ptr = retrace_v4l2_ctrl_h264_scaling_matrix_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_PRED_WEIGHTS:
			ctrl.ptr = retrace_v4l2_ctrl_h264_pred_weights_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_SLICE_PARAMS:
			ctrl.ptr = retrace_v4l2_ctrl_h264_slice_params_pointer(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_H264_DECODE_PARAMS:
			ctrl.ptr = retrace_v4l2_ctrl_h264_decode_params(ctrl_obj);
			break;
		case V4L2_CID_STATELESS_FWHT_PARAMS:
			ctrl.ptr = retrace_v4l2_ctrl_fwht_params(ctrl_obj);
		default:
			break;
		}
	}
	return ctrl;
}

struct v4l2_ext_control *retrace_v4l2_ext_control_array_pointer(json_object *ext_controls_obj, int count)
{
	struct v4l2_ext_control *ctrl_array_pointer = (struct v4l2_ext_control *) calloc(count, sizeof(v4l2_ext_control));

	for (int i = 0; i < count; i++)
		ctrl_array_pointer[i] = retrace_v4l2_ext_control(ext_controls_obj, i);

	return ctrl_array_pointer;
}

void retrace_vidioc_s_ext_ctrls(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_ext_controls ext_controls;
	memset(&ext_controls, 0, sizeof(ext_controls));

	json_object *ext_controls_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_ext_controls", &ext_controls_obj);

	json_object *which_obj;
	json_object_object_get_ex(ext_controls_obj, "which", &which_obj);
	ext_controls.which = json_object_get_int(which_obj);

	json_object *count_obj;
	json_object_object_get_ex(ext_controls_obj, "count", &count_obj);
	ext_controls.count = json_object_get_int(count_obj);

	/* request_fd is only valid for V4L2_CTRL_WHICH_REQUEST_VAL */
	if (ext_controls.which == V4L2_CTRL_WHICH_REQUEST_VAL) {
		json_object *request_fd_obj;
		json_object_object_get_ex(ext_controls_obj, "request_fd", &request_fd_obj);
		int request_fd_trace = json_object_get_int(request_fd_obj);
		ext_controls.request_fd = get_fd_retrace_from_fd_trace(request_fd_trace);
	}

	ext_controls.controls = retrace_v4l2_ext_control_array_pointer(ext_controls_obj, ext_controls.count);

	ioctl(fd_retrace, VIDIOC_S_EXT_CTRLS, &ext_controls);

	if (verbose || (errno != 0))
		perror("VIDIOC_S_EXT_CTRLS");

	/* Free controls working backwards from the end of the controls array. */
	for (int i = ((int) ext_controls.count - 1); i >= 0 ; i--) {
		if (ext_controls.controls[i].ptr != nullptr)
			free(ext_controls.controls[i].ptr);
	}

	if (ext_controls.controls != nullptr)
		free(ext_controls.controls);
}

void retrace_query_ext_ctrl(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_query_ext_ctrl query_ext_ctrl;
	memset(&query_ext_ctrl, 0, sizeof(v4l2_query_ext_ctrl));

	json_object *query_ext_ctrl_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_query_ext_ctrl", &query_ext_ctrl_obj);

	json_object *id_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "id", &id_obj);
	query_ext_ctrl.id = json_object_get_int64(id_obj);

	ioctl(fd_retrace, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl);

	if (verbose) {
		perror("VIDIOC_QUERY_EXT_CTRL");
		fprintf(stderr, "id: %u\n\n", query_ext_ctrl.id);
	}
}

void retrace_vidioc_enum_fmt(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_fmtdesc fmtdesc;
	memset(&fmtdesc, 0, sizeof(v4l2_fmtdesc));

	json_object *v4l2_fmtdesc_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_fmtdesc", &v4l2_fmtdesc_obj);

	json_object *index_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "index", &index_obj);
	fmtdesc.index = json_object_get_int(index_obj);

	json_object *type_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "type", &type_obj);
	fmtdesc.type = json_object_get_int(type_obj);

	json_object *mbus_code_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "mbus_code", &mbus_code_obj);
	fmtdesc.mbus_code = json_object_get_int64(mbus_code_obj);

	ioctl(fd_retrace, VIDIOC_ENUM_FMT, &fmtdesc);

	if (verbose) {
		perror("VIDIOC_ENUM_FMT");
		fprintf(stderr, "index: %u\n", fmtdesc.index);
		fprintf(stderr, "type: %u\n", fmtdesc.type);
		fprintf(stderr, "flags: %u\n", fmtdesc.flags);
		fprintf(stderr, "description: %s\n", fmtdesc.description);
		fprintf(stderr, "pixelformat: %u\n", fmtdesc.pixelformat);
		fprintf(stderr, "mbus_code: %u\n\n", fmtdesc.mbus_code);
	}
}

void retrace_vidioc_querycap(int fd_retrace)
{
	struct v4l2_capability argp;
	memset(&argp, 0, sizeof(v4l2_capability));

	ioctl(fd_retrace, VIDIOC_QUERYCAP, &argp);

	if (verbose || (errno != 0))
		perror("VIDIOC_QUERYCAP");
}

void retrace_media_ioc_request_alloc(int fd_retrace, json_object *ioctl_args)
{
	/* Get the original request file descriptor from the original trace file. */
	json_object *request_fd_trace_obj;
	json_object_object_get_ex(ioctl_args, "request_fd", &request_fd_trace_obj);
	int request_fd_trace = json_object_get_int(request_fd_trace_obj);

	/* Allocate a request in the retrace context. */
	__s32 request_fd_retrace = 0;
	ioctl(fd_retrace, MEDIA_IOC_REQUEST_ALLOC, &request_fd_retrace);

	/* Associate the original request file descriptor with the current request file descriptor. */
	add_fd(request_fd_trace, request_fd_retrace);

	if (verbose || (errno != 0))
		perror("MEDIA_IOC_REQUEST_ALLOC");
}

void retrace_dma_buf_ioctl_sync(int fd_retrace, json_object *ioctl_args_user)
{
	struct dma_buf_sync sync;
	memset(&sync, 0, sizeof(dma_buf_sync));

	json_object *sync_obj;
	json_object_object_get_ex(ioctl_args_user, "dma_buf_sync",&sync_obj);

	json_object *flags_obj;
	json_object_object_get_ex(sync_obj, "flags",&flags_obj);
	sync.flags = json_object_get_int(flags_obj);

	ioctl(fd_retrace, DMA_BUF_IOCTL_SYNC, &sync);

	if (verbose || (errno != 0))
		perror("DMA_BUF_IOCTL_SYNC");
}

void retrace_ioctl_media(int fd_retrace, long request, json_object *ioctl_args_driver)
{
	switch (request){
	case MEDIA_IOC_DEVICE_INFO:
	case MEDIA_IOC_ENUM_ENTITIES:
	case MEDIA_IOC_ENUM_LINKS:
	case MEDIA_IOC_SETUP_LINK:
	case MEDIA_IOC_G_TOPOLOGY: {
		struct media_v2_topology top;
		memset(&top, 0, sizeof(media_v2_topology));
		ioctl(fd_retrace, MEDIA_IOC_G_TOPOLOGY, &top);
		if (verbose || (errno != 0))
			perror("MEDIA_IOC_G_TOPOLOGY");
		break;
	}
	case MEDIA_IOC_REQUEST_ALLOC:
		retrace_media_ioc_request_alloc(fd_retrace, ioctl_args_driver);
		break;
	case MEDIA_REQUEST_IOC_QUEUE: {
		ioctl(fd_retrace, MEDIA_REQUEST_IOC_QUEUE);
		if (verbose || (errno != 0))
			perror("MEDIA_REQUEST_IOC_QUEUE");
		break;
	}
	case MEDIA_REQUEST_IOC_REINIT: {
		ioctl(fd_retrace, MEDIA_REQUEST_IOC_REINIT);
		if (verbose || (errno != 0))
			perror("MEDIA_REQUEST_IOC_REINIT");
		break;
	}
	default:
		break;
	}
}

void retrace_ioctl_video(int fd_retrace, long request, json_object *ioctl_args_user, json_object *ioctl_args_driver)
{
	switch (request) {
	case VIDIOC_QUERYCAP:
		retrace_vidioc_querycap(fd_retrace);
		break;
	case VIDIOC_ENUM_FMT:
		retrace_vidioc_enum_fmt(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_G_FMT:
		retrace_vidioc_g_fmt(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_S_FMT:
		retrace_vidioc_s_fmt(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_REQBUFS:
		retrace_vidioc_reqbufs(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_QUERYBUF:
		retrace_vidioc_querybuf(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_QBUF:
		retrace_vidioc_qbuf(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_EXPBUF:
		retrace_vidioc_expbuf(fd_retrace, ioctl_args_user, ioctl_args_driver);
		break;
	case VIDIOC_DQBUF:
		retrace_vidioc_dqbuf(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_STREAMON:
		retrace_vidioc_streamon(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_STREAMOFF:
		retrace_vidioc_streamoff(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_S_EXT_CTRLS:
		retrace_vidioc_s_ext_ctrls(fd_retrace, ioctl_args_user);
		break;
	case VIDIOC_QUERY_EXT_CTRL:
		retrace_query_ext_ctrl(fd_retrace, ioctl_args_user);
		break;
	default:
		break;
	}
}

void retrace_ioctl(json_object *syscall_obj)
{
	int fd_retrace = 0;
	__u8 ioctl_type = 0;
	long request = 0;

	json_object *fd_trace_obj;
	json_object_object_get_ex(syscall_obj, "fd", &fd_trace_obj);
	fd_retrace = get_fd_retrace_from_fd_trace(json_object_get_int(fd_trace_obj));

	json_object *request_obj;
	json_object_object_get_ex(syscall_obj, "request", &request_obj);
	request = json_object_get_int64(request_obj);

	json_object *ioctl_args_user;
	json_object_object_get_ex(syscall_obj, "ioctl_args_from_userspace", &ioctl_args_user);

	json_object *ioctl_args_driver;
	json_object_object_get_ex(syscall_obj, "ioctl_args_from_driver", &ioctl_args_driver);

	ioctl_type = _IOC_TYPE(request);
	switch (ioctl_type) {
	case 'V':
		retrace_ioctl_video(fd_retrace, request, ioctl_args_user, ioctl_args_driver);
		break;
	case '|':
		retrace_ioctl_media(fd_retrace, request, ioctl_args_driver);
		break;
	case 'b':
		if (request == DMA_BUF_IOCTL_SYNC)
			retrace_dma_buf_ioctl_sync(fd_retrace, ioctl_args_user);
		break;
	default:
		break;
	}
}

void write_to_output_buffer(unsigned char *buffer_pointer, int bytesused, json_object *mem_obj)
{
	std::string data;
	int byteswritten = 0;
	json_object *line_obj;
	size_t number_of_lines;

	json_object *mem_array_obj;
	json_object_object_get_ex(mem_obj, "mem_array", &mem_array_obj);
	number_of_lines = json_object_array_length(mem_array_obj);

	for (long unsigned int i = 0; i < number_of_lines; i++) {
		line_obj = json_object_array_get_idx(mem_array_obj, i);
		data = json_object_get_string(line_obj);

		for (long unsigned i = 0; i < data.length(); i++) {
			if (std::isspace(data[i]))
				continue;
			try {
				/* Two values from the string e.g. "D9" are needed to write one byte. */
				*buffer_pointer = (char) std::stoi(data.substr(i,2), nullptr, 16);
				buffer_pointer++;
				i++;
				byteswritten++;
			} catch (std::invalid_argument& ia) {
				fprintf(stderr, "Warning: \'%s\' is an invalid argument; %s line: %d.\n",
				        data.substr(i,2).c_str(), __func__, __LINE__);
			} catch (std::out_of_range& oor) {
				fprintf(stderr, "Warning: \'%s\' is out of range; %s line: %d.\n",
				        data.substr(i,2).c_str(), __func__, __LINE__);
			}
		}
	}

	if (verbose) {
		fprintf(stderr, "\nWrite to Output Buffer\n");
		fprintf(stderr, "bytesused: %d, byteswritten: %d\n", bytesused, byteswritten);
		fprintf(stderr, "\n");
	}
}

void write_decoded_frames_to_yuv_file_retrace(unsigned char *buffer_pointer, int bytesused)
{
	int byteswritten = 0;
	int expected_length = (int) get_expected_length_retrace();

	FILE *fp = fopen(retrace_filename.c_str(), "a");
	for (int i = 0; i < bytesused; i++) {
		if (i < expected_length) {
			fwrite(&buffer_pointer[i], sizeof(unsigned char), 1, fp);
			byteswritten++;
		}
	}
	fclose(fp);

	if (verbose){
		fprintf(stderr, "\nWrite to File\n");
		fprintf(stderr, "%s\n", retrace_filename.c_str());
		fprintf(stderr, "buffer_pointer address: %ld, bytesused: %d, byteswritten: %d\n", (long) buffer_pointer, bytesused, byteswritten);
		fprintf(stderr, "\n");
		print_context();
	}
}

void retrace_mem(json_object *mem_obj)
{
	json_object *type_obj;
	json_object_object_get_ex(mem_obj, "type", &type_obj);
	v4l2_buf_type type = (v4l2_buf_type) json_object_get_int(type_obj);

	json_object *bytesused_obj;
	json_object_object_get_ex(mem_obj, "bytesused", &bytesused_obj);
	int bytesused = json_object_get_int64(bytesused_obj);
	if (!bytesused)
		return;

	json_object *offset_obj;
	json_object_object_get_ex(mem_obj, "offset", &offset_obj);
	__u32 offset = json_object_get_int64(offset_obj);

	json_object *address_obj;
	json_object_object_get_ex(mem_obj, "address", &address_obj);
	long buffer_address_trace = json_object_get_int64(address_obj);

	long buffer_address_retrace = get_retrace_address_from_trace_address(buffer_address_trace);

	unsigned char *buffer_pointer = (unsigned char *) buffer_address_retrace;

	/* Get the encoded data from the json file and write it to output buffer memory. */
	if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE || type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
		write_to_output_buffer(buffer_pointer, bytesused, mem_obj);

	/* Get the decoded capture buffer from memory and write it to a binary yuv file. */
	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE || type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		write_decoded_frames_to_yuv_file_retrace(buffer_pointer, bytesused);

	if (verbose) {
		fprintf(stderr, "\n%s\n", __func__);
		fprintf(stderr, "%s, bytesused: %d, offset: %d, buffer_address_retrace: %ld\n", buftype2s(type).c_str(), bytesused, offset, buffer_address_retrace);
		print_context();
	}
}

void retrace_object(json_object *jobj)
{
	json_object *syscall_obj;
	int ret = json_object_object_get_ex(jobj, "syscall", &syscall_obj);
	int syscall = json_object_get_int(syscall_obj);

	/* If the json object doesn't hold a syscall, check if it holds a memory dump. */
	if (ret == 0) {
		json_object *temp_obj;
		if (json_object_object_get_ex(jobj, "mem_dump", &temp_obj))
			retrace_mem(jobj);
		return;
	}

	errno = 0;

	switch (syscall) {
	case LIBTRACER_SYSCALL_IOCTL:
		retrace_ioctl(jobj);
		break;
	case LIBTRACER_SYSCALL_OPEN:
		retrace_open(jobj, false);
		break;
	case LIBTRACER_SYSCALL_OPEN64:
		retrace_open(jobj, true);
		break;
	case LIBTRACER_SYSCALL_CLOSE:
		retrace_close(jobj);
		break;
	case LIBTRACER_SYSCALL_MMAP:
		retrace_mmap(jobj, false);
		break;
	case LIBTRACER_SYSCALL_MMAP64:
		retrace_mmap(jobj, true);
		break;
	case LIBTRACER_SYSCALL_MUNMAP:
		retrace_munmap(jobj);
		break;
	default:
		break;
	}
}

void retrace_array(json_object *root_array_obj)
{
	json_object *jobj;
	struct array_list *al = json_object_get_array(root_array_obj);
	size_t json_objects_in_file = array_list_length(al);

	for (size_t i = 0; i < json_objects_in_file; i++) {
		jobj = (json_object *) array_list_get_idx(al, i);
		retrace_object(jobj);
	}
}

int get_options_retrace(int argc, char *argv[])
{
	int ch;
	do {
		ch = getopt_long(argc, argv, short_options_retracer, long_options_retracer, NULL);
		switch (ch){
		case OptSetDevice_Retracer: {
			std::string device = optarg;
			if (device[0] >= '0' && device[0] <= '9' && device.length() <= 3) {
				static char newdev[20];
				sprintf(newdev, "/dev/video%s", optarg);
				dev_path_video = newdev;
				fprintf(stderr, "Using: %s\n", dev_path_video.c_str());
			} else {
				return 1;
			}
			break;
		}
		case OptSetMediaDevice_Retracer: {
			std::string device = optarg;
			if (device[0] >= '0' && device[0] <= '9' && device.length() <= 3) {
				static char newdev[20];
				sprintf(newdev, "/dev/media%s", optarg);
				device = newdev;
				dev_path_media = newdev;
				fprintf(stderr, "Using: %s\n", dev_path_media.c_str());
			} else {
				return 1;
			}
			break;
		}
		case OptVerbose_Retracer:
			verbose = true;
			break;
		case '?':
		case ':':
			return 1;
		default:
			break;
		}
	} while (ch != -1);

	return 0;
}

void print_help_retracer(void)
{
		fprintf(stderr, "v4l2-tracer retrace [retrace options] -- <trace_file>.json\n"
	        "\t-d, --device <dev>   Use a different video device than specified in the trace file.\n"
	        "\t                     <dev> must be a digit corresponding to an existing /dev/video<dev> \n"
	        "\t-h, --help           Display retrace help.\n"
	        "\t-m, --media <dev>    Use a different media device than specified in the trace file.\n"
	        "\t                     <dev> must be a digit corresponding to an existing /dev/media<dev> \n"
	        "\t-v, --verbose        Turn on verbose reporting\n\n");
}

int retracer(int argc, char *argv[])
{
	if ((get_options_retrace(argc, argv)) || (optind == argc)) {
		print_help_retracer();
		return 1;
	}

	std::string trace_filename = argv[optind];

	if (trace_filename.substr(trace_filename.length()-4, trace_filename.npos) != "json") {
		fprintf(stderr, "Trace file must be json-formatted: %s\n", trace_filename.c_str());
		print_help_retracer();
		return 1;
	}

	FILE *trace_file = fopen(trace_filename.c_str(), "r");
	if (trace_file == NULL) {
		fprintf(stderr, "Trace file error: %s\n", trace_filename.c_str());
		return 1;
	}
	fclose(trace_file);

	fprintf(stderr, "Retracing: %s\n", trace_filename.c_str());

	/* Create file to hold the decoded frames.  Discard previous retraced file if any. */
	retrace_filename = trace_filename;
	retrace_filename = retrace_filename.replace(5, retrace_filename.npos, "_retrace");
	retrace_filename += ".yuv";

	/* Discard any previous retrace with same name. */
	fclose(fopen(retrace_filename.c_str(), "w"));

	json_object *root_array_obj = json_object_from_file(trace_filename.c_str());
	retrace_array(root_array_obj);
	json_object_put(root_array_obj);

	fprintf(stderr, "Retracing complete in %s\n", retrace_filename.c_str());

	return 0;
}
