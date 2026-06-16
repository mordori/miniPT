#include "utils.h"

ssize_t putstr_fd(const char* s, int fd) {
	ssize_t bytes;
	ssize_t totalbytes;
	size_t len;

	if (!s)
		return (ERROR);
	totalbytes = 0;
	len = strlen(s);
	while (len > 0) {
		bytes = write(fd, s, len);
		if (bytes < 0)
			return (ERROR);
		totalbytes += bytes;
		len -= bytes;
	}
	return (totalbytes);
}

ssize_t putendl_fd(const char* s, int fd) {
	ssize_t bytes;

	if (!s)
		return (ERROR);
	bytes = putstr_fd(s, fd);
	if (bytes == ERROR || write(fd, "\n", 1) == ERROR)
		return (ERROR);
	return (bytes + 1);
}

size_t count_digits(long long n, const size_t len) {
	size_t count;
	long long base;

	if (!len)
		return (ERROR);
	base = (long long)len;
	count = 1;
	n /= base;
	while (n) {
		++count;
		n /= base;
	}
	return (count);
}

void int_to_str(int n, char* str) {
	size_t i;
	long long num;

	num = n;
	if (n < 0)
		num = -num;
	i = count_digits(num, 10) + (n < 0);
	if (n == 0)
		str[0] = '0';
	str[i] = 0;
	while (i--) {
		str[i] = '0' + (num % 10);
		num /= 10;
	}
	if ((n < 0))
		str[0] = '-';
}
