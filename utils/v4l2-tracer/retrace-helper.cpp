/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer-common.h"

struct buffer_retrace {
	int fd;
	__u32 type; /* enum v4l2_buf_type */
	__u32 index;
	__u32 offset;
	long address_trace;
	long address_retrace;
};

struct retrace_context {
	pthread_mutex_t lock;
	/* Key is a file descriptor from the trace, value is the corresponding fd in the retrace. */
	std::unordered_map<int, int> retrace_fds;
	/* List of output and capture buffers being retraced. */
	std::list<struct buffer_retrace> buffers;
};

static struct retrace_context ctx_retrace = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

std::pair<std::string, std::string> retrace_paths;

void set_retrace_paths(std::string path_media, std::string path_video)
{
	retrace_paths = std::make_pair(path_media, path_video);
}

std::pair<std::string, std::string> get_retrace_paths()
{
	return retrace_paths;
}

std::pair<std::string, std::string> search_for_retrace_paths(std::string driver, std::list<std::string> linked_entities_in_json_file)
{
	std::pair<std::string, std::string> retrace_paths;

	DIR *dp = opendir("/dev");
	if (dp == nullptr)
		return retrace_paths;

	struct dirent *ep;

	while ((ep = readdir(dp))) {

		const char *name = ep->d_name;
		if (memcmp(name, "media", 5) || !isdigit(name[5]))
			continue;

		std::string media_entity_name;
		std::string media_devname = std::string("/dev/") + name;
		int media_fd = open(media_devname.c_str(), O_RDONLY);
		if (media_fd < 0)
			continue;

		struct media_device_info info;
		if (ioctl(media_fd, MEDIA_IOC_DEVICE_INFO, &info) || info.driver != driver) {
			close(media_fd);
			continue;
		}

		std::string path_video = get_path_video_from_fd_media(media_fd);
		if (path_video.empty()) {
			close(media_fd);
			continue;
		}

		std::list<std::string> linked_entities  = get_entities_linked_to_path_video(media_fd, path_video);
		if (linked_entities.size() == 0) {
			close(media_fd);
			continue;
		}

		//just find one match that's enough
		for (auto &e : linked_entities_in_json_file) {
			if (find(linked_entities.begin(), linked_entities.end(), e) != linked_entities.end()) {
				retrace_paths = std::make_pair(media_devname, path_video);
				break;
			}
		}
		close(media_fd);
		if (!retrace_paths.first.empty())
			break;
	}
	closedir(dp);
	return retrace_paths;
}

bool buffer_in_retrace_context(int fd, __u32 offset)
{
	bool buffer_in_retrace_context = false;
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			buffer_in_retrace_context = true;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return buffer_in_retrace_context;
}

int get_buffer_fd_retrace(__u32 type, __u32 index)
{
	int fd = 0;  ///can i change to -1?    why is this 0?
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.type == type) && (b.index == index)) {
			fd = b.fd;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return fd;
}

void add_buffer_retrace(int fd, __u32 type, __u32 index, __u32 offset)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	struct buffer_retrace buf = {};
	buf.fd = fd;
	buf.type = type;
	buf.index = index;
	buf.offset = offset;
	ctx_retrace.buffers.push_front(buf);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void remove_buffer_retrace(int fd)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto it = ctx_retrace.buffers.begin(); it != ctx_retrace.buffers.end(); ++it) {
		if (it->fd == fd) {
			ctx_retrace.buffers.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void set_buffer_address_retrace(int fd, __u32 offset, long address_trace, long address_retrace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if ((b.fd == fd) && (b.offset == offset)) {
			b.address_trace = address_trace;
			b.address_retrace = address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

long get_retrace_address_from_trace_address(long address_trace)
{
	long address_retrace = 0;
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		if (b.address_trace == address_trace) {
			address_retrace = b.address_retrace;
			break;
		}
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
	return address_retrace;
}

void print_buffers_retrace(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	for (auto &b : ctx_retrace.buffers) {
		fprintf(stderr, "fd: %d, offset: %d, address_trace:%ld, address_retrace:%ld\n",
		        b.fd, b.offset, b.address_trace, b.address_retrace);
	}
	pthread_mutex_unlock(&ctx_retrace.lock);
}

/*
 * Create a new file descriptor entry in retrace context.
 * Add both the fd from the trace context and the corresponding fd from the retrace context.
 */
void add_fd(int fd_trace, int fd_retrace)
{
	std::pair<int, int> new_pair;
	new_pair = std::make_pair(fd_trace, fd_retrace);
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.retrace_fds.insert(new_pair);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

int get_fd_retrace_from_fd_trace(int fd_trace)
{
	int fd_retrace = -1;
	std::unordered_map<int, int>::const_iterator it;

	pthread_mutex_lock(&ctx_retrace.lock);
	it = ctx_retrace.retrace_fds.find(fd_trace);
	if (it != ctx_retrace.retrace_fds.end())
		fd_retrace = it->second;
	pthread_mutex_unlock(&ctx_retrace.lock);

	return fd_retrace;
}

void remove_fd(int fd_trace)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	ctx_retrace.retrace_fds.erase(fd_trace);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void print_fds(void)
{
	pthread_mutex_lock(&ctx_retrace.lock);
	if (ctx_retrace.retrace_fds.empty())
		fprintf(stderr, "all devices closed\n");
	for (auto it = ctx_retrace.retrace_fds.cbegin(); it != ctx_retrace.retrace_fds.cend(); ++it)
		fprintf(stderr, "fd_trace: %d, fd_retrace: %d\n", it->first, it->second);
	pthread_mutex_unlock(&ctx_retrace.lock);
}

void print_context(void)
{
	print_fds();
	print_buffers_retrace();
	fprintf(stderr, "\n");
}
