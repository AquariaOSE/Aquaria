#ifndef BBGE_IMAGE_H
#define BBGE_IMAGE_H

#include <stddef.h>

bool tgaSaveRGBA(const char *filename, size_t width, size_t height, unsigned char *data);
bool zgaSaveRGBA(const char *filename, size_t width, size_t height, unsigned char *data);
bool pngSaveRGBA(const char *filename, size_t width, size_t height, unsigned char *data, unsigned compressLevel);

struct ImageData
{
	unsigned char *pixels; // NULL when failed to load
	size_t w, h;
	unsigned channels;
};

ImageData imageLoadGeneric(const char *filename, bool forceRGBA);
ImageData imageLoadZGA(const char *filename);
ImageData imageLoadQOI(const char *filename, bool forceRGBA);

#endif
