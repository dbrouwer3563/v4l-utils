/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <iostream> //hex     
#include <sstream> //   ?
#include "v4l2-tracer-common.h"


/* If the value is 0, return an empty string, otherwise return a hex string. */
std::string val2s_hex(long val)
{
	if (!val)
		return "";

	std::stringstream ss;
	ss << std::hex << val;
	return "0x" + ss.str();
}

/*
 * If the value has a corresponding string in def, return the string.
 * Otherwise, if the value is 0, return an empty string, or
 * if the value is not zero return a hex string.
 * Same as flags2s except only one match is possible.
 */
std::string val2s(long val, const val_def *def)
{
	while ((def->val != -1) && (def->val != val))
		def++;

	if (def->val == val)
		return def->str;

	if (val)
		return val2s_hex(val);

	return "";
}

/* If the ioctl cmd is not in videodev2.h or media.h, return an empty string. */
std::string ioctl2s(unsigned long cmd)
{
	std::string s;

	s = val2s((long) cmd, ioctl_video_val_def);
	if (s.empty() || s.find("0x") != std::string::npos)
		s = val2s((long) cmd, ioctl_media_val_def);

	/* If the ioctl is unknown, return an empty string instead of a hex value. */
	if (s.find("0x") != std::string::npos)
		s = "";

	return s;
}
