/* moti0ntrails.c
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

#define MAX_NUM_FRAMES 60
#define DEFAULT_NUM_FRAMES 5
#define DEFAULT_STARTING_RATIO 0.5f
#define DEFAULT_DECAY_AMOUNT 0.5f


typedef struct moti0ntrails_instance
{
  unsigned int width;
  unsigned int height;

  uint32_t* previous_frames[MAX_NUM_FRAMES];

  double frame_ratios[MAX_NUM_FRAMES];
  unsigned int framebuffer_index;

  unsigned int num_frames;
  double starting_ratio;
  double decay_amount;
  short exponential_decay;
} moti0ntrails_instance_t;

/* Calculate the frame ratios. */
void update_ratios(moti0ntrails_instance_t *inst)
{
  unsigned int i = 0;
  double ratio = inst->starting_ratio;
  for(i = 0; i < inst->num_frames; ++i)
  {
    inst->frame_ratios[i] = ratio;
    if(inst->exponential_decay)
    {
      ratio *= (1.0f - inst->decay_amount);
    }
    else
    {
      ratio -= inst->decay_amount;
      if(ratio < 0.0f)
      {
        ratio = 0.0f;
      }
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
  info->name = "moti0ntrails";
  info->author = "Nik Reiman";
  info->plugin_type = F0R_PLUGIN_TYPE_FILTER;
  info->color_model = F0R_COLOR_MODEL_RGBA8888;
  info->frei0r_version = FREI0R_MAJOR_VERSION;
  info->major_version = 0;
  info->minor_version = 0;
  info->num_params = 4;
  info->explanation = "Add motion trails to video";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
  switch(param_index)
  {
  case 0:
    info->name = "Num frames";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Number of frames to use in the delay buffer";
    break;
  case 1:
    info->name = "Starting ratio";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Amount of transparency to apply to the first delayed frame";
    break;
  case 2:
    info->name = "Decay amount";
    info->type = F0R_PARAM_DOUBLE;
    info->explanation = "Amount to decrease transparency for each subsequent frame";
    break;
  case 3:
    info->name = "Decay type";
    info->type = F0R_PARAM_BOOL;
    info->explanation ="If true, use exponential decay, otherwise use linear decay";
    break;
  }
}

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
  moti0ntrails_instance_t* inst = (moti0ntrails_instance_t*)calloc(1, sizeof(*inst));
  inst->framebuffer_index = 0;
  inst->width = width;
  inst->height = height;
  for(unsigned int i = 0; i < MAX_NUM_FRAMES; ++i) {
    inst->previous_frames[i] = (uint32_t*)malloc(width * height * sizeof(uint32_t));
  }

  inst->num_frames = DEFAULT_NUM_FRAMES;
  inst->starting_ratio = DEFAULT_STARTING_RATIO;
  inst->decay_amount = DEFAULT_DECAY_AMOUNT;
  inst->exponential_decay = 0;
  update_ratios(inst);

  return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
  moti0ntrails_instance_t* inst = (moti0ntrails_instance_t*)instance;
  for(unsigned int i = 0; i < MAX_NUM_FRAMES; ++i)
  {
    free(inst->previous_frames[i]);
  }
  free(instance);
}

//stretch [0...1] to parameter range [min...max] linear
float map_value_forward(double v, float min, float max)
{
	return (float)(min + (max - min) * v);
}

//collapse from parameter range [min...max] to [0...1] linear
double map_value_backward(float v, float min, float max)
{
	return (v - min) / (max - min);
}

void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0ntrails_instance_t* inst = (moti0ntrails_instance_t*)instance;
  switch(param_index)
  {
  case 0:
    inst->num_frames = map_value_forward(*((double*)param), 0, MAX_NUM_FRAMES);
    update_ratios(instance);
    break;
  case 1:
    inst->starting_ratio = *((double*)param);
    update_ratios(instance);
    break;
  case 2:
    inst->decay_amount = *((double*)param);
    update_ratios(instance);
    break;
  case 3:
    inst->exponential_decay = map_value_forward(*((double*)param), 1.0, 0.0);//BOOL!!
    update_ratios(instance);
    break;
  }
}

void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
  assert(instance);
  moti0ntrails_instance_t* inst = (moti0ntrails_instance_t*)instance;

  switch(param_index)
  {
  case 0:
    *((double*)param) = map_value_backward(inst->num_frames, 0.0, MAX_NUM_FRAMES);
    break;
  case 1:
    *((double*)param) = inst->starting_ratio;
    break;
  case 2:
    *((double*)param) = inst->decay_amount;
    break;
  case 3:
    *((double*)param) = map_value_backward(inst->exponential_decay, 0.0, 1.0);//BOOL!!
    break;
  }
}

void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
  assert(instance);
  moti0ntrails_instance_t* inst = (moti0ntrails_instance_t*)instance;

  unsigned int frames_processed = 0;
  unsigned int frame_index = inst->framebuffer_index;
  double r, g, b, a = 0.0;
  double r_old, g_old, b_old, a_old = 0.0;
  double r_blend, g_blend, b_blend, a_blend = 0.0;
  const unsigned int frame_size = inst->width * inst->height;
  const size_t frame_bytes = frame_size * sizeof(uint32_t);
  memcpy(outframe, inframe, frame_bytes);
  for(frames_processed = 0; frames_processed < inst->num_frames; ++frames_processed)
  {
    double old_ratio = inst->frame_ratios[frames_processed];
    double new_ratio = 1.0f - old_ratio;

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
      b_blend = (b * new_ratio + b_old * old_ratio);
      g_blend = (g * new_ratio + g_old * old_ratio);
      r_blend = (r * new_ratio + r_old * old_ratio);

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
      frame_index = inst->num_frames - 1;
    }
  }

  if(++inst->framebuffer_index == inst->num_frames)
  {
    inst->framebuffer_index = 0;
  }

  memcpy(inst->previous_frames[inst->framebuffer_index], inframe, frame_bytes);
}
