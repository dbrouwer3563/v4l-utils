/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#ifndef RETRACE_H264_H
#define RETRACE_H264_H

struct v4l2_ctrl_h264_sps *retrace_v4l2_ctrl_h264_sps_pointer(json_object *ctrl_obj);
struct v4l2_ctrl_h264_pps *retrace_v4l2_ctrl_h264_pps_pointer(json_object *ctrl_obj);
struct v4l2_ctrl_h264_scaling_matrix *retrace_v4l2_ctrl_h264_scaling_matrix_pointer(json_object *ctrl_obj);
struct v4l2_ctrl_h264_pred_weights *retrace_v4l2_ctrl_h264_pred_weights_pointer(json_object *ctrl_obj);
struct v4l2_ctrl_h264_slice_params *retrace_v4l2_ctrl_h264_slice_params_pointer(json_object *h264_weight_factors_obj);
struct v4l2_ctrl_h264_decode_params *retrace_v4l2_ctrl_h264_decode_params(json_object *ctrl_obj);

#endif
