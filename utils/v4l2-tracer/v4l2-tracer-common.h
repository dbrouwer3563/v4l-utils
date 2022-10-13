/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef V4L2_TRACER_COMMON_H
#define V4L2_TRACER_COMMON_H

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <list>
#include <unordered_map>
#include <algorithm> /* for std::find */
#include <linux/dma-buf.h>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <json.h>
#include "codec-fwht.h"
#include "v4l2-info.h"

struct val_def {
	unsigned long val;
	const char *str;
};

/* Use val2s when there is one, unique value expected, otherwise use flags2s. */
std::string val2s(unsigned long val, const val_def *def);
std::string ioctl2s(unsigned long cmd);

#include "v4l2-tracer-info-gen.h"

enum LIBV4L2TRACER_SYSCALL {
	LIBV4L2TRACER_SYSCALL_IOCTL = 1,
	LIBV4L2TRACER_SYSCALL_OPEN,
	LIBV4L2TRACER_SYSCALL_OPEN64,
	LIBV4L2TRACER_SYSCALL_CLOSE,
	LIBV4L2TRACER_SYSCALL_MMAP,
	LIBV4L2TRACER_SYSCALL_MMAP64,
	LIBV4L2TRACER_SYSCALL_MUNMAP,
};

constexpr val_def defs_libv4l2tracer_syscall[] = {
	{ LIBV4L2TRACER_SYSCALL_IOCTL,	"ioctl" },
	{ LIBV4L2TRACER_SYSCALL_OPEN,	"open" },
	{ LIBV4L2TRACER_SYSCALL_OPEN64,	"open64" },
	{ LIBV4L2TRACER_SYSCALL_CLOSE,	"close" },
	{ LIBV4L2TRACER_SYSCALL_MMAP,	"mmap" },
	{ LIBV4L2TRACER_SYSCALL_MMAP64,	"mmap64" },
	{ LIBV4L2TRACER_SYSCALL_MUNMAP,	"munmap" },
	{ 0, "" }
};

#endif
