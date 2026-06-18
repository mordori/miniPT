#include <stdbool.h>
#include <stdint.h>

#include "defines.h"
#include "editing.h"
#include "lib_math.h"
#include "lights.h"
#include "objects.h"
#include "scene.h"
#include "utils.h"

void add_object(t_context* ctx, t_object* obj, bool is_selected) {
	t_object* new_obj = try_malloc(ctx, sizeof(t_object));
	*new_obj = *obj;
	if (new_obj->transform.scale.x == 0.0f)
		new_obj->transform.scale = g_one;
	new_obj->transform.rot_euler = quat_to_euler(new_obj->transform.rot);
	update_transform(&new_obj->transform);
	update_bounds(new_obj);
	if (get_max_bounds_dim(new_obj) > WORLD_LIMIT) {
		new_obj->transform.scale = g_one;
		update_transform(&new_obj->transform);
		update_bounds(new_obj);
	}

	new_obj->mat = ((t_material**)ctx->scene.assets.materials.items)[new_obj->material_id];
	new_obj->flags = new_obj->mat->flags;

	if (new_obj->mat->is_emissive) {
		new_obj->id = ctx->scene.light_id_counter++;
	} else if (new_obj->type == OBJ_MESH) {
		uint32_t idx = 0;
		for (uint32_t i = 0; i < ctx->loaded_mesh_count; ++i) {
			if (strcmp(ctx->lib_mesh[i].name, new_obj->shape.mesh.name) == 0) {
				idx = i;
				break;
			}
		}
		new_obj->id = ctx->scene.mesh_id_counters[idx]++;
	} else {
		new_obj->id = ctx->scene.obj_id_counters[new_obj->type]++;
	}

	if (is_selected) {
		ctx->editor.selected_obj = new_obj;
		ctx->scene.cam.distance = fmaxf(vec3_dist(ctx->scene.cam.transform.pos, new_obj->transform.pos), 0.01f);
		ctx->editor.selection_time = engine_time();
		ctx->editor.request_obj_tab = true;
	} else {
		vector_try_add(ctx, &ctx->scene.geo.objs, new_obj);
		init_bvh(ctx);
	}
}

bool del_object(t_context* ctx) {
	t_object* obj = ctx->editor.selected_obj;
	if (!obj)
		return false;

	t_renderer* r = &ctx->renderer;
	atomic_store(&r->render_cancel, true);
	while (r->threads_running)
		pthread_cond_wait(&r->cond, &r->mutex);

	ctx->editor.selected_obj = NULL;
	if (ctx->editor.is_selected_light) {
		for (uint32_t i = 0; i < ctx->scene.env.lights.total; ++i) {
			t_light* light = ctx->scene.env.lights.items[i];
			if (light->obj == obj) {
				vector_del2(&ctx->scene.env.lights, light);
				for (uint32_t j = 0; j < ctx->scene.env.lights.total; ++j)
					((t_light**)ctx->scene.env.lights.items)[j]->idx = j;
				ctx->bn_stride = (BN_CO_U + ((ctx->scene.env.lights.total + 1) * 2) + 3) & ~3;
				break;
			}
		}
	}
	vector_del2(&ctx->scene.geo.objs, obj);
	return true;
}

bool dup_object(t_context* ctx) {
	t_object* obj = ctx->editor.selected_obj;
	if (!obj)
		return false;

	t_renderer* r = &ctx->renderer;
	atomic_store(&r->render_cancel, true);
	while (r->threads_running)
		pthread_cond_wait(&r->cond, &r->mutex);

	deselect_object(ctx);
	if (ctx->editor.is_selected_light)
		dup_light(ctx, obj);
	else
		add_object(ctx, obj, true);

	ctx->editor.orig_transform = ctx->editor.selected_obj->transform;
	begin_edit_action(ctx, EDIT_TRANSLATE);
	ctx->editor.mode = EDIT_TRANSLATE;
	return true;
}

bool hit_object(const t_object* obj, const t_ray* ray, t_hit* hit) {
	if (!obj)
		return false;

	const t_shape* shape = &obj->shape;
	t_ray r = ray_world_to_object(ray, &obj->transform.world_to_object);
	bool result = false;
	switch (obj->type) {
		case OBJ_SPHERE: result = hit_sphere(shape, &r, hit); break;
		case OBJ_MESH: result = hit_mesh(shape, &r, hit, obj->mat->is_double_sided); break;
	}
	if (result) {
		hit->obj = (t_object*)obj;
		hit_object_to_world(hit, &obj->transform);
	}
	return result;
}

void update_transform(t_transform* t) {
	t_mat4 translation = mat4_translate(t->pos);
	t_mat4 rotation = quat_to_mat4(t->rot);
	t_mat4 scale = mat4_scale(t->scale);

	t->object_to_world = mat4_mul(&translation, &rotation);
	t->object_to_world = mat4_mul(&t->object_to_world, &scale);

	mat4_inverse(&t->object_to_world, &t->world_to_object);
}
