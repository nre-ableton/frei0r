/* moti0nblur.c
 * Copyright (C) 2005 Jean-Sebastien Senecal (Drone)
 * This file is a Frei0r plugin.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <math.h>
#include <assert.h>
#include <string.h>

#include "frei0r.h"

#define NUM_FRAMES 4

typedef struct moti0nblur_instance
{
  uint32_t previous_frames[NUM_FRAMES];
  float frame_ratios[NUM_FRAMES];
  unsigned int ringbuffer_index;
} moti0nblur_instance_t;

/* Calculate the frame ratios. */
void update_ratios(moti0nblur_instance_t *inst)
{
  unsigned int i = 0;
  float current_ratio = 0.75f;
  for(i = 0; i < NUM_FRAMES; ++i)
  {
    inst->frame_ratios[i] = current_ratio;
    current_ratio = current_ratio / 2.0f;
  }
}

int f0r_init()
{
  return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* info)
{
  info->name = "Moti0nblur";
  info->author = "Nik Reiman";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0;
  info->minor_version = 0;
  info->num_params =  1;
  info->explanation = "Shifts the hue of a source image";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Hue";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "The shift value";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)calloc(1, sizeof(*inst));
  inst->width = width; inst->height = height;
  /* init transformation matrix */
  inst->hueshift = 0;
  update_ratios(inst);
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;

  switch(param_index)
  {
    int val;
  case 0:
    /* constrast */
    val = (int) (*((double*)param) * 360.0); /* remap to [0, 360] */
    if (val != inst->hueshift)
    {
      inst->hueshift = val;
      update_ratios(inst);
    }
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;
  
  switch(param_index)
  {
  case 0:
    *((double*)param) = (double) (inst->hueshift / 360.0);
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;
  unsigned int len = inst->width * inst->height;
  
  memcpy(outframe, inframe, len*sizeof(uint32_t));
  applymatrix((unsigned long*)outframe, inst->mat, len);
}

