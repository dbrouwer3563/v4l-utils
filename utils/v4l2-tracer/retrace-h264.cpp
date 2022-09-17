/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer.h"

struct v4l2_ctrl_h264_sps *retrace_v4l2_ctrl_h264_sps_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_sps *h264_sps;
	h264_sps = (struct v4l2_ctrl_h264_sps *) malloc(sizeof(v4l2_ctrl_h264_sps));
	memset(h264_sps, 0, sizeof(v4l2_ctrl_h264_sps));

	json_object *h264_sps_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_sps", &h264_sps_obj);

	json_object *profile_idc;
	json_object_object_get_ex(h264_sps_obj, "profile_idc", &profile_idc);
	h264_sps->profile_idc = json_object_get_int(profile_idc);

	json_object *constraint_set_flags;
	json_object_object_get_ex(h264_sps_obj, "constraint_set_flags", &constraint_set_flags);
	h264_sps->constraint_set_flags = json_object_get_int(constraint_set_flags);

	json_object *level_idc;
	json_object_object_get_ex(h264_sps_obj, "level_idc", &level_idc);
	h264_sps->level_idc = json_object_get_int(level_idc);

	json_object *seq_parameter_set_id;
	json_object_object_get_ex(h264_sps_obj, "seq_parameter_set_id", &seq_parameter_set_id);
	h264_sps->seq_parameter_set_id = json_object_get_int(seq_parameter_set_id);

	json_object *chroma_format_idc;
	json_object_object_get_ex(h264_sps_obj, "chroma_format_idc", &chroma_format_idc);
	h264_sps->chroma_format_idc = json_object_get_int(chroma_format_idc);

	json_object *bit_depth_luma_minus8;
	json_object_object_get_ex(h264_sps_obj, "bit_depth_luma_minus8", &bit_depth_luma_minus8);
	h264_sps->bit_depth_luma_minus8 = json_object_get_int(bit_depth_luma_minus8);

	json_object *bit_depth_chroma_minus8;
	json_object_object_get_ex(h264_sps_obj, "bit_depth_chroma_minus8", &bit_depth_chroma_minus8);
	h264_sps->bit_depth_chroma_minus8 = json_object_get_int(bit_depth_chroma_minus8);

	json_object *log2_max_frame_num_minus4;
	json_object_object_get_ex(h264_sps_obj, "log2_max_frame_num_minus4", &log2_max_frame_num_minus4);
	h264_sps->log2_max_frame_num_minus4 = json_object_get_int(log2_max_frame_num_minus4);

	json_object *pic_order_cnt_type;
	json_object_object_get_ex(h264_sps_obj, "pic_order_cnt_type", &pic_order_cnt_type);
	h264_sps->pic_order_cnt_type = json_object_get_int(pic_order_cnt_type);

	json_object *log2_max_pic_order_cnt_lsb_minus4;
	json_object_object_get_ex(h264_sps_obj, "log2_max_pic_order_cnt_lsb_minus4", &log2_max_pic_order_cnt_lsb_minus4);
	h264_sps->log2_max_pic_order_cnt_lsb_minus4 = json_object_get_int(log2_max_pic_order_cnt_lsb_minus4);

	json_object *max_num_ref_frames;
	json_object_object_get_ex(h264_sps_obj, "max_num_ref_frames", &max_num_ref_frames);
	h264_sps->max_num_ref_frames = json_object_get_int(max_num_ref_frames);

	json_object *num_ref_frames_in_pic_order_cnt_cycle;
	json_object_object_get_ex(h264_sps_obj, "num_ref_frames_in_pic_order_cnt_cycle", &num_ref_frames_in_pic_order_cnt_cycle);
	h264_sps->num_ref_frames_in_pic_order_cnt_cycle = json_object_get_int(num_ref_frames_in_pic_order_cnt_cycle);

	/* 	__s32 offset_for_ref_frame[255] */
	json_object *offset_for_ref_frame;
	json_object_object_get_ex(h264_sps_obj, "offset_for_ref_frame", &offset_for_ref_frame);
	for (size_t i = 0; i < ARRAY_SIZE(h264_sps->offset_for_ref_frame); i++)
		h264_sps->offset_for_ref_frame[i] = json_object_get_int(json_object_array_get_idx(offset_for_ref_frame, i));

	json_object *offset_for_non_ref_pic;
	json_object_object_get_ex(h264_sps_obj, "offset_for_non_ref_pic", &offset_for_non_ref_pic);
	h264_sps->offset_for_non_ref_pic = json_object_get_int(offset_for_non_ref_pic);

	json_object *offset_for_top_to_bottom_field;
	json_object_object_get_ex(h264_sps_obj, "offset_for_top_to_bottom_field", &offset_for_top_to_bottom_field);
	h264_sps->offset_for_top_to_bottom_field = json_object_get_int(offset_for_top_to_bottom_field);

	json_object *pic_width_in_mbs_minus1;
	json_object_object_get_ex(h264_sps_obj, "pic_width_in_mbs_minus1", &pic_width_in_mbs_minus1);
	h264_sps->pic_width_in_mbs_minus1 = json_object_get_int(pic_width_in_mbs_minus1);

	json_object *pic_height_in_map_units_minus1;
	json_object_object_get_ex(h264_sps_obj, "pic_height_in_map_units_minus1", &pic_height_in_map_units_minus1);
	h264_sps->pic_height_in_map_units_minus1 = json_object_get_int(pic_height_in_map_units_minus1);

	json_object *flags;
	json_object_object_get_ex(h264_sps_obj, "flags", &flags);
	h264_sps->flags = json_object_get_int(flags);

	return h264_sps;
}

struct v4l2_ctrl_h264_pps *retrace_v4l2_ctrl_h264_pps_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_pps *h264_pps;
	h264_pps = (struct v4l2_ctrl_h264_pps *) malloc(sizeof(v4l2_ctrl_h264_pps));
	memset(h264_pps, 0, sizeof(v4l2_ctrl_h264_pps));

	json_object *h264_pps_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_pps", &h264_pps_obj);

	json_object *pic_parameter_set_id;
	json_object_object_get_ex(h264_pps_obj, "pic_parameter_set_id", &pic_parameter_set_id);
	h264_pps->pic_parameter_set_id = json_object_get_int(pic_parameter_set_id);

	json_object *seq_parameter_set_id;
	json_object_object_get_ex(h264_pps_obj, "seq_parameter_set_id", &seq_parameter_set_id);
	h264_pps->seq_parameter_set_id = json_object_get_int(seq_parameter_set_id);

	json_object *num_slice_groups_minus1;
	json_object_object_get_ex(h264_pps_obj, "num_slice_groups_minus1", &num_slice_groups_minus1);
	h264_pps->num_slice_groups_minus1 = json_object_get_int(num_slice_groups_minus1);

	json_object *num_ref_idx_l0_default_active_minus1;
	json_object_object_get_ex(h264_pps_obj, "num_ref_idx_l0_default_active_minus1", &num_ref_idx_l0_default_active_minus1);
	h264_pps->num_ref_idx_l0_default_active_minus1 = json_object_get_int(num_ref_idx_l0_default_active_minus1);

	json_object *num_ref_idx_l1_default_active_minus1;
	json_object_object_get_ex(h264_pps_obj, "num_ref_idx_l1_default_active_minus1", &num_ref_idx_l1_default_active_minus1);
	h264_pps->num_ref_idx_l1_default_active_minus1 = json_object_get_int(num_ref_idx_l1_default_active_minus1);

	json_object *weighted_bipred_idc;
	json_object_object_get_ex(h264_pps_obj, "weighted_bipred_idc", &weighted_bipred_idc);
	h264_pps->weighted_bipred_idc = json_object_get_int(weighted_bipred_idc);

	json_object *pic_init_qp_minus26;
	json_object_object_get_ex(h264_pps_obj, "pic_init_qp_minus26", &pic_init_qp_minus26);
	h264_pps->pic_init_qp_minus26 = json_object_get_int(pic_init_qp_minus26);

	json_object *pic_init_qs_minus26;
	json_object_object_get_ex(h264_pps_obj, "pic_init_qs_minus26", &pic_init_qs_minus26);
	h264_pps->pic_init_qs_minus26 = json_object_get_int(pic_init_qs_minus26);

	json_object *chroma_qp_index_offset;
	json_object_object_get_ex(h264_pps_obj, "chroma_qp_index_offset", &chroma_qp_index_offset);
	h264_pps->chroma_qp_index_offset = json_object_get_int(chroma_qp_index_offset);

	json_object *second_chroma_qp_index_offset;
	json_object_object_get_ex(h264_pps_obj, "second_chroma_qp_index_offset", &second_chroma_qp_index_offset);
	h264_pps->second_chroma_qp_index_offset = json_object_get_int(second_chroma_qp_index_offset);

	json_object *flags;
	json_object_object_get_ex(h264_pps_obj, "flags", &flags);
	h264_pps->flags = json_object_get_int(flags);

	return h264_pps;
}

struct v4l2_ctrl_h264_scaling_matrix *retrace_v4l2_ctrl_h264_scaling_matrix_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_scaling_matrix *h264_scaling_matrix;
	h264_scaling_matrix = (struct v4l2_ctrl_h264_scaling_matrix *) malloc(sizeof(v4l2_ctrl_h264_scaling_matrix));
	memset(h264_scaling_matrix, 0, sizeof(v4l2_ctrl_h264_scaling_matrix));

	json_object *h264_scaling_matrix_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_scaling_matrix", &h264_scaling_matrix_obj);

	int count = 0;

	/* __u8 scaling_list_4x4[6][16] */
	json_object *scaling_list_4x4;
	json_object_object_get_ex(h264_scaling_matrix_obj, "scaling_list_4x4", &scaling_list_4x4);
	for (size_t i = 0; i < ARRAY_SIZE(h264_scaling_matrix->scaling_list_4x4); i++)
		for (size_t j = 0; j < ARRAY_SIZE(h264_scaling_matrix->scaling_list_4x4[0]); j++)
			h264_scaling_matrix->scaling_list_4x4[i][j] = json_object_get_int(json_object_array_get_idx(scaling_list_4x4, count++));

	/* __u8 scaling_list_8x8[6][64] */
	count = 0;
	json_object *scaling_list_8x8;
	json_object_object_get_ex(h264_scaling_matrix_obj, "scaling_list_8x8", &scaling_list_8x8);
	for (size_t i = 0; i < ARRAY_SIZE(h264_scaling_matrix->scaling_list_8x8); i++)
		for (size_t j = 0; j < ARRAY_SIZE(h264_scaling_matrix->scaling_list_8x8[0]); j++)
			h264_scaling_matrix->scaling_list_8x8[i][j] = json_object_get_int(json_object_array_get_idx(scaling_list_8x8, count++));

	return h264_scaling_matrix;
}

struct v4l2_h264_weight_factors retrace_v4l2_h264_weight_factors(json_object *h264_weight_factors_obj)
{
	struct v4l2_h264_weight_factors h264_weight_factors;
	memset(&h264_weight_factors, 0, sizeof(h264_weight_factors));

	json_object *luma_weight;
	json_object_object_get_ex(h264_weight_factors_obj, "luma_weight", &luma_weight);
	for (size_t i = 0; i < ARRAY_SIZE(h264_weight_factors.luma_weight); i++)
		h264_weight_factors.luma_weight[i] = json_object_get_int(json_object_array_get_idx(luma_weight, i));

	json_object *luma_offset;
	json_object_object_get_ex(h264_weight_factors_obj, "luma_offset", &luma_offset);
	for (size_t i = 0; i < ARRAY_SIZE(h264_weight_factors.luma_offset); i++)
		h264_weight_factors.luma_offset[i] = json_object_get_int(json_object_array_get_idx(luma_offset, i));

	int count = 0;
	/* __s16 chroma_weight[32][2] */
	json_object *chroma_weight;
	json_object_object_get_ex(h264_weight_factors_obj, "chroma_weight", &chroma_weight);
	for (size_t i = 0; i < ARRAY_SIZE(h264_weight_factors.chroma_weight); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(h264_weight_factors.chroma_weight[0]); j++) {
			int value = json_object_get_int(json_object_array_get_idx(chroma_weight, count++));
			h264_weight_factors.chroma_weight[i][j] = (__s16) value;
		}
	}

	/* __s16 chroma_offset[32][2] */
	count = 0;
	json_object *chroma_offset;
	json_object_object_get_ex(h264_weight_factors_obj, "chroma_offset", &chroma_offset);
	for (size_t i = 0; i < ARRAY_SIZE(h264_weight_factors.chroma_offset); i++) {
		for (size_t j = 0; j < ARRAY_SIZE(h264_weight_factors.chroma_offset[0]); j++) {
			int value = json_object_get_int(json_object_array_get_idx(chroma_offset, count++));
			h264_weight_factors.chroma_offset[i][j] = (__s16) value;
		}
	}

	return h264_weight_factors;
}

struct v4l2_ctrl_h264_pred_weights *retrace_v4l2_ctrl_h264_pred_weights_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_pred_weights *h264_pred_weights;
	h264_pred_weights = (struct v4l2_ctrl_h264_pred_weights *) malloc(sizeof(v4l2_ctrl_h264_pred_weights));
	memset(h264_pred_weights, 0, sizeof(v4l2_ctrl_h264_pred_weights));

	json_object *h264_pred_weights_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_pred_weights", &h264_pred_weights_obj);

	json_object *luma_log2_weight_denom;
	json_object_object_get_ex(h264_pred_weights_obj, "luma_log2_weight_denom", &luma_log2_weight_denom);
	h264_pred_weights->luma_log2_weight_denom = json_object_get_int(luma_log2_weight_denom);

	json_object *chroma_log2_weight_denom;
	json_object_object_get_ex(h264_pred_weights_obj, "chroma_log2_weight_denom", &chroma_log2_weight_denom);
	h264_pred_weights->chroma_log2_weight_denom = json_object_get_int(chroma_log2_weight_denom);

	json_object *weight_factors;
	json_object_object_get_ex(h264_pred_weights_obj, "weight_factors", &weight_factors);

	/* struct v4l2_h264_weight_factors weight_factors[2] */
	for (size_t i = 0; i < ARRAY_SIZE(h264_pred_weights->weight_factors); i++)
		h264_pred_weights->weight_factors[i] = retrace_v4l2_h264_weight_factors(json_object_array_get_idx(weight_factors, i));

	return h264_pred_weights;
}

struct v4l2_h264_reference retrace_v4l2_h264_reference(json_object *h264_reference_obj)
{
	struct v4l2_h264_reference h264_reference;
	memset(&h264_reference, 0, sizeof(v4l2_h264_reference));

	json_object *fields_obj;
	json_object_object_get_ex(h264_reference_obj, "fields", &fields_obj);
	h264_reference.fields = json_object_get_int(fields_obj);

	json_object *index_obj;
	json_object_object_get_ex(h264_reference_obj, "index", &index_obj);
	h264_reference.index = json_object_get_int(index_obj);

	return h264_reference;
}

struct v4l2_ctrl_h264_slice_params *retrace_v4l2_ctrl_h264_slice_params_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_slice_params *h264_slice_params;
	h264_slice_params = (struct v4l2_ctrl_h264_slice_params *) malloc(sizeof(v4l2_ctrl_h264_slice_params));
	memset(h264_slice_params, 0, sizeof(v4l2_ctrl_h264_slice_params));

	json_object *h264_slice_params_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_slice_params", &h264_slice_params_obj);

	json_object *header_bit_size_obj;
	json_object_object_get_ex(h264_slice_params_obj, "header_bit_size", &header_bit_size_obj);
	h264_slice_params->header_bit_size = json_object_get_int(header_bit_size_obj);

	json_object *first_mb_in_slice_obj;
	json_object_object_get_ex(h264_slice_params_obj, "first_mb_in_slice", &first_mb_in_slice_obj);
	h264_slice_params->first_mb_in_slice = json_object_get_int(first_mb_in_slice_obj);

	json_object *slice_type_obj;
	json_object_object_get_ex(h264_slice_params_obj, "slice_type", &slice_type_obj);
	h264_slice_params->slice_type = json_object_get_int(slice_type_obj);

	json_object *colour_plane_id_obj;
	json_object_object_get_ex(h264_slice_params_obj, "colour_plane_id", &colour_plane_id_obj);
	h264_slice_params->colour_plane_id = json_object_get_int(colour_plane_id_obj);

	json_object *redundant_pic_cnt_obj;
	json_object_object_get_ex(h264_slice_params_obj, "redundant_pic_cnt", &redundant_pic_cnt_obj);
	h264_slice_params->redundant_pic_cnt = json_object_get_int(redundant_pic_cnt_obj);

	json_object *cabac_init_idc_obj;
	json_object_object_get_ex(h264_slice_params_obj, "cabac_init_idc", &cabac_init_idc_obj);
	h264_slice_params->cabac_init_idc = json_object_get_int(cabac_init_idc_obj);

	json_object *slice_qp_delta_obj;
	json_object_object_get_ex(h264_slice_params_obj, "slice_qp_delta", &slice_qp_delta_obj);
	h264_slice_params->slice_qp_delta = json_object_get_int(slice_qp_delta_obj);

	json_object *slice_qs_delta_obj;
	json_object_object_get_ex(h264_slice_params_obj, "slice_qs_delta", &slice_qs_delta_obj);
	h264_slice_params->slice_qs_delta = json_object_get_int(slice_qs_delta_obj);

	json_object *disable_deblocking_filter_idc_obj;
	json_object_object_get_ex(h264_slice_params_obj, "disable_deblocking_filter_idc", &disable_deblocking_filter_idc_obj);
	h264_slice_params->disable_deblocking_filter_idc = json_object_get_int(disable_deblocking_filter_idc_obj);

	json_object *slice_alpha_c0_offset_div2_obj;
	json_object_object_get_ex(h264_slice_params_obj, "slice_alpha_c0_offset_div2", &slice_alpha_c0_offset_div2_obj);
	h264_slice_params->slice_alpha_c0_offset_div2 = json_object_get_int(slice_alpha_c0_offset_div2_obj);

	json_object *slice_beta_offset_div2_obj;
	json_object_object_get_ex(h264_slice_params_obj, "slice_beta_offset_div2", &slice_beta_offset_div2_obj);
	h264_slice_params->slice_beta_offset_div2 = json_object_get_int(slice_beta_offset_div2_obj);

	json_object *num_ref_idx_l0_active_minus1_obj;
	json_object_object_get_ex(h264_slice_params_obj, "num_ref_idx_l0_active_minus1", &num_ref_idx_l0_active_minus1_obj);
	h264_slice_params->num_ref_idx_l0_active_minus1 = json_object_get_int(num_ref_idx_l0_active_minus1_obj);

	json_object *num_ref_idx_l1_active_minus1_obj;
	json_object_object_get_ex(h264_slice_params_obj, "num_ref_idx_l1_active_minus1", &num_ref_idx_l1_active_minus1_obj);
	h264_slice_params->num_ref_idx_l1_active_minus1 = json_object_get_int(num_ref_idx_l1_active_minus1_obj);

	/*	struct v4l2_h264_reference ref_pic_list0[V4L2_H264_REF_LIST_LEN] */
	json_object *ref_pic_list0_obj;
	json_object_object_get_ex(h264_slice_params_obj, "ref_pic_list0", &ref_pic_list0_obj);
	for (size_t i = 0; i < ARRAY_SIZE(h264_slice_params->ref_pic_list0); i++)
		h264_slice_params->ref_pic_list0[i] = retrace_v4l2_h264_reference(json_object_array_get_idx(ref_pic_list0_obj, i));

	/* struct v4l2_h264_reference ref_pic_list1[V4L2_H264_REF_LIST_LEN] */
	json_object *ref_pic_list1_obj;
	json_object_object_get_ex(h264_slice_params_obj, "ref_pic_list1", &ref_pic_list1_obj);
	for (size_t i = 0; i < ARRAY_SIZE(h264_slice_params->ref_pic_list1); i++)
		h264_slice_params->ref_pic_list1[i] = retrace_v4l2_h264_reference(json_object_array_get_idx(ref_pic_list1_obj, i));

	json_object *flags_obj;
	json_object_object_get_ex(h264_slice_params_obj, "flags", &flags_obj);
	h264_slice_params->flags = json_object_get_int(flags_obj);

	return h264_slice_params;
}

struct v4l2_h264_dpb_entry retrace_h264_dpb_entry(json_object *h264_dpb_entry_obj)
{
	struct v4l2_h264_dpb_entry entry;
	memset(&entry, 0, sizeof(v4l2_h264_dpb_entry));

	json_object *reference_ts_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "reference_ts", &reference_ts_obj);
	entry.reference_ts = json_object_get_int(reference_ts_obj);

	json_object *pic_num_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "pic_num", &pic_num_obj);
	entry.pic_num = json_object_get_int(pic_num_obj);

	json_object *frame_num_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "frame_num", &frame_num_obj);
	entry.frame_num = json_object_get_int(frame_num_obj);

	json_object *fields_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "fields", &fields_obj);
	entry.fields = json_object_get_int(fields_obj);

	json_object *top_field_order_cnt_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "top_field_order_cnt", &top_field_order_cnt_obj);
	entry.top_field_order_cnt = json_object_get_int(top_field_order_cnt_obj);

	json_object *bottom_field_order_cnt_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "bottom_field_order_cnt", &bottom_field_order_cnt_obj);
	entry.bottom_field_order_cnt = json_object_get_int(bottom_field_order_cnt_obj);

	json_object *flags_obj;
	json_object_object_get_ex(h264_dpb_entry_obj, "flags", &flags_obj);
	entry.flags = json_object_get_int(flags_obj);

	return entry;
}

struct v4l2_ctrl_h264_decode_params *retrace_v4l2_ctrl_h264_decode_params(json_object *ctrl_obj)
{
	struct v4l2_ctrl_h264_decode_params *h264_decode_params;
	h264_decode_params = (struct v4l2_ctrl_h264_decode_params *) malloc(sizeof(v4l2_ctrl_h264_decode_params));
	memset(h264_decode_params, 0, sizeof(v4l2_ctrl_h264_decode_params));

	json_object *h264_decode_params_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_h264_decode_params", &h264_decode_params_obj);

	/* struct v4l2_h264_dpb_entry dpb[V4L2_H264_NUM_DPB_ENTRIES]; */
	json_object *dpb_obj;
	json_object_object_get_ex(h264_decode_params_obj, "dpb", &dpb_obj);
	for (size_t i = 0; i < ARRAY_SIZE(h264_decode_params->dpb); i++)
		h264_decode_params->dpb[i] = retrace_h264_dpb_entry(json_object_array_get_idx(dpb_obj, i));

	json_object *nal_ref_idc_obj;
	json_object_object_get_ex(h264_decode_params_obj, "nal_ref_idc", &nal_ref_idc_obj);
	h264_decode_params->nal_ref_idc = json_object_get_int(nal_ref_idc_obj);

	json_object *frame_num_obj;
	json_object_object_get_ex(h264_decode_params_obj, "frame_num", &frame_num_obj);
	h264_decode_params->frame_num = json_object_get_int(frame_num_obj);

	json_object *top_field_order_cnt_obj;
	json_object_object_get_ex(h264_decode_params_obj, "top_field_order_cnt", &top_field_order_cnt_obj);
	h264_decode_params->top_field_order_cnt = json_object_get_int(top_field_order_cnt_obj);

	json_object *bottom_field_order_cnt_obj;
	json_object_object_get_ex(h264_decode_params_obj, "bottom_field_order_cnt", &bottom_field_order_cnt_obj);
	h264_decode_params->bottom_field_order_cnt = json_object_get_int(bottom_field_order_cnt_obj);

	json_object *idr_pic_id_obj;
	json_object_object_get_ex(h264_decode_params_obj, "idr_pic_id", &idr_pic_id_obj);
	h264_decode_params->idr_pic_id = json_object_get_int(idr_pic_id_obj);

	json_object *pic_order_cnt_lsb_obj;
	json_object_object_get_ex(h264_decode_params_obj, "pic_order_cnt_lsb", &pic_order_cnt_lsb_obj);
	h264_decode_params->pic_order_cnt_lsb = json_object_get_int(pic_order_cnt_lsb_obj);

	json_object *delta_pic_order_cnt_bottom_obj;
	json_object_object_get_ex(h264_decode_params_obj, "delta_pic_order_cnt_bottom", &delta_pic_order_cnt_bottom_obj);
	h264_decode_params->delta_pic_order_cnt_bottom = json_object_get_int(delta_pic_order_cnt_bottom_obj);

	json_object *delta_pic_order_cnt0_obj;
	json_object_object_get_ex(h264_decode_params_obj, "delta_pic_order_cnt0", &delta_pic_order_cnt0_obj);
	h264_decode_params->delta_pic_order_cnt0 = json_object_get_int(delta_pic_order_cnt0_obj);

	json_object *delta_pic_order_cnt1_obj;
	json_object_object_get_ex(h264_decode_params_obj, "delta_pic_order_cnt1", &delta_pic_order_cnt1_obj);
	h264_decode_params->delta_pic_order_cnt1 = json_object_get_int(delta_pic_order_cnt1_obj);

	json_object *dec_ref_pic_marking_bit_size_obj;
	json_object_object_get_ex(h264_decode_params_obj, "dec_ref_pic_marking_bit_size", &dec_ref_pic_marking_bit_size_obj);
	h264_decode_params->dec_ref_pic_marking_bit_size = json_object_get_int(dec_ref_pic_marking_bit_size_obj);

	json_object *pic_order_cnt_bit_size_obj;
	json_object_object_get_ex(h264_decode_params_obj, "pic_order_cnt_bit_size", &pic_order_cnt_bit_size_obj);
	h264_decode_params->pic_order_cnt_bit_size = json_object_get_int(pic_order_cnt_bit_size_obj);

	json_object *slice_group_change_cycle_obj;
	json_object_object_get_ex(h264_decode_params_obj, "slice_group_change_cycle", &slice_group_change_cycle_obj);
	h264_decode_params->slice_group_change_cycle = json_object_get_int(slice_group_change_cycle_obj);

	json_object *flags_obj;
	json_object_object_get_ex(h264_decode_params_obj, "flags", &flags_obj);
	h264_decode_params->flags = json_object_get_int(flags_obj);

	return h264_decode_params;
}
