/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef TRACE_H264_H
#define TRACE_H264_H

void trace_v4l2_ctrl_h264_decode_mode(struct v4l2_ext_control ctrl, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_start_code(struct v4l2_ext_control ctrl, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_sps(void *p_h264_sps, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_pps(void *p_h264_pps, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_scaling_matrix(void *p_h264_scaling_matrix, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_pred_weights(void *p_h264_pred_weights, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_slice_params(void *p_h264_slice_params, json_object *ctrl_obj);
void trace_v4l2_ctrl_h264_decode_params(void *p_h264_decode_params, json_object *ctrl_obj);

#endif
