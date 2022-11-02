/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef V4L2_TRACER_COMMON_H
#define V4L2_TRACER_COMMON_H


#include <poll.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
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
#include "media-info.h"
#include "config.h"

#define STR(x) #x
#define STRING(x) STR(x)

struct val_def {
	long val;
	const char *str;
};

extern std::string path_media_global;
extern std::string path_video_global;

bool is_verbose(void);
bool is_debug(void);

void print_v4l2_tracer_info(void);
void print_usage(void);

std::string number2s_oct(long num);
std::string number2s(long num);
std::string val2s(long val, const val_def *def);
std::string ioctl2s(unsigned long cmd);
std::string fl2s(unsigned val, const flag_def *def);
std::string fl2s_buffer(__u32 flags);
std::string fl2s_fwht(__u32 flags);

long s2number(std::string s);
long s2val(std::string s, const val_def *def);
long s2ioctl(std::string s);
unsigned long s2flags(std::string s, const flag_def *def);
unsigned long s2flags_buffer(std::string s);
unsigned long s2flags_fwht(std::string s);

std::string which2s(unsigned long which);

std::string get_path_media_from_path_video(std::string path_video_arg);
std::string get_path_video_from_fd_media(int fd);
std::list<std::string> get_linked_entities(int media_fd, std::string path_video);

#include "v4l2-tracer-info-gen.h"

constexpr val_def which_val_def[] = {
	{ V4L2_CTRL_WHICH_CUR_VAL,	"V4L2_CTRL_WHICH_CUR_VAL" },
	{ V4L2_CTRL_WHICH_DEF_VAL,	"V4L2_CTRL_WHICH_DEF_VAL" },
	{ V4L2_CTRL_WHICH_REQUEST_VAL,	"V4L2_CTRL_WHICH_REQUEST_VAL" },
	{ -1, "" }
};

//to do: add more flags and convert from val to flag
constexpr val_def open_val_def[] = {
	{ O_RDONLY,	"O_RDONLY" },
	{ O_WRONLY,	"O_WRONLY" },
	{ O_RDWR,	"O_RDWR" },
	{ -1, "" }
};

#endif
