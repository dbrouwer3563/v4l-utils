/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <time.h>
#include <sys/wait.h>
#include "v4l2-tracer-common.h"

int retracer(std::string trace_filename);

enum Options {
	RetracerOptSetDevice = 'd',
    TracerOptPrettyPrintMemoryOnly = 'e',
    V4l2TracerOptHelp = 'h',
    TracerOptPrettyPrintAll = 'p',
    TracerOptWriteDecodedToJsonFile = 'r',
    V4l2TracerOptVerbose = 'v',
    V4l2TracerOptDebug = 'w',
    V4l2TracerOptWriteDecodedToYUVFile = 'y',
};

static struct option long_options[] = {
	{ "device", required_argument, nullptr, RetracerOptSetDevice },
	{ "prettymem", no_argument, nullptr, TracerOptPrettyPrintMemoryOnly },
	{ "help", no_argument, nullptr, V4l2TracerOptHelp },
	{ "pretty", no_argument, nullptr, TracerOptPrettyPrintAll },
	{ "raw", no_argument, nullptr, TracerOptWriteDecodedToJsonFile },
	{ "verbose", no_argument, nullptr, V4l2TracerOptVerbose },
	{ "debug", no_argument, nullptr, V4l2TracerOptDebug },
	{ "yuv", no_argument, nullptr, V4l2TracerOptWriteDecodedToYUVFile },
	{ nullptr, 0, nullptr, 0 }
};

char short_options[] = {
	RetracerOptSetDevice, ':',
	TracerOptPrettyPrintMemoryOnly,
	V4l2TracerOptHelp,
	TracerOptPrettyPrintAll,
	TracerOptWriteDecodedToJsonFile,
	V4l2TracerOptVerbose,
	V4l2TracerOptDebug,
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

		/* Avoid reading the options for the application to be traced. */
		if ((strcmp(argv[optind], "trace") == 0) || (strcmp(argv[optind], "retrace") == 0))
			return 0;

		ch = getopt_long(argc, argv, short_options, long_options, NULL);

		switch (ch) {
		case RetracerOptSetDevice: {
			std::string device_num = optarg;
			if (device_num[0] >= '0' && device_num[0] <= '9' && device_num.length() <= 3) {
				std::string path_video = "/dev/video";
				path_video += optarg;
				std::string path_media = get_path_media_from_path_video(path_video);
				if (path_media.empty()) {
					fprintf(stderr, "Cannot use \'%s\'\n", path_video.c_str());
					return -1;
				}
				set_retrace_paths(path_media, path_video);
				setenv("V4L2_TRACER_OPTION_SET_DEVICE", "true", 0);
			} else {
				fprintf(stderr, "Cannot use \'%s\'.\n", device_num.c_str());
				return -1;
			}
			break;
		}
		case TracerOptPrettyPrintMemoryOnly:
			setenv("V4L2_TRACER_OPTION_PRETTY_PRINT_MEM", "true", 0);
			break;
		case V4l2TracerOptHelp:
			print_usage();
			return -1;
		case TracerOptPrettyPrintAll:
			setenv("V4L2_TRACER_OPTION_PRETTY_PRINT_ALL", "true", 0);
			break;
		case TracerOptWriteDecodedToJsonFile:
			setenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_JSON_FILE", "true", 0);
			break;
		case V4l2TracerOptVerbose:
			setenv("V4L2_TRACER_OPTION_VERBOSE", "true", 0);
			break;
		case V4l2TracerOptDebug:
			setenv("V4L2_TRACER_OPTION_VERBOSE", "true", 0);
			setenv("V4L2_TRACER_OPTION_DEBUG", "true", 0);
			break;
		case V4l2TracerOptWriteDecodedToYUVFile:
			setenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_YUV_FILE", "true", 0);
			break;
		default:
			break;
		}
		if (optopt > 0) {
			return -1; /* invalid option */
		}
	} while (ch != -1);

	return 0;
}

int tracer(int argc, char *argv[])
{
	/* Get the application to be traced from the command line. */
	int count = 0;
	std::string tracee;
	char *exec_array[argc];
	while (optind < argc) {
		tracee += argv[optind];
		exec_array[count++] = argv[optind++];
	}
	exec_array[count] = nullptr;

	std::string trace_id;

	/* v4l2-tracer will trace itself when retracing. */
	if ((tracee.find("v4l2-tracer") != tracee.npos) && (tracee.find("__retrace") != tracee.npos)) {
		std::string json_file_name;
		if (getenv("V4L2_TRACER_OPTION_SET_DEVICE"))
			json_file_name = exec_array[3];
		else
			json_file_name = exec_array[2];
		if (json_file_name.find("json") == json_file_name.npos) {
			fprintf(stderr, "Trace file \'%s\' must be json-formatted\n", json_file_name.c_str());
			print_usage();
			return -1;
		}
		trace_id = json_file_name.substr(0, json_file_name.find("_"));
		trace_id += "_retrace";
	} else {
		trace_id = std::to_string(time(nullptr));
		trace_id = trace_id.substr(5, trace_id.npos) + "_trace";
	}
	setenv("TRACE_ID", trace_id.c_str(), 0);

	/* Create the trace file to hold the json-objects as a json array. */
	std::string trace_filename = trace_id + ".json";
	FILE *trace_file = fopen(trace_filename.c_str(), "w");

	if (trace_file == nullptr) {
		fprintf(stderr, "Could not open trace file: %s\n", trace_filename.c_str());
		perror("");
		return errno;
	}

	fputs("[\n", trace_file);

	/* Add the info about v4l2-tracer to top of the trace file. */
	json_object *v4l2_tracer_info_obj = json_object_new_object();
	json_object_object_add(v4l2_tracer_info_obj, "package_version", json_object_new_string(PACKAGE_VERSION));
	std::string git_commit_cnt = STRING(GIT_COMMIT_CNT);
	git_commit_cnt = git_commit_cnt.erase(0, 1); /* remove the hyphen in front of number */
	json_object_object_add(v4l2_tracer_info_obj, "git_commit_cnt", json_object_new_string(git_commit_cnt.c_str()));
	json_object_object_add(v4l2_tracer_info_obj, "git_sha", json_object_new_string(STRING(GIT_SHA)));
	json_object_object_add(v4l2_tracer_info_obj, "git_commit_date", json_object_new_string(STRING(GIT_COMMIT_DATE)));

	int flags;
	if (getenv("TRACE_OPTION_PRETTY_PRINT_ALL"))
		flags = JSON_C_TO_STRING_PRETTY;
	else
		flags = JSON_C_TO_STRING_PLAIN;
	std::string json_str = json_object_to_json_string_ext(v4l2_tracer_info_obj, flags);
	fwrite(json_str.c_str(), sizeof(char), json_str.length(), trace_file);
	fputs(",\n", trace_file);
	json_object_put(v4l2_tracer_info_obj);
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
	fprintf(stderr, "Loading libv4l2tracer: %s\n", libv4l2tracer_path.c_str());
	setenv("LD_PRELOAD", libv4l2tracer_path.c_str(), 0);

	if (fork() == 0) {
		execvpe(exec_array[0], exec_array, environ);
		fprintf(stderr, "Failed to trace application: %s\n", exec_array[0]);
		perror("Could not execute application");
		return errno;
	}

	int tracee_result;
	wait(&tracee_result);

	if (WEXITSTATUS(tracee_result)) {
		fprintf(stderr, "Trace error: %s\n", trace_filename.c_str());
		exit(EXIT_FAILURE);
	}

	/* Close the json-array and the trace file. */
	trace_file = fopen(trace_filename.c_str(), "a");
	fseek(trace_file, 0L, SEEK_END);
	fputs("\n]\n", trace_file);
	fclose(trace_file);

	if ((tracee.find("v4l2-tracer") != tracee.npos) && (tracee.find("__retrace") != tracee.npos))
		fprintf(stderr, "Retrace complete: ");
	else
		fprintf(stderr, "Trace complete: ");
	if (getenv("V4L2_TRACER_OPTION_WRITE_DECODED_TO_YUV_FILE"))
		fprintf(stderr, "%s, ", (trace_id + ".yuv").c_str());
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

	if (ret < 0)
		return ret;

	if (optind == argc) {
		print_usage();
		return ret;
	}

	std::string command = argv[optind++];
	if (optind == argc) {
		print_usage();
		return ret;
	}

	if (command == "trace") {

		if (getenv("V4L2_TRACER_OPTION_SET_DEVICE"))
			fprintf(stderr, "Warning: ignoring option -d. Cannot set device for trace.\n");
		ret = tracer(argc, argv);

	} else if (command == "__retrace") {
		/*
		 * Only the tracer itself should call this option internally.
		 * So, it is ok to hard code the location of the json file.
		 */
		if (getenv("V4L2_TRACER_OPTION_SET_DEVICE"))
			ret = retracer(argv[3]);
		else
			ret = retracer(argv[2]);

	} else if (command == "retrace") {

		/* Preserve the device option when tracing the retrace. */
		if (getenv("V4L2_TRACER_OPTION_SET_DEVICE")) {
			std::string device_option = "-d";
			const char *c = get_retrace_paths().second.c_str();
			while (*c) {
				if (isdigit(*c))
					device_option += *c;
				c++;
			}
			int size = 4;
			const char *retrace_command_with_device[size];
			retrace_command_with_device[0] = argv[0]; /* v4l2-tracer */
			retrace_command_with_device[1] = device_option.c_str(); //put the video device back into the options
			retrace_command_with_device[2] = "__retrace";
			retrace_command_with_device[3] = argv[optind]; /* the json trace file */
			optind = 0;
			tracer(size, (char **)retrace_command_with_device);
		} else {
			int size = 3;
			const char *retrace_command[size];
			retrace_command[0] = argv[0]; /* v4l2-tracer */
			retrace_command[1] = "__retrace";
			retrace_command[2] = argv[optind]; /* the json trace file */
			optind = 0;
			tracer(size, (char **)retrace_command);
		}
	} else {
		print_usage();
	}

	return ret;
}
