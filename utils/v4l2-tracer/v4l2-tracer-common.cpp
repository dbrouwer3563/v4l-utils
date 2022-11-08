/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <sstream>
#include <iostream>
#include <iomanip>
#include "v4l2-tracer-common.h"

bool is_verbose()
{
	return getenv("V4L2_TRACER_OPTION_VERBOSE");
}

bool is_debug()
{
	return getenv("V4L2_TRACER_OPTION_DEBUG");
	return false;
}

void print_v4l2_tracer_info(void)
{
	fprintf(stderr, "v4l2-tracer %s%s\n", PACKAGE_VERSION, STRING(GIT_COMMIT_CNT));
	if (strlen(STRING(GIT_SHA)))
		fprintf(stderr, "v4l2-tracer SHA: '%s' %s\n", STRING(GIT_SHA), STRING(GIT_COMMIT_DATE));
}

void print_usage(void)
{
	print_v4l2_tracer_info();
	fprintf(stderr, "Usage:\n\tv4l2-tracer [options] trace <tracee>\n"
	        "\tv4l2-tracer [options] retrace <trace_file>.json\n\n"

	        "\tCommon options:\n"
	        "\t\t-e, --prettymem   Add whitespace in JSON file just to the memory arrays.\n"
	        "\t\t-g, --debug       Turn on verbose reporting plus additional info for debugging.\n"
	        "\t\t-h, --help        Display this message.\n"
	        "\t\t-i, --info        Include ioctls that are informational only.\n"
	        "\t\t-p, --pretty      Add whitespace in JSON file to improve readability.\n"
	        "\t\t-r  --raw         Write decoded video frame data to JSON file.\n"
	        "\t\t-v, --verbose     Turn on verbose reporting.\n"
	        "\t\t-y, --yuv         Write decoded video frame data to yuv file.\n\n"

	        "\tRetrace options:\n"
	        "\t\t-d, --video_device <dev>   Retrace with a specific video device.\n"
	        "\t\t                           <dev> must be a digit corresponding to\n"
	        "\t\t                           /dev/video<dev> \n\n"
	        "\t\t-m, --media_device <dev>   Retrace with a specific media device.\n"
	        "\t\t                           <dev> must be a digit corresponding to\n"
	        "\t\t                           /dev/media<dev> \n\n");
}

std::string ver2s(unsigned int version)
{
	char buf[16];
	sprintf(buf, "%d.%d.%d", version >> 16, (version >> 8) & 0xff, version & 0xff);
	return buf;
}

/* Convert a long val to an octal string. If num is 0, return an empty string. */
std::string number2s_oct(long num)
{
	std::stringstream ss;
	ss << std::setfill ('0') << std::setw(5) << std::oct << num;
	return ss.str();
}

/* Convert a long val to a hex string. If num is 0, return an empty string. */
std::string number2s(long num)
{
	if (!num)
		return "";

	std::stringstream ss;
	ss << std::hex << num;
	return "0x" + ss.str();
}

/*
 * Convert a long val to its corresponding string in def. If there is no match,
 * and the value is 0, return an empty string, otherwise return a hex string.
 */
std::string val2s(long val, const val_def *def)
{
	std::string s;
	if (def == nullptr)
		return number2s(val);

	while ((def->val != -1) && (def->val != val))
		def++;

	if (def->val == val)
		return def->str;

	return number2s(val);
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

	/* Undo any conversion of unknown cmd to hex. */
	if (s.find("0x") != std::string::npos)
		s = "";

	return s;
}

/* Return a string of flags separated by '|' or hex value if unknown. */
std::string fl2s(unsigned val, const flag_def *def)
{
	std::string s;
	if (def == nullptr)
		return number2s(val);

	while (def->flag) {
		if (val & def->flag) {
			if (!s.empty())
				s += "|";
			s += def->str;
			val &= ~def->flag;
		}
		def++;
	}
	if (val) {
		if (!s.empty())
			s += "|";
		s += number2s(val);
	}
	return s;
}

std::string fl2s_buffer(__u32 flags)
{
	const unsigned ts_mask = V4L2_BUF_FLAG_TIMESTAMP_MASK | V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	std::string s = fl2s(flags & ~ts_mask, v4l2_buf_flag_def);

	if (!s.empty())
		s += "|";

	switch (flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) {
	case V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN:
		s += "V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN";
		break;
	case V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC:
		s += "V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC";
		break;
	case V4L2_BUF_FLAG_TIMESTAMP_COPY:
		s += "V4L2_BUF_FLAG_TIMESTAMP_COPY";
		break;
	default:
		s += "ts-invalid";
		break;
	}

	if (!s.empty())
		s += "|";

	switch (flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK) {
	case V4L2_BUF_FLAG_TSTAMP_SRC_EOF:
		s += "V4L2_BUF_FLAG_TSTAMP_SRC_EOF";
		break;
	case V4L2_BUF_FLAG_TSTAMP_SRC_SOE:
		s += "V4L2_BUF_FLAG_TSTAMP_SRC_SOE";
		break;
	default:
		s += "ts-src-invalid";
		break;
	}
	return s;
}

std::string fl2s_fwht(__u32 flags)
{
	const unsigned pixenc_mask = V4L2_FWHT_FL_COMPONENTS_NUM_MSK | V4L2_FWHT_FL_PIXENC_MSK;
	std::string s = fl2s(flags & ~pixenc_mask, v4l2_ctrl_fwht_params_flag_def);
	if (!s.empty())
		s += "|";

	switch (flags & V4L2_FWHT_FL_PIXENC_MSK) {
	case V4L2_FWHT_FL_PIXENC_YUV:
		s += "V4L2_FWHT_FL_PIXENC_YUV";
		break;
	case V4L2_FWHT_FL_PIXENC_RGB:
		s += "V4L2_FWHT_FL_PIXENC_RGB";
		break;
	case V4L2_FWHT_FL_PIXENC_HSV:
		s += "V4L2_FWHT_FL_PIXENC_HSV";
		break;
	default:
		s += "pixenc-invalid";
		break;
	}

	if (!s.empty())
		s += "|";
	s += number2s(flags & V4L2_FWHT_FL_COMPONENTS_NUM_MSK);

	return s;
}

/*
 * Convert a numeric string (hex, dec or oct) to a long. If the string is empty return 0.
 * Returns 0xBAD00000 if string is non-numeric.
 */
long s2number(std::string s)
{
	long num = 0;
	if (s.empty())
		return num;
	try {
		num = std::stoul(s, nullptr, 0); /* base is auto-detected */
	} catch (std::invalid_argument& ia) {
		num = 0xBAD00000;
	} catch (std::out_of_range& oor) {
		num = 0xBAD00000;
	}
	return num;
}

/*
 * Convert a string to its corresponding long val in def.
 * If there is no match, and the string is empty return 0, otherwise try to convert
 * the unknown string to hex and return 0xBAD00000 if the string is non-numeric.
 */
long s2val(std::string s, const val_def *def)
{
	if (s.empty())
		return 0;

	if (def == nullptr)
		return s2number(s);

	while ((def->val != -1) && (def->str != s))
		def++;

	if (def->str == s)
		return def->val;

	return s2number(s);
}

/*
 * If the string matches an ioctl cmd in videodev2.h or media.h, return the long cmd.
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
 * Take a string of flags separated '|' and convert into the sum of its corresponding values in
 * def. If a flag is unknown but numeric, add its hex value.
 * If a flag is unknown and not numeric, add 0xBAD00000 and return;
 * Reverse of fl2s.
 */
unsigned long s2flags(std::string s, const flag_def *def)
{
	size_t idx;
	unsigned long flags = 0;

	if (def == nullptr)
		return s2number(s);

	while (def->flag) {
		idx = s.find(def->str);
		if (idx == std::string::npos) {
			def++;
			continue;
		}
		flags += def->flag;

		s.erase(idx, strlen(def->str));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);

		def++;
	}

	if (s.empty())
		return flags;

	std::stringstream ss(s);
	std::string unknown_flag;

	while (ss >> unknown_flag) {
		idx = unknown_flag.find("|");
		if (idx != std::string::npos)
			unknown_flag.erase(idx, 1);

		try {
			flags += std::stoul(unknown_flag, nullptr, 0);
		} catch (std::invalid_argument& ia) {
			flags += 0xBAD00000;
			break;
		} catch (std::out_of_range& oor) {
			flags += 0xBAD00000;
			break;
		}
	}
	return flags;
}

/* reverse of buffer_fl2s */
unsigned long s2flags_buffer(std::string s)
{
	size_t idx;
	unsigned long flags = 0;

	idx = s.find("V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN");
	if (idx != std::string::npos) {
		s.erase(idx, strlen("V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	idx = s.find("V4L2_BUF_FLAG_TSTAMP_SRC_EOF");
	if (idx != std::string::npos) {
		s.erase(idx, strlen("V4L2_BUF_FLAG_TSTAMP_SRC_EOF"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	idx = s.find("V4L2_BUF_FLAG_TIMESTAMP_COPY");
	if (idx != std::string::npos) {
		flags += V4L2_BUF_FLAG_TIMESTAMP_COPY;
		s.erase(idx, strlen("V4L2_BUF_FLAG_TIMESTAMP_COPY"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	} else {
		idx = s.find("V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC");
		if (idx != std::string::npos) {
			flags += V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
			s.erase(idx, strlen("V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC"));
			idx = s.find("|");
			if (idx != std::string::npos)
				s.erase(idx, 1);
		}
	}

	idx = s.find("V4L2_BUF_FLAG_TSTAMP_SRC_SOE");
	if (idx != std::string::npos) {
		flags += V4L2_BUF_FLAG_TSTAMP_SRC_SOE;
		s.erase(idx, strlen("V4L2_BUF_FLAG_TSTAMP_SRC_SOE"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	return s2flags(s, v4l2_buf_flag_def);
}

unsigned long s2flags_fwht(std::string s)
{
	size_t idx;
	unsigned long flags = 0;

	idx = s.find("V4L2_FWHT_FL_PIXENC_YUV");
	if (idx != std::string::npos) {
		flags += V4L2_FWHT_FL_PIXENC_YUV;
		s.erase(idx, strlen("V4L2_FWHT_FL_PIXENC_YUV"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	idx = s.find("V4L2_FWHT_FL_PIXENC_RGB");
	if (idx != std::string::npos) {
		flags += V4L2_FWHT_FL_PIXENC_RGB;
		s.erase(idx, strlen("V4L2_FWHT_FL_PIXENC_RGB"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	idx = s.find("V4L2_FWHT_FL_PIXENC_HSV");
	if (idx != std::string::npos) {
		flags += V4L2_FWHT_FL_PIXENC_HSV;
		s.erase(idx, strlen("V4L2_FWHT_FL_PIXENC_HSV"));
		idx = s.find("|");
		if (idx != std::string::npos)
			s.erase(idx, 1);
	}

	flags += s2flags(s, v4l2_ctrl_fwht_params_flag_def);

	return flags;
}

std::string get_path_media(std::string driver)
{
	std::string path_media;

	DIR *dp = opendir("/dev");
	if (dp == nullptr)
		return path_media;

	struct dirent *ep;
	while ((ep = readdir(dp))) {

		const char *name = ep->d_name;
		if (memcmp(name, "media", 5) || !isdigit(name[5]))
			continue;

		std::string media_devname = std::string("/dev/") + name;

		setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
		int media_fd = open(media_devname.c_str(), O_RDONLY);
		unsetenv("V4L2_TRACER_PAUSE_TRACE");

		if (media_fd < 0)
			continue;

		struct media_device_info info = {};
		if (ioctl(media_fd, MEDIA_IOC_DEVICE_INFO, &info) || info.driver != driver) {
			close(media_fd);
			continue;
		} else {
			path_media = media_devname;
			close(media_fd);
			break;
		}
	}
	closedir(dp);

	return path_media;
}

std::string get_path_video(int media_fd, std::list<std::string> linked_entities)
{
	std::string path_video;

	/* Get topology */
	struct media_v2_topology topology = {};

	int err = 0;
	setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
	err = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);
	unsetenv("V4L2_TRACER_PAUSE_TRACE");
	if (err < 0)
		return path_video;

	auto ifaces = new media_v2_interface[topology.num_interfaces];
	topology.ptr_interfaces = (uintptr_t)ifaces;
	auto links = new media_v2_link[topology.num_links];
	topology.ptr_links = (uintptr_t)links;
	auto ents = new media_v2_entity[topology.num_entities];
	topology.ptr_entities = (uintptr_t)ents;

	setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
	err = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);
	unsetenv("V4L2_TRACER_PAUSE_TRACE");

	if (err < 0) {
		delete [] ifaces;
		delete [] links;
		delete [] ents;
		return path_video;
	}

	for (auto &name : linked_entities) {

		/* Find an entity listed in the video device's linked_entities. */
		for (__u32 i = 0; i < topology.num_entities; i++) {
			if (ents[i].name != name)
				continue;

			/* Find the first link connected to that entity. */
			for (__u32 j = 0; j < topology.num_links; j++) {
				if (links[j].sink_id != ents[i].id)
					continue;

				/* Find the interface connected to that link. */
				for (__u32 k = 0; k < topology.num_interfaces; k++) {
					if (ifaces[k].id != links[j].source_id)
						continue;

					std::string video_devname = mi_media_get_device(ifaces[k].devnode.major, ifaces[k].devnode.minor);

					if (!video_devname.empty()) {
						path_video = video_devname;
						break;
					}
				}
			}
		}
	}
	return path_video;
}

std::list<std::string> get_linked_entities(int media_fd, std::string path_video)
{
	std::list<std::string> linked_entities;

	/* Get topology */
	int err = 0;
	struct media_v2_topology topology = {};
	setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
	err = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);
	unsetenv("V4L2_TRACER_PAUSE_TRACE");
	if (err < 0)
		return linked_entities;

	auto ifaces = new media_v2_interface[topology.num_interfaces];
	topology.ptr_interfaces = (uintptr_t)ifaces;
	auto links = new media_v2_link[topology.num_links];
	topology.ptr_links = (uintptr_t)links;
	auto ents = new media_v2_entity[topology.num_entities];
	topology.ptr_entities = (uintptr_t)ents;

	setenv("V4L2_TRACER_PAUSE_TRACE", "true", 0);
	err = ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);
	unsetenv("V4L2_TRACER_PAUSE_TRACE");
	if (err < 0) {
		delete [] ifaces;
		delete [] links;
		delete [] ents;
		return linked_entities;
	}

	/* find the interface corresponding to the path_video */
	for (__u32 i = 0; i < topology.num_interfaces; i++) {
		if (path_video != mi_media_get_device(ifaces[i].devnode.major, ifaces[i].devnode.minor))
			continue;

		/* find the links from that interface */
		for (__u32 j = 0; j < topology.num_links; j++) {
			if (links[j].source_id != ifaces[i].id)
				continue;

			/* find the entities connected by that link to the interface */
			for (__u32 k = 0; k < topology.num_entities; k++) {
				if (ents[k].id != links[j].sink_id)
					continue;
				linked_entities.push_back(ents[k].name);
			}
		}
		if (linked_entities.size())
			break;
	}

	delete [] ents;
	delete [] links;
	delete [] ifaces;

	return linked_entities;
}
