#include <stddef.h>

#include "defines.h"
#include "lib_math.h"
#include "lights.h"
#include "objects.h"
#include "scene.h"
#include "utils.h"

static inline void init_dir_light(t_context* ctx, t_light* light, t_object* obj);

void add_light(t_context* ctx, uint32_t mat_id, bool is_selected) {
	t_light* light = try_malloc(ctx, sizeof(t_light));

	t_material* mat = ((t_material**)ctx->scene.assets.materials.items)[mat_id];
	light->emission = mat->emission;
	light->max_radiance = MAX_RADIANCE;
	light->intensity = 1.0f;
	light->radius = 0.1f;
	light->radius_sq = light->radius * light->radius;

	t_object obj = { //
		.type = OBJ_SPHERE,
		.transform.pos = (t_vec3){ { 0.0f, 1.1f, 0.0f, 1.0f } },
		.shape.sphere.radius = light->radius,
		.shape.sphere.radius_sq = light->radius_sq,
		.material_id = mat_id,
		.flags = mat->flags
	};

	add_object(ctx, &obj, is_selected);

	if (is_selected) {
		light->obj = ctx->editor.selected_obj;
		ctx->editor.is_selected_light = true;
	} else {
		light->obj = (t_object*)vector_getlast(&ctx->scene.geo.objs);
	}

	vector_try_add(ctx, &ctx->scene.env.lights, light);
	light->idx = (uint32_t)ctx->scene.env.lights.total - 1;
	ctx->bn_stride = (BN_CO_U + ((ctx->scene.env.lights.total + 1) * 2) + 3) & ~3;
}

void dup_light(t_context* ctx, t_object* obj) {
	t_light* light = try_malloc(ctx, sizeof(t_light));

	for (uint32_t i = 0; i < ctx->scene.env.lights.total; ++i) {
		t_light* l = ((t_light**)ctx->scene.env.lights.items)[i];
		if (l->obj == obj) {
			*light = *l;
			break;
		}
	}

	add_object(ctx, obj, true);
	light->obj = ctx->editor.selected_obj;
	ctx->editor.is_selected_light = true;

	vector_try_add(ctx, &ctx->scene.env.lights, light);
	light->idx = (uint32_t)ctx->scene.env.lights.total - 1;
	ctx->bn_stride = (BN_CO_U + ((ctx->scene.env.lights.total + 1) * 2) + 3) & ~3;
}

void init_point_light(t_context* ctx, t_light* light, uint32_t mat_id, t_vec3 pos) {
	t_light* l = try_malloc(ctx, sizeof(*l));
	*l = *light;

	t_material* mat = ((t_material**)ctx->scene.assets.materials.items)[mat_id];
	l->emission = mat->emission;
	l->max_radiance = MAX_RADIANCE;
	l->radius_sq = l->radius * l->radius;

	t_object obj = { 0 };
	obj.type = OBJ_SPHERE;
	obj.shape.sphere.radius = l->radius;
	obj.shape.sphere.radius_sq = l->radius_sq;
	obj.material_id = mat_id;
	obj.transform.pos = pos;

	if (vec3_length(obj.transform.pos) > 500.0f) {
		init_dir_light(ctx, l, &obj);
		return;
	}

	add_object(ctx, &obj, false);

	l->obj = (t_object*)vector_getlast(&ctx->scene.geo.objs);
	vector_try_add(ctx, &ctx->scene.env.lights, l);
	l->idx = (uint32_t)ctx->scene.env.lights.total - 1;
}

static inline void init_dir_light(t_context* ctx, t_light* light, t_object* obj) {
	t_object* new_obj = try_malloc(ctx, sizeof(t_object));
	obj->transform.rot.w = 1.0f;
	obj->transform.scale = vec3_n(1.0f);
	update_transform(&obj->transform);
	update_bounds(obj);
	*new_obj = *obj;
	new_obj->mat = ((t_material**)ctx->scene.assets.materials.items)[obj->material_id];

	light->obj = new_obj;
	light->intensity = 4000000000000.0f;
	light->max_radiance = 2.0f;
	light->idx = 0;
	ctx->scene.cam.directional_light = *light;
	ctx->scene.env.has_dir_light = true;
	free(light);
}

void update_light_radius(t_context* ctx) {
	size_t i = 0;
	while (i < ctx->scene.env.lights.total) {
		t_light* light = ((t_light**)ctx->scene.env.lights.items)[i++];
		float max_scale = fminf(fminf(light->obj->transform.scale.x, light->obj->transform.scale.y), light->obj->transform.scale.z);
		light->radius = light->obj->shape.sphere.radius * max_scale;
		light->radius_sq = light->radius * light->radius;
	}
}
