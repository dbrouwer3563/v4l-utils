/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef RETRACE_HELPER_H
#define RETRACE_HELPER_H

void add_buffer_retrace(int fd);
void set_buffer_address_retrace(int fd, long address_trace, long address_retrace);
long int get_buffer_address_retrace(long address_trace);
int get_fd(int fd_trace);
void add_fd(int fd_trace, int fd_retrace);
void remove_fd(int fd_trace);
void print_context(void);
int retrace_v4l2_ext_control_value(json_object *ctrl_obj);

#endif
