#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>

#include <cstring>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

extern "C" {
#include "camera.h"
#include "defines.h"
#include "editing.h"
#include "lib_math.h"
#include "lights.h"
#include "materials.h"
#include "objects.h"
#include "rendering.h"
#include "ui.hpp"
#include "utils.h"
}

static bool g_ui_dirty = false;
static bool g_ui_interacting = false;

extern "C" bool ui_check_dirty(void) {
	return g_ui_dirty;
}

extern "C" bool ui_is_interacting(void) {
	return g_ui_interacting;
}

extern "C" bool ui_want_mouse(void) {
	return ImGui::GetIO().WantCaptureMouse;
}

extern "C" bool ui_want_keyboard(void) {
	return ImGui::GetIO().WantCaptureKeyboard;
}

static bool g_ui_transform_dirty = false;

extern "C" bool ui_check_transform_dirty(void) {
	return g_ui_transform_dirty;
}

static inline void apply_widget_state() {
	g_ui_dirty |= ImGui::IsItemDeactivatedAfterEdit();
	g_ui_interacting |= ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left);
}

void init_ui() {
	GLFWwindow* window = glfwGetCurrentContext();
	if (!window)
		return;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");
}

void render_ui(void* param) {
	t_context* ctx = (t_context*)param;
	t_renderer* r = &ctx->renderer;
	g_ui_dirty = false;
	g_ui_interacting = false;
	g_ui_transform_dirty = false;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	int ww, wh;
	glfwGetWindowSize(glfwGetCurrentContext(), &ww, &wh);
	ImGui::SetNextWindowPos(ImVec2(ww - 350, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(350, wh), ImGuiCond_Always);
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoNav;
	ImGui::Begin("Side Panel", NULL, window_flags);
	if (ImGui::BeginTabBar("SideBarTabs")) {
		ImGuiTabItemFlags tab_scene_flags = ImGuiTabItemFlags_None;
		if (ctx->editor.request_scene_tab) {
			tab_scene_flags |= ImGuiTabItemFlags_SetSelected;
			ctx->editor.request_scene_tab = false;
			ctx->editor.request_obj_tab = false;
		}
		if (ImGui::BeginTabItem("Scene", NULL, tab_scene_flags)) {
			ImGui::BeginDisabled();
			if (ImGui::CollapsingHeader("Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Spacing();
				ImGui::Spacing();

				const char* items[] = { "Empty", "Cornell Box" };
				static int current_item = 0;
				if (ImGui::BeginCombo("##scene_presets", items[current_item])) {
					for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
						bool is_selected = (current_item == n);
						if (ImGui::Selectable(items[n], is_selected)) {
							current_item = n;
							g_ui_dirty = true;
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::EndDisabled();

			if (ImGui::CollapsingHeader("Add Object", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Spacing();
				ImGui::Spacing();

				if (ImGui::BeginTable("AddObjectTable", 2)) {
					ImVec2 button_size(-FLT_MIN, 0.0f);

					ImGui::TableNextColumn();
					ImGui::BeginDisabled();
					if (ImGui::Button("Cube", button_size)) {}
					ImGui::EndDisabled();

					ImGui::TableNextColumn();
					if (ImGui::Button("Sphere", button_size)) {
						atomic_store(&r->render_cancel, true);
						pthread_mutex_lock(&r->mutex);
						while (r->threads_running)
							pthread_cond_wait(&r->cond, &r->mutex);

						deselect_object(ctx);
						add_sphere(ctx, 0, true);

						pthread_mutex_unlock(&r->mutex);
						g_ui_dirty = true;
					}

					ImGui::EndTable();
				}
			}
			ImGui::Spacing();

			// float refresh_width = ImGui::CalcTextSize("Refresh").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
			// float spacing = ImGui::GetStyle().ItemSpacing.x;

			if (ImGui::Button("Mesh...", ImVec2(-FLT_MIN, 0.0f)))
				ImGui::OpenPopup("MeshDropdown");

			// ImGui::SameLine();
			// if (ImGui::Button("Refresh", ImVec2(refresh_width, 0.0f))) {
			// 	// TODO: scan_for_new_meshes(ctx)
			// }

			if (ImGui::BeginPopup("MeshDropdown")) {
				if (ctx->loaded_mesh_count == 0 || !ctx->lib_mesh) {
					ImGui::TextDisabled("No meshes in library");
				} else {
					for (uint32_t i = 0; i < ctx->loaded_mesh_count; ++i) {
						if (ImGui::Selectable(ctx->lib_mesh[i].name)) {
							atomic_store(&r->render_cancel, true);
							pthread_mutex_lock(&r->mutex);
							while (r->threads_running)
								pthread_cond_wait(&r->cond, &r->mutex);

							deselect_object(ctx);
							add_mesh(ctx, ctx->lib_mesh[i].name, 0, true);

							pthread_mutex_unlock(&r->mutex);
							g_ui_dirty = true;
						}
					}
				}
				ImGui::EndPopup();
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Add Light", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Spacing();
				ImGui::Spacing();

				if (ImGui::BeginTable("AddLightTable", 2)) {
					ImVec2 button_size(-FLT_MIN, 0.0f);

					ImGui::TableNextColumn();
					if (ImGui::Button("Area", button_size)) {
						atomic_store(&r->render_cancel, true);
						pthread_mutex_lock(&r->mutex);
						while (r->threads_running)
							pthread_cond_wait(&r->cond, &r->mutex);

						deselect_object(ctx);
						add_light(ctx, 1, true);

						pthread_mutex_unlock(&r->mutex);
						g_ui_dirty = true;
					}
					ImGui::EndTable();
				}
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
				t_camera* cam = &ctx->scene.cam;
				ImGui::Spacing();
				ImGui::Spacing();
				ImGuiSliderFlags cam_flags = ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp;

				bool update = false;
				ImGui::Text("Position");
				if (ImGui::DragFloat3("##cam_pos", (float*)&cam->transform.pos, 0.005f, 0.0f, 0.0f, "%.2f")) {
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
						update = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
					update = true;
				g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

				ImGui::Text("Rotation");
				t_vec3 degrees = { //
					rad_to_degrees(-cam->transform.rot_euler.x),
					rad_to_degrees(cam->transform.rot_euler.y),
					0.0f
				};
				if (ImGui::DragFloat3("##cam_rot", (float*)&degrees, 0.05f, 0.0f, 0.0f, "%.2f")) {
					cam->transform.rot_euler = { //
						degrees_to_rad(-degrees.x),
						degrees_to_rad(degrees.y),
						0.0f
					};
					cam->transform.rot = quat_from_euler(cam->transform.rot_euler);
					cam->pitch = -cam->transform.rot_euler.x;
					cam->yaw = cam->transform.rot_euler.y;
					if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
						update = true;
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
					update = true;
				g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));

				if (update) {
					g_ui_transform_dirty = true;
					g_ui_dirty = true;
				}
				ImGui::Spacing();
				ImGui::Spacing();
				if (ImGui::BeginTable("AddObjectTable", 2)) {
					ImVec2 button_size(-FLT_MIN, 0.0f);

					ImGui::TableNextColumn();
					if (ImGui::Button("Reset", button_size)) {
						pthread_mutex_lock(&r->mutex);
						reset_camera(ctx);
						pthread_mutex_unlock(&r->mutex);
					}

					ImGui::TableNextColumn();
					if (ImGui::Button("Save", button_size))
						set_default_view(ctx);
					ImGui::EndTable();
				}
				ImGui::Spacing();
				ImGui::Spacing();

				if (ImGui::SliderFloat("Focal Length", &ctx->scene.cam.focal_len_mm, 14.0f, 200.0f, "%.1f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();

				if (ImGui::SliderFloat("Focus Dist", &ctx->scene.cam.focus_dist, 0.1f, 100.0f, "%.1f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();

				if (ImGui::SliderFloat("F-stop", &ctx->scene.cam.f_stop, 1.0f, 128.0f, "%.1f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();

				ImGui::BeginDisabled();
				if (ImGui::SliderFloat("Shutter Speed", &ctx->scene.cam.shutter_speed, 0.01f, 10000.0f, "%.2f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();

				if (ImGui::SliderFloat("ISO", &ctx->scene.cam.iso, 100.0f, 3000.0f, "%.0f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();
				ImGui::EndDisabled();

				ImGui::Spacing();
				ImGui::Spacing();
				if (ImGui::SliderFloat("Exposure", &ctx->scene.cam.exposure, 0.0f, 10.0f, "%.2f", cam_flags))
					g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
				apply_widget_state();
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Spacing();
				ImGui::Spacing();

				const char* items[] = { "Solid", "Gradient", "Image" };
				static uint32_t current_item = (uint32_t)ctx->scene.env.bg_mode;
				if (ImGui::BeginCombo("##env_presets", items[current_item])) {
					for (uint32_t n = 0; n < IM_ARRAYSIZE(items); n++) {
						bool is_selected = (current_item == n);
						if (ImGui::Selectable(items[n], is_selected)) {
							current_item = n;
							ctx->scene.env.bg_mode = (t_bg_mode)current_item;
							g_ui_dirty = true;
						}
						if (is_selected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("Show", &ctx->scene.env.show_background))
					g_ui_dirty = true;
				ImGui::Spacing();
				ImGui::Spacing();

				ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs;
				switch (ctx->scene.env.bg_mode) {
					case BG_SOLID: {
						g_ui_dirty |= ImGui::ColorEdit3("Color", ctx->scene.env.ambient.data, color_flags);
						apply_widget_state();
					} break;
					case BG_GRADIENT: {
						g_ui_dirty |= ImGui::ColorEdit3("Color 1", ctx->scene.env.gradient_1.data, color_flags);
						apply_widget_state();
						ImGui::SameLine(0, 30);
						g_ui_dirty |= ImGui::ColorEdit3("Color 2", ctx->scene.env.gradient_2.data, color_flags);
						apply_widget_state();
					} break;
					case BG_IMAGE: {
						ImGui::BeginDisabled();
						const char* current_tex = ctx->scene.env.skydome.pixels ? "sky.png" : "None";
						if (ImGui::BeginCombo("##skydome", current_tex)) {
							for (int i = 0; i < ctx->scene.assets.tex_count; i++) {
								if (!ctx->scene.assets.textures[i].loaded)
									continue;

								bool is_selected = false;
								if (ImGui::Selectable(ctx->scene.assets.textures[i].name, is_selected)) {
									ctx->scene.env.skydome = ctx->scene.assets.textures[i].texture;
									g_ui_dirty = true;
								}
							}
							ImGui::EndCombo();
						}
						ImGui::EndDisabled();
						static bool is_sun = ctx->scene.env.has_dir_light;
						if (is_sun) {
							ImGui::SameLine();
							if (ImGui::Checkbox("Sun", &ctx->scene.env.has_dir_light))
								g_ui_dirty = true;
						}
						float temp = ctx->scene.cam.skydome_uv_offset.u * 360.0f;
						if (ImGui::SliderFloat("Rotate", &temp, 0.0f, 360.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp)) {
							ctx->scene.cam.skydome_uv_offset.u = temp / 360.0f;
							if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
								rotate_skydome(ctx);
								g_ui_dirty = true;
							}
						}
						if (ImGui::IsItemDeactivatedAfterEdit()) {
							rotate_skydome(ctx);
							g_ui_dirty = true;
						}
						g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
					} break;
					default: break;
				}
			}
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::Spacing();

			if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Spacing();
				ImGui::Spacing();

				uint32_t min_samples = 1u, max_samples = 256u;
				uint32_t min_bounces = 1u, max_bounces = 16u;
				if (ImGui::SliderScalar(
						"Samples", ImGuiDataType_U32, &r->render_samples, &min_samples, &max_samples, "%u", ImGuiSliderFlags_Logarithmic)) {
					bool was_finished = r->frame >= r->render_samples && r->mode == RENDERED;
					if (r->render_samples > 256u) {
						r->render_samples = 256u;
					} else if (was_finished && r->frame < r->render_samples) {
						--r->frame;
						blit(ctx, r, false);
						++r->frame;
					}
				}
				g_ui_dirty |= ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &r->render_bounces, &min_bounces, &max_bounces);
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();
				ImGui::Spacing();

				float progress = 0.0f;
				char progress_text[64];
				if (r->mode == RENDERED) {
					progress = (float)r->frame / (float)r->render_samples;
					if (progress >= 1.0f)
						progress = 1.0f;
					if (r->render_complete) {
						float elapsed_time = (r->end_time - r->start_time) / 1000.0f;
						snprintf(progress_text, sizeof(progress_text), "Done! (%.1fs)", elapsed_time);
					} else {
						snprintf(progress_text, sizeof(progress_text), "Rendering %u / %u", r->frame, r->render_samples);
					}
				} else if (r->mode == PREVIEW) {
					snprintf(progress_text, sizeof(progress_text), "Preview Mode");
				} else {
					snprintf(progress_text, sizeof(progress_text), "Edit Mode");
				}
				if (r->render_complete) {
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
				} else if (r->mode != RENDERED) {
					progress = 1.0f;
					if (r->mode == SOLID)
						ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.2f, 0.8f, 1.0f));
				}
				ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), progress_text);
				static uint64_t save_time = 0;
				static char last_saved_file[256] = "";
				if (r->render_complete || r->mode == SOLID)
					ImGui::PopStyleColor();
				if (r->render_complete) {
					ImGui::Spacing();
					ImGui::Spacing();
					float button_width = 150.0f;
					float avail_width = ImGui::GetContentRegionAvail().x;
					float offset = (avail_width - button_width) * 0.5f;
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
					if (ImGui::Button("Save Render", ImVec2(button_width, 0.0f))) {
						screenshot(ctx, last_saved_file, sizeof(last_saved_file));
						open_image_viewer(last_saved_file);
						save_time = time_now();
					}
					if (save_time > 0 && (time_now() - save_time) < 1000) {
						ImGui::Spacing();
						const char* text = "Image saved!";
						float text_width = ImGui::CalcTextSize(text).x;
						float text_offset = (ImGui::GetContentRegionAvail().x - text_width) * 0.5f;
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + text_offset);
						ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s", text);
					}
				}
			}
			ImGui::EndTabItem();
		}

		bool is_selecting{ ctx->editor.selected_obj != NULL };
		if (is_selecting) {
			t_object* obj = ctx->editor.selected_obj;
			ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_None;
			if (ctx->editor.request_obj_tab) {
				tab_flags |= ImGuiTabItemFlags_SetSelected;
				ctx->editor.request_obj_tab = false;
			}

			static char tab_obj_name[64] = "Object";
			static t_object* last_obj = NULL;

			if (obj != last_obj) {
				last_obj = obj;
				const char* base = "Object";

				if (ctx->editor.is_selected_light) {
					base = "Light";
				} else {
					switch (obj->type) {
						case OBJ_MESH: base = obj->shape.mesh.name; break;
						case OBJ_SPHERE: base = "Sphere"; break;
						default: break;
					}
				}
				if (obj->id == 0)
					snprintf(tab_obj_name, sizeof(tab_obj_name), "%s", base);
				else
					snprintf(tab_obj_name, sizeof(tab_obj_name), "%s(%d)", base, obj->id);
			}

			if (ImGui::BeginTabItem(tab_obj_name, NULL, tab_flags)) {
				ImGui::Spacing();
				ImGui::Spacing();
				bool is_enabled = !(obj->flags & FLAG_OBJ_HIDDEN_SCENE);
				if (ImGui::Checkbox("Enabled", &is_enabled)) {
					if (is_enabled)
						obj->flags &= ~FLAG_OBJ_HIDDEN_SCENE;
					else
						obj->flags |= FLAG_OBJ_HIDDEN_SCENE;
					g_ui_dirty = true;
				}

				if (is_enabled) {
					ImGui::Spacing();
					ImGui::Spacing();
					uint32_t flags = obj->flags;
					const char* text_flags = "Flags...";
					if (flags == FLAG_OBJ_NONE)
						text_flags = "Everything";
					else if (flags == (FLAG_OBJ_HIDDEN_SCENE | FLAG_OBJ_HIDDEN_CAM | FLAG_OBJ_NO_CAST_SHADOW))
						text_flags = "None";
					else
						text_flags = "Mixed";

					if (is_enabled) {
						if (ImGui::BeginCombo("Flags", text_flags)) {
							bool is_visible = !(obj->flags & FLAG_OBJ_HIDDEN_CAM);
							if (ImGui::Checkbox("Visible", &is_visible)) {
								if (is_visible)
									obj->flags &= ~FLAG_OBJ_HIDDEN_CAM;
								else
									obj->flags |= FLAG_OBJ_HIDDEN_CAM;
								g_ui_dirty = true;
							}
							bool is_shadows = !(obj->flags & FLAG_OBJ_NO_CAST_SHADOW);
							if (ImGui::Checkbox("Cast Shadows", &is_shadows)) {
								if (is_shadows)
									obj->flags &= ~FLAG_OBJ_NO_CAST_SHADOW;
								else
									obj->flags |= FLAG_OBJ_NO_CAST_SHADOW;
								g_ui_dirty = true;
							}
							ImGui::EndCombo();
						}
					}

					ImGui::Spacing();
					ImGui::Spacing();
					if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Spacing();
						ImGui::Spacing();
						bool update = false;

						ImGui::Text("Position");
						if (ImGui::DragFloat3("##pos", (float*)&obj->transform.pos, 0.005f, 0.0f, 0.0f, "%.2f"))
							update |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
						if (ImGui::IsItemDeactivatedAfterEdit())
							update = true;
						g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
						ImGui::Text("Rotation");
						t_vec3 degrees = { //
							rad_to_degrees(obj->transform.rot_euler.x),
							rad_to_degrees(obj->transform.rot_euler.y),
							rad_to_degrees(obj->transform.rot_euler.z)
						};
						if (ImGui::DragFloat3("##rot", (float*)&degrees, 0.5f, 0.0f, 0.0f, "%.2f")) {
							obj->transform.rot_euler = { //
								degrees_to_rad(degrees.x),
								degrees_to_rad(degrees.y),
								degrees_to_rad(degrees.z)
							};
							obj->transform.rot = quat_from_euler(obj->transform.rot_euler);
							update |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
						}
						if (ImGui::IsItemDeactivatedAfterEdit())
							update = true;
						g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
						static bool is_uniform = false;
						ImGui::Text("Scale");
						if (ImGui::DragFloat3("##scale", (float*)&obj->transform.scale, 0.002f, 0.01f, FLT_MAX, "%.2f"))
							update |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
						if (ImGui::IsItemDeactivatedAfterEdit())
							update = true;
						g_ui_interacting |= (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left));
						ImGui::BeginDisabled();
						ImGui::SameLine();
						if (ImGui::Checkbox("Uniform", &is_uniform))
							g_ui_dirty = true;
						ImGui::EndDisabled();

						if (update) {
							g_ui_dirty = true;
							g_ui_transform_dirty = true;
						}
					}
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();

					ImGuiSliderFlags mat_flags = ImGuiSliderFlags_AlwaysClamp;
					if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
						ImGui::Spacing();
						ImGui::Spacing();

						char preview[64];
						if (obj->material_id == 0)
							snprintf(preview, sizeof(preview), "Material 0 (Default)");
						else if (obj->material_id == 1)
							snprintf(preview, sizeof(preview), "Material 1 (Default Emissive)");
						else
							snprintf(preview, sizeof(preview), "Material %u", obj->material_id);

						if (ImGui::BeginCombo("##mat_select", preview)) {
							for (uint32_t i = 0; i < ctx->scene.assets.materials.total; i++) {
								char label[64];
								if (i == 0)
									snprintf(label, sizeof(label), "Material 0 (Default)");
								else if (i == 1)
									snprintf(label, sizeof(label), "Material 1 (Default Emissive)");
								else
									snprintf(label, sizeof(label), "Material %u", i);

								bool is_selected = (obj->material_id == i);
								if (ImGui::Selectable(label, is_selected)) {
									obj->material_id = i;
									obj->mat = ((t_material**)ctx->scene.assets.materials.items)[i];
									g_ui_dirty = true;

									if (ctx->editor.is_selected_light) {
										for (uint32_t j = 0; j < ctx->scene.env.lights.total; ++j) {
											t_light* light = ((t_light**)ctx->scene.env.lights.items)[j];
											if (light->obj == obj)
												light->emission = obj->mat->emission;
										}
									}
								}
								if (is_selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::Separator();

							if (ImGui::Selectable("Create New...")) {
								t_material clone = *obj->mat;

								atomic_store(&r->render_cancel, true);
								pthread_mutex_lock(&r->mutex);
								while (r->threads_running)
									pthread_cond_wait(&r->cond, &r->mutex);

								new_material(ctx, &clone);
								obj->material_id = ctx->scene.assets.materials.total - 1;
								obj->mat = ((t_material**)ctx->scene.assets.materials.items)[obj->material_id];

								pthread_mutex_unlock(&r->mutex);
								g_ui_dirty = true;
							}
							ImGui::EndCombo();
						}

						bool is_default = obj->material_id == 0 || obj->material_id == 1;

						ImGui::BeginDisabled();
						if (!is_default) {
							ImGui::SameLine();
							if (ImGui::Button("Delete")) {
								// TODO: scan_for_new_meshes(ctx)
							}
						}
						ImGui::EndDisabled();

						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();

						if (is_default) {
							ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Default Materials are Read-Only");
							ImGui::BeginDisabled();
							ImGui::Spacing();
							ImGui::Spacing();
						}

						ImGuiColorEditFlags color_flags = ImGuiColorEditFlags_NoInputs;

						if (ImGui::Checkbox("Emissive", &obj->mat->is_emissive))
							g_ui_dirty = true;
						apply_widget_state();

						if (obj->mat->is_emissive) {
							ImGui::Spacing();
							ImGui::Spacing();
							if (ImGui::ColorEdit3("Color", obj->mat->emission_color.data, color_flags)) {
								obj->mat->emission = vec3_scale(obj->mat->emission_color, obj->mat->emission_strength);
								g_ui_dirty = true;

								for (uint32_t i = 0; i < ctx->scene.env.lights.total; ++i) {
									t_light* light = ((t_light**)ctx->scene.env.lights.items)[i];
									if (light->obj->material_id == obj->material_id)
										light->emission = obj->mat->emission;
								}
							}
							apply_widget_state();
							ImGui::Spacing();
							ImGui::Spacing();

							// clang-format off
							if (ImGui::SliderFloat("Strength", &obj->mat->emission_strength,
									0.0f, 1000.0f, "%.2f", mat_flags | ImGuiSliderFlags_Logarithmic)) {
								obj->mat->emission = vec3_scale(obj->mat->emission_color, obj->mat->emission_strength);
								g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);

								for (uint32_t i = 0; i < ctx->scene.env.lights.total; ++i) {
									t_light* light = ((t_light**)ctx->scene.env.lights.items)[i];
									if (light->obj->material_id == obj->material_id)
										light->emission = light->obj->mat->emission;
								}
							}
							apply_widget_state();
							// clang-format on
						} else {
							ImGui::Spacing();
							ImGui::Spacing();

							g_ui_dirty |= ImGui::ColorEdit3("Albedo", obj->mat->albedo.data, color_flags);
							apply_widget_state();

							ImGui::SameLine(0, 30);
							ImGui::BeginDisabled();
							if (ImGui::Checkbox("Texture", &obj->mat->is_texture))
								g_ui_dirty = true;
							apply_widget_state();

							if (obj->mat->is_texture) {
								// TODO: tex dropdown
								apply_widget_state();
							}
							ImGui::EndDisabled();

							ImGui::Spacing();
							ImGui::Spacing();

							float roughness = obj->mat->roughness;
							if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f, "%.2f", mat_flags)) {
								obj->mat->roughness = fmaxf(roughness, 0.045f);
								g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
							}
							apply_widget_state();

							if (ImGui::SliderFloat("Metallic", &obj->mat->metallic, 0.0f, 1.0f, "%.2f", mat_flags))
								g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
							apply_widget_state();

							if (ImGui::SliderFloat("IOR", &obj->mat->ior, 1.0f, 2.4f, "%.2f", mat_flags)) {
								obj->mat->f0_dielectric = vec3_n(reflectance(obj->mat->ior));
								g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
							}
							apply_widget_state();

							ImGui::BeginDisabled();
							float transmission = obj->mat->transmission;
							if (ImGui::SliderFloat("Transmission", &transmission, 1.0f, 2.4f, "%.2f", mat_flags)) {
								obj->mat->transmission = fmaxf(transmission, 1.0f);
								g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
							}
							apply_widget_state();
							ImGui::Spacing();
							ImGui::Spacing();
							ImGui::EndDisabled();

							ImGui::BeginDisabled();
							if (ImGui::Checkbox("Normal Map", &obj->mat->is_normal_map))
								g_ui_dirty = true;
							apply_widget_state();
							ImGui::Spacing();
							ImGui::Spacing();

							if (obj->mat->is_normal_map) {
								if (ImGui::SliderFloat("Strength", &obj->mat->normal_strength, 0.0f, 1.0f, "%.2f", mat_flags))
									g_ui_dirty |= ImGui::IsMouseDragging(ImGuiMouseButton_Left);
								apply_widget_state();
								ImGui::Spacing();
								ImGui::Spacing();
							}
							ImGui::EndDisabled();

							if (ImGui::Checkbox("Double-sided", &obj->mat->is_double_sided))
								g_ui_dirty = true;
							apply_widget_state();
							ImGui::Spacing();
							ImGui::Spacing();

							if (ImGui::Checkbox("Receive Shadows", &obj->mat->is_shadowed))
								g_ui_dirty = true;
							apply_widget_state();
							ImGui::Spacing();
							ImGui::Spacing();
						}

						if (is_default)
							ImGui::EndDisabled();
					}
				}
				ImGui::EndTabItem();
			}
		}
	}
	ImGui::EndTabBar();
	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(0, wh - UI_BOTTOM), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(ww - 350, UI_BOTTOM), ImGuiCond_Always);
	ImGuiWindowFlags bar_flags = 0;
	bar_flags |= ImGuiWindowFlags_NoTitleBar;
	bar_flags |= ImGuiWindowFlags_NoResize;
	bar_flags |= ImGuiWindowFlags_NoMove;
	bar_flags |= ImGuiWindowFlags_NoScrollbar;
	bar_flags |= ImGuiWindowFlags_NoSavedSettings;
	bar_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
	bar_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	ImGui::Begin("ActionBar", NULL, bar_flags);

	bool is_selecting = (ctx->editor.selected_obj != NULL);
	ImGui::TextDisabled("TAB:");
	ImGui::SameLine(0, 4);
	if (ctx->renderer.mode != SOLID)
		ImGui::Text("Edit Mode");
	else
		ImGui::Text("Render Mode");

	if (ctx->editor.mode != EDIT_DEFAULT) {
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("LMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Apply");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("RMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Cancel");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("X Y Z:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Axis Constraint");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("Shift+X Y Z:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Planar Constraint");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("V:");
		ImGui::SameLine(0, 4);
		ImGui::Text("View Constraint");
	} else {
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("LMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Select");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("RMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Look");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("Alt+LMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Orbit");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("Alt+RMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Zoom");
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("Alt+MMB:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Pan");
		if (is_selecting) {
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("T:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Translate");
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("R:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Rotate");
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("S:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Scale");
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("F:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Frame");
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("Shift+D:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Duplicate");
			ImGui::SameLine(0, 30);
			ImGui::TextDisabled("DEL:");
			ImGui::SameLine(0, 4);
			ImGui::Text("Delete");
			ImGui::SameLine(0, 30);
		}
		ImGui::SameLine(0, 30);
		ImGui::TextDisabled("ESC:");
		ImGui::SameLine(0, 4);
		ImGui::Text("Quit");
	}

	ImGui::SameLine(ImGui::GetWindowWidth() - 65.0f);
	ImGui::TextDisabled("%.1f FPS", ImGui::GetIO().Framerate);

	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void cleanup_ui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
