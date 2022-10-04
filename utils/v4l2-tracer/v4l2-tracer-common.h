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

enum LIBTRACER_SYSCALL {
	LIBTRACER_SYSCALL_IOCTL,
	LIBTRACER_SYSCALL_OPEN,
	LIBTRACER_SYSCALL_OPEN64,
	LIBTRACER_SYSCALL_CLOSE,
	LIBTRACER_SYSCALL_MMAP,
	LIBTRACER_SYSCALL_MMAP64,
	LIBTRACER_SYSCALL_MUNMAP,
};

#endif
