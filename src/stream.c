
#include <seom/stream.h>

struct seomStream *seomStreamCreate(int fileDescriptor)
{
	struct seomStream *stream = malloc(sizeof(struct seomStream));
	if (stream == NULL)
		return NULL;

	stream->fileDescriptor = fileDescriptor;

	stream->buffer.size = 0;
	stream->buffer.data = NULL;

	return stream;
}

void seomStreamPut(struct seomStream *stream, struct seomPacket *packet)
{
	if (packet->size > stream->buffer.size) {
		void *data = realloc(stream->buffer.data, packet->size * 2 + 4096);
		if (data == NULL)
			goto out;

		stream->buffer.size = packet->size * 2 + 4096;
		stream->buffer.data = data;
	}

	const void *end = seomCodecEncode(stream->buffer.data, seomPacketPayload(packet), packet->size);
	uint64_t size = end - stream->buffer.data;

	const struct iovec vec[] = {
		{ packet, sizeof(struct seomPacket) },
		{ &size, sizeof(uint64_t) },
		{ stream->buffer.data, end - stream->buffer.data },
	};

	writev(stream->fileDescriptor, vec, 3);

out:
	seomPacketDestroy(packet);
}

struct seomPacket *seomStreamGet(struct seomStream *stream)
{
	struct seomPacket header;
	uint64_t size;

	const struct iovec vec[] = {
		{ &header, sizeof(struct seomPacket) },
		{ &size, sizeof(uint64_t) },
	};

	if (readv(stream->fileDescriptor, vec, 2) < (ssize_t) (sizeof(struct seomPacket) + sizeof(uint64_t)))
		return NULL;
	
	if (read(stream->fileDescriptor, stream->buffer.data, size) < (ssize_t) size)
		return NULL;

	struct seomPacket *packet = seomPacketCreate(header.type, header.size);
	if (packet == NULL)
		return NULL;

	memcpy(packet, &header, sizeof(struct seomPacket));
	seomCodecDecode(seomPacketPayload(packet), stream->buffer.data, header.size);

	return packet;
}

void seomStreamDestroy(struct seomStream *stream)
{
	close(stream->fileDescriptor);
	free(stream);
}
