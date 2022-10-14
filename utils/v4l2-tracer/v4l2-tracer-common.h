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
	long val;
	const char *str;
};

std::string val2s_hex(long val);
/* Use val2s when there is one, unique value expected, otherwise use flags2s. */
std::string val2s(long val, const val_def *def);
std::string ioctl2s(unsigned long cmd);
std::string which2s(unsigned long which);


#include "v4l2-tracer-info-gen.h"

enum LIBV4L2TRACER_SYSCALL {
	LIBV4L2TRACER_SYSCALL_IOCTL,
	LIBV4L2TRACER_SYSCALL_OPEN,
	LIBV4L2TRACER_SYSCALL_OPEN64,
	LIBV4L2TRACER_SYSCALL_CLOSE,
	LIBV4L2TRACER_SYSCALL_MMAP,
	LIBV4L2TRACER_SYSCALL_MMAP64,
	LIBV4L2TRACER_SYSCALL_MUNMAP,
};

constexpr val_def libv4l2tracer_syscall_val_def[] = {
	{ LIBV4L2TRACER_SYSCALL_IOCTL,	"ioctl" },
	{ LIBV4L2TRACER_SYSCALL_OPEN,	"open" },
	{ LIBV4L2TRACER_SYSCALL_OPEN64,	"open64" },
	{ LIBV4L2TRACER_SYSCALL_CLOSE,	"close" },
	{ LIBV4L2TRACER_SYSCALL_MMAP,	"mmap" },
	{ LIBV4L2TRACER_SYSCALL_MMAP64,	"mmap64" },
	{ LIBV4L2TRACER_SYSCALL_MUNMAP,	"munmap" },
	{ -1, "" }
};

constexpr val_def which_val_def[] = {
	{ V4L2_CTRL_WHICH_CUR_VAL,	"V4L2_CTRL_WHICH_CUR_VAL" },
	{ V4L2_CTRL_WHICH_DEF_VAL,	"V4L2_CTRL_WHICH_DEF_VAL" },
	{ V4L2_CTRL_WHICH_REQUEST_VAL,	"V4L2_CTRL_WHICH_REQUEST_VAL" },
	{ -1, "" }
};

/* Use with V4L2_BUF_FLAG_TIMESTAMP_MASK */
constexpr val_def v4l2_buf_timestamp_val_def[] = {
	{ V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC, "V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC" },
	{ V4L2_BUF_FLAG_TIMESTAMP_COPY, "V4L2_BUF_FLAG_TIMESTAMP_COPY" },
	{ V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN, "V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN" },
	{ -1, "" }
};

/* Use with V4L2_BUF_FLAG_TSTAMP_SRC_MASK */
constexpr val_def v4l2_buf_tstamp_val_def[] = {
	{ V4L2_BUF_FLAG_TSTAMP_SRC_SOE, "V4L2_BUF_FLAG_TSTAMP_SRC_SOE" },
	{ V4L2_BUF_FLAG_TSTAMP_SRC_EOF, "V4L2_BUF_FLAG_TSTAMP_SRC_EOF" },
	{ -1, "" }
};

#endif
