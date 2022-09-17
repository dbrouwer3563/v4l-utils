/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef LIBTRACER_H
#define LIBTRACER_H

#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h> /* for pow */
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <dirent.h>
#include <config.h>
#include <pthread.h>
#include <unistd.h> /* for close */
#include <linux/dma-buf.h>
#include <json-c/json.h>
#include <unordered_map>
#include <algorithm> /* for std::find */
#include <list>
#include <v4l2-info.h>
#include "trace.h"
#include "trace-info.h"
#include "trace-helper.h"
#include "trace-vp8.h"
#include "trace-h264.h"
#include "trace-fwht.h"

#endif
