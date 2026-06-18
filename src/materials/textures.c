#include <stdint.h>

#include "MLX42.h"
#include "defines.h"
#include "lib_math.h"
#include "materials.h"
#include "utils.h"

static inline void tex_to_linear(t_texture* texture, uint8_t* src, bool is_srgb);

static float g_lut[256];

t_texture load_texture(t_context* ctx, char* file, bool is_srgb) {
	printf("\033[1;33mLoading texture:    %s\033[0m\n", file);
	t_texture tex = (t_texture){ 0 };
	mlx_texture_t* mlx_tex = mlx_load_png(file);
	if (!mlx_tex)
		fatal_error(ctx, errors(ERR_TEX), __FILE__, __LINE__);

	if (!is_pot(mlx_tex->width) || !is_pot(mlx_tex->height))
		fatal_error(ctx, errors(ERR_TEXNPOT), __FILE__, __LINE__);

	size_t size = (sizeof(float) * mlx_tex->width * mlx_tex->height * 4);
	tex.pixels = try_aligned_alloc(ctx, 64, size);
	tex.width = mlx_tex->width;
	tex.height = mlx_tex->height;
	tex_to_linear(&tex, mlx_tex->pixels, is_srgb);
	mlx_delete_texture(mlx_tex);
	return tex;
}

static inline void tex_to_linear(t_texture* texture, uint8_t* src, bool is_srgb) {
	float* dst = texture->pixels;
	float* end = dst + (texture->width * texture->height * 4);
	if (is_srgb) {
		while (dst < end) {
			*dst++ = g_lut[*src++];
			*dst++ = g_lut[*src++];
			*dst++ = g_lut[*src++];
			*dst++ = (((float)(*src++)) * M_1_255f);
		}
	} else {
		while (dst < end)
			*dst++ = (((float)(*src++)) * M_1_255f);
	}
}

void lut_srgb_to_linear(void) {
	static bool init_lut;

	if (!init_lut) {
		size_t i = 0;
		while (i < 256) {
			g_lut[i] = powf((float)i * M_1_255f, 2.2f);
			++i;
		}
		init_lut = true;
	}
}

void free_texture(t_texture* texture) {
	if (texture->pixels)
		free(texture->pixels);
}

t_vec3 sample_texture(const t_texture* tex, t_vec2 uv) {
	uv.u = uv.u - floorf(uv.u);
	uv.v = uv.v - floorf(uv.v);
	t_vec3 color;
	uint32_t x = (uint32_t)(uv.u * (float)tex->width) % tex->width;
	uint32_t y = (uint32_t)(uv.v * (float)tex->height) % tex->height;
	uint32_t idx = (uint32_t)(y * tex->width + x) * 4;
	memcpy(&color, &tex->pixels[idx], sizeof(t_vec3));
	return color;
}

t_vec3 get_surface_color(const t_material* mat, const t_hit* hit) {
	if (mat->is_texture && mat->texture.pixels)
		return sample_texture(&mat->texture, hit->uv);
	return mat->albedo;
}
