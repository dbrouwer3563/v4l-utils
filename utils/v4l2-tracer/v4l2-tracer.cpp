/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer.h"

#define STR(x) #x
#define STRING(x) STR(x)

enum Option_Tracer {
    TracerOptPrettyPrintMemoryOnly = 'e',
    TracerOptHelp = 'h',
    TracerOptPrettyPrintAll = 'p',
    TracerOptWriteDecodedToJsonFile = 'r',
    TracerOptVerbose = 'v',
    TracerOptWriteDecodedToYUVFile = 'y',
};

static struct option long_options_tracer[] = {
	{ "prettymem", no_argument, nullptr, TracerOptPrettyPrintMemoryOnly },
	{ "help", no_argument, nullptr, TracerOptHelp },
	{ "pretty", no_argument, nullptr, TracerOptPrettyPrintAll },
	{ "raw", no_argument, nullptr, TracerOptWriteDecodedToJsonFile },
	{ "verbose", no_argument, nullptr, TracerOptVerbose },
	{ "yuv", no_argument, nullptr, TracerOptWriteDecodedToYUVFile },
	{ nullptr, 0, nullptr, 0 }
};

char short_options_tracer[] = {
	TracerOptPrettyPrintMemoryOnly,
	TracerOptHelp,
	TracerOptPrettyPrintAll,
	TracerOptWriteDecodedToJsonFile,
	TracerOptVerbose,
	TracerOptWriteDecodedToYUVFile
};

void print_help_tracer(void)
{
	fprintf(stderr, "v4l2-tracer trace [trace options] -- <tracee>\n"

	        "\t-e, --prettymem   Add whitespace in json file to improve readability of memory arrays.\n"
	        "\t-h, --help        Display trace help.\n"
	        "\t-p, --pretty      Add whitespace in json file to improve readability.\n"
	        "\t-r  --raw         Write decoded data to json file.\n"
	        "\t-v, --verbose     Turn on verbose reporting.\n"
	        "\t-y, --yuv         Write decoded data to yuv file.\n\n");
}

void print_usage(void)
{
	fprintf(stderr, "v4l2-tracer usage:\n\tv4l2-tracer {trace|retrace}\n");
	fprintf(stderr, "\tv4l2-tracer trace [trace options] -- <tracee>\n");
	fprintf(stderr, "\tv4l2-tracer retrace [retrace options] -- <trace_file>.json\n");
}

void get_options_trace(int argc, char *argv[])
{
	int ch;

	do {
		ch = getopt_long(argc, argv, short_options_tracer, long_options_tracer, NULL);
		switch (ch){
		case TracerOptPrettyPrintMemoryOnly:
			setenv("TRACE_OPTION_PRETTY_PRINT_MEM", "true", 0);
			break;
		case TracerOptHelp:
			break;
		case TracerOptPrettyPrintAll:
			setenv("TRACE_OPTION_PRETTY_PRINT_ALL", "true", 0);
			break;
		case TracerOptWriteDecodedToJsonFile:
			setenv("TRACE_OPTION_WRITE_DECODED_TO_JSON_FILE", "true", 0);
			break;
		case TracerOptVerbose:
			setenv("TRACE_OPTION_VERBOSE", "true", 0);
			break;
		case TracerOptWriteDecodedToYUVFile:
			setenv("TRACE_OPTION_WRITE_DECODED_TO_YUV_FILE", "true", 0);
			break;
		}
	} while (ch != -1);
}

int tracer(int argc, char *argv[])
{
	get_options_trace(argc, argv);

	if (optind == argc) {
		print_help_tracer();
		return 1;
	}

	/* Get the application to be traced from the command line. */
	int count = 0;
	char *exec_array[argc];
	while (optind < argc)
		exec_array[count++] = argv[optind++];
	exec_array[count] = nullptr;

	/* Use a substring of the time to create a unique id for the trace. */
	std::string trace_id = std::to_string(time(nullptr));
	trace_id = trace_id.substr(5, trace_id.npos) + "_trace";
	setenv("TRACE_ID", trace_id.c_str(), 0);

	/* Create the trace file to hold the json-objects as a json array. */
	std::string trace_filename = trace_id + ".json";
	FILE *trace_file = fopen(trace_filename.c_str(), "w");
	fputs("[\n", trace_file);
	fclose(trace_file);

	/*
	 * Preload the libtracer library. If the program is installed, load the library
	 * from its installed location, otherwise load it locally.
	 */
	std::string libtracer_path;
	std::string program = argv[0];
	std::size_t idx = program.rfind("/v4l2-tracer");
	if (idx != program.npos) {
		libtracer_path = program.replace(program.begin() + idx + 1, program.end(), ".libs");
		DIR *dp = opendir(libtracer_path.c_str());
		if (dp == nullptr)
			libtracer_path = program.replace(program.begin() + idx, program.end(), "./.libs");
		closedir(dp);
	} else {
		libtracer_path = STRING(LIBTRACER_PATH);
	}
	libtracer_path += "/libtracer.so";
	fprintf(stderr, "Loading libtracer: %s\n", libtracer_path.c_str());
	setenv("LD_PRELOAD", libtracer_path.c_str(), 0);

	if (fork() == 0) {
		execvpe(exec_array[0], exec_array, environ);
		perror("Option -h for help.  Could not execute application to trace.");
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

	fprintf(stderr, "Trace complete: ");
	if (getenv("TRACE_OPTION_WRITE_DECODED_TO_YUV_FILE"))
		fprintf(stderr, "%s, ", (trace_id + ".yuv").c_str());
	fprintf(stderr, "%s", trace_filename.c_str());
	fprintf(stderr, "\n");

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 1;

	if (argc <= 1) {
		print_usage();
		return ret;
	}

	std::string command = argv[1];
	optind = 2; /* start looking for options only after the trace/retrace command */
	if (command == "trace")
		ret = tracer(argc, argv);
	else if (command == "retrace")
		ret = retracer(argc, argv);
	else
		print_usage();

	return ret;
}
