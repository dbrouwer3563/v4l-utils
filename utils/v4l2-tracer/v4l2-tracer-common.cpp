/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <iostream> //hex     
#include <sstream> //   ?
#include "v4l2-tracer-common.h"

std::string val2s(unsigned long val, const val_def *def)
{
	if (val == 0)
		return "";

	while ((def->val) && (def->val != val))
		def++;

	if (def->val == val)
		return def->str;

	std::stringstream ss;
	ss << std::hex << val;
	return "0x" + ss.str();
}

std::string ioctl2s(unsigned long cmd)
{
	std::string s;
	__u8 ioctl_type = _IOC_TYPE(cmd);
	switch (ioctl_type) {
		case 'V': {
			s = val2s(cmd, defs_ioctl_video);
			return s;
		}
		case '|': {
			s = val2s(cmd, defs_ioctl_media);
			return s;
		}
		default:
			break;
	}
	return s;
}