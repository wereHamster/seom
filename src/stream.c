
#include <seom/seom.h>

seomStream *seomStreamCreate(seomStreamOps *ops, void *priv)
{
	seomStream *stream = malloc(sizeof(seomStream));
	
	stream->ops = ops;
	stream->priv = priv;
	
	stream->ops->get(stream->priv, stream->size, sizeof(stream->size));
	stream->ops->put(stream->priv, stream->size, sizeof(stream->size));
	
	stream->size[0] = ntohl(stream->size[0]);
	stream->size[1] = ntohl(stream->size[1]);
	
	return stream;
}

void seomStreamPut(seomStream *stream, seomFrame *frame)
{
	uint64_t pts = frame->pts;
	
	static uint32_t tmp[1280 * 1024 * 4 * 8];	
	
	uint32_t *end = seomCodecEncode(tmp, frame->data, stream->size[0], stream->size[1]);
	uint32_t len = (end - &tmp[0]) * sizeof(uint32_t);
	
	stream->ops->put(stream->priv, &pts, sizeof(pts));
	stream->ops->put(stream->priv, &len, sizeof(len));
	stream->ops->put(stream->priv, tmp, len);
}

uint64_t seomStreamPos(seomStream *stream, uint64_t pos)
{
	return stream->ops->pos(stream->priv, pos);
}

seomFrame *seomStreamGet(seomStream *stream)
{
	seomFrame *frame = seomFrameCreate('c', stream->size[0], stream->size[1]);
	
	uint64_t pts;
	stream->ops->get(stream->priv, &pts, sizeof(pts));
	
	uint32_t len;
	stream->ops->get(stream->priv, &len, sizeof(len));
	
	static uint32_t tmp[1280 * 1024 * 4];
	stream->ops->get(stream->priv, tmp, len);
	
	uint32_t *end = seomCodecDecode(frame->data, tmp, stream->size[0], stream->size[1]);
	
	if ((end - &tmp[0]) * sizeof(uint32_t) != len) {
		return frame;
	}
	
	return frame;
}

void seomStreamDestroy(seomStream *stream)
{
	free(stream);
}
