/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <time.h>
#include <sys/wait.h>
#include "v4l2-tracer-common.h"

int tracer(int argc, char *argv[], bool retrace = false);
int retracer(std::string trace_filename);

enum Options {
	V4l2TracerOptSetVideoDevice = 'd',
	V4l2TracerOptPrettyPrintMemory = 'e',
	V4l2TracerOptDebug = 'g',
	V4l2TracerOptHelp = 'h',
	V4l2TracerOptIoctlsInfo = 'i',
	V4l2TracerOptSetMediaDevice = 'm',
	V4l2TracerOptPrettyPrint = 'p',
	V4l2TracerOptWriteDecodedToJson = 'r',
	V4l2TracerOptVerbose = 'v',
	V4l2TracerOptWriteDecodedToYUVFile = 'y',
};

static struct option long_options[] = {
	{ "video_device", required_argument, nullptr, V4l2TracerOptSetVideoDevice },
	{ "prettymem", no_argument, nullptr, V4l2TracerOptPrettyPrintMemory },
	{ "debug", no_argument, nullptr, V4l2TracerOptDebug },
	{ "help", no_argument, nullptr, V4l2TracerOptHelp },
	{ "info", no_argument, nullptr, V4l2TracerOptIoctlsInfo },
	{ "media_device", required_argument, nullptr, V4l2TracerOptSetMediaDevice },
	{ "pretty", no_argument, nullptr, V4l2TracerOptPrettyPrint },
	{ "raw", no_argument, nullptr, V4l2TracerOptWriteDecodedToJson },
	{ "verbose", no_argument, nullptr, V4l2TracerOptVerbose },
	{ "yuv", no_argument, nullptr, V4l2TracerOptWriteDecodedToYUVFile },
	{ nullptr, 0, nullptr, 0 }
};

char short_options[] = {
	V4l2TracerOptSetVideoDevice, ':',
	V4l2TracerOptPrettyPrintMemory,
	V4l2TracerOptDebug,
	V4l2TracerOptHelp,
	V4l2TracerOptIoctlsInfo,
	V4l2TracerOptSetMediaDevice, ':',
	V4l2TracerOptPrettyPrint,
	V4l2TracerOptWriteDecodedToJson,
	V4l2TracerOptVerbose,
	V4l2TracerOptWriteDecodedToYUVFile
};

int get_options(int argc, char *argv[])
{
	int ch;

	do {
		/* If there are no commands after the valid options, return err. */
		if (optind == argc) {
			print_usage();
			return -1;
		}

		/* Avoid reading the tracee's options. */
		if ((strcmp(argv[optind], "trace") == 0) || (strcmp(argv[optind], "retrace") == 0))
			return 0;

		ch = getopt_long(argc, argv, short_options, long_options, NULL);
		switch (ch) {
		case V4l2TracerOptSetVideoDevice: {
			std::string device_num = optarg;
			try {
				std::stoi(device_num, nullptr, 0);
			} catch (std::exception& e) {
				fprintf(stderr, "%d: <dev> \'%s\' must be a digit\n", __LINE__, device_num.c_str());
				return -1;
			}
			if (device_num[0] >= '0' && device_num[0] <= '9' && device_num.length() <= 3) {
				std::string path_video = "/dev/video";
				path_video += optarg;
				setenv("V4L2_TRACER_OPTION_SET_VIDEO_DEVICE", path_video.c_str(), 0);
			} else {
				fprintf(stderr, "%d: cannot use \'%s\'\n", __LINE__, device_num.c_str());
				return -1;
			}
			break;
		}
		case V4l2TracerOptPrettyPrintMemory:
			setenv("V4L2_TRACER_OPTION_PRETTY_PRINT_MEM", "true", 0);
			break;
		case V4l2TracerOptDebug:
			setenv("V4L2_TRACER_OPTION_VERBOSE", "true", 0);
			setenv("V4L2_TRACER_OPTION_DEBUG", "true", 0);
			break;
		case V4l2TracerOptHelp:
			print_usage();
			return -1;
		case V4l2TracerOptIoctlsInfo:
			setenv("V4L2_TRACER_OPTION_IOCTLS_INFO", "true", 0);
			break;
		case V4l2TracerOptSetMediaDevice: {
			std::string device_num = optarg;
			try {
				std::stoi(device_num, nullptr, 0);
			} catch (std::exception& e) {
				fprintf(stderr, "%d: <dev> \'%s\' must be a digit\n", __LINE__, device_num.c_str());
				return -1;
			}
			if (device_num[0] >= '0' && device_num[0] <= '9' && device_num.length() <= 3) {
				std::string path_media = "/dev/media";
				path_media += optarg;
				setenv("V4L2_TRACER_OPTION_SET_MEDIA_DEVICE", path_media.c_str(), 0);
			} else {
				fprintf(stderr, "%d: cannot use \'%s\'\n", __LINE__, device_num.c_str());
				return -1;
			}
			break;
		}
		case V4l2TracerOptPrettyPrint:
			setenv("V4L2_TRACER_OPTION_PRETTY_PRINT_ALL", "true", 0);
			break;
		case V4l2TracerOptWriteDecodedToJson:
			setenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_JSON_FILE", "true", 0);
			break;
		case V4l2TracerOptVerbose:
			setenv("V4L2_TRACER_OPTION_VERBOSE", "true", 0);
			break;
		case V4l2TracerOptWriteDecodedToYUVFile:
			setenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_YUV_FILE", "true", 0);
			break;
		default:
			break;
		}

		/* invalid option */
		if (optopt > 0) {
			print_usage();
			return -1;
		}

	} while (ch != -1);

	return 0;
}

int tracer(int argc, char *argv[], bool retrace)
{
	char *exec[argc];
	int exec_index = 0;

	char retrace_command[] = "__retrace";

	if (retrace) {
		std::string trace_file = argv[optind];
		if (trace_file.find(".json") == trace_file.npos) {
			fprintf(stderr, "Trace file \'%s\' must be JSON-formatted\n", trace_file.c_str());
			if (is_verbose())
				fprintf(stderr, "error: %s:%d\n", __func__, __LINE__);
			print_usage();
			return -1;
		}
	}

	/* Get the application to be traced. */
	if (retrace) {
		exec[exec_index++] = argv[0]; /* tracee is v4l2-tracer, local or installed */
		exec[exec_index++] = retrace_command;
		exec[exec_index++] = argv[optind]; /* json file to be retraced */
	} else {
		while (optind < argc)
			exec[exec_index++] = argv[optind++];
	}
	exec[exec_index] = nullptr;

	/* Create a unique trace filename and open a file. */
	std::string trace_id;
	if (retrace) {
		std::string json_file_name = argv[optind];
		trace_id = json_file_name.substr(0, json_file_name.find(".json"));
		trace_id += "_retrace";
	} else {
		trace_id = std::to_string(time(nullptr));
		trace_id = trace_id.substr(5, trace_id.npos) + "_trace";
	}
	setenv("TRACE_ID", trace_id.c_str(), 0);
	std::string trace_filename = trace_id + ".json";
	FILE *trace_file = fopen(trace_filename.c_str(), "w");
	if (trace_file == nullptr) {
		fprintf(stderr, "Could not open trace file: %s\n", trace_filename.c_str());
		perror("");
		return errno;
	}

	/* Open the json array.*/
	fputs("[\n", trace_file);

	/* Add v4l-utils package and git info to the top of the trace file. */
	std::string json_str;
	json_object *v4l2_tracer_info_obj = json_object_new_object();
	json_object_object_add(v4l2_tracer_info_obj, "package_version", json_object_new_string(PACKAGE_VERSION));
	std::string git_commit_cnt = STRING(GIT_COMMIT_CNT);
	git_commit_cnt = git_commit_cnt.erase(0, 1); /* remove the hyphen in front of git_commit_cnt */
	json_object_object_add(v4l2_tracer_info_obj, "git_commit_cnt", json_object_new_string(git_commit_cnt.c_str()));
	json_object_object_add(v4l2_tracer_info_obj, "git_sha", json_object_new_string(STRING(GIT_SHA)));
	json_object_object_add(v4l2_tracer_info_obj, "git_commit_date", json_object_new_string(STRING(GIT_COMMIT_DATE)));
	json_str = json_object_to_json_string(v4l2_tracer_info_obj);
	fwrite(json_str.c_str(), sizeof(char), json_str.length(), trace_file);
	fputs(",\n", trace_file);
	json_object_put(v4l2_tracer_info_obj);

	/* Add v4l2-tracer command line to the top of the trace file. */
	json_object *tracee_obj = json_object_new_object();
	std::string tracee;
	for (int i = 0; i < argc; i++) {
		tracee += argv[i];
		tracee += " ";
	}
	json_object_object_add(tracee_obj, "Trace", json_object_new_string(tracee.c_str()));
	const time_t t = time(nullptr);
	json_object_object_add(tracee_obj, "Timestamp", json_object_new_string(ctime(&t)));

	json_str = json_object_to_json_string(tracee_obj);
	fwrite(json_str.c_str(), sizeof(char), json_str.length(), trace_file);
	fputs(",\n", trace_file);
	json_object_put(tracee_obj);
	fclose(trace_file);

	/*
	 * Preload the libv4l2tracer library. If the program is installed, load the library
	 * from its installed location, otherwise load it locally. If it's loaded locally,
	 * use ./configure --disable-dyn-libv4l.
	 */
	std::string libv4l2tracer_path;
	std::string program = argv[0];
	std::size_t idx = program.rfind("/v4l2-tracer");
	if (idx != program.npos) {
		libv4l2tracer_path = program.replace(program.begin() + idx + 1, program.end(), ".libs");
		DIR *dp = opendir(libv4l2tracer_path.c_str());
		if (dp == nullptr)
			libv4l2tracer_path = program.replace(program.begin() + idx, program.end(), "./.libs");
		closedir(dp);
	} else {
		libv4l2tracer_path = STRING(LIBTRACER_PATH);
	}
	libv4l2tracer_path += "/libv4l2tracer.so";
	if (is_verbose())
		fprintf(stderr, "Loading libv4l2tracer: %s\n", libv4l2tracer_path.c_str());
	setenv("LD_PRELOAD", libv4l2tracer_path.c_str(), 0);

	if (fork() == 0) {

		if (is_debug()) {
			fprintf(stderr, "tracee: ");
			for (int i = 0; i < exec_index; i++)
				fprintf(stderr,"%s ", exec[i]);
			fprintf(stderr, "\n");
		}

		execvpe(exec[0], (char* const*) exec, environ);

		if (is_verbose())
			fprintf(stderr, "error: v4l2-tracer.cpp:%d, ", __LINE__);

		fprintf(stderr, "%s: ", exec[0]);
		perror("could not execute application");
		return errno;
	}

	int exec_result;
	wait(&exec_result);

	if (WEXITSTATUS(exec_result)) {
		fprintf(stderr, "Trace error: %s\n", trace_filename.c_str());

		trace_file = fopen(trace_filename.c_str(), "a");
		fseek(trace_file, 0L, SEEK_END);
		fputs("\n]\n", trace_file);
		fclose(trace_file);

		exit(EXIT_FAILURE);
	}

	/* Close the json-array and the trace file. */
	trace_file = fopen(trace_filename.c_str(), "a");
	fseek(trace_file, 0L, SEEK_END);
	fputs("\n]\n", trace_file);
	fclose(trace_file);

	if (retrace)
		fprintf(stderr, "Retrace complete: ");
	else
		fprintf(stderr, "Trace complete: ");
	fprintf(stderr, "%s", trace_filename.c_str());
	fprintf(stderr, "\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = -1;

	if (argc <= 1) {
		print_usage();
		return ret;
	}

	ret = get_options(argc, argv);

	if (ret < 0) {
		if (is_verbose())
			fprintf(stderr, "error: v4l2-tracer.cpp:%d\n", __LINE__);
		return ret;
	}

	if (optind == argc) {
		if (is_verbose())
			fprintf(stderr, "error: v4l2-tracer.cpp:%d\n", __LINE__);
		print_usage();
		return ret;
	}

	std::string command = argv[optind++];

	if (optind == argc) {
		if (is_verbose())
			fprintf(stderr, "error: v4l2-tracer.cpp:%d\n", __LINE__);
		print_usage();
		return ret;
	}

	if (command == "trace") {
		ret = tracer(argc, argv);
	} else if (command == "retrace") {
		ret = tracer(argc, argv, true);
	} else if (command == "__retrace") {
		/*
		 * This command is meant to be used internally only to allow
		 * v4l2-tracer to recursively trace itself during a retrace.
		 */
		ret = retracer(argv[optind]);
	} else {
		if (is_verbose()) {
			fprintf(stderr, "error: v4l2-tracer.cpp:%d\n", __LINE__);
			fprintf(stderr, "tracee: ");
			for (int i = 0; i < argc; i++)
				fprintf(stderr,"%s ", argv[i]);
			fprintf(stderr, "\n");
		}
		print_usage();
	}

	return ret;
}
