#ifndef LIGHTS_H
#define LIGHTS_H

#include "defines.h"

void init_point_light(t_context* ctx, t_light* light, uint32_t mat_id, t_vec3 pos);
void update_light_radius(t_context* ctx);
void add_light(t_context* ctx, uint32_t mat_id, bool is_selected);
void dup_light(t_context* ctx, t_object* obj);

#endif
