/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef V4L2_TRACER_H
#define V4L2_TRACER_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/wait.h>
#include <time.h>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <poll.h>
#include <unordered_map>
#include <stdexcept>
#include <list>
#include <json.h>
#include <v4l2-info.h>
#include "trace-info.h"
#include "retrace-vp8.h"
#include "retrace-h264.h"
#include "retrace-fwht.h"
#include "retrace-helper.h"

void print_help_retracer(void);
int retracer(int argc, char *argv[]);

#endif
