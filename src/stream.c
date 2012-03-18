
#include <seom/stream.h>

struct seomStream *seomStreamCreate(struct seomStreamOps *ops, void *private)
{
	struct seomStream *stream = malloc(sizeof(struct seomStream));
	if (stream == NULL)
		return NULL;

	stream->ops = ops;
	stream->private = private;
	
	stream->context = malloc(4096 * sizeof(void *));
	if (stream->context == NULL)
		return NULL;

	stream->buffer.size = 0;
	stream->buffer.data = NULL;

	return stream;
}

void seomStreamPut(struct seomStream *stream, struct seomPacket *packet)
{
	if (packet->size > stream->buffer.size) {
		void *data = realloc(stream->buffer.data, packet->size * 2 + 4096 * sizeof(void *));
		if (data == NULL)
			goto out;

		stream->buffer.size = packet->size * 2 + 4096;
		stream->buffer.data = data;
	}

	const void *end = seomCodecEncode(stream->buffer.data, seomPacketPayload(packet), packet->size, stream->context);
	uint64_t size = end - stream->buffer.data;

	const struct iovec vec[] = {
		{ packet, sizeof(struct seomPacket) },
		{ &size, sizeof(uint64_t) },
		{ stream->buffer.data, size },
	};

	stream->ops->put(stream->private, vec, 3);

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

	if (stream->ops->get(stream->private, vec, 2) == 0)
		return NULL;

	if (header.size > stream->buffer.size) {
		void *data = realloc(stream->buffer.data, header.size * 2 + 4096 * sizeof(void *));
		if (data == NULL)
			return NULL;

		stream->buffer.size = header.size * 2 + 4096;
		stream->buffer.data = data;
	}

	const struct iovec data[] = {
		{ stream->buffer.data, size },
	};

	if (stream->ops->get(stream->private, data, 1) == 0)
		return NULL;

	struct seomPacket *packet = seomPacketCreate(header.type, header.size);
	if (packet == NULL)
		return NULL;

	memcpy(packet, &header, sizeof(struct seomPacket) + 8);
	seomCodecDecode(seomPacketPayload(packet), stream->buffer.data, header.size);

	return packet;
}

void seomStreamDestroy(struct seomStream *stream)
{
	free(stream->context);
	free(stream->buffer.data);
	free(stream);
}
