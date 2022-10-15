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
#include <vector>
#include <unordered_map>
#include <algorithm> /* for std::find */
#include <linux/dma-buf.h>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <json.h>
#include "codec-fwht.h"
#include "v4l2-info.h"

#define STR(x) #x
#define STRING(x) STR(x)

struct val_def {
	long val;
	const char *str;
};

std::string val2s_hex(long val);
long s2val_hex(std::string s);
std::string val2s(long val, const val_def *def);
long s2val(std::string s, const val_def *def);

unsigned long s2flags(std::string s, const flag_def *def);

std::string ioctl2s(unsigned long cmd);
long s2ioctl(std::string s);

std::string which2s(unsigned long which);


#include "v4l2-tracer-info-gen.h"

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
