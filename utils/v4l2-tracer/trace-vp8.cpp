/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

json_object *trace_v4l2_vp8_segment(struct v4l2_vp8_segment segment)
{
	json_object *vp8_segment_obj = json_object_new_object();

	/* __s8 quant_update[4] */
	json_object *quant_update_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(segment.quant_update); i++)
		json_object_array_add(quant_update_obj, json_object_new_int(segment.quant_update[i]));
	json_object_object_add(vp8_segment_obj, "quant_update", quant_update_obj);

	/* __s8 lf_update[4] */
	json_object *lf_update_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(segment.lf_update); i++)
		json_object_array_add(lf_update_obj, json_object_new_int(segment.lf_update[i]));
	json_object_object_add(vp8_segment_obj, "lf_update", lf_update_obj);

	/* __u8 segment_probs[3] */
	json_object *segment_probs_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(segment.segment_probs); i++)
		json_object_array_add(segment_probs_obj, json_object_new_int(segment.segment_probs[i]));
	json_object_object_add(vp8_segment_obj, "segment_probs", segment_probs_obj);

	json_object_object_add(vp8_segment_obj, "padding", json_object_new_int(segment.padding));
	json_object_object_add(vp8_segment_obj, "flags", json_object_new_uint64(segment.flags));
	json_object_object_add(vp8_segment_obj, "flags_str",
						   json_object_new_string(vp8_segment_flag2s(segment.flags).c_str()));

	return vp8_segment_obj;
}

json_object *trace_v4l2_vp8_loop_filter(struct v4l2_vp8_loop_filter lf)
{
	json_object *vp8_loop_filter_obj = json_object_new_object();

	/* 	__s8 ref_frm_delta[4] */
	json_object *ref_frm_delta_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(lf.ref_frm_delta); i++)
		json_object_array_add(ref_frm_delta_obj, json_object_new_uint64(lf.ref_frm_delta[i]));
	json_object_object_add(vp8_loop_filter_obj, "ref_frm_delta", ref_frm_delta_obj);

	/* 	__s8 mb_mode_delta[4] */
	json_object *mb_mode_delta_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(lf.mb_mode_delta); i++)
		json_object_array_add(mb_mode_delta_obj, json_object_new_int(lf.mb_mode_delta[i]));
	json_object_object_add(vp8_loop_filter_obj, "mb_mode_delta", mb_mode_delta_obj);

	json_object_object_add(vp8_loop_filter_obj, "sharpness_level", json_object_new_int(lf.sharpness_level));
	json_object_object_add(vp8_loop_filter_obj, "level", json_object_new_int(lf.level));
	json_object_object_add(vp8_loop_filter_obj, "padding", json_object_new_int(lf.padding));
	json_object_object_add(vp8_loop_filter_obj, "flags", json_object_new_uint64(lf.flags));
	json_object_object_add(vp8_loop_filter_obj, "flags_str", json_object_new_string(vp8_loop_filter_flag2s(lf.flags).c_str()));

	return vp8_loop_filter_obj;
}

json_object *trace_v4l2_vp8_quantization(struct v4l2_vp8_quantization quant)
{
	json_object *vp8_quantization_obj = json_object_new_object();

	json_object_object_add(vp8_quantization_obj, "y_ac_qi", json_object_new_int(quant.y_ac_qi));
	json_object_object_add(vp8_quantization_obj, "y_dc_delta", json_object_new_int(quant.y_dc_delta));
	json_object_object_add(vp8_quantization_obj, "y2_dc_delta", json_object_new_int(quant.y2_dc_delta));
	json_object_object_add(vp8_quantization_obj, "y2_ac_delta", json_object_new_int(quant.y2_ac_delta));
	json_object_object_add(vp8_quantization_obj, "uv_dc_delta", json_object_new_int(quant.uv_dc_delta));
	json_object_object_add(vp8_quantization_obj, "uv_ac_delta", json_object_new_int(quant.uv_ac_delta));
	json_object_object_add(vp8_quantization_obj, "padding", json_object_new_int(quant.padding));

	return vp8_quantization_obj;
}

json_object *trace_v4l2_vp8_entropy(struct v4l2_vp8_entropy entropy)
{
	json_object *vp8_entropy_obj = json_object_new_object();

	/*__u8 coeff_probs[4][8][3][V4L2_VP8_COEFF_PROB_CNT] */
	json_object *coeff_probs_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(entropy.coeff_probs); i++)
		for (size_t j = 0; j < ARRAY_SIZE(entropy.coeff_probs[0]); j++)
			for (size_t k = 0; k < ARRAY_SIZE(entropy.coeff_probs[0][0]); k++)
				for (size_t l = 0; l < V4L2_VP8_COEFF_PROB_CNT; l++)
					json_object_array_add(coeff_probs_obj,
					                      json_object_new_int(entropy.coeff_probs[i][j][k][l]));
	json_object_object_add(vp8_entropy_obj, "coeff_probs", coeff_probs_obj);

	/* __u8 y_mode_probs[4] */
	json_object *y_mode_probs_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(entropy.y_mode_probs); i++)
		json_object_array_add(y_mode_probs_obj, json_object_new_int(entropy.y_mode_probs[i]));
	json_object_object_add(vp8_entropy_obj, "y_mode_probs", y_mode_probs_obj);

	/* __u8 uv_mode_probs[3] */
	json_object *uv_mode_probs_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(entropy.uv_mode_probs); i++)
		json_object_array_add(uv_mode_probs_obj, json_object_new_int(entropy.uv_mode_probs[i]));
	json_object_object_add(vp8_entropy_obj, "uv_mode_probs", uv_mode_probs_obj);

	/* __u8 mv_probs[2][V4L2_VP8_MV_PROB_CNT] */
	json_object *mv_probs_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(entropy.mv_probs); i++)
		for (size_t j = 0; j < V4L2_VP8_MV_PROB_CNT; j++)
			json_object_array_add(mv_probs_obj, json_object_new_int(entropy.mv_probs[i][j]));
	json_object_object_add(vp8_entropy_obj, "mv_probs", mv_probs_obj);

	/*__u8 padding[3] */
	json_object *padding_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(entropy.padding); i++)
		json_object_array_add(padding_obj, json_object_new_int(entropy.padding[i]));
	json_object_object_add(vp8_entropy_obj, "padding", padding_obj);

	return vp8_entropy_obj;
}

json_object *trace_v4l2_vp8_entropy_coder_state(struct v4l2_vp8_entropy_coder_state coder_state)
{
	json_object *vp8_entropy_coder_state_obj = json_object_new_object();

	json_object_object_add(vp8_entropy_coder_state_obj, "range", json_object_new_int(coder_state.range));
	json_object_object_add(vp8_entropy_coder_state_obj, "value", json_object_new_int(coder_state.value));
	json_object_object_add(vp8_entropy_coder_state_obj, "bit_count", json_object_new_int(coder_state.bit_count));
	json_object_object_add(vp8_entropy_coder_state_obj, "padding", json_object_new_int(coder_state.padding));

	return vp8_entropy_coder_state_obj;
}

void trace_v4l2_ctrl_vp8_frame(void *p_vp8_frame, json_object *ctrl_obj)
{
	json_object *vp8_frame_obj = json_object_new_object();
	struct v4l2_ctrl_vp8_frame *vp8_frame = static_cast<struct v4l2_ctrl_vp8_frame*>(p_vp8_frame);

	/* struct v4l2_vp8_segment segment */
	json_object *v4l2_vp8_segment_obj = trace_v4l2_vp8_segment(vp8_frame->segment);
	json_object_object_add(vp8_frame_obj, "segment", v4l2_vp8_segment_obj);

	/* struct v4l2_vp8_loop_filter lf */
	json_object *v4l2_vp8_loop_filter_obj = trace_v4l2_vp8_loop_filter(vp8_frame->lf);
	json_object_object_add(vp8_frame_obj, "lf", v4l2_vp8_loop_filter_obj);

	/* struct v4l2_vp8_quantization quant */
	json_object *v4l2_vp8_quantization_obj = trace_v4l2_vp8_quantization(vp8_frame->quant);
	json_object_object_add(vp8_frame_obj, "quant", v4l2_vp8_quantization_obj);

	/* struct v4l2_vp8_entropy entropy */
	json_object *v4l2_vp8_entropy_obj = trace_v4l2_vp8_entropy(vp8_frame->entropy);
	json_object_object_add(vp8_frame_obj, "entropy", v4l2_vp8_entropy_obj);

	/* struct v4l2_vp8_entropy_coder_state coder_state */
	json_object *v4l2_vp8_entropy_coder_state_obj = trace_v4l2_vp8_entropy_coder_state(vp8_frame->coder_state);
	json_object_object_add(vp8_frame_obj, "coder_state", v4l2_vp8_entropy_coder_state_obj);

	json_object_object_add(vp8_frame_obj, "width", json_object_new_int(vp8_frame->width));
	json_object_object_add(vp8_frame_obj, "height", json_object_new_int(vp8_frame->height));
	json_object_object_add(vp8_frame_obj, "horizontal_scale", json_object_new_int(vp8_frame->horizontal_scale));
	json_object_object_add(vp8_frame_obj, "vertical_scale", json_object_new_int(vp8_frame->vertical_scale));
	json_object_object_add(vp8_frame_obj, "version", json_object_new_int(vp8_frame->version));
	json_object_object_add(vp8_frame_obj, "prob_skip_false", json_object_new_int(vp8_frame->prob_skip_false));
	json_object_object_add(vp8_frame_obj, "prob_intra", json_object_new_int(vp8_frame->prob_intra));
	json_object_object_add(vp8_frame_obj, "prob_last", json_object_new_int(vp8_frame->prob_last));
	json_object_object_add(vp8_frame_obj, "prob_gf", json_object_new_int(vp8_frame->prob_gf));
	json_object_object_add(vp8_frame_obj, "num_dct_parts", json_object_new_int(vp8_frame->num_dct_parts));
	json_object_object_add(vp8_frame_obj, "first_part_size", json_object_new_uint64(vp8_frame->first_part_size));
	json_object_object_add(vp8_frame_obj, "first_part_header_bits", json_object_new_uint64(vp8_frame->first_part_header_bits));

	/* __u32 dct_part_sizes[8] */
	json_object *dct_part_sizes_obj = json_object_new_array();
	for (size_t i = 0; i < ARRAY_SIZE(vp8_frame->dct_part_sizes); i++)
		json_object_array_add(dct_part_sizes_obj, json_object_new_uint64(vp8_frame->dct_part_sizes[i]));
	json_object_object_add(vp8_frame_obj, "dct_part_sizes", dct_part_sizes_obj);

	json_object_object_add(vp8_frame_obj, "last_frame_ts", json_object_new_uint64(vp8_frame->last_frame_ts));
	json_object_object_add(vp8_frame_obj, "golden_frame_ts", json_object_new_uint64(vp8_frame->golden_frame_ts));
	json_object_object_add(vp8_frame_obj, "alt_frame_ts", json_object_new_uint64(vp8_frame->alt_frame_ts));
	json_object_object_add(vp8_frame_obj, "flags", json_object_new_uint64(vp8_frame->flags));
	json_object_object_add(vp8_frame_obj, "flags_str", json_object_new_string(vp8_frame_flag2s(vp8_frame->flags).c_str()));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_vp8_frame", vp8_frame_obj);
}
