#include "Image.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "qoi.h"
#include "DeflateCompressor.h"
#include "ttvfs_stdio.h"
#include "Base.h"

bool pngSaveRGBA(const char *filename, size_t width, size_t height, unsigned char *data, unsigned compressLevel)
{
	const int oldlevel = stbi_write_png_compression_level;
	stbi_write_png_compression_level = compressLevel; // HACK: ugly API but what can you do
	bool ok = !!stbi_write_png(filename, (int)width, (int)height, 4, data, width * 4);
	stbi_write_png_compression_level = oldlevel;
	return ok;
}

bool tgaSaveRGBA(const char *filename, size_t width, size_t height, unsigned char *data)
{
	return !!stbi_write_tga(filename, (int)width, (int)height, 4, data);
}


// Aquaria special: zlib-compressed TGA
bool zgaSaveRGBA(const char *filename, size_t w, size_t h, unsigned char *data)
{
	ByteBuffer::uint8 type ,mode,aux, pixelDepth = 32;
	ByteBuffer::uint8 cGarbage = 0;
	ByteBuffer::uint16 iGarbage = 0;
	ByteBuffer::uint16 width = w, height = h;

	// open file and check for errors
	FILE *file = fopen(filename, "wb");
	if (file == NULL)
		return false;

	// compute image type: 2 for RGB(A), 3 for greyscale
	mode = pixelDepth / 8;
	if ((pixelDepth == 24) || (pixelDepth == 32))
		type = 2;
	else
		type = 3;

	// convert the image data from RGBA to BGRA
	if (mode >= 3)
		for (int i=0; i < width * height * mode ; i+= mode)
		{
			aux = data[i];
			data[i] = data[i+2];
			data[i+2] = aux;
		}

	ZlibCompressor z;
	z.SetForceCompression(true);
	z.reserve(width * height * mode + 30);
	z	<< cGarbage
		<< cGarbage
		<< type
		<< iGarbage
		<< iGarbage
		<< cGarbage
		<< iGarbage
		<< iGarbage
		<< width
		<< height
		<< pixelDepth
		<< cGarbage;

	z.append(data, width * height * mode);
	z.Compress(3);

	bool ok  = fwrite(z.contents(), 1, z.size(), file) == z.size();

	fclose(file);

	return ok;
}


static VFILE *imageLoadOpenFile(const char *filename)
{
	return vfopen(filename, "rb");
}

// fill 'data' with 'size' bytes.  return number of bytes actually read
static int cbread(void *user,char *data,int size)
{
	VFILE *vf = (VFILE*)user;
	return (int)vfread(data, 1, size, vf);
}

// skip the next 'n' bytes, or 'unget' the last -n bytes if negative
void cbskip(void *user,int n)
{
	VFILE *vf = (VFILE*)user;
	vfseek(vf, n, SEEK_CUR);
}

// returns nonzero if we are at end of file/data
int cbeof(void *user)
{
	VFILE *vf = (VFILE*)user;
	return vfeof(vf);
}

static const stbi_io_callbacks iocb = { cbread, cbskip, cbeof };

ImageData imageLoadGeneric(const char *filename, bool forceRGBA)
{
	ImageData ret = {};
	VFILE *fh = imageLoadOpenFile(filename);
	if(fh)
	{
		int x = 0, y = 0, comp = 0;
		stbi_uc *pix = stbi_load_from_callbacks(&iocb, fh, &x, &y, &comp, forceRGBA ? 4 : 0);
		ret.pixels = pix;
		ret.channels = forceRGBA ? 4 : comp;
		ret.w = x;
		ret.h = y;
		vfclose(fh);
	}
	return ret;
}

// this actually loads anything that looks fine after uncompressing the data
ImageData imageLoadZGA(const char *filename)
{
	ImageData ret = {};
	size_t size = 0;
	char *buf = readCompressedFile(filename, &size);
	if(buf)
	{
		int x = 0, y = 0, comp = 0;
		stbi_uc *pix = stbi_load_from_memory((stbi_uc*)buf, size, &x, &y, &comp, 0);
		delete [] buf;
		ret.pixels = pix;
		ret.channels = comp;
		ret.w = x;
		ret.h = y;
	}
	return ret;
}

ImageData imageLoadQOI(const char* filename, bool forceRGBA)
{
	ImageData ret = {};
	size_t size = 0;
	void *buf = readFile(filename, &size);
	if(buf)
	{
		qoi_desc d;
		stbi_uc *pix = (stbi_uc*)qoi_decode(buf, size, &d, forceRGBA ? 4 : 0);
		if(pix)
		{
			ret.w = d.width;
			ret.h = d.height;
			ret.channels = forceRGBA ? 4 : d.channels;
			ret.pixels = pix;
		}
		free(buf);
	}
	return ret;
}
