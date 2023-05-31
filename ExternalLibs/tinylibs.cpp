#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#include "stb_image.h"

#include <miniz.h>

static unsigned char * miniz_stbi_compress(unsigned char *data, int data_len, int *out_len, int quality)
{
	mz_ulong maxsz = mz_compressBound(data_len);
	unsigned char *packed = (unsigned char*)malloc(maxsz);
	if(packed)
	{
		int result = mz_compress2(packed, &maxsz, data, data_len, quality);
		if(result)
		{
			free(packed);
			return NULL;
		}
	}
	*out_len = (int)maxsz;
	return packed;
}

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ZLIB_COMPRESS miniz_stbi_compress
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STBIR_ASSERT(x) assert(x)
#include "stb_image_resize.h"

#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "qoi.h"
