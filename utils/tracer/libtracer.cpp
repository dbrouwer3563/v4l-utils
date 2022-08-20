/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

int open64(const char *path, int oflag, ...)
{
	errno = 0;
	mode_t mode = 0;

	if (oflag & O_CREAT) {
		va_list ap;
		va_start(ap, oflag);
		mode = va_arg(ap, PROMOTED_MODE_T);
		va_end(ap);
	}

	int (*original_open64)(const char *path, int oflag, ...);
	original_open64 = (int (*)(const char*, int, ...)) dlsym(RTLD_NEXT, "open64");
	int fd = (*original_open64)(path, oflag, mode);

	/* Only trace calls to video or media devices. */
	std::string dev_path_video = "/dev/video";
	std::string dev_path_media = "/dev/media";
	if (strncmp(path, dev_path_video.c_str(), dev_path_video.length()) &&
	    strncmp(path, dev_path_media.c_str(), dev_path_media.length()))
		return fd;

	/* Set trace options if this is the first call to libtracer. */
	if (!options_are_set())
		set_options();

	add_device(fd, path);

	json_object *open_obj = json_object_new_object();
	json_object_object_add(open_obj, "syscall_str", json_object_new_string("open64"));
	json_object_object_add(open_obj, "syscall", json_object_new_int(LIBTRACER_SYSCALL_OPEN64));
	json_object_object_add(open_obj, "fd", json_object_new_int(fd));
	json_object *open_args = trace_open(path, oflag, mode);
	json_object_object_add(open_obj, "open_args", open_args);

	write_json_object_to_json_file(open_obj);
	json_object_put(open_obj);

	return fd;
}

void *mmap64(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	errno = 0;

	void *(*original_mmap64)(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
	original_mmap64 = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap64");
	void *buf_address_pointer = (*original_mmap64)(addr, len, prot, flags, fildes, off);

	/* Only trace mmap64 calls for output or capture buffers. */
	__u32 type = get_buffer_type_trace(fildes);
	if (!type)
		return buf_address_pointer;

	/* Save the buffer address for future reference. */
	set_buffer_address_trace(fildes, (long int) buf_address_pointer);

	json_object *mmap_obj = json_object_new_object();
	if (errno) {
		json_object_object_add(mmap_obj, "errno", json_object_new_int(errno));
		json_object_object_add(mmap_obj, "errno_str", json_object_new_string(strerror(errno)));
	}
	json_object_object_add(mmap_obj, "syscall_str", json_object_new_string("mmap64"));
	json_object_object_add(mmap_obj, "syscall", json_object_new_int(LIBTRACER_SYSCALL_MMAP64));

	json_object *mmap64_args = trace_mmap64(addr, len, prot, flags, fildes, off);
	json_object_object_add(mmap_obj, "mmap64_args", mmap64_args);

	json_object_object_add(mmap_obj, "buffer_address", json_object_new_int64((long int) buf_address_pointer));
	write_json_object_to_json_file(mmap_obj);
	json_object_put(mmap_obj);

	/*
	 * The capture buffer is traced for the first time here when the buffer is first mapped.
	 * Subsequently, the capture buffer will be traced when VIDIOC_DQBUF is called.
	 */
	if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		trace_mem(fildes);

	return buf_address_pointer;
}

int munmap(void *start, size_t length)
{
	errno = 0;

	int(*original_munmap)(void *start, size_t length);
	original_munmap = (int(*)(void *, size_t)) dlsym(RTLD_NEXT, "munmap");
	int ret = (*original_munmap)(start, length);

	/* Only trace the unmapping if the original mapping was traced. */
	if (!buffer_is_mapped((long int) start))
		return ret;

	json_object *munmap_obj = json_object_new_object();
	json_object_object_add(munmap_obj, "syscall_str", json_object_new_string("munmap"));
	json_object_object_add(munmap_obj, "syscall", json_object_new_int(LIBTRACER_SYSCALL_MUNMAP));

	if (errno) {
		json_object_object_add(munmap_obj, "errno", json_object_new_int(errno));
		json_object_object_add(munmap_obj, "errno_str", json_object_new_string(strerror(errno)));
	}

	json_object *munmap_args = json_object_new_object();
	json_object_object_add(munmap_args, "start", json_object_new_int64((int64_t)start));
	json_object_object_add(munmap_args, "length", json_object_new_uint64(length));
	json_object_object_add(munmap_obj, "munmap_args", munmap_args);

	write_json_object_to_json_file(munmap_obj);
	json_object_put(munmap_obj);

	return ret;
}

int ioctl(int fd, unsigned long int request, ...)
{
	errno = 0;

	va_list ap;
	va_start(ap, request);
	void *arg = va_arg(ap, void *);
	va_end(ap);

	int (*original_ioctl)(int fd, unsigned long int request, ...);
	original_ioctl = (int (*)(int, long unsigned int, ...)) dlsym(RTLD_NEXT, "ioctl");

	/* If the ioctl is queuing an output buffer, trace the output buffer before tracing the ioctl. */
	if ((request == VIDIOC_QBUF) && (_IOC_TYPE(request) == 'V')) {
		struct v4l2_buffer *buf = static_cast<struct v4l2_buffer*>(arg);
		if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
			int buf_fd = get_buffer_fd_trace(buf->type, buf->index);
			if (buf_fd) {
				set_buffer_bytesused_trace(buf_fd, buf->m.planes[0].bytesused);
				trace_mem(buf_fd);
			}
		}
	}

	json_object *ioctl_obj = json_object_new_object();
	json_object_object_add(ioctl_obj, "syscall_str", json_object_new_string("ioctl"));
	json_object_object_add(ioctl_obj, "syscall", json_object_new_int(LIBTRACER_SYSCALL_IOCTL));
	json_object_object_add(ioctl_obj, "fd", json_object_new_int(fd));
	json_object_object_add(ioctl_obj, "request", json_object_new_uint64(request));
	json_object_object_add(ioctl_obj, "request_str", json_object_new_string(get_ioctl_request_str(request).c_str()));

	/* Trace the ioctl arguments provided by userspace if relevant. */
	json_object *ioctl_args_userspace = trace_ioctl_args(fd, request, arg);
	if (json_object_object_length(ioctl_args_userspace))
		json_object_object_add(ioctl_obj, "ioctl_args_from_userspace", ioctl_args_userspace);

	/* Make the original ioctl call. */
	int ret = (*original_ioctl)(fd, request, arg);

	if (errno) {
		json_object_object_add(ioctl_obj, "errno", json_object_new_int(errno));
		json_object_object_add(ioctl_obj, "errno_str", json_object_new_string(strerror(errno)));
	}

	/* Also trace the ioctl arguments as modified by the driver if relevant. */
	json_object *ioctl_args_driver = trace_ioctl_args(fd, request, arg, false);
	if (json_object_object_length(ioctl_args_driver))
		json_object_object_add(ioctl_obj, "ioctl_args_from_driver", ioctl_args_driver);

	write_json_object_to_json_file(ioctl_obj);
	json_object_put(ioctl_obj);

	/* If the ioctl is exporting a buffer, store the buffer file descriptor and index for future access. */
	if ((request == VIDIOC_EXPBUF) && (_IOC_TYPE(request) == 'V')) {
		struct v4l2_exportbuffer *export_buffer = static_cast<struct v4l2_exportbuffer*>(arg);
		add_buffer_trace(export_buffer->fd, export_buffer->type, export_buffer->index);
	}

	/* If the ioctl is dequeuing a capture buffer, trace the buffer. */
	if ((request == VIDIOC_DQBUF) && (_IOC_TYPE(request) == 'V')) {
		struct v4l2_buffer *buf = static_cast<struct v4l2_buffer*>(arg);
		if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
			int buf_fd = get_buffer_fd_trace(buf->type, buf->index);
			if (buf_fd) {
				/* TODO tracer only works for decoded formats with one plane e.g. V4L2_PIX_FMT_NV12 */
				set_buffer_bytesused_trace(buf_fd, buf->m.planes[0].bytesused);
				trace_mem(buf_fd);
			}
		}
	}

	return ret;
}

int close(int fd)
{
	std::string path = get_device(fd);

	/* Only trace the close if a corresponding open was also traced. */
	if (!path.empty()) {
		json_object *close_obj = json_object_new_object();
		json_object_object_add(close_obj, "syscall_str", json_object_new_string("close"));
		json_object_object_add(close_obj, "syscall", json_object_new_int(LIBTRACER_SYSCALL_CLOSE));
		json_object_object_add(close_obj, "fd", json_object_new_int(fd));
		json_object_object_add(close_obj, "path", json_object_new_string(path.c_str()));
		write_json_object_to_json_file(close_obj);
		json_object_put(close_obj);
		remove_device(fd);
	}

	int (*original_close)(int fd);
	original_close = (int (*)(int)) dlsym(RTLD_NEXT, "close");

	return (*original_close)(fd);
}
