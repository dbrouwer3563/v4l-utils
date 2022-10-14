/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <dlfcn.h>
#include <stdarg.h>
#include <config.h> /* For PROMOTED_MODE_T */
#include "v4l2-tracer-common.h"
#include "trace-helper.h"

/* from trace.cpp */
void trace_open(int fd, const char *path, int oflag, mode_t mode, bool is_open64);
void trace_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off,
                unsigned long buf_address, bool is_mmap64);
void trace_mem(int fd, __u32 offset, __u32 type, int index, __u32 bytesused, unsigned long start);
void trace_mem_encoded(int fd, __u32 offset);
json_object *trace_ioctl_args(int fd, unsigned long cmd, void *arg,
                              bool from_userspace = true);

int open(const char *path, int oflag, ...)
{
	errno = 0;
	mode_t mode = 0;

	if (oflag & O_CREAT) {
		va_list ap;
		va_start(ap, oflag);
		mode = va_arg(ap, PROMOTED_MODE_T);
		va_end(ap);
	}

	int (*original_open)(const char *path, int oflag, ...);
	original_open = (int (*)(const char*, int, ...)) dlsym(RTLD_NEXT, "open");
	int fd = (*original_open)(path, oflag, mode);

	if (is_video_or_media_device(path)) {
		if (!options_are_set())
			set_options();
		add_device(fd, path);
		trace_open(fd, path, oflag, mode, false);
	}

	return fd;
}

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

	if (is_video_or_media_device(path)) {
		if (!options_are_set())
			set_options();
		add_device(fd, path);
		trace_open(fd, path, oflag, mode, true);
	}

	return fd;
}

int close(int fd)
{
	errno = 0;
	std::string path = get_device(fd);

	/* Only trace the close if a corresponding open was also traced. */
	if (!path.empty()) {
		json_object *close_obj = json_object_new_object();
		json_object_object_add(close_obj, "syscall", json_object_new_string(val2s(LIBV4L2TRACER_SYSCALL_CLOSE, libv4l2tracer_syscall_val_def).c_str()));
		json_object_object_add(close_obj, "fd", json_object_new_int(fd));
		json_object_object_add(close_obj, "path", json_object_new_string(path.c_str()));
		write_json_object_to_json_file(close_obj);
		json_object_put(close_obj);
		remove_device(fd);

		/* If we removed the last device, close the json trace file. */
		if (!count_devices())
			close_json_file();
	}

	int (*original_close)(int fd);
	original_close = (int (*)(int)) dlsym(RTLD_NEXT, "close");

	return (*original_close)(fd);
}

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	errno = 0;
	void *(*original_mmap)(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
	original_mmap = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
	void *buf_address_pointer = (*original_mmap)(addr, len, prot, flags, fildes, off);

	set_buffer_address_trace(fildes, off, (unsigned long) buf_address_pointer);

	if (buffer_in_trace_context(fildes, off))
		trace_mmap(addr, len, prot, flags, fildes, off, (unsigned long) buf_address_pointer, false);

	return buf_address_pointer;
}

void *mmap64(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	errno = 0;
	void *(*original_mmap64)(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
	original_mmap64 = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap64");
	void *buf_address_pointer = (*original_mmap64)(addr, len, prot, flags, fildes, off);

	set_buffer_address_trace(fildes, off, (unsigned long) buf_address_pointer);

	if (buffer_in_trace_context(fildes, off))
		trace_mmap(addr, len, prot, flags, fildes, off, (unsigned long) buf_address_pointer, true);

	return buf_address_pointer;
}

int munmap(void *start, size_t length)
{
	errno = 0;
	int(*original_munmap)(void *start, size_t length);
	original_munmap = (int(*)(void *, size_t)) dlsym(RTLD_NEXT, "munmap");
	int ret = (*original_munmap)(start, length);

	/* Only trace the unmapping if the original mapping was traced. */
	if (!buffer_is_mapped((unsigned long) start))
		return ret;

	json_object *munmap_obj = json_object_new_object();
	json_object_object_add(munmap_obj, "syscall", json_object_new_string(val2s(LIBV4L2TRACER_SYSCALL_MUNMAP, libv4l2tracer_syscall_val_def).c_str()));

	if (errno)
		json_object_object_add(munmap_obj, "errno", json_object_new_string(strerrorname_np(errno)));

	json_object *munmap_args = json_object_new_object();
	json_object_object_add(munmap_args, "start", json_object_new_int64((int64_t)start));
	json_object_object_add(munmap_args, "length", json_object_new_uint64(length));
	json_object_object_add(munmap_obj, "munmap_args", munmap_args);

	write_json_object_to_json_file(munmap_obj);
	json_object_put(munmap_obj);

	return ret;
}

int ioctl(int fd, unsigned long cmd, ...)
{
	errno = 0;
	va_list ap;
	va_start(ap, cmd);
	void *arg = va_arg(ap, void *);
	va_end(ap);

	int (*original_ioctl)(int fd, unsigned long cmd, ...);
	original_ioctl = (int (*)(int, long unsigned int, ...)) dlsym(RTLD_NEXT, "ioctl");

	std::string ioctl_str = ioctl2s(cmd);
	/* Don't trace ioctls that are not in videodev2.h or media.h */
	if (ioctl_str.empty())
		return (*original_ioctl)(fd, cmd, arg);

	if (cmd == VIDIOC_S_EXT_CTRLS)
		s_ext_ctrls_setup(static_cast<struct v4l2_ext_controls*>(arg));

	if (cmd == VIDIOC_QBUF)
		qbuf_setup(static_cast<struct v4l2_buffer*>(arg));

	if (cmd == VIDIOC_STREAMOFF)
		streamoff_cleanup(*(static_cast<v4l2_buf_type*>(arg)));

	json_object *ioctl_obj = json_object_new_object();
	json_object_object_add(ioctl_obj, "syscall", json_object_new_string(val2s(LIBV4L2TRACER_SYSCALL_IOCTL, libv4l2tracer_syscall_val_def).c_str()));
	json_object_object_add(ioctl_obj, "fd", json_object_new_int(fd));
	json_object_object_add(ioctl_obj, "cmd", json_object_new_string(ioctl_str.c_str()));


	/* Trace the ioctl arguments provided by userspace. */
	json_object *ioctl_args_userspace = trace_ioctl_args(fd, cmd, arg);
	if (json_object_object_length(ioctl_args_userspace))
		json_object_object_add(ioctl_obj, "from_userspace", ioctl_args_userspace);

	/* Make the original ioctl call. */
	int ret = (*original_ioctl)(fd, cmd, arg);

	if (errno)
		json_object_object_add(ioctl_obj, "errno", json_object_new_string(strerrorname_np(errno)));

	/* Also trace the ioctl arguments as modified by the driver. */
	json_object *ioctl_args_driver = trace_ioctl_args(fd, cmd, arg, false);
	if (json_object_object_length(ioctl_args_driver))
		json_object_object_add(ioctl_obj, "from_driver", ioctl_args_driver);

	write_json_object_to_json_file(ioctl_obj);
	json_object_put(ioctl_obj);

	if (cmd == VIDIOC_G_FMT)
		g_fmt_setup_trace(static_cast<struct v4l2_format*>(arg));

	if (cmd == VIDIOC_S_FMT)
		s_fmt_setup(static_cast<struct v4l2_format*>(arg));

	if (cmd == VIDIOC_EXPBUF)
		expbuf_setup(static_cast<struct v4l2_exportbuffer*>(arg));

	if (cmd == VIDIOC_QUERYBUF)
		querybuf_setup(fd, static_cast<struct v4l2_buffer*>(arg));

	return ret;
}
