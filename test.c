
#include <seom/codec.h>

#define size 48

static void dump(const unsigned char *buffer)
{
	for (int i = 0; i < size / 8; ++i) {
		for (int j = 0; j < 8; ++j)
			fprintf(stderr, "0x%02x ", buffer[i * 8 + j]);
		fprintf(stderr, "\n"); 
	}
}

int main(int argc, char *argv[])
{
	static unsigned long long hashtable[4096];
	static unsigned char buffer[3][1024 * 1024 * 5 + 20000 * sizeof(void *)];

	srand((unsigned int)argv);

	for (int i = 0; i < 64 * 1; ++i)
		buffer[0][i] = (unsigned char) rand();

	const unsigned char *end = seomCodecEncode(buffer[1], buffer[0], size * 1, &hashtable);
	const unsigned char *end2 = seomCodecDecode(buffer[2], buffer[1], size * 1);

	for (int i = 0; i < size; ++i) {
		if (buffer[0][i] != buffer[2][i]) {
			fprintf(stderr, "%u -> %u\n", end - buffer[1], end2 - buffer[2]);
			fprintf(stderr, "0x%02x - 0x%02x\n", buffer[0][i], buffer[2][i]);
			fprintf(stderr, "<<<\n");
			dump(buffer[0]);
			fprintf(stderr, ">>>\n");
			dump(buffer[2]);
			return i;
		}
	}

	return 0;
}
