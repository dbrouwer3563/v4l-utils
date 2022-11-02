/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef RETRACE_HELPER_H
#define RETRACE_HELPER_H

std::pair<std::string, std::string> find_devices(std::string driver, std::list<std::string> linked_entities_in_json_file);

bool buffer_in_retrace_context(int fd, __u32 offset = 0);
int get_buffer_fd_retrace(__u32 type, __u32 index);
void add_buffer_retrace(int fd, __u32 type, __u32 index, __u32 offset = 0);
void remove_buffer_retrace(int fd);
void set_buffer_address_retrace(int fd, __u32 offset, long address_trace, long address_retrace);
long get_retrace_address_from_trace_address(long address_trace);
void add_fd(int fd_trace, int fd_retrace);
int get_fd_retrace_from_fd_trace(int fd_trace);
void remove_fd(int fd_trace);
void print_context(void);

#endif