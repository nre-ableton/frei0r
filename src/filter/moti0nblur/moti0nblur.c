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
#include <stdio.h>
#if defined(_MSC_VER)
#define _USE_MATH_DEFINES
#endif /* _MSC_VER */
#include <math.h>
#include <assert.h>
#include <string.h>

#include "frei0r.h"

#define NUM_FRAMES 10
#define STARTING_RATIO 0.5f

typedef struct moti0nblur_instance
{
  unsigned int width;
  unsigned int height;
  uint32_t* previous_frames[NUM_FRAMES];
  float frame_ratios[NUM_FRAMES];
  unsigned int framebuffer_index;
} moti0nblur_instance_t;

/* Calculate the frame ratios. */
void update_ratios(moti0nblur_instance_t *inst)
{
  unsigned int i = 0;
  float ratio = STARTING_RATIO;
  for(i = 0; i < NUM_FRAMES; ++i)
  {
    inst->frame_ratios[i] = ratio;
    printf("RATIO %d: %f\n", i, inst->frame_ratios[i]);
    ratio -= 0.05f;
    if(ratio < 0.0) {
      ratio = 0.0f;
    }
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
  info->explanation = "";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Name";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)calloc(1, sizeof(*inst));
  inst->framebuffer_index = 0;
  inst->width = width;
  inst->height = height;
  update_ratios(inst);
  for(unsigned int i = 0; i < NUM_FRAMES; ++i) {
    inst->previous_frames[i] = (uint32_t*)malloc(width * height * sizeof(uint32_t));
  }
  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;
  for(unsigned int i = 0; i < NUM_FRAMES; ++i) {
    free(inst->previous_frames[i]);
  }
  free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  moti0nblur_instance_t* inst = (moti0nblur_instance_t*)instance;

  unsigned int frames_processed = 0;
  unsigned int frame_index = inst->framebuffer_index;
  const unsigned char* pixel = 0;
  double r, g, b, a = 0.0;
  double r_old, g_old, b_old, a_old = 0.0;
  double r_blend, g_blend, b_blend, a_blend = 0.0;
  const unsigned int frame_size = inst->width * inst->height;
  memcpy(outframe, inframe, frame_size * sizeof(uint32_t));
  for(frames_processed = 0; frames_processed < NUM_FRAMES; ++frames_processed)
  {
    float old_ratio = inst->frame_ratios[frames_processed];
    float new_ratio = 1.0f - old_ratio;

    for(unsigned int i = 0; i < frame_size; ++i)
    {
      a = (double)((outframe[i] >> 24) & 0xff);
      b = (double)((outframe[i] >> 16) & 0xff);
      g = (double)((outframe[i] >> 8) & 0xff);
      r = (double)(outframe[i] & 0xff);

      const uint32_t* old_frame = inst->previous_frames[frame_index];
      a_old = (double)((old_frame[i] >> 24) & 0xff);
      b_old = (double)((old_frame[i] >> 16) & 0xff);
      g_old = (double)((old_frame[i] >> 8) & 0xff);
      r_old = (double)(old_frame[i] & 0xff);

      a_blend = (a * new_ratio + a_old * old_ratio);
      if (a_blend > 255.0) { a_blend = 255.0; }
      b_blend = (b * new_ratio + b_old * old_ratio);
      if (b_blend > 255.0) { b_blend = 255.0; }
      g_blend = (g * new_ratio + g_old * old_ratio);
      if (g_blend > 255.0) { g_blend = 255.0; }
      r_blend = (r * new_ratio + r_old * old_ratio);
      if (r_blend > 255.0) { r_blend = 255.0; }

      uint32_t out_pixel = (
        ((uint8_t)a_blend << 24) |
        ((uint8_t)b_blend << 16) |
        ((uint8_t)g_blend << 8) |
        (uint8_t)r_blend
      );
      outframe[i] = out_pixel;
    }

    if(frame_index-- == 0)
    {
      frame_index = NUM_FRAMES - 1;
    }
  }

  if(++inst->framebuffer_index == NUM_FRAMES)
  {
    inst->framebuffer_index = 0;
  }

  memcpy(inst->previous_frames[inst->framebuffer_index], inframe,
         frame_size * sizeof(uint32_t));
}
