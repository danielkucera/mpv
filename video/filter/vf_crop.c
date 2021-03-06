/*
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "common/msg.h"
#include "options/options.h"

#include "video/img_format.h"
#include "video/mp_image.h"
#include "vf.h"

#include "options/m_option.h"

#include "config.h"
#if !HAVE_GPL
#error GPL only
#endif

static const struct vf_priv_s {
    int crop_w,crop_h;
    int crop_x,crop_y;
} vf_priv_dflt = {
  -1,-1,
  -1,-1
};

//===========================================================================//

static int reconfig(struct vf_instance *vf, struct mp_image_params *in,
                    struct mp_image_params *out)
{
    int width = in->w, height = in->h;

    // calculate the missing parameters:
    if(vf->priv->crop_w<=0 || vf->priv->crop_w>width) vf->priv->crop_w=width;
    if(vf->priv->crop_h<=0 || vf->priv->crop_h>height) vf->priv->crop_h=height;
    if(vf->priv->crop_x<0) vf->priv->crop_x=(width-vf->priv->crop_w)/2;
    if(vf->priv->crop_y<0) vf->priv->crop_y=(height-vf->priv->crop_h)/2;
    // rounding:

    int orig_x = vf->priv->crop_x;
    int orig_y = vf->priv->crop_y;

    struct mp_imgfmt_desc fmt = mp_imgfmt_get_desc(in->imgfmt);

    if (fmt.flags & MP_IMGFLAG_HWACCEL) {
        vf->priv->crop_x = 0;
        vf->priv->crop_y = 0;
    } else {
        vf->priv->crop_x = MP_ALIGN_DOWN(vf->priv->crop_x, fmt.align_x);
        vf->priv->crop_y = MP_ALIGN_DOWN(vf->priv->crop_y, fmt.align_y);
    }

    if (vf->priv->crop_x != orig_x || vf->priv->crop_y != orig_y) {
        MP_WARN(vf, "Adjusting crop origin to %d/%d for pixel format alignment.\n",
                vf->priv->crop_x, vf->priv->crop_y);
    }

    // check:
    if(vf->priv->crop_w+vf->priv->crop_x>width ||
       vf->priv->crop_h+vf->priv->crop_y>height){
        MP_WARN(vf, "Bad position/width/height - cropped area outside of the original!\n");
        return -1;
    }

    *out = *in;
    out->w = vf->priv->crop_w;
    out->h = vf->priv->crop_h;
    return 0;
}

static struct mp_image *filter(struct vf_instance *vf, struct mp_image *mpi)
{
    if (mpi->fmt.flags & MP_IMGFLAG_HWACCEL) {
        mp_image_set_size(mpi, vf->fmt_out.w, vf->fmt_out.h);
    } else {
        mp_image_crop(mpi, vf->priv->crop_x, vf->priv->crop_y,
                      vf->priv->crop_x + vf->priv->crop_w,
                      vf->priv->crop_y + vf->priv->crop_h);
    }
    return mpi;
}

static int query_format(struct vf_instance *vf, unsigned int fmt)
{
    return vf_next_query_format(vf, fmt);
}

static int vf_open(vf_instance_t *vf){
    MP_WARN(vf, "This filter is deprecated. Use lavfi crop instead.\n");
    vf->reconfig=reconfig;
    vf->filter=filter;
    vf->query_format=query_format;
    return 1;
}

#define OPT_BASE_STRUCT struct vf_priv_s
static const m_option_t vf_opts_fields[] = {
    OPT_INT("w", crop_w, M_OPT_MIN, .min = 0),
    OPT_INT("h", crop_h, M_OPT_MIN, .min = 0),
    OPT_INT("x", crop_x, M_OPT_MIN, .min = -1),
    OPT_INT("y", crop_y, M_OPT_MIN, .min = -1),
    {0}
};

const vf_info_t vf_info_crop = {
    .description = "cropping",
    .name = "crop",
    .open = vf_open,
    .priv_size = sizeof(struct vf_priv_s),
    .priv_defaults = &vf_priv_dflt,
    .options = vf_opts_fields,
};

//===========================================================================//
