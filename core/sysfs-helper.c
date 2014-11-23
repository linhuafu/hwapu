#include <stdio.h>
#include <string.h>

int sysfs_read(const char *path, char *buffer, size_t len)
{
	FILE *fp;
	int ret = -1;

	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;

	ret = fgets(buffer, len, fp);
	if (!ret)
		goto out;

	len = strlen(buffer);
	if (len > 0 && buffer[len - 1] == '\n') {
		len--;
		buffer[len] = '\0';
	}

	ret = len;
out:
	fclose(fp);
	return ret;
}

int sysfs_write(const char *path, const char *val)
{
	FILE *fp;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;

	fputs(val, fp);
	fclose(fp);

	return 0;
}

int sysfs_read_integer(const char *path, int *val)
{
	FILE *fp;
	int n;
	int ret = 0;

	fp = fopen(path, "r");
	if (fp == NULL) {
		*val = 0;
		return -1;
	}

	n = fscanf(fp, "%d", val);
	if (n != 1) {
		*val = 0;
		ret = -1;
	}

	fclose(fp);
	return ret;
}

int sysfs_write_integer(const char *path, int val)
{
	FILE *fp;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "%d", val);
	fclose(fp);

	return 0;
}
