/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

void trace_v4l2_ctrl_h264_decode_mode(struct v4l2_ext_control ctrl, json_object *ctrl_obj)
{
	json_object *h264_decode_mode_obj = json_object_new_object();
	json_object_object_add(h264_decode_mode_obj, "value", json_object_new_int(ctrl.value));
	json_object_object_add(h264_decode_mode_obj, "value_str", json_object_new_string(h264_decode_mode2s(ctrl.value).c_str()));
	json_object_object_add(ctrl_obj, "V4L2_CID_STATELESS_H264_DECODE_MODE", h264_decode_mode_obj);
}

void trace_v4l2_ctrl_h264_start_code(struct v4l2_ext_control ctrl, json_object *ctrl_obj)
{
	json_object *h264_start_code_obj = json_object_new_object();
	json_object_object_add(h264_start_code_obj, "value", json_object_new_int(ctrl.value));
	json_object_object_add(h264_start_code_obj, "value_str", json_object_new_string(h264_start_code2s(ctrl.value).c_str()));
	json_object_object_add(ctrl_obj, "V4L2_CID_STATELESS_H264_START_CODE", h264_start_code_obj);
}

void trace_v4l2_ctrl_h264_sps(void *p_h264_sps, json_object *ctrl_obj)
{
	json_object *h264_sps_obj = json_object_new_object();
	struct v4l2_ctrl_h264_sps *h264_sps = static_cast<struct v4l2_ctrl_h264_sps*>(p_h264_sps);

	json_object_object_add(h264_sps_obj, "profile_idc", json_object_new_int(h264_sps->profile_idc));
	json_object_object_add(h264_sps_obj, "constraint_set_flags", json_object_new_int(h264_sps->constraint_set_flags));
	json_object_object_add(h264_sps_obj, "level_idc", json_object_new_int(h264_sps->level_idc));
	json_object_object_add(h264_sps_obj, "seq_parameter_set_id", json_object_new_int(h264_sps->seq_parameter_set_id));
	json_object_object_add(h264_sps_obj, "chroma_format_idc", json_object_new_int(h264_sps->chroma_format_idc));
	json_object_object_add(h264_sps_obj, "bit_depth_luma_minus8", json_object_new_int(h264_sps->bit_depth_luma_minus8));
	json_object_object_add(h264_sps_obj, "bit_depth_chroma_minus8", json_object_new_int(h264_sps->bit_depth_chroma_minus8));
	json_object_object_add(h264_sps_obj, "log2_max_frame_num_minus4", json_object_new_int(h264_sps->log2_max_frame_num_minus4));
	json_object_object_add(h264_sps_obj, "pic_order_cnt_type", json_object_new_int(h264_sps->pic_order_cnt_type));
	json_object_object_add(h264_sps_obj, "log2_max_pic_order_cnt_lsb_minus4", json_object_new_int(h264_sps->log2_max_pic_order_cnt_lsb_minus4));
	json_object_object_add(h264_sps_obj, "max_num_ref_frames", json_object_new_int(h264_sps->max_num_ref_frames));
	json_object_object_add(h264_sps_obj, "num_ref_frames_in_pic_order_cnt_cycle", json_object_new_int(h264_sps->num_ref_frames_in_pic_order_cnt_cycle));

	/* 	__s32 offset_for_ref_frame[255] */
	json_object *offset_for_ref_frame_obj =  json_object_new_array_ext(255);
	for (size_t index = 0; index < ARRAY_SIZE(h264_sps->offset_for_ref_frame); index++)
		json_object_array_add(offset_for_ref_frame_obj, json_object_new_int(h264_sps->offset_for_ref_frame[index]));

	json_object_object_add(h264_sps_obj, "offset_for_ref_frame", offset_for_ref_frame_obj);
	json_object_object_add(h264_sps_obj, "offset_for_non_ref_pic", json_object_new_int(h264_sps->offset_for_non_ref_pic));
	json_object_object_add(h264_sps_obj, "offset_for_top_to_bottom_field", json_object_new_int(h264_sps->offset_for_top_to_bottom_field));
	json_object_object_add(h264_sps_obj, "pic_width_in_mbs_minus1", json_object_new_int(h264_sps->pic_width_in_mbs_minus1));
	json_object_object_add(h264_sps_obj, "pic_height_in_map_units_minus1", json_object_new_int(h264_sps->pic_height_in_map_units_minus1));
	json_object_object_add(h264_sps_obj, "flags", json_object_new_uint64(h264_sps->flags));
	json_object_object_add(h264_sps_obj, "flags_str", json_object_new_string(h264_sps_flag2s(h264_sps->flags).c_str()));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_sps", h264_sps_obj);
}

void trace_v4l2_ctrl_h264_pps(void *p_h264_pps, json_object *ctrl_obj)
{
	json_object *h264_pps_obj = json_object_new_object();
	struct v4l2_ctrl_h264_pps *h264_pps = static_cast<struct v4l2_ctrl_h264_pps*>(p_h264_pps);

	json_object_object_add(h264_pps_obj, "pic_parameter_set_id", json_object_new_int(h264_pps->pic_parameter_set_id));
	json_object_object_add(h264_pps_obj, "seq_parameter_set_id", json_object_new_int(h264_pps->seq_parameter_set_id));
	json_object_object_add(h264_pps_obj, "num_slice_groups_minus1", json_object_new_int(h264_pps->num_slice_groups_minus1));
	json_object_object_add(h264_pps_obj, "num_ref_idx_l0_default_active_minus1", json_object_new_int(h264_pps->num_ref_idx_l0_default_active_minus1));
	json_object_object_add(h264_pps_obj, "num_ref_idx_l1_default_active_minus1", json_object_new_int(h264_pps->num_ref_idx_l1_default_active_minus1));
	json_object_object_add(h264_pps_obj, "weighted_bipred_idc", json_object_new_int(h264_pps->weighted_bipred_idc));
	json_object_object_add(h264_pps_obj, "pic_init_qp_minus26", json_object_new_int(h264_pps->pic_init_qp_minus26));
	json_object_object_add(h264_pps_obj, "pic_init_qs_minus26", json_object_new_int(h264_pps->pic_init_qs_minus26));
	json_object_object_add(h264_pps_obj, "chroma_qp_index_offset", json_object_new_int(h264_pps->chroma_qp_index_offset));
	json_object_object_add(h264_pps_obj, "second_chroma_qp_index_offset", json_object_new_int(h264_pps->second_chroma_qp_index_offset));
	json_object_object_add(h264_pps_obj, "flags", json_object_new_int(h264_pps->flags));
	json_object_object_add(h264_pps_obj, "flags_str", json_object_new_string(h264_pps_flag2s(h264_pps->flags).c_str()));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_pps", h264_pps_obj);
}

void trace_v4l2_ctrl_h264_scaling_matrix(void *p_h264_scaling_matrix, json_object *ctrl_obj)
{
	json_object *h264_scaling_matrix_obj = json_object_new_object();
	struct v4l2_ctrl_h264_scaling_matrix *h264_scaling_matrix;
	h264_scaling_matrix = static_cast<struct v4l2_ctrl_h264_scaling_matrix*>(p_h264_scaling_matrix);

	/*__u8 scaling_list_4x4[6][16] */
	json_object *scaling_list_4x4_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(h264_scaling_matrix->scaling_list_4x4); i++)
		for (size_t j = 0; j < ARRAY_SIZE(h264_scaling_matrix->scaling_list_4x4[0]); j++)
			json_object_array_add(scaling_list_4x4_obj, json_object_new_int(h264_scaling_matrix->scaling_list_4x4[i][j]));

	json_object_object_add(h264_scaling_matrix_obj, "scaling_list_4x4", scaling_list_4x4_obj);

	/*	__u8 scaling_list_8x8[6][64] */
	json_object *scaling_list_8x8_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(h264_scaling_matrix->scaling_list_8x8); i++)
		for (size_t j = 0; j < ARRAY_SIZE(h264_scaling_matrix->scaling_list_8x8[0]); j++)
			json_object_array_add(scaling_list_8x8_obj, json_object_new_int(h264_scaling_matrix->scaling_list_8x8[i][j]));

	json_object_object_add(h264_scaling_matrix_obj, "scaling_list_8x8", scaling_list_8x8_obj);

	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_scaling_matrix", h264_scaling_matrix_obj);
}

json_object *trace_v4l2_h264_weight_factors(struct v4l2_h264_weight_factors wf)
{
	json_object *weight_factors_obj = json_object_new_object();

	/*__s16 luma_weight[32] */
	json_object *luma_weight_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(wf.luma_weight); i++)
			json_object_array_add(luma_weight_obj, json_object_new_int(wf.luma_weight[i]));
	json_object_object_add(weight_factors_obj, "luma_weight", luma_weight_obj);

	/* __s16 luma_offset[32] */
	json_object *luma_offset_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(wf.luma_offset); i++)
			json_object_array_add(luma_offset_obj, json_object_new_int(wf.luma_offset[i]));
	json_object_object_add(weight_factors_obj, "luma_offset", luma_offset_obj);

	/* __s16 chroma_weight[32][2] */
	json_object *chroma_weight_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(wf.chroma_weight); i++)
		for (size_t j = 0; j < ARRAY_SIZE(wf.chroma_weight[0]); j++)
			json_object_array_add(chroma_weight_obj, json_object_new_int(wf.chroma_weight[i][j]));
	json_object_object_add(weight_factors_obj, "chroma_weight", chroma_weight_obj);

	/* __s16 chroma_offset[32][2] */
	json_object *chroma_offset_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(wf.chroma_offset); i++)
		for (size_t j = 0; j < ARRAY_SIZE(wf.chroma_offset[0]); j++)
				json_object_array_add(chroma_offset_obj, json_object_new_int(wf.chroma_offset[i][j]));
	json_object_object_add(weight_factors_obj, "chroma_offset", chroma_offset_obj);

	return weight_factors_obj;
}

void trace_v4l2_ctrl_h264_pred_weights(void *p_h264_pred_weights, json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_pred_weights *h264_pred_weights;
	h264_pred_weights = static_cast<struct v4l2_ctrl_h264_pred_weights*>(p_h264_pred_weights);

	json_object *h264_pred_weights_obj = json_object_new_object();

	json_object_object_add(h264_pred_weights_obj, "luma_log2_weight_denom",
	                       json_object_new_int(h264_pred_weights->luma_log2_weight_denom));

	json_object_object_add(h264_pred_weights_obj, "chroma_log2_weight_denom",
	                       json_object_new_int(h264_pred_weights->chroma_log2_weight_denom));

	/* struct v4l2_h264_weight_factors weight_factors[2] */
	json_object *weight_factors_array_obj = json_object_new_array_ext(2);
	for (size_t i = 0; i < ARRAY_SIZE(h264_pred_weights->weight_factors); i++) {
		json_object *wf = trace_v4l2_h264_weight_factors(h264_pred_weights->weight_factors[i]);
		json_object_array_add(weight_factors_array_obj, wf);
	}
	json_object_object_add(h264_pred_weights_obj, "weight_factors", weight_factors_array_obj);
	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_pred_weights", h264_pred_weights_obj);
}

json_object *trace_v4l2_h264_reference(struct v4l2_h264_reference ref)
{
	json_object *h264_reference_obj = json_object_new_object();

	json_object_object_add(h264_reference_obj, "fields", json_object_new_int(ref.fields));
	json_object_object_add(h264_reference_obj, "index", json_object_new_int(ref.index));

	return h264_reference_obj;
}

void trace_v4l2_ctrl_h264_slice_params(void *p_h264_slice_params, json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_slice_params *h264_slice_params;
	h264_slice_params = static_cast<struct v4l2_ctrl_h264_slice_params*>(p_h264_slice_params);

	json_object *h264_slice_params_obj = json_object_new_object();

	json_object_object_add(h264_slice_params_obj, "header_bit_size",
	                       json_object_new_uint64(h264_slice_params->header_bit_size));
	json_object_object_add(h264_slice_params_obj, "first_mb_in_slice",
	                       json_object_new_uint64(h264_slice_params->first_mb_in_slice));
	json_object_object_add(h264_slice_params_obj, "slice_type",
	                       json_object_new_int(h264_slice_params->slice_type));
	json_object_object_add(h264_slice_params_obj, "colour_plane_id",
	                       json_object_new_int(h264_slice_params->colour_plane_id));
	json_object_object_add(h264_slice_params_obj, "redundant_pic_cnt",
	                       json_object_new_int(h264_slice_params->redundant_pic_cnt));
	json_object_object_add(h264_slice_params_obj, "cabac_init_idc",
	                       json_object_new_int(h264_slice_params->cabac_init_idc));
	json_object_object_add(h264_slice_params_obj, "slice_qp_delta",
	                       json_object_new_int(h264_slice_params->slice_qp_delta));
	json_object_object_add(h264_slice_params_obj, "slice_qs_delta",
	                       json_object_new_int(h264_slice_params->slice_qs_delta));
	json_object_object_add(h264_slice_params_obj, "disable_deblocking_filter_idc",
	                       json_object_new_int(h264_slice_params->disable_deblocking_filter_idc));
	json_object_object_add(h264_slice_params_obj, "slice_alpha_c0_offset_div2",
	                       json_object_new_int(h264_slice_params->slice_alpha_c0_offset_div2));
	json_object_object_add(h264_slice_params_obj, "slice_beta_offset_div2",
	                       json_object_new_int(h264_slice_params->slice_beta_offset_div2));
	json_object_object_add(h264_slice_params_obj, "num_ref_idx_l0_active_minus1",
	                       json_object_new_int(h264_slice_params->num_ref_idx_l0_active_minus1));
	json_object_object_add(h264_slice_params_obj, "num_ref_idx_l1_active_minus1",
	                       json_object_new_int(h264_slice_params->num_ref_idx_l1_active_minus1));

	/* 	struct v4l2_h264_reference ref_pic_list0[V4L2_H264_REF_LIST_LEN] */
	json_object *ref_pic_list0_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(h264_slice_params->ref_pic_list0); i++)
		json_object_array_add(ref_pic_list0_obj, trace_v4l2_h264_reference(h264_slice_params->ref_pic_list0[i]));
	json_object_object_add(h264_slice_params_obj, "ref_pic_list0", ref_pic_list0_obj);

	/* 	struct v4l2_h264_reference ref_pic_list1[V4L2_H264_REF_LIST_LEN] */
	json_object *ref_pic_list1_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(h264_slice_params->ref_pic_list1); i++) {
		json_object_array_add(ref_pic_list1_obj, trace_v4l2_h264_reference(h264_slice_params->ref_pic_list1[i]));
	}
	json_object_object_add(h264_slice_params_obj, "ref_pic_list1", ref_pic_list1_obj);

	json_object_object_add(h264_slice_params_obj, "flags", json_object_new_uint64(h264_slice_params->flags));
	json_object_object_add(h264_slice_params_obj, "flags_str", json_object_new_string(h264_slice_flag2s(h264_slice_params->flags).c_str()));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_slice_params", h264_slice_params_obj);
}

json_object *trace_v4l2_h264_dpb_entry(struct v4l2_h264_dpb_entry entry)
{
	json_object *h264_dpb_entry_obj = json_object_new_object();

	json_object_object_add(h264_dpb_entry_obj, "reference_ts", json_object_new_uint64(entry.reference_ts));
	json_object_object_add(h264_dpb_entry_obj, "pic_num", json_object_new_uint64(entry.pic_num));
	json_object_object_add(h264_dpb_entry_obj, "frame_num", json_object_new_int(entry.frame_num));
	json_object_object_add(h264_dpb_entry_obj, "fields", json_object_new_int(entry.fields));
	json_object_object_add(h264_dpb_entry_obj, "top_field_order_cnt", json_object_new_int(entry.top_field_order_cnt));
	json_object_object_add(h264_dpb_entry_obj, "bottom_field_order_cnt", json_object_new_int(entry.bottom_field_order_cnt));
	json_object_object_add(h264_dpb_entry_obj, "flags", json_object_new_uint64(entry.flags));
	json_object_object_add(h264_dpb_entry_obj, "flags_str", json_object_new_string(h264_dpb_entry_flag2s(entry.flags).c_str()));

	return h264_dpb_entry_obj;
}

void trace_v4l2_ctrl_h264_decode_params(void *p_h264_decode_params, json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_decode_params *h264_decode_params;
	h264_decode_params = static_cast<struct v4l2_ctrl_h264_decode_params*>(p_h264_decode_params);

	json_object *h264_decode_params_obj = json_object_new_object();

	/* 	struct v4l2_h264_dpb_entry dpb[V4L2_H264_NUM_DPB_ENTRIES] */
	json_object *dpb_obj = json_object_new_array();
	for (int i = 0; i < V4L2_H264_NUM_DPB_ENTRIES; i++)
		json_object_array_add(dpb_obj, trace_v4l2_h264_dpb_entry(h264_decode_params->dpb[i]));
	json_object_object_add(h264_decode_params_obj, "dpb", dpb_obj);

	json_object_object_add(h264_decode_params_obj, "nal_ref_idc",
	                       json_object_new_int(h264_decode_params->nal_ref_idc));
	json_object_object_add(h264_decode_params_obj, "frame_num",
	                       json_object_new_int(h264_decode_params->frame_num));
	json_object_object_add(h264_decode_params_obj, "top_field_order_cnt",
	                       json_object_new_int(h264_decode_params->top_field_order_cnt));
	json_object_object_add(h264_decode_params_obj, "bottom_field_order_cnt",
	                       json_object_new_int(h264_decode_params->bottom_field_order_cnt));
	json_object_object_add(h264_decode_params_obj, "idr_pic_id",
	                       json_object_new_int(h264_decode_params->idr_pic_id));
	json_object_object_add(h264_decode_params_obj, "pic_order_cnt_lsb",
	                       json_object_new_int(h264_decode_params->pic_order_cnt_lsb));
	json_object_object_add(h264_decode_params_obj, "delta_pic_order_cnt_bottom",
	                       json_object_new_int(h264_decode_params->delta_pic_order_cnt_bottom));
	json_object_object_add(h264_decode_params_obj, "delta_pic_order_cnt0",
	                       json_object_new_int(h264_decode_params->delta_pic_order_cnt0));
	json_object_object_add(h264_decode_params_obj, "delta_pic_order_cnt1",
	                       json_object_new_int(h264_decode_params->delta_pic_order_cnt1));
	json_object_object_add(h264_decode_params_obj, "dec_ref_pic_marking_bit_size",
	                       json_object_new_uint64(h264_decode_params->dec_ref_pic_marking_bit_size));
	json_object_object_add(h264_decode_params_obj, "pic_order_cnt_bit_size",
	                       json_object_new_uint64(h264_decode_params->pic_order_cnt_bit_size));
	json_object_object_add(h264_decode_params_obj, "slice_group_change_cycle",
	                       json_object_new_uint64(h264_decode_params->slice_group_change_cycle));
	json_object_object_add(h264_decode_params_obj, "flags", json_object_new_uint64(h264_decode_params->flags));
	json_object_object_add(h264_decode_params_obj, "flags_str",
						   json_object_new_string(h264_decode_params_flag2s(h264_decode_params->flags).c_str()));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_h264_decode_params", h264_decode_params_obj);
}
