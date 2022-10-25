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
		fprintf(stderr, "v4l2-tracer SHA: %s %s\n", STRING(GIT_SHA), STRING(GIT_COMMIT_DATE));
}

void print_usage(void)
{
	print_v4l2_tracer_info();
	fprintf(stderr, "Usage:\n\tv4l2-tracer [options] trace <tracee>\n"
	        "\tv4l2-tracer [options] retrace <trace_file>.json\n\n"

	        "\tCommon options:\n"
	        "\t\t-h, --help        Display this message.\n"
	        "\t\t-p, --pretty      Add whitespace in json file to improve readability.\n"
	        "\t\t-e, --prettymem   Add whitespace in json file just to the memory arrays.\n"
	        "\t\t-r  --raw         Write decoded video frame data to json file.\n"
	        "\t\t-v, --verbose     Turn on verbose reporting.\n"
	        "\t\t-w, --debug       Turn on verbose reporting plus many more details.\n"
	        "\t\t-y, --yuv         Write decoded video frame data to yuv file.\n\n"

	        "\tRetrace options:\n"
			"\t\t-d, --device <dev>   Retrace with a specific video device.\n"
	        "\t\t                     <dev> must be a digit corresponding to\n"
	        "\t\t                     /dev/video<dev> \n\n");
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

std::string get_path_media_from_path_video(std::string path_video_arg)
{
	std::string path_media;

	DIR *dp = opendir("/dev");
	if (dp == nullptr)
		return "";

	struct dirent *ep;

	while ((ep = readdir(dp)) && path_media.empty()) {

		const char *name = ep->d_name;
		if (memcmp(name, "media", 5) || !isdigit(name[5]))
			continue;

		std::string media_entity_name;
		std::string media_devname = std::string("/dev/") + name;
		int media_fd = open(media_devname.c_str(), O_RDONLY);
		if (media_fd < 0)
			continue;

		struct media_v2_topology topology = {};
		if (ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology)) {
			closedir(dp);
			close(media_fd);
			return "";
		}

		auto ents = new media_v2_entity[topology.num_entities];
		topology.ptr_entities = (uintptr_t)ents;
		auto links = new media_v2_link[topology.num_links];
		topology.ptr_links = (uintptr_t)links;
		auto ifaces = new media_v2_interface[topology.num_interfaces];
		topology.ptr_interfaces = (uintptr_t)ifaces;
		auto pads = new media_v2_pad[topology.num_pads];
		topology.ptr_pads = (uintptr_t)pads;

		ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);

		/* Start traversing the topology. */
		__u32 pad_sink_id = 0;
		for (__u32 i = 0; i < topology.num_entities; i++) {

			if (ents[i].function != MEDIA_ENT_F_PROC_VIDEO_DECODER)
				continue;

			media_entity_name = ents[i].name;

			/*
			 * Find the source pad on the media decoder and follow its link to the
			 * sink pad on the sink i/o entity. This is an arbitrary choice of direction since
			 * both the sink and source eventually lead to the video device.
			 */
			for (__u32 j = 0; j < topology.num_pads; j++) {
				if (pads[j].entity_id != ents[i].id)
					continue;

				for (__u32 k = 0; k < topology.num_links; k++) {
					if (links[k].source_id != pads[j].id)
						continue;
					pad_sink_id = links[k].sink_id;
				}
			}
		}

		if (!pad_sink_id) {
			delete [] ents;
			delete [] links;
			delete [] ifaces;
			delete [] pads;
			close(media_fd);
			continue;
		}

		/* Find the interface linked to the sink i/o entity. */
		__u32 interface_id = 0;
		for (__u32 i = 0; i < topology.num_entities; i++) {
			if (ents[i].function != MEDIA_ENT_F_IO_V4L)
				continue;

			for (__u32 j = 0; j < topology.num_pads; j++) {
				if ((pads[j].entity_id != ents[i].id) || (pads[j].id != pad_sink_id))
					continue;

				for (__u32 k = 0; k < topology.num_links; k++) {
					if (links[k].sink_id != ents[i].id)
						continue;
					interface_id = links[k].source_id;
				}
			}
		}

		if (!interface_id) {
			delete [] ents;
			delete [] links;
			delete [] ifaces;
			delete [] pads;
			close(media_fd);
			continue;
		}

		/* Get the interface's /dev/video path string. */
		std::string path_video;

		for (__u32 i = 0; i < topology.num_interfaces; i++) {
			if (ifaces[i].intf_type != MEDIA_INTF_T_V4L_VIDEO)
				continue;
			if (ifaces[i].id != interface_id)
				continue;
			path_video = mi_media_get_device(ifaces[i].devnode.major, ifaces[i].devnode.minor);
		}

		delete [] ents;
		delete [] links;
		delete [] ifaces;
		delete [] pads;
		close(media_fd);

		if (path_video.empty() || ((!path_video_arg.empty()) && (path_video_arg != path_video)))
			continue;

		path_media = media_devname;
	}
	closedir(dp);
	return path_media;
}

std::string get_path_video_from_fd_media(int fd)
{
	std::string path_video;

	struct media_v2_topology topology = {};
	if (ioctl(fd, MEDIA_IOC_G_TOPOLOGY, &topology))
		return "";

	auto ents = new media_v2_entity[topology.num_entities];
	topology.ptr_entities = (uintptr_t)ents;
	auto links = new media_v2_link[topology.num_links];
	topology.ptr_links = (uintptr_t)links;
	auto ifaces = new media_v2_interface[topology.num_interfaces];
	topology.ptr_interfaces = (uintptr_t)ifaces;
	auto pads = new media_v2_pad[topology.num_pads];
	topology.ptr_pads = (uintptr_t)pads;

	__u32 interface_id = 0;
	ioctl(fd, MEDIA_IOC_G_TOPOLOGY, &topology);
	for (__u32 i = 0; i < topology.num_entities; i++) {
		if (ents[i].function != MEDIA_ENT_F_IO_V4L)
				continue;

		for (__u32 j = 0; j < topology.num_links; j++) {
			if (links[j].sink_id != ents[i].id)
				continue;
			interface_id = links[j].source_id;
		}
	}

	if (interface_id) {
		for (__u32 i = 0; i < topology.num_interfaces; i++) {
			if (ifaces[i].id != interface_id)
				continue;
			path_video = mi_media_get_device(ifaces[i].devnode.major, ifaces[i].devnode.minor);
		}
	}

	delete [] ents;
	delete [] links;
	delete [] ifaces;
	delete [] pads;

	return path_video;
}

std::list<std::string> get_entities_linked_to_path_video(int media_fd, std::string path_video)
{
	std::list<std::string> linked_entities;

	struct media_v2_topology topology = {};
	if (ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology))
		return linked_entities;

	auto ents = new media_v2_entity[topology.num_entities];
	topology.ptr_entities = (uintptr_t)ents;
	auto links = new media_v2_link[topology.num_links];
	topology.ptr_links = (uintptr_t)links;
	auto ifaces = new media_v2_interface[topology.num_interfaces];
	topology.ptr_interfaces = (uintptr_t)ifaces;
	auto pads = new media_v2_pad[topology.num_pads];
	topology.ptr_pads = (uintptr_t)pads;

	ioctl(media_fd, MEDIA_IOC_G_TOPOLOGY, &topology);

	for (__u32 i = 0; i < topology.num_interfaces; i++) {
		if (path_video != mi_media_get_device(ifaces[i].devnode.major, ifaces[i].devnode.minor))
			continue;

		for (__u32 j = 0; j < topology.num_links; j++) {
			if (links[j].source_id != ifaces[i].id)
				continue;
			for (__u32 k = 0; k < topology.num_entities; k++) {
				if (ents[k].id != links[j].sink_id)
					continue;
				linked_entities.push_back(ents[k].name);
			}
		}
	}
	delete [] ents;
	delete [] links;
	delete [] ifaces;
	delete [] pads;

	return linked_entities;
}
