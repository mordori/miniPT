#include "utils.h"

void fatal_error(t_context* ctx, const char* msg, const char* file, int line) {
	char str[INT32_LENGTH + 1];

	putstr_fd("\nError:\n", STDERR_FILENO);
	if (msg)
		putendl_fd(msg, STDERR_FILENO);
	if (file)
		putstr_fd(file, STDERR_FILENO);
	if (ctx && line > 0) {
		putstr_fd(", line: ", STDERR_FILENO);
		int_to_str(line, str);
		putendl_fd(str, STDERR_FILENO);
	}
	if (ctx && ctx->mlx) {
		putstr_fd("MLX42: ", STDERR_FILENO);
		perror(mlx_strerror(mlx_errno));
	}
	exit(EXIT_FAILURE);
}

char* errors(t_err_code code) {
	// clang-format off
	static char* e[] = {
		"invalid number of arguments",
		"mlx init failed",
		"mlx img failed",
		"vector init failed",
		"vector add failed",
		"resize failed",
		"write: src is longer than SSIZE_MAX",
		"write file failed",
		"open file failed",
		"read file failed",
		"GNL failed",
		"texture load failed",
		"invalid entity type",
		"renderer init failed",
		"object add failed",
		"point light add failed",
		"bvh failed",
		"material add failed",
		"NPOT texture",
		"make dir failed",
		"malloc failed",
		"could not open directory",
	};
	// clang-format on
	return e[code];
}
