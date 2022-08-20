/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/wait.h>
#include <unistd.h>
#include <json.h>
#include <time.h>
#include <cstring>
#include <string>

int main(int argc, char *argv[])
{
	int ch;
	char short_options[] = {'e', 'p', 'r', 'y'};

	if (argc <= 1) {
		fprintf(stderr, "usage: tracer [-e] [-p] [-r] [-y] <tracee>\n");
		return -1;
	}

	do {
		ch = getopt(argc, argv, short_options);
		switch (ch){
		case 'e':
			setenv("TRACE_OPTION_PRETTY_PRINT_MEM", "true", 0);
			break;
		case 'p':
			setenv("TRACE_OPTION_PRETTY_PRINT_ALL", "true", 0);
			break;
		case 'r':
			setenv("TRACE_OPTION_DECODED_TO_JSON", "true", 0);
			break;
		case 'y':
			setenv("TRACE_OPTION_CREATE_YUV_FILE", "true", 0);
			break;
		}
	} while (ch != -1);

	/* Get the tracee from the command line. */
	int count = 0;
	char *exec_array[argc];
	while (optind < argc)
		exec_array[count++] = argv[optind++];
	exec_array[count] = nullptr;

	/* Use a substring of the time to create a unique id for the trace. */
	std::string trace_id = std::to_string(time(nullptr));
	trace_id = trace_id.substr(5, trace_id.npos) + "_trace";

	/* Create the trace file to hold the json-objects as a large json array. */
	std::string trace_filename = trace_id + ".json";
	FILE *trace_file = fopen(trace_filename.c_str(), "w");
	fputs("[\n", trace_file);
	fflush(trace_file);

	setenv("TRACE_ID", trace_id.c_str(), 0);
	setenv("LD_PRELOAD", ".libs/libtracer.so", 0);

	if (fork() == 0) {
		execvpe(exec_array[0], exec_array, environ);
		perror("Could not execute tracee");
		return -1;
	}

	int tracee_result;
	wait(&tracee_result);
	unsetenv("TRACE_ID");
	unsetenv("LD_PRELOAD");

	if (WEXITSTATUS(tracee_result)) {
		fprintf(stderr, "Trace error: %s\n", trace_filename.c_str());
		exit(EXIT_FAILURE);
	}

	/* Close the json-array and the trace file. */
	trace_file = fopen(trace_filename.c_str(), "r+");
	fseek(trace_file, 0L, SEEK_END);
	fseek(trace_file, (ftell(trace_file) - 2), SEEK_SET);
	std::string end = "\n]\n";
	fwrite(end.c_str(), sizeof(char), end.length(), trace_file);
	fclose(trace_file);

	fprintf(stderr, "Trace complete: %s\n", trace_filename.c_str());
	return 0;
}
