/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <sstream>
#include <iostream>
#include "v4l2-tracer-common.h"

/* Convert a long val to a hex string. If val is 0, return an empty string. */
std::string val2s_hex(long val)
{
	if (!val)
		return "";

	std::stringstream ss;
	ss << std::hex << val;
	return "0x" + ss.str();
}

/*
 * Convert a long val to its corresponding string in def.
 * If there is no match, and the value is 0, return an empty string, otherwise return a hex string.
 * Unlike flags2s() the val can be zero and only one match is expected.
 */
std::string val2s(long val, const val_def *def)
{
	std::string s;

	while ((def->val != -1) && (def->val != val))
		def++;

	if (def->val == val)
		return def->str;

	return val2s_hex(val);
}

/*
 * Convert a hex string to a long. If the string is empty return 0.
 * Returns 0xBAD00000 if string is non-numeric.
 */
long s2val_hex(std::string s)
{
	if (s.empty())
		return 0;

	long val;
	try {
		/* Since base is autodetected, it will also convert to octal/decimal. */
		val = std::stoul(s, nullptr, 0);
	} catch (std::invalid_argument& ia) {
		val = 0xBAD00000;
	} catch (std::out_of_range& oor) {
		val = 0xBAD00000;
	}

	return val;
}

/*
 * Convert a string to its corresponding long val in def.
 * If there is no match, and the string is empty return 0, otherwise convert
 * numeric strings to a long. Returns 0xBAD00000 if string is non-numeric.
 */
long s2val(std::string s, const val_def *def)
{
	if (s.empty())
		return 0;

	while ((def->val != -1) && (def->str != s))
		def++;

	if (def->str == s)
		return def->val;

	return s2val_hex(s);
}

/*
 * If the ioctl cmd is in videodev2.h or media.h, convert it to a string.
 * Otherwise, return an empty string.
 */
std::string ioctl2s(unsigned long cmd)
{
	std::string s = val2s((long) cmd, ioctl_video_val_def);
	if (s.empty() || s.find("0x") != std::string::npos)
		s = val2s((long) cmd, ioctl_media_val_def);
	if (s.find("0x") != std::string::npos)
		s = "";
	return s;
}

/*
 * If the string matches an ioctl cmd is in videodev2.h or media.h, return the long cmd.
 * If there is no match and the string is empty return 0, otherwise return 0xBAD00000.
 */
long s2ioctl(std::string s)
{
	long val = s2val(s, ioctl_video_val_def);
	if (val == 0xBAD00000)
		val = s2val(s, ioctl_media_val_def);

	return val;
}

/*
 * Take a comma-separated string of flags and convert into the sum of its corresponding
 * values in def. If a flag is unknown but numeric, add its hex value.
 * Reverse of flag2s.
 */
unsigned long s2flags(std::string s, const flag_def *def)
{
	size_t idx;
	unsigned long flags = 0;

	while (def->flag) {

		idx = s.find(def->str);

		if (idx == std::string::npos) {
			def++;
			continue;
		}

		/* Special treatment for flag values that have an offset */
		std::string def_str = def->str;
		if (def_str == "V4L2_FWHT_FL_COMPONENTS_NUM_MSK") {
			flags += (def->flag & (2 << V4L2_FWHT_FL_COMPONENTS_NUM_OFFSET));
		} else if (def_str == "V4L2_FWHT_FL_PIXENC_MSK") {
			flags += (def->flag & (1 << V4L2_FWHT_FL_PIXENC_OFFSET));
		} else {
			flags += def->flag;
		}

		s.erase(idx, strlen(def->str));
		idx = s.find(", ");
		if (idx != std::string::npos)
			s.erase(idx, 2);

		def++;
	}

	if (s.empty())
		return flags;

	std::stringstream ss(s);
	std::string unknown_flag;

	while (ss >> unknown_flag) {
		idx = unknown_flag.find(",");
		if (idx != std::string::npos)
			unknown_flag.erase(idx, 1);

		if ((unknown_flag == "V4L2_BUF_FLAG_TSTAMP_SRC_EOF") || (unknown_flag == "V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN"))
			continue;

		if (unknown_flag == "V4L2_BUF_FLAG_TIMESTAMP_COPY") {
			flags += V4L2_BUF_FLAG_TIMESTAMP_COPY;
		} else if (unknown_flag == "V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC") {
			flags += V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		} else if (unknown_flag == "V4L2_BUF_FLAG_TSTAMP_SRC_SOE") {
			flags += V4L2_BUF_FLAG_TSTAMP_SRC_SOE;
		} else {
			try {
				flags += std::stoul(unknown_flag, nullptr, 0);
			} catch (std::invalid_argument& ia) {
				flags += 0xBAD00000;
			} catch (std::out_of_range& oor) {
				flags += 0xBAD00000;
			}
		}
	}
	return flags;
}
