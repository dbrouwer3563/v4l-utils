/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef TRACE_H
#define TRACE_H

void trace_open(int fd, const char *path, int oflag, mode_t mode, bool is_open64);
void trace_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off, unsigned long buf_address, bool is_mmap64);
void trace_mem(int fd, __u32 offset, __u32 type, int index, __u32 bytesused, unsigned long start);
void trace_mem_encoded(int fd, __u32 offset);

std::string get_ioctl_request_str(unsigned long request);
json_object *trace_ioctl_args(int fd, unsigned long request, void *arg, bool from_userspace = true);

#endif
