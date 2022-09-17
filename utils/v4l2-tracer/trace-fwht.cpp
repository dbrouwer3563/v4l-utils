/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "libtracer.h"

void trace_v4l2_ctrl_fwht_params(void *p_fwht_params, json_object *ctrl_obj)
{
	json_object *fwht_params_obj = json_object_new_object();
	struct v4l2_ctrl_fwht_params *fwht_params = static_cast<struct v4l2_ctrl_fwht_params*>(p_fwht_params);

	json_object_object_add(fwht_params_obj, "backward_ref_ts", json_object_new_uint64(fwht_params->backward_ref_ts));
	json_object_object_add(fwht_params_obj, "version", json_object_new_int64(fwht_params->version));
	json_object_object_add(fwht_params_obj, "width", json_object_new_int64(fwht_params->width));
	json_object_object_add(fwht_params_obj, "height", json_object_new_int64(fwht_params->height));
	json_object_object_add(fwht_params_obj, "flags", json_object_new_int64(fwht_params->flags));
	json_object_object_add(fwht_params_obj, "colorspace", json_object_new_int64(fwht_params->colorspace));
	json_object_object_add(fwht_params_obj, "xfer_func", json_object_new_int64(fwht_params->xfer_func));
	json_object_object_add(fwht_params_obj, "ycbcr_enc", json_object_new_int64(fwht_params->ycbcr_enc));
	json_object_object_add(fwht_params_obj, "quantization", json_object_new_int64(fwht_params->quantization));

	json_object_object_add(ctrl_obj, "v4l2_ctrl_fwht_params", fwht_params_obj);
}
