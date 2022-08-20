/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "retracer.h"

struct v4l2_vp8_loop_filter retrace_v4l2_vp8_loop_filter(json_object *lf_obj)
{
	struct v4l2_vp8_loop_filter lf;
	memset(&lf, 0, sizeof(lf));

	/* __s8 ref_frm_delta[4] */
	json_object *ref_frm_delta_obj;
	json_object_object_get_ex(lf_obj, "ref_frm_delta", &ref_frm_delta_obj);
	for (size_t i = 0; i < ARRAY_SIZE(lf.ref_frm_delta); i++)
		lf.ref_frm_delta[i] = (__s8) json_object_get_int(json_object_array_get_idx(ref_frm_delta_obj, i));

	/* __s8 mb_mode_delta[4] */
	json_object *mb_mode_delta_obj;
	json_object_object_get_ex(lf_obj, "mb_mode_delta", &mb_mode_delta_obj);
	for (size_t i = 0; i < ARRAY_SIZE(lf.mb_mode_delta); i++)
		lf.mb_mode_delta[i] = (__s8) json_object_get_int(json_object_array_get_idx(mb_mode_delta_obj, i));

	json_object *sharpness_level_obj;
	json_object_object_get_ex(lf_obj, "sharpness_level", &sharpness_level_obj);
	lf.sharpness_level = json_object_get_int(sharpness_level_obj);

	json_object *level_obj;
	json_object_object_get_ex(lf_obj, "level", &level_obj);
	lf.level = json_object_get_int(level_obj);

	json_object *padding_obj;
	json_object_object_get_ex(lf_obj, "padding", &padding_obj);
	lf.padding = json_object_get_int(padding_obj);

	json_object *flags_obj;
	json_object_object_get_ex(lf_obj, "flags", &flags_obj);
	lf.flags = json_object_get_int(flags_obj);

	return lf;
}

struct v4l2_vp8_segment retrace_v4l2_vp8_segment(json_object *segment_obj)
{
	struct v4l2_vp8_segment segment;
	memset(&segment, 0, sizeof(v4l2_vp8_segment));

	/*	__s8 quant_update[4] */
	json_object *quant_update_obj;
	json_object_object_get_ex(segment_obj, "quant_update", &quant_update_obj);
	for (size_t i = 0; i < ARRAY_SIZE(segment.quant_update); i++)
		segment.quant_update[i] = (__s8) json_object_get_int(json_object_array_get_idx(quant_update_obj, i));

	/* __s8 lf_update[4] */
	json_object *lf_update_obj;
	json_object_object_get_ex(segment_obj, "lf_update", &lf_update_obj);
	for (size_t i = 0; i < ARRAY_SIZE(segment.lf_update); i++)
		segment.lf_update[i] = (__s8) json_object_get_int(json_object_array_get_idx(lf_update_obj, i));

	/* __u8 segment_probs[3] */
	json_object *segment_probs_obj;
	json_object_object_get_ex(segment_obj, "segment_probs", &segment_probs_obj);
	for (size_t i = 0; i < ARRAY_SIZE(segment.segment_probs); i++)
		segment.segment_probs[i] = json_object_get_int(json_object_array_get_idx(segment_probs_obj, i));

	json_object *padding_obj;
	json_object_object_get_ex(segment_obj, "padding", &padding_obj);
	segment.padding = json_object_get_int(padding_obj);

	json_object *flags_obj;
	json_object_object_get_ex(segment_obj, "flags", &flags_obj);
	segment.flags = json_object_get_int(flags_obj);

	return segment;
}

struct v4l2_vp8_quantization retrace_v4l2_vp8_quantization(json_object *quant_obj)
{
	struct v4l2_vp8_quantization quant;
	memset(&quant, 0, sizeof(quant));

	json_object *y_ac_qi_obj;
	json_object_object_get_ex(quant_obj, "y_ac_qi", &y_ac_qi_obj);
	quant.y_ac_qi = json_object_get_int(y_ac_qi_obj);

	json_object *y_dc_delta_obj;
	json_object_object_get_ex(quant_obj, "y_dc_delta", &y_dc_delta_obj);
	quant.y_dc_delta = (__s8) json_object_get_int(y_dc_delta_obj);

	json_object *y2_dc_delta_obj;
	json_object_object_get_ex(quant_obj, "y2_dc_delta", &y2_dc_delta_obj);
	quant.y2_dc_delta = (__s8) json_object_get_int(y2_dc_delta_obj);

	json_object *y2_ac_delta_obj;
	json_object_object_get_ex(quant_obj, "y2_ac_delta", &y2_ac_delta_obj);
	quant.y2_ac_delta = (__s8) json_object_get_int(y2_ac_delta_obj);

	json_object *uv_dc_delta_obj;
	json_object_object_get_ex(quant_obj, "uv_dc_delta", &uv_dc_delta_obj);
	quant.uv_dc_delta = (__s8) json_object_get_int(uv_dc_delta_obj);

	json_object *uv_ac_delta_obj;
	json_object_object_get_ex(quant_obj, "uv_ac_delta", &uv_ac_delta_obj);
	quant.uv_ac_delta = (__s8) json_object_get_int(uv_ac_delta_obj);

	json_object *padding_obj;
	json_object_object_get_ex(quant_obj, "padding", &padding_obj);
	quant.padding = json_object_get_int(padding_obj);

	return quant;
}

struct v4l2_vp8_entropy retrace_v4l2_vp8_entropy(json_object *entropy_obj)
{
	struct v4l2_vp8_entropy entropy;
	memset(&entropy, 0, sizeof(entropy));

	int count = 0;

	/* __u8 coeff_probs[4][8][3][V4L2_VP8_COEFF_PROB_CNT] */
	json_object *coeff_probs_obj;
	json_object_object_get_ex(entropy_obj, "coeff_probs", &coeff_probs_obj);

	for (size_t i = 0; i < ARRAY_SIZE(entropy.coeff_probs); i++)
		for (size_t j = 0; j < ARRAY_SIZE(entropy.coeff_probs[0]); j++)
			for (size_t k = 0; k < ARRAY_SIZE(entropy.coeff_probs[0][0]); k++)
				for (size_t l = 0; l < V4L2_VP8_COEFF_PROB_CNT; l++)
					entropy.coeff_probs[i][j][k][l] = json_object_get_int(json_object_array_get_idx(coeff_probs_obj, count++));

	/* __u8 y_mode_probs[4] */
	json_object *y_mode_probs_obj;
	json_object_object_get_ex(entropy_obj, "y_mode_probs", &y_mode_probs_obj);
	for (size_t i = 0; i < ARRAY_SIZE(entropy.y_mode_probs); i++)
		entropy.y_mode_probs[i] = json_object_get_int(json_object_array_get_idx(y_mode_probs_obj, i));

	/* __u8 uv_mode_probs[3] */
	json_object *uv_mode_probs_obj;
	json_object_object_get_ex(entropy_obj, "uv_mode_probs", &uv_mode_probs_obj);
	for (size_t i = 0; i < ARRAY_SIZE(entropy.uv_mode_probs); i++)
		entropy.uv_mode_probs[i] = json_object_get_int(json_object_array_get_idx(uv_mode_probs_obj, i));

	/*  __u8 mv_probs[2][V4L2_VP8_MV_PROB_CNT] */
	count = 0;
	json_object *mv_probs_obj;
	json_object_object_get_ex(entropy_obj, "mv_probs", &mv_probs_obj);
	for (size_t i = 0; i < ARRAY_SIZE(entropy.mv_probs); i++)
		for (size_t j = 0; j < V4L2_VP8_MV_PROB_CNT; j++)
			entropy.mv_probs[i][j] = json_object_get_int(json_object_array_get_idx(mv_probs_obj, count++));

	/* __u8 padding[3] */
	json_object *padding_obj;
	json_object_object_get_ex(entropy_obj, "padding", &padding_obj);
	for (size_t i = 0; i < ARRAY_SIZE(entropy.padding); i++)
		entropy.padding[i] = json_object_get_int(json_object_array_get_idx(padding_obj, i));

	return entropy;
}

struct v4l2_vp8_entropy_coder_state retrace_v4l2_vp8_entropy_coder_state(json_object *coder_state_obj)
{
	struct v4l2_vp8_entropy_coder_state coder_state;
	memset(&coder_state, 0, sizeof(coder_state));

	json_object *range_obj;
	json_object_object_get_ex(coder_state_obj, "range", &range_obj);
	coder_state.range = json_object_get_int(range_obj);

	json_object *value_obj;
	json_object_object_get_ex(coder_state_obj, "value", &value_obj);
	coder_state.value = json_object_get_int(value_obj);

	json_object *bit_count_obj;
	json_object_object_get_ex(coder_state_obj, "bit_count", &bit_count_obj);
	coder_state.bit_count = json_object_get_int(bit_count_obj);

	json_object *padding_obj;
	json_object_object_get_ex(coder_state_obj, "padding", &padding_obj);
	coder_state.padding = json_object_get_int(padding_obj);

	return coder_state;
}

struct v4l2_ctrl_vp8_frame *retrace_v4l2_ctrl_vp8_frame_pointer(json_object *ctrl_obj)
{
	struct v4l2_ctrl_vp8_frame *vp8_frame_pointer = (struct v4l2_ctrl_vp8_frame *) malloc(sizeof(v4l2_ctrl_vp8_frame));
	memset(vp8_frame_pointer, 0, sizeof(v4l2_ctrl_vp8_frame));

	json_object *vp8_frame_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_vp8_frame", &vp8_frame_obj);

	/* struct v4l2_vp8_segment segment */
	json_object *segment_obj;
	json_object_object_get_ex(vp8_frame_obj, "segment", &segment_obj);
	vp8_frame_pointer->segment = retrace_v4l2_vp8_segment(segment_obj);

	/* struct v4l2_vp8_loop_filter lf */
	json_object *lf_obj;
	json_object_object_get_ex(vp8_frame_obj, "lf", &lf_obj);
	vp8_frame_pointer->lf = retrace_v4l2_vp8_loop_filter(lf_obj);

	/* struct v4l2_vp8_quantization quant */
	json_object *quant_obj;
	json_object_object_get_ex(vp8_frame_obj, "quant", &quant_obj);
	vp8_frame_pointer->quant = retrace_v4l2_vp8_quantization(quant_obj);

	/* struct v4l2_vp8_entropy entropy */
	json_object *entropy_obj;
	json_object_object_get_ex(vp8_frame_obj, "entropy", &entropy_obj);
	vp8_frame_pointer->entropy = retrace_v4l2_vp8_entropy(entropy_obj);

	/* struct v4l2_vp8_entropy_coder_state coder_state */
	json_object *coder_state_obj;
	json_object_object_get_ex(vp8_frame_obj, "coder_state", &coder_state_obj);
	vp8_frame_pointer->coder_state = retrace_v4l2_vp8_entropy_coder_state(coder_state_obj);

	json_object *width_obj;
	json_object_object_get_ex(vp8_frame_obj, "width", &width_obj);
	vp8_frame_pointer->width = json_object_get_int(width_obj);

	json_object *height_obj;
	json_object_object_get_ex(vp8_frame_obj, "height", &height_obj);
	vp8_frame_pointer->height = json_object_get_int(height_obj);

	json_object *horizontal_scale_obj;
	json_object_object_get_ex(vp8_frame_obj, "horizontal_scale", &horizontal_scale_obj);
	vp8_frame_pointer->horizontal_scale = json_object_get_int(horizontal_scale_obj);

	json_object *vertical_scale_obj;
	json_object_object_get_ex(vp8_frame_obj, "vertical_scale", &vertical_scale_obj);
	vp8_frame_pointer->vertical_scale = json_object_get_int(vertical_scale_obj);

	json_object *version_obj;
	json_object_object_get_ex(vp8_frame_obj, "version", &version_obj);
	vp8_frame_pointer->version = json_object_get_int(version_obj);

	json_object *prob_skip_false_obj;
	json_object_object_get_ex(vp8_frame_obj, "prob_skip_false", &prob_skip_false_obj);
	vp8_frame_pointer->prob_skip_false = json_object_get_int(prob_skip_false_obj);

	json_object *prob_intra_obj;
	json_object_object_get_ex(vp8_frame_obj, "prob_intra", &prob_intra_obj);
	vp8_frame_pointer->prob_intra = json_object_get_int(prob_intra_obj);

	json_object *prob_last_obj;
	json_object_object_get_ex(vp8_frame_obj, "prob_last", &prob_last_obj);
	vp8_frame_pointer->prob_last = json_object_get_int(prob_last_obj);

	json_object *prob_gf_obj;
	json_object_object_get_ex(vp8_frame_obj, "prob_gf", &prob_gf_obj);
	vp8_frame_pointer->prob_gf = json_object_get_int(prob_gf_obj);

	json_object *num_dct_parts_obj;
	json_object_object_get_ex(vp8_frame_obj, "num_dct_parts", &num_dct_parts_obj);
	vp8_frame_pointer->num_dct_parts = json_object_get_int(num_dct_parts_obj);

	json_object *first_part_size_obj;
	json_object_object_get_ex(vp8_frame_obj, "first_part_size", &first_part_size_obj);
	vp8_frame_pointer->first_part_size = json_object_get_int(first_part_size_obj);

	json_object *first_part_header_bits_obj;
	json_object_object_get_ex(vp8_frame_obj, "first_part_header_bits", &first_part_header_bits_obj);
	vp8_frame_pointer->first_part_header_bits = json_object_get_int(first_part_header_bits_obj);

	/* __u32 dct_part_sizes[8] */
	json_object *dct_part_sizes_obj;
	json_object_object_get_ex(vp8_frame_obj, "dct_part_sizes", &dct_part_sizes_obj);
	for (int i = 0; i < 8; i++)
		vp8_frame_pointer->dct_part_sizes[i] = json_object_get_int(json_object_array_get_idx(dct_part_sizes_obj, i));

	json_object *last_frame_ts_obj;
	json_object_object_get_ex(vp8_frame_obj, "last_frame_ts", &last_frame_ts_obj);
	vp8_frame_pointer->last_frame_ts = json_object_get_int(last_frame_ts_obj);

	json_object *golden_frame_ts_obj;
	json_object_object_get_ex(vp8_frame_obj, "golden_frame_ts", &golden_frame_ts_obj);
	vp8_frame_pointer->golden_frame_ts = json_object_get_int(golden_frame_ts_obj);

	json_object *alt_frame_ts_obj;
	json_object_object_get_ex(vp8_frame_obj, "alt_frame_ts", &alt_frame_ts_obj);
	vp8_frame_pointer->alt_frame_ts = json_object_get_int(alt_frame_ts_obj);

	json_object *flags_obj;
	json_object_object_get_ex(vp8_frame_obj, "flags", &flags_obj);
	vp8_frame_pointer->flags = json_object_get_int(flags_obj);

	return vp8_frame_pointer;
}