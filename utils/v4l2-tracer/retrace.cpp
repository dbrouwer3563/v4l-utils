/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */
#include <stdexcept>
#include "v4l2-tracer-common.h"
#include "retrace-helper.h"
#include "retrace-ctrls-gen.h"

void compare_program_versions(json_object *v4l2_tracer_info_obj)
{
	json_object *package_version_obj;
	json_object_object_get_ex(v4l2_tracer_info_obj, "package_version", &package_version_obj);
	std::string package_version_trace = json_object_get_string(package_version_obj);
	std::string package_version_retrace = PACKAGE_VERSION;
	if (package_version_trace != package_version_retrace) {
		fprintf(stderr, "Retrace warning: package version in trace file \'%s\' does not match this version:\n",
		        package_version_trace.c_str());
		print_v4l2_tracer_info();
		return;
	}

	json_object *git_sha_obj;
	json_object_object_get_ex(v4l2_tracer_info_obj, "git_sha", &git_sha_obj);
	std::string git_sha_trace = json_object_get_string(git_sha_obj);
	std::string git_sha_retrace = (STRING(GIT_SHA));
	if (git_sha_trace != git_sha_retrace) {
		fprintf(stderr, "Retrace warning: sha in trace file \'%s\' does not match this sha:\n",
		        git_sha_trace.c_str());
		print_v4l2_tracer_info();
		return;
	}
}

void retrace_mmap(json_object *mmap_obj, bool is_mmap64)
{
	json_object *mmap_args_obj;
	if (is_mmap64)
		json_object_object_get_ex(mmap_obj, "mmap64", &mmap_args_obj);
	else
		json_object_object_get_ex(mmap_obj, "mmap", &mmap_args_obj);

	json_object *len_obj;
	json_object_object_get_ex(mmap_args_obj, "len", &len_obj);
	size_t len = (size_t) json_object_get_int(len_obj);

	json_object *prot_obj;
	json_object_object_get_ex(mmap_args_obj, "prot", &prot_obj);
	int prot = json_object_get_int(prot_obj);

	json_object *flags_obj;
	json_object_object_get_ex(mmap_args_obj, "flags", &flags_obj);
	int flags = s2number(json_object_get_string(flags_obj));

	json_object *fildes_obj;
	json_object_object_get_ex(mmap_args_obj, "fildes", &fildes_obj);
	int fd_trace = json_object_get_int(fildes_obj);

	json_object *off_obj;
	json_object_object_get_ex(mmap_args_obj, "off", &off_obj);
	off_t off = (off_t) json_object_get_int64(off_obj);

	int fd_retrace = get_fd_retrace_from_fd_trace(fd_trace);
	if (fd_retrace < 0) {
		fprintf(stderr, "Retrace error: %s: bad file descriptor\n", __func__);
		return;
	}

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
		if (is_debug())
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
	set_buffer_address_retrace(fd_retrace, off, buf_address_trace,
	                           (long) buf_address_retrace_pointer);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "fd: %d, offset: %ld, ", fd_retrace, off);
		if (is_mmap64)
			perror("mmap64");
		else
			perror("mmap");
		if (is_debug())
			print_context();
	}
}

void retrace_munmap(json_object *syscall_obj)
{
	json_object *munmap_args_obj;
	json_object_object_get_ex(syscall_obj, "munmap", &munmap_args_obj);

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

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "unmapped: %ld, ", buffer_address_retrace);
		perror("munmap");
	}
}

void retrace_open(json_object *jobj, bool is_open64)
{
	json_object *fd_trace_obj;
	json_object_object_get_ex(jobj, "fd", &fd_trace_obj);
	int fd_trace = json_object_get_int(fd_trace_obj);

	json_object *open_args_obj;
	if (is_open64)
		json_object_object_get_ex(jobj, "open64", &open_args_obj);
	else
		json_object_object_get_ex(jobj, "open", &open_args_obj);

	json_object *path_obj;
	json_object_object_get_ex(open_args_obj, "path", &path_obj);
	std::string path = json_object_get_string(path_obj);

	/* If user did not specify either a video or media device, try to match the driver. */
	if (!getenv("V4L2_TRACER_OPTION_SET_VIDEO_DEVICE") &&
	    !getenv("V4L2_TRACER_OPTION_SET_MEDIA_DEVICE")) {

		std::string driver;
		json_object *driver_obj;
		if (json_object_object_get_ex(open_args_obj, "driver", &driver_obj))
			driver = json_object_get_string(driver_obj);

		json_object *linked_entities_obj;
		json_object_object_get_ex(open_args_obj, "linked_entities", &linked_entities_obj);
		std::list<std::string> linked_entities;
		for (size_t i = 0; i < array_list_length(json_object_get_array(linked_entities_obj)); i++) {
			std::string entity_name = json_object_get_string(json_object_array_get_idx(linked_entities_obj, i));
			linked_entities.push_back(entity_name);
		}

		std::pair<std::string, std::string> device_paths = find_devices(driver, linked_entities);

		if (!device_paths.first.empty()) {
			path_media_global = device_paths.first;
			path_video_global = device_paths.second;
			if (is_verbose())
				fprintf(stderr, "Retracing on: %s, %s, %s\n",
				        driver.c_str(), device_paths.first.c_str(), device_paths.second.c_str());
		} else {
			/*
			 * Don't try to open media devices that can't be found. The trace file might have
			 * multiple open calls and only the last one needs to work.
			 */
			return;
		}
	}

	if (path.find("media") != path.npos)
		path = path_media_global;
	else if (path.find("video") != path.npos)
		path = path_video_global;

	json_object *oflag_obj;
	json_object_object_get_ex(open_args_obj, "oflag", &oflag_obj);
	int oflag = s2val(json_object_get_string(oflag_obj), open_val_def);

	int mode = 0;
	json_object *mode_obj;
	if (json_object_object_get_ex(open_args_obj, "mode", &mode_obj))
		mode = s2number(json_object_get_string(mode_obj));

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

	if (is_verbose() || errno != 0) {
		fprintf(stderr, "path: %s ", path.c_str());
		if (is_open64)
			perror("open64");
		else
			perror("open");
		if (is_debug())
			print_context();
	}
}

void retrace_close(json_object *jobj)
{
	json_object *fd_trace_obj;
	json_object_object_get_ex(jobj, "fd", &fd_trace_obj);
	int fd_retrace = get_fd_retrace_from_fd_trace(json_object_get_int(fd_trace_obj));

	/* Only close devices that were opened in the retrace context. */
	if (fd_retrace < 0)
		return;

	close(fd_retrace);
	remove_fd(json_object_get_int(fd_trace_obj));

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "fd: %d ", fd_retrace);
		perror("close");
		if (is_debug())
			print_context();
	}
}

struct v4l2_requestbuffers retrace_v4l2_requestbuffers(json_object *ioctl_args)
{
	struct v4l2_requestbuffers request_buffers = {};

	json_object *requestbuffers_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_requestbuffers", &requestbuffers_obj);

	json_object *count_obj;
	json_object_object_get_ex(requestbuffers_obj, "count", &count_obj);
	request_buffers.count = json_object_get_int(count_obj);

	json_object *type_obj;
	json_object_object_get_ex(requestbuffers_obj, "type", &type_obj);
	request_buffers.type = s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	json_object *memory_obj;
	json_object_object_get_ex(requestbuffers_obj, "memory", &memory_obj);
	request_buffers.memory = s2val(json_object_get_string(memory_obj), v4l2_memory_val_def);

	json_object *flags_obj;
	json_object_object_get_ex(requestbuffers_obj, "flags", &flags_obj);
	request_buffers.flags = s2flags(json_object_get_string(flags_obj), v4l2_memory_flag_def);

	return request_buffers;
}

void retrace_vidioc_reqbufs(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_requestbuffers request_buffers = retrace_v4l2_requestbuffers(ioctl_args);

	ioctl(fd_retrace, VIDIOC_REQBUFS, &request_buffers);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, count: %d, ", buftype2s(request_buffers.type).c_str(), request_buffers.count);
		perror("VIDIOC_REQBUFS");
		if (is_debug())
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
	buf->type = s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	json_object *bytesused_obj;
	json_object_object_get_ex(buf_obj, "bytesused", &bytesused_obj);
	buf->bytesused = (__u32) json_object_get_uint64(bytesused_obj);

	json_object *flags_obj;
	json_object_object_get_ex(buf_obj, "flags", &flags_obj);
	std::string flags_str = json_object_get_string(flags_obj);
	buf->flags = (__u32) s2flags_buffer(flags_str);

	json_object *field_obj;
	json_object_object_get_ex(buf_obj, "field", &field_obj);
	buf->field = (__u32) s2val(json_object_get_string(field_obj), v4l2_field_val_def);

	json_object *timestamp_obj;
	json_object_object_get_ex(buf_obj, "timestamp", &timestamp_obj);

	struct timeval tv = {};
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
	buf->memory = s2val(json_object_get_string(memory_obj), v4l2_memory_val_def);

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
		 /* TODO add planes > 0 */
		json_object *plane_obj = json_object_array_get_idx(planes_obj, 0);
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
		if (buf->request_fd < 0)
			fprintf(stderr, "Retrace error: %s: bad file descriptor\n", __func__);
	}

	return buf;
}

void retrace_vidioc_querybuf(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_buffer *buf = retrace_v4l2_buffer(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_QUERYBUF, buf);

	switch (buf->memory) {
	case V4L2_MEMORY_MMAP: {
		__u32 offset = 0;
		if ((buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
		    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT))
			offset = buf->m.offset;
		if ((buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) ||
		    (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE))
			offset = buf->m.planes->m.mem_offset;
		if (!get_buffer_fd_retrace(buf->type, buf->index))
			add_buffer_retrace(fd_retrace, buf->type, buf->index, offset);
		break;
	}
	case V4L2_MEMORY_USERPTR:
	case V4L2_MEMORY_DMABUF:
	default:
		break;
	}

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes != nullptr) {
			free(buf->m.planes);
		}
	}

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, index: %d, fd: %d, ", buftype2s(buf->type).c_str(), buf->index, fd_retrace);
		perror("VIDIOC_QUERYBUF");
		if (is_debug())
			print_context();
	}

	free(buf);
}

void retrace_vidioc_qbuf(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_buffer *buf = retrace_v4l2_buffer(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_QBUF, buf);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ||
	    buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
		if (buf->m.planes != nullptr) {
			free(buf->m.planes);
		}
	}

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, index: %d, fd: %d, ", buftype2s(buf->type).c_str(), buf->index, fd_retrace);
		perror("VIDIOC_QBUF");
		if (is_debug())
			print_context();
	}

	free(buf);
}

struct v4l2_exportbuffer retrace_v4l2_exportbuffer(json_object *ioctl_args)
{
	struct v4l2_exportbuffer export_buffer = {};

	json_object *exportbuffer_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_exportbuffer", &exportbuffer_obj);

	json_object *type_obj;
	json_object_object_get_ex(exportbuffer_obj, "type", &type_obj);
	export_buffer.type = s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	json_object *index_obj;
	json_object_object_get_ex(exportbuffer_obj, "index", &index_obj);
	export_buffer.index = json_object_get_int(index_obj);

	json_object *plane_obj;
	json_object_object_get_ex(exportbuffer_obj, "plane", &plane_obj);
	export_buffer.plane = json_object_get_int(plane_obj);

	json_object *flags_obj;
	json_object_object_get_ex(exportbuffer_obj, "flags", &flags_obj);
	export_buffer.flags = s2number(json_object_get_string(flags_obj));

	json_object *fd_obj;
	json_object_object_get_ex(exportbuffer_obj, "fd", &fd_obj);
	export_buffer.fd = json_object_get_int(fd_obj);

	return export_buffer;
}

void retrace_vidioc_expbuf(int fd_retrace, json_object *ioctl_args_user, json_object *ioctl_args_driver)
{
	struct v4l2_exportbuffer export_buffer = retrace_v4l2_exportbuffer(ioctl_args_user);
	ioctl(fd_retrace, VIDIOC_EXPBUF, &export_buffer);

	int buf_fd_retrace = export_buffer.fd;

	/*
	 * If a buffer was previously added to the retrace context using the video device
	 * file descriptor, replace the video fd with the more specific buffer fd from EXPBUF.
	 */
	int fd_found_in_retrace_context = get_buffer_fd_retrace(export_buffer.type, export_buffer.index);
	if (fd_found_in_retrace_context)
		remove_buffer_retrace(fd_found_in_retrace_context);

	add_buffer_retrace(buf_fd_retrace, export_buffer.type, export_buffer.index);

	/*
	 * Get the export buffer file descriptor as provided by the driver in the original
	 * trace context. Then associate this original file descriptor with the current
	 * file descriptor in the retrace context.
	 */
	memset(&export_buffer, 0, sizeof(v4l2_exportbuffer));
	export_buffer = retrace_v4l2_exportbuffer(ioctl_args_driver);
	int buf_fd_trace = export_buffer.fd;
	add_fd(buf_fd_trace, buf_fd_retrace);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, index: %d, fd: %d, ", buftype2s(export_buffer.type).c_str(), export_buffer.index, buf_fd_retrace);
		perror("VIDIOC_EXPBUF");
		if (is_debug())
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

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, index: %d, fd: %d, ", buftype2s(buf->type).c_str(), buf->index, fd_retrace);
		perror("VIDIOC_DQBUF");
		if (is_debug())
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
	json_object *type_obj;
	json_object_object_get_ex(ioctl_args, "type", &type_obj);
	v4l2_buf_type buf_type = (v4l2_buf_type) s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	ioctl(fd_retrace, VIDIOC_STREAMON, &buf_type);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, ", buftype2s(buf_type).c_str());
		perror("VIDIOC_STREAMON");
	}
}

void retrace_vidioc_streamoff(int fd_retrace, json_object *ioctl_args)
{
	json_object *type_obj;
	json_object_object_get_ex(ioctl_args, "type", &type_obj);
	v4l2_buf_type buf_type = (v4l2_buf_type) s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	ioctl(fd_retrace, VIDIOC_STREAMOFF, &buf_type);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, ", buftype2s(buf_type).c_str());
		perror("VIDIOC_STREAMOFF");
	}
}

struct v4l2_plane_pix_format get_v4l2_plane_pix_format(json_object *pix_mp_obj, int plane)
{
	std::string key;
	struct v4l2_plane_pix_format plane_fmt = {};

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

struct v4l2_pix_format retrace_v4l2_pix_format(json_object *v4l2_format_obj)
{
	struct v4l2_pix_format pix = {};

	json_object *pix_obj;
	json_object_object_get_ex(v4l2_format_obj, "v4l2_pix_format", &pix_obj);

	json_object *width_obj;
	json_object_object_get_ex(pix_obj, "width", &width_obj);
	pix.width = json_object_get_int(width_obj);

	json_object *height_obj;
	json_object_object_get_ex(pix_obj, "height", &height_obj);
	pix.height = json_object_get_int(height_obj);

	json_object *pixelformat_obj;
	json_object_object_get_ex(pix_obj, "pixelformat", &pixelformat_obj);
	pix.pixelformat = s2val(json_object_get_string(pixelformat_obj), v4l2_pix_fmt_val_def);

	json_object *field_obj;
	json_object_object_get_ex(pix_obj, "field", &field_obj);
	pix.field = s2val(json_object_get_string(field_obj), v4l2_field_val_def);

	json_object *bytesperline_obj;
	json_object_object_get_ex(pix_obj, "bytesperline", &bytesperline_obj);
	pix.bytesperline = json_object_get_uint64(bytesperline_obj);

	json_object *sizeimage_obj;
	json_object_object_get_ex(pix_obj, "sizeimage", &sizeimage_obj);
	pix.sizeimage = json_object_get_uint64(sizeimage_obj);

	json_object *colorspace_obj;
	json_object_object_get_ex(pix_obj, "colorspace", &colorspace_obj);
	pix.colorspace = s2val(json_object_get_string(colorspace_obj), v4l2_colorspace_val_def);

	json_object *priv_obj;
	json_object_object_get_ex(pix_obj, "priv", &priv_obj);
	std::string priv = json_object_get_string(priv_obj);

	if (priv != "V4L2_PIX_FMT_PRIV_MAGIC") {
		return pix;
	}
	pix.priv = V4L2_PIX_FMT_PRIV_MAGIC;
	json_object *flags_obj;
	json_object_object_get_ex(pix_obj, "flags", &flags_obj);
	pix.flags = s2flags(json_object_get_string(flags_obj), v4l2_pix_fmt_flag_def);

	json_object *ycbcr_enc_obj;
	json_object_object_get_ex(pix_obj, "ycbcr_enc", &ycbcr_enc_obj);
	pix.ycbcr_enc = s2val(json_object_get_string(ycbcr_enc_obj), v4l2_ycbcr_encoding_val_def);

	json_object *quantization_obj;
	json_object_object_get_ex(pix_obj, "quantization", &quantization_obj);
	pix.quantization = s2val(json_object_get_string(quantization_obj), v4l2_quantization_val_def);

	json_object *xfer_func_obj;
	json_object_object_get_ex(pix_obj, "xfer_func", &xfer_func_obj);
	pix.xfer_func = s2val(json_object_get_string(xfer_func_obj), v4l2_xfer_func_val_def);

	return pix;
}

struct v4l2_pix_format_mplane retrace_v4l2_pix_format_mplane(json_object *v4l2_format_obj)
{
	struct v4l2_pix_format_mplane pix_mp = {};

	json_object *pix_mp_obj;
	json_object_object_get_ex(v4l2_format_obj, "v4l2_pix_format_mplane", &pix_mp_obj);

	json_object *width_obj;
	json_object_object_get_ex(pix_mp_obj, "width", &width_obj);
	pix_mp.width = json_object_get_int(width_obj);

	json_object *height_obj;
	json_object_object_get_ex(pix_mp_obj, "height", &height_obj);
	pix_mp.height = json_object_get_int(height_obj);

	json_object *pixelformat_obj;
	if (json_object_object_get_ex(pix_mp_obj, "pixelformat", &pixelformat_obj))
		pix_mp.pixelformat = s2val(json_object_get_string(pixelformat_obj), v4l2_pix_fmt_val_def);

	json_object *field_obj;
	json_object_object_get_ex(pix_mp_obj, "field", &field_obj);
	pix_mp.field = s2val(json_object_get_string(field_obj), v4l2_field_val_def);

	json_object *colorspace_obj;
	json_object_object_get_ex(pix_mp_obj, "colorspace", &colorspace_obj);
	pix_mp.colorspace = s2val(json_object_get_string(colorspace_obj), v4l2_colorspace_val_def);

	json_object *num_planes_obj;
	json_object_object_get_ex(pix_mp_obj, "num_planes", &num_planes_obj);
	pix_mp.num_planes = json_object_get_int(num_planes_obj);

	for (int i = 0; i < pix_mp.num_planes; i++)
		pix_mp.plane_fmt[i] = get_v4l2_plane_pix_format(pix_mp_obj, i);

	json_object *flags_obj;
	if (json_object_object_get_ex(pix_mp_obj, "flags", &flags_obj))
		pix_mp.flags = s2flags(json_object_get_string(flags_obj), v4l2_pix_fmt_flag_def);

	json_object *ycbcr_enc_obj;
	json_object_object_get_ex(pix_mp_obj, "ycbcr_enc", &ycbcr_enc_obj);
	pix_mp.ycbcr_enc = s2val(json_object_get_string(ycbcr_enc_obj), v4l2_ycbcr_encoding_val_def);

	json_object *quantization_obj;
	json_object_object_get_ex(pix_mp_obj, "quantization", &quantization_obj);
	pix_mp.quantization = s2val(json_object_get_string(quantization_obj), v4l2_quantization_val_def);

	json_object *xfer_func_obj;
	json_object_object_get_ex(pix_mp_obj, "xfer_func", &xfer_func_obj);
	pix_mp.xfer_func = s2val(json_object_get_string(xfer_func_obj), v4l2_xfer_func_val_def);

	return pix_mp;
}

struct v4l2_format retrace_v4l2_format(json_object *ioctl_args)
{
	struct v4l2_format format = {};

	json_object *v4l2_format_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_format", &v4l2_format_obj);

	json_object *type_obj;
	json_object_object_get_ex(v4l2_format_obj, "type", &type_obj);
	format.type = s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	switch (format.type) {
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
		format.fmt.pix = retrace_v4l2_pix_format(v4l2_format_obj);
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
	struct v4l2_format format = retrace_v4l2_format(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_G_FMT, &format);

	if (is_verbose() || (errno != 0))
		perror("VIDIOC_G_FMT");
}

void retrace_vidioc_s_fmt(int fd_retrace, json_object *ioctl_args_user)
{
	struct v4l2_format format = retrace_v4l2_format(ioctl_args_user);

	ioctl(fd_retrace, VIDIOC_S_FMT, &format);

	if (is_verbose() || (errno != 0)) {
		fprintf(stderr, "%s, %s, ", buftype2s(format.type).c_str(), fcc2s(format.fmt.pix_mp.pixelformat).c_str());
		perror("VIDIOC_S_FMT");
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
	struct v4l2_ext_control ctrl = {};

	std::string unique_key_for_control = "v4l2_ext_control_";
	unique_key_for_control += std::to_string(ctrl_idx);

	json_object *ctrl_obj;
	json_object_object_get_ex(ext_controls_obj, unique_key_for_control.c_str(), &ctrl_obj);

	json_object *id_obj;
	json_object_object_get_ex(ctrl_obj, "id", &id_obj);
	ctrl.id = s2val(json_object_get_string(id_obj), stateless_controls_val_def);

	json_object *size_obj;
	json_object_object_get_ex(ctrl_obj, "size", &size_obj);
	ctrl.size = json_object_get_int64(size_obj);

	switch (ctrl.id) {
	case V4L2_CID_STATELESS_VP8_FRAME:
		ctrl.ptr = retrace_v4l2_ctrl_vp8_frame_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_MODE:
	case V4L2_CID_STATELESS_H264_START_CODE:
		ctrl.value = retrace_v4l2_ext_control_value(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_SPS:
		ctrl.ptr = retrace_v4l2_ctrl_h264_sps_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_PPS:
		ctrl.ptr = retrace_v4l2_ctrl_h264_pps_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_SCALING_MATRIX:
		ctrl.ptr = retrace_v4l2_ctrl_h264_scaling_matrix_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_PRED_WEIGHTS:
		ctrl.ptr = retrace_v4l2_ctrl_h264_pred_weights_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_SLICE_PARAMS:
		ctrl.ptr = retrace_v4l2_ctrl_h264_slice_params_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_H264_DECODE_PARAMS:
		ctrl.ptr = retrace_v4l2_ctrl_h264_decode_params_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_FWHT_PARAMS:
		ctrl.ptr = retrace_v4l2_ctrl_fwht_params_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_VP9_FRAME:
		ctrl.ptr = retrace_v4l2_ctrl_vp9_frame_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_VP9_COMPRESSED_HDR:
		ctrl.ptr = retrace_v4l2_ctrl_vp9_compressed_hdr_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SPS:
		ctrl.ptr = retrace_v4l2_ctrl_hevc_sps_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_PPS:
		ctrl.ptr = retrace_v4l2_ctrl_hevc_pps_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SLICE_PARAMS:
		ctrl.ptr = retrace_v4l2_ctrl_hevc_slice_params_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_SCALING_MATRIX:
		ctrl.ptr = retrace_v4l2_ctrl_hevc_scaling_matrix_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_HEVC_DECODE_PARAMS:
		ctrl.ptr = retrace_v4l2_ctrl_hevc_decode_params_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_SEQUENCE:
		ctrl.ptr = retrace_v4l2_ctrl_mpeg2_sequence_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_PICTURE:
		ctrl.ptr = retrace_v4l2_ctrl_mpeg2_picture_gen(ctrl_obj);
		break;
	case V4L2_CID_STATELESS_MPEG2_QUANTISATION:
		ctrl.ptr = retrace_v4l2_ctrl_mpeg2_quantisation_gen(ctrl_obj);
		break;
	default:
		fprintf(stderr, "Unknown control: %d\n", ctrl.id);
		break;
	}
	return ctrl;
}

struct v4l2_ext_control *retrace_v4l2_ext_control_array_pointer(json_object *ext_controls_obj, int count)
{
	struct v4l2_ext_control *ctrl_array_pointer;
	ctrl_array_pointer = (struct v4l2_ext_control *) calloc(count, sizeof(v4l2_ext_control));

	for (int i = 0; i < count; i++)
		ctrl_array_pointer[i] = retrace_v4l2_ext_control(ext_controls_obj, i);

	return ctrl_array_pointer;
}

void retrace_vidioc_s_ext_ctrls(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_ext_controls ext_controls = {};

	json_object *ext_controls_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_ext_controls", &ext_controls_obj);

	json_object *which_obj;
	json_object_object_get_ex(ext_controls_obj, "which", &which_obj);
	ext_controls.which = s2val(json_object_get_string(which_obj), which_val_def);

	json_object *count_obj;
	json_object_object_get_ex(ext_controls_obj, "count", &count_obj);
	ext_controls.count = json_object_get_int(count_obj);

	/* request_fd is only valid for V4L2_CTRL_WHICH_REQUEST_VAL */
	if (ext_controls.which == V4L2_CTRL_WHICH_REQUEST_VAL) {
		json_object *request_fd_obj;
		json_object_object_get_ex(ext_controls_obj, "request_fd", &request_fd_obj);
		int request_fd_trace = json_object_get_int(request_fd_obj);
		ext_controls.request_fd = get_fd_retrace_from_fd_trace(request_fd_trace);
		if (ext_controls.request_fd < 0) {
			fprintf(stderr, "Retrace error: %s: bad file descriptor\n", __func__);
			return;
		}
	}

	ext_controls.controls = retrace_v4l2_ext_control_array_pointer(ext_controls_obj, ext_controls.count);

	ioctl(fd_retrace, VIDIOC_S_EXT_CTRLS, &ext_controls);

	if (is_verbose() || (errno != 0))
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
	struct v4l2_query_ext_ctrl query_ext_ctrl = {};

	json_object *query_ext_ctrl_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_query_ext_ctrl", &query_ext_ctrl_obj);

	json_object *id_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "id", &id_obj);
	query_ext_ctrl.id = s2number(json_object_get_string(id_obj));

	json_object *type_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "type", &type_obj);
	query_ext_ctrl.type = s2val(json_object_get_string(type_obj), v4l2_ctrl_type_val_def);

	json_object *name_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "name", &name_obj);
	strcpy(query_ext_ctrl.name, json_object_get_string(name_obj));

	json_object *minimum_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "minimum", &minimum_obj);
	query_ext_ctrl.minimum = json_object_get_int64(minimum_obj);

	json_object *maximum_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "maximum", &maximum_obj);
	query_ext_ctrl.maximum = json_object_get_int64(maximum_obj);

	json_object *step_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "step", &step_obj);
	query_ext_ctrl.step = json_object_get_uint64(step_obj);

	json_object *default_value_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "default_value", &default_value_obj);
	query_ext_ctrl.default_value = json_object_get_int64(default_value_obj);

	json_object *flags_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "flags", &flags_obj);
	query_ext_ctrl.flags = s2flags(json_object_get_string(flags_obj), v4l2_ctrl_flag_def);

	json_object *elem_size_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "elem_size", &elem_size_obj);
	query_ext_ctrl.elem_size = json_object_get_uint64(elem_size_obj);

	json_object *elems_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "elems", &elems_obj);
	query_ext_ctrl.elems = json_object_get_uint64(elems_obj);

	json_object *nr_of_dims_obj;
	json_object_object_get_ex(query_ext_ctrl_obj, "nr_of_dims", &nr_of_dims_obj);
	query_ext_ctrl.nr_of_dims = json_object_get_uint64(nr_of_dims_obj);

	ioctl(fd_retrace, VIDIOC_QUERY_EXT_CTRL, &query_ext_ctrl);

	if (is_verbose()) {
		fprintf(stderr, "id: %x\n\n", query_ext_ctrl.id);
		perror("VIDIOC_QUERY_EXT_CTRL");
	}
}

void retrace_vidioc_enum_fmt(int fd_retrace, json_object *ioctl_args)
{
	struct v4l2_fmtdesc fmtdesc = {};

	json_object *v4l2_fmtdesc_obj;
	json_object_object_get_ex(ioctl_args, "v4l2_fmtdesc", &v4l2_fmtdesc_obj);

	json_object *index_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "index", &index_obj);
	fmtdesc.index = json_object_get_int(index_obj);

	json_object *type_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "type", &type_obj);
	fmtdesc.type = s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

	json_object *flags_obj;
	if (json_object_object_get_ex(v4l2_fmtdesc_obj, "flags", &flags_obj))
		fmtdesc.flags = s2flags(json_object_get_string(flags_obj), v4l2_fmt_flag_def);

	json_object *pixelformat_obj;
	if (json_object_object_get_ex(v4l2_fmtdesc_obj, "pixelformat", &pixelformat_obj))
		fmtdesc.pixelformat = s2val(json_object_get_string(pixelformat_obj), v4l2_pix_fmt_val_def);

	json_object *mbus_code_obj;
	json_object_object_get_ex(v4l2_fmtdesc_obj, "mbus_code", &mbus_code_obj);
	fmtdesc.mbus_code = json_object_get_int64(mbus_code_obj);

	ioctl(fd_retrace, VIDIOC_ENUM_FMT, &fmtdesc);

	if (is_verbose())
		perror("VIDIOC_ENUM_FMT");
}

void retrace_vidioc_querycap(int fd_retrace)
{
	struct v4l2_capability argp = {};

	ioctl(fd_retrace, VIDIOC_QUERYCAP, &argp);

	if (is_verbose() || (errno != 0))
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

	if (is_verbose() || (errno != 0))
		perror("MEDIA_IOC_REQUEST_ALLOC");
}

void retrace_ioctl_media(int fd_retrace, long cmd, json_object *ioctl_args_driver)
{
	switch (cmd) {
	case MEDIA_IOC_REQUEST_ALLOC:
		retrace_media_ioc_request_alloc(fd_retrace, ioctl_args_driver);
		break;
	case MEDIA_REQUEST_IOC_QUEUE: {
		ioctl(fd_retrace, MEDIA_REQUEST_IOC_QUEUE);
		if (is_verbose() || (errno != 0))
			perror("MEDIA_REQUEST_IOC_QUEUE");
		break;
	}
	case MEDIA_REQUEST_IOC_REINIT: {
		ioctl(fd_retrace, MEDIA_REQUEST_IOC_REINIT);
		if (is_verbose() || (errno != 0))
			perror("MEDIA_REQUEST_IOC_REINIT");
		break;
	}
	default:
		break;
	}
}

void retrace_ioctl_video(int fd_retrace, long cmd, json_object *ioctl_args_user, json_object *ioctl_args_driver)
{
	switch (cmd) {
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
	long cmd = 0;

	json_object *fd_trace_obj;
	json_object_object_get_ex(syscall_obj, "fd", &fd_trace_obj);
	fd_retrace = get_fd_retrace_from_fd_trace(json_object_get_int(fd_trace_obj));
	if (fd_retrace < 0) {
		fprintf(stderr, "Retrace error: %s: bad file descriptor\n", __func__);
		return;
	}

	json_object *cmd_obj;
	json_object_object_get_ex(syscall_obj, "ioctl", &cmd_obj);
	cmd = s2ioctl(json_object_get_string(cmd_obj));

	json_object *ioctl_args_user;
	json_object_object_get_ex(syscall_obj, "from_userspace", &ioctl_args_user);

	json_object *ioctl_args_driver;
	json_object_object_get_ex(syscall_obj, "from_driver", &ioctl_args_driver);

	ioctl_type = _IOC_TYPE(cmd);
	switch (ioctl_type) {
	case 'V':
		retrace_ioctl_video(fd_retrace, cmd, ioctl_args_user, ioctl_args_driver);
		break;
	case '|':
		retrace_ioctl_media(fd_retrace, cmd, ioctl_args_driver);
		break;
	default:
		fprintf(stderr, "Warning: unrecognized ioctl \'0x%lx\', %s line: %d.\n", cmd, __func__, __LINE__);
		break;
	}
}

void write_to_output_buffer(unsigned char *buffer_pointer, int bytesused, json_object *mem_obj)
{
	int byteswritten = 0;
	json_object *line_obj;
	size_t number_of_lines;
	std::string compressed_video_data;

	json_object *mem_array_obj;
	json_object_object_get_ex(mem_obj, "mem_array", &mem_array_obj);
	number_of_lines = json_object_array_length(mem_array_obj);

	for (long unsigned int i = 0; i < number_of_lines; i++) {
		line_obj = json_object_array_get_idx(mem_array_obj, i);
		compressed_video_data = json_object_get_string(line_obj);

		for (long unsigned i = 0; i < compressed_video_data.length(); i++) {
			if (std::isspace(compressed_video_data[i]))
				continue;
			try {
				/* Two values from the string e.g. "D9" are needed to write one byte. */
				*buffer_pointer = (char) std::stoi(compressed_video_data.substr(i,2), nullptr, 16);
				buffer_pointer++;
				i++;
				byteswritten++;
			} catch (std::invalid_argument& ia) {
				fprintf(stderr, "Warning: \'%s\' is an invalid argument; %s line: %d.\n",
				        compressed_video_data.substr(i,2).c_str(), __func__, __LINE__);
			} catch (std::out_of_range& oor) {
				fprintf(stderr, "Warning: \'%s\' is out of range; %s line: %d.\n",
				        compressed_video_data.substr(i,2).c_str(), __func__, __LINE__);
			}
		}
	}

	if (is_verbose())
		fprintf(stderr, "%s: bytesused: %d, byteswritten: %d\n", __func__, bytesused, byteswritten);
}

void retrace_mem(json_object *mem_obj)
{
	json_object *type_obj;
	json_object_object_get_ex(mem_obj, "mem_dump", &type_obj);
	v4l2_buf_type type = (v4l2_buf_type) s2val(json_object_get_string(type_obj), v4l2_buf_type_val_def);

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

	if (is_verbose()) {
		fprintf(stderr, "%s: %s, bytesused: %d, offset: %d, addr: %ld\n",
		        __func__, buftype2s(type).c_str(), bytesused, offset, buffer_address_retrace);
		if (is_debug())
			print_context();
	}
}

void retrace_object(json_object *jobj)
{
	errno = 0;
	json_object *temp_obj;
	if (json_object_object_get_ex(jobj, "ioctl", &temp_obj)) {
		retrace_ioctl(jobj);
		return;
	}

	if (json_object_object_get_ex(jobj, "open", &temp_obj)) {
		retrace_open(jobj, false);
		return;
	}

	if (json_object_object_get_ex(jobj, "open64", &temp_obj)) {
		retrace_open(jobj, true);
		return;
	}

	if (json_object_object_get_ex(jobj, "close", &temp_obj)) {
		retrace_close(jobj);
		return;
	}

	if (json_object_object_get_ex(jobj, "mmap", &temp_obj)) {
		retrace_mmap(jobj, false);
		return;
	}

	if (json_object_object_get_ex(jobj, "mmap64", &temp_obj)) {
		retrace_mmap(jobj, true);
		return;
	}

	if (json_object_object_get_ex(jobj, "munmap", &temp_obj)) {
		retrace_munmap(jobj);
		return;
	}

	if (json_object_object_get_ex(jobj, "mem_dump", &temp_obj)) {
		retrace_mem(jobj);
		return;
	}

	if (json_object_object_get_ex(jobj, "package_version", &temp_obj)) {
		compare_program_versions(jobj);
		return;
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

int retracer (std::string trace_filename)
{
	FILE *trace_file = fopen(trace_filename.c_str(), "r");
	if (trace_file == NULL) {
		fprintf(stderr, "Trace file error: %s\n", trace_filename.c_str());
		return 1;
	}
	fclose(trace_file);

	fprintf(stderr, "Retracing: %s\n", trace_filename.c_str());

	json_object *root_array_obj = json_object_from_file(trace_filename.c_str());

	if (root_array_obj == nullptr) {
		fprintf(stderr, "Retrace error from file: %s\n", trace_filename.c_str());
		return 1;
	}

	retrace_array(root_array_obj);
	json_object_put(root_array_obj);

	return 0;
}