#ifndef UTILS_H
#define UTILS_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "defines.h"

char* errors(t_err_code code);
void fatal_error(t_context* ctx, const char* message, const char* file, int line);
void clean_context(t_context* ctx);

void vector_try_init(t_context* ctx, t_vector* vec, bool is_shrink, void (*del)(void*));
void vector_try_add(t_context* ctx, t_vector* vec, void* item);
int try_open(t_context* ctx, const char* file, int o_flag, int p_flag);
ssize_t try_write(t_context* ctx, int fd, const char* src);
ssize_t try_write_endl(t_context* ctx, int fd, const char* src);
ssize_t try_read(t_context* ctx, int fd, char* buf, size_t n_bytes);

int try_gnl(t_context* ctx, int fd, char** line);

t_vec3 get_point(const t_ray* ray, float t);

bool timer(uint32_t time, uint32_t target);
uint32_t time_now(void);

float blue_noise(const t_texture* tex, const t_pixel* pixel, uint32_t dim);
t_vec2 r2_sequence(uint32_t n, t_vec2 offset);
float r1_sequence(uint32_t n, float offset);
t_vec2 r4_sequence_d12(uint32_t n, t_vec2 offset);

void* try_aligned_alloc(t_context* ctx, size_t alignment, size_t size);
void* try_malloc(t_context* ctx, size_t size);

t_ray new_ray(t_vec3 origin, t_vec3 dir);

void random_uv(const t_context* ctx, t_path* path, t_pixel* pixel, t_bn_channel c);

void wait_until(uint32_t end);
t_hit new_hit(uint32_t bounce);
void printf_init(void);

char* timestamp(char* buf);
float static_blue_noise(const t_texture* tex, const t_pixel* pixel, uint32_t dim);
uint32_t engine_time(void);

t_ray ray_world_to_object(const t_ray* ray, const t_mat4* world_to_object);
void hit_object_to_world(t_hit* hit, const t_transform* t);

void eval_hit_normal(const t_ray* ray, t_hit* hit, t_vec3 n);

void make_dir(t_context* ctx, const char* path);

bool is_wsl(void);
void open_image_viewer(const char* filepath);

void eval_hit_normal_geo(const t_ray* ray, t_hit* hit, t_vec3 geo_n, t_vec3 n);

bool vector_add(t_vector* vec, void* item);
bool vector_free(t_vector* vec, ...);
bool vector_del(t_vector* vec, size_t index);
void* vector_get(t_vector* vec, size_t index);
bool vector_init(t_vector* vec, bool is_shrink, void (*del)(void*));
bool vector_set(t_vector* vec, size_t index, void* item);
size_t vector_size(t_vector* vec);
size_t vector_total(t_vector* vec);
void* vector_getlast(t_vector* vec);
bool vector_insert(t_vector* vec, void* item, size_t index);
void vector_clean_items(t_vector* vec, void (*del)(void*));
bool vector_remove(t_vector* vec, void* item);
bool vector_del2(t_vector* vec, void* item);

ssize_t putstr_fd(const char* s, int fd);
ssize_t putendl_fd(const char* s, int fd);
size_t count_digits(long long n, const size_t len);
void int_to_str(int n, char* str);

#endif
