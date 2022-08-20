/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef TRACE_INFO_H
#define TRACE_INFO_H

#include <cstring>
#include <linux/videodev2.h>
#include <linux/media.h>
#include <linux/dma-buf.h>

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

enum LIBTRACER_SYSCALL {
	LIBTRACER_SYSCALL_IOCTL,
	LIBTRACER_SYSCALL_OPEN,
	LIBTRACER_SYSCALL_OPEN64,
	LIBTRACER_SYSCALL_CLOSE,
	LIBTRACER_SYSCALL_MMAP64,
	LIBTRACER_SYSCALL_MUNMAP,
	LIBTRACER_SYSCALL_SELECT,
	LIBTRACER_SYSCALL_POLL,
};

/* Convert an ioctl request of type 'V' to string. */
std::string ioctl2s_video(unsigned long request);

/* Convert an ioctl request of type '|' to string. */
std::string ioctl2s_media(unsigned long request);

/* Convert capability flags to common separated string. */
std::string capflag2s(unsigned long cap);

/* Convert a decimal number to a string with kernel_version.major_revision.minor_revision. */
std::string ver2s(unsigned int version);

/* Convert v4l2_ext_controls member "which" to string. */
std::string which2s(unsigned long which);

/* Convert control class to string. */
std::string ctrlclass2s(__u32 id);

/* Convert control type to string. */
std::string ctrltype2s(__u32 type);

/* Convert struct dma_buf_sync flags to string. */
std::string bufsyncflag2s(unsigned long flag);

/* Convert v4l2_vp8_segment flags to string. */
std::string vp8_segment_flag2s(unsigned long flag);

/* Convert v4l2_vp8_loop_filter flags to string. */
std::string vp8_loop_filter_flag2s(unsigned long flag);

/* Convert v4l2_ctrl_vp8_frame flags to string. */
std::string vp8_frame_flag2s(unsigned long flag);

/* Convert v4l2_requestbuffers flags to string. */
std::string request_buffers_flag2s(unsigned int flag);

/* Convert enum v4l2_memory type to string. */
std::string v4l2_memory2s(__u32 id);

/* Convert v4l2_timecode type to string. */
std::string tc_type2s(__u32 id);

/* Convert v4l2_timecode flags to string. */
std::string tc_flag2s(unsigned int flag);

#endif
