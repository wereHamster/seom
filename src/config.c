
#include <seom/seom.h>

static int seomConfigOption(const char *ns, const char *option, char *buffer, int length)
{
	char path[4096];

	snprintf(path, 4096, "%s/.seom/%s/%s", getenv("HOME"), ns, option);

	int fd = open(path, O_RDONLY);
	if (fd >= 0) {
		struct stat statBuffer;
		fstat(fd, &statBuffer);
		int size = statBuffer.st_size > length ? length : statBuffer.st_size;
		read(fd, buffer, size);
		buffer[size - 1] = 0;
		close(fd);
		return size;
	}
	
	return 0;
}

void seomConfigServer(const char *ns, char server[256])
{
	if (seomConfigOption(ns, "server", server, 256) == 0) {
		strncpy(server, "127.0.0.1 9000", 256);
	}
}

void seomConfigInterval(const char *ns, double *v)
{
	char interval[64];
	
	*v = 36000.0;
	
	if (seomConfigOption(ns, "interval", interval, 64) == 0) {
		return;
	}
	
	unsigned int ret = 0;
	int success = sscanf(interval, "%u", &ret);
	if (success) {
		*v = (double) ret;
	}
}

void seomConfigScale(const char *ns, char scale[64])
{
	if (seomConfigOption(ns, "scale", scale, 64) == 0) {
		strncpy(scale, "half", 64);
	}
}

void seomConfigInsets(const char *ns, uint32_t v[4])
{
	char insets[64];
	
	v[0] = v[1] = v[2] = v[3] = 0;
	
	if (seomConfigOption(ns, "insets", insets, 64) == 0) {
		return;
	}
	
	unsigned int ins[4];
	int success = sscanf(insets, "%u %u %u %u", &ins[0], &ins[1], &ins[2], &ins[3]);
	if (success) {
		v[0] = ins[0];
		v[1] = ins[1];
		v[2] = ins[2];
		v[3] = ins[3];
	}
}

seomConfig *seomConfigCreate()
{
	return NULL;
}
