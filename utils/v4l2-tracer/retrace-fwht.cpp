/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2022 Collabora Ltd.
 */

#include "v4l2-tracer.h"

struct v4l2_ctrl_fwht_params *retrace_v4l2_ctrl_fwht_params(json_object *ctrl_obj)
{
	struct v4l2_ctrl_fwht_params *fwht_params;
	fwht_params = (struct v4l2_ctrl_fwht_params *) malloc(sizeof(v4l2_ctrl_fwht_params));
	memset(fwht_params, 0, sizeof(v4l2_ctrl_fwht_params));

	json_object *fwht_params_obj;
	json_object_object_get_ex(ctrl_obj, "v4l2_ctrl_fwht_params", &fwht_params_obj);

	json_object *backward_ref_ts_obj;
	json_object_object_get_ex(fwht_params_obj, "backward_ref_ts", &backward_ref_ts_obj);
	fwht_params->backward_ref_ts = json_object_get_uint64(backward_ref_ts_obj);

	json_object *version_obj;
	json_object_object_get_ex(fwht_params_obj, "version", &version_obj);
	fwht_params->version = json_object_get_int(version_obj);

	json_object *width_obj;
	json_object_object_get_ex(fwht_params_obj, "width", &width_obj);
	fwht_params->width = json_object_get_int(width_obj);

	json_object *height_obj;
	json_object_object_get_ex(fwht_params_obj, "height", &height_obj);
	fwht_params->height = json_object_get_int(height_obj);

	json_object *flags_obj;
	json_object_object_get_ex(fwht_params_obj, "flags", &flags_obj);
	fwht_params->flags = json_object_get_int(flags_obj);

	json_object *colorspace_obj;
	json_object_object_get_ex(fwht_params_obj, "colorspace", &colorspace_obj);
	fwht_params->colorspace = json_object_get_int(colorspace_obj);

	json_object *xfer_func_obj;
	json_object_object_get_ex(fwht_params_obj, "xfer_func", &xfer_func_obj);
	fwht_params->xfer_func = json_object_get_int(xfer_func_obj);

	json_object *ycbcr_enc_obj;
	json_object_object_get_ex(fwht_params_obj, "ycbcr_enc", &ycbcr_enc_obj);
	fwht_params->ycbcr_enc = json_object_get_int(ycbcr_enc_obj);

	json_object *quantization_obj;
	json_object_object_get_ex(fwht_params_obj, "quantization", &quantization_obj);
	fwht_params->quantization = json_object_get_int(quantization_obj);

	return fwht_params;
}
