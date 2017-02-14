/*
 * PNG loader library for OpenGL v1.46 (02/06/14)
 * by Ben Wyatt ben@wyatt100.freeserve.co.uk
 * Using LibPNG 1.0.2 and ZLib 1.1.3
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the author be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject to
 * the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented. You must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered versions must be plainly marked as such and must not be
 *    misrepresented as being the original source.
 * 3. This notice must not be removed or altered from any source distribution.
 */

/* Changed things slightly to be compatible with compilers not having stdint.h,
 * plus #include changes for static linking into Aquaria. -- fg */

#ifdef _WIN32 /* Stupid Windows needs to include windows.h before gl.h */
	#undef FAR
	#include <windows.h>
#endif

//#define GL_GLEXT_PROTOTYPES

//#include <GL/glpng.h>
#include "GL/glpng.h"
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _MSC_VER
typedef unsigned __int64 uint64;
#else
# include <stdint.h>
typedef uint64_t uint64;
#endif

#include <png.h>

#include "SDL.h"

static PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;
static int s_got_glGenerateMipmapEXT = 0;

/* Used to decide if GL/gl.h supports the paletted extension */
/*#ifdef GL_COLOR_INDEX1_EXT
#define SUPPORTS_PALETTE_EXT
#endif*/

static unsigned char DefaultAlphaCallback(unsigned char red, unsigned char green, unsigned char blue) {
	return 255;
}

static unsigned char StencilRed = 0, StencilGreen = 0, StencilBlue = 0;
static unsigned char (*AlphaCallback)(unsigned char red, unsigned char green, unsigned char blue) = DefaultAlphaCallback;
static int StandardOrientation = 0;

#ifdef SUPPORTS_PALETTE_EXT
#ifdef _WIN32
static PFNGLCOLORTABLEEXTPROC glColorTableEXT = NULL;
#endif
#endif

static int PalettedTextures = -1;
static GLint MaxTextureSize = 0;

/* screenGamma = displayGamma/viewingGamma
 * displayGamma = CRT has gamma of ~2.2
 * viewingGamma depends on platform. PC is 1.0, Mac is 1.45, SGI defaults
 * to 1.7, but this can be checked and changed w/ /usr/sbin/gamma command.
 * If the environment variable VIEWING_GAMMA is set, adjust gamma per this value.
 */
#ifdef _MAC
	static double screenGamma = 2.2 / 1.45;
#elif SGI
	static double screenGamma = 2.2 / 1.7;
#else /* PC/default */
	static double screenGamma = 2.2 / 1.0;
#endif

static char gammaExplicit = 0;	/*if  */

static void checkForGammaEnv()
{
	double viewingGamma;
	char *gammaEnv = getenv("VIEWING_GAMMA");

	if(gammaEnv && !gammaExplicit)
	{
		sscanf(gammaEnv, "%lf", &viewingGamma);
		screenGamma = 2.2/viewingGamma;
	}
}

/* Returns a safe texture size to use (ie a power of 2), based on the current texture size "i" */
static int SafeSize(int i) {
	int p;

	if (i > MaxTextureSize) return MaxTextureSize;

	for (p = 0; p < 24; p++)
		if (i <= (1<<p))
			return 1<<p;

	return MaxTextureSize;
}

/* Resize the texture since gluScaleImage doesn't work on everything */
static void Resize(int components, const png_bytep d1, int w1, int h1, png_bytep d2, int w2, int h2) {
	const float sx = (float) w1/w2, sy = (float) h1/h2;
	int x, y, xx, yy, c;
	png_bytep d;

	for (y = 0; y < h2; y++) {
		yy = (int) (y*sy)*w1;

		for (x = 0; x < w2; x++) {
			xx = (int) (x*sx);
			d = d1 + (yy+xx)*components;

			for (c = 0; c < components; c++)
				*d2++ = *d++;
		}
	}
}

#ifdef SUPPORTS_PALETTE_EXT
#ifdef _WIN32
static int ExtSupported(const char *x) {
	static const GLubyte *ext = NULL;
	const char *c;
	int xlen = strlen(x);

	if (ext == NULL) ext = glGetString(GL_EXTENSIONS);

	c = (const char*)ext;

	while (*c != '\0') {
		if (strcmp(c, x) == 0 && (c[xlen] == '\0' || c[xlen] == ' ')) return 1;
		c++;
	}

	return 0;
}
#endif
#endif

#define GET(o) ((int)*(data + (o)))

static int HalfSize(GLint components, GLint width, GLint height, const unsigned char *data, unsigned char *d, int filter) {
	int x, y, c;
	int line = width*components;

	if (width > 1 && height > 1) {
		if (filter)
			for (y = 0; y < height; y += 2) {
				for (x = 0; x < width; x += 2) {
					for (c = 0; c < components; c++) {
						*d++ = (GET(0)+GET(components)+GET(line)+GET(line+components)) / 4;
						data++;
					}
					data += components;
				}
				data += line;
			}
		else
			for (y = 0; y < height; y += 2) {
				for (x = 0; x < width; x += 2) {
					for (c = 0; c < components; c++) {
						*d++ = GET(0);
						data++;
					}
					data += components;
				}
				data += line;
			}
	}
	else if (width > 1 && height == 1) {
		if (filter)
			for (y = 0; y < height; y += 1) {
				for (x = 0; x < width; x += 2) {
					for (c = 0; c < components; c++) {
						*d++ = (GET(0)+GET(components)) / 2;
						data++;
					}
					data += components;
				}
			}
		else
			for (y = 0; y < height; y += 1) {
				for (x = 0; x < width; x += 2) {
					for (c = 0; c < components; c++) {
						*d++ = GET(0);
						data++;
					}
					data += components;
				}
			}
	}
	else if (width == 1 && height > 1) {
		if (filter)
			for (y = 0; y < height; y += 2) {
				for (x = 0; x < width; x += 1) {
					for (c = 0; c < components; c++) {
						*d++ = (GET(0)+GET(line)) / 2;
						data++;
					}
				}
				data += line;
			}
		else
			for (y = 0; y < height; y += 2) {
				for (x = 0; x < width; x += 1) {
					for (c = 0; c < components; c++) {
						*d++ = GET(0);
						data++;
					}
				}
				data += line;
			}
	}
	else {
		return 0;
	}

	return 1;
}

#undef GET

/* Replacement for gluBuild2DMipmaps so GLU isn't needed */
static void Build2DMipmaps(GLint components, GLint width, GLint height, GLenum format, const unsigned char *data, int filter) {
	
	int level = 0;
	unsigned char *d;
	const unsigned char *last = data;

	glTexImage2D(GL_TEXTURE_2D, level, components, width, height, 0, format, GL_UNSIGNED_BYTE, data);

genmip:
	if(glGenerateMipmapEXT)
	{
		glHint(GL_GENERATE_MIPMAP_HINT, filter ? GL_NICEST : GL_FASTEST);
		glGenerateMipmapEXT(GL_TEXTURE_2D);
		return;
	}
	else if(!s_got_glGenerateMipmapEXT)
	{
		s_got_glGenerateMipmapEXT = 1;
		glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)SDL_GL_GetProcAddress("glGenerateMipmapEXT");
		goto genmip;
	}
	
	level++;
	d = (unsigned char *) malloc((width/2)*(height/2)*components+4);

	while (HalfSize(components, width, height, last, d, filter)) {
		if (width  > 1) width  /= 2;
		if (height > 1) height /= 2;

		glTexImage2D(GL_TEXTURE_2D, level, components, width, height, 0, format, GL_UNSIGNED_BYTE, d);
		level++;
		last = d;
	}

	free(d);
}

int pngLoadRaw(const char *filename, pngRawInfo *pinfo) {
	int result;
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) return 0;

	result = pngLoadRawF(fp, pinfo);

	if (fclose(fp) != 0) {
		if (result) {
			free(pinfo->Data);
			free(pinfo->Palette);
		}
		return 0;
	}

	return result;
}

int pngLoadRawF(FILE *fp, pngRawInfo *pinfo) {
	unsigned char header[8];
	png_structp png;
	png_infop   info;
	png_infop   endinfo;
	double      fileGamma;

	png_uint_32 width, height, i;
	int depth, color;

	png_bytep data = NULL;
	png_bytep *row_p = NULL;

	if (pinfo == NULL) return 0;

	if (fread(header, 1, 8, fp) != 8) return 0;
	if (!png_check_sig(header, 8)) return 0;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) return 0;
	info = png_create_info_struct(png);
	if (!info) return 0;
	endinfo = png_create_info_struct(png);
	if (!endinfo) return 0;

	if (setjmp(png_jmpbuf(png)))
		goto error;

	png_init_io(png, fp);
	png_set_sig_bytes(png, 8);
	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &depth, &color, NULL, NULL, NULL);

	pinfo->Width  = width;
	pinfo->Height = height;
	pinfo->Depth  = depth;

	/*--GAMMA--*/
	checkForGammaEnv();
	if (png_get_gAMA(png, info, &fileGamma))
		png_set_gamma(png, screenGamma, fileGamma);
	else
		png_set_gamma(png, screenGamma, 1.0/2.2);

	png_read_update_info(png, info);

	/* HDG: We allocate all the png data in one linear array, thus
	   height * png_get_rowbytes() may not be > PNG_UINT_32_MAX !
	   This check fixes CVE-2010-1519. */
	if ((uint64)height * png_get_rowbytes(png, info) > PNG_UINT_32_MAX)
		goto error;

	data = (png_bytep) malloc(png_get_rowbytes(png, info)*height);
	row_p = (png_bytep *) malloc(sizeof(png_bytep)*height);
	if (!data || !row_p)
		goto error;

	for (i = 0; i < height; i++) {
		if (StandardOrientation)
			row_p[height - 1 - i] = &data[png_get_rowbytes(png, info)*i];
		else
			row_p[i] = &data[png_get_rowbytes(png, info)*i];
	}

	png_read_image(png, row_p);
	free(row_p);
	row_p = NULL;

	if (color == PNG_COLOR_TYPE_PALETTE) {
		int cols;
		png_get_PLTE(png, info, (png_colorp *) &pinfo->Palette, &cols);
	}
	else {
		pinfo->Palette = NULL;
	}

	if (color&PNG_COLOR_MASK_ALPHA) {
		if (color&PNG_COLOR_MASK_PALETTE || color == PNG_COLOR_TYPE_GRAY_ALPHA)
			pinfo->Components = 2;
		else
			pinfo->Components = 4;
		pinfo->Alpha = 8;
	}
	else {
		if (color&PNG_COLOR_MASK_PALETTE || color == PNG_COLOR_TYPE_GRAY)
			pinfo->Components = 1;
		else
			pinfo->Components = 3;
		pinfo->Alpha = 0;
	}

	pinfo->Data = data;

	png_read_end(png, endinfo);
	png_destroy_read_struct(&png, &info, &endinfo);
	return 1;

error:
	png_destroy_read_struct(&png, &info, &endinfo);
	free(data);
	free(row_p);
	return 0;
}

static int pngLoadCommon(int mipmap, int trans, pngInfo *pinfo, png_structp png, png_infop info, png_infop endinfo) {
	GLint pack, unpack;
	double fileGamma;

	png_uint_32 width, height, rw, rh, i;
	int depth, color;

	png_bytep data = NULL, data2 = NULL;
	png_bytep *row_p = NULL;

	int ret = 0;

	png_set_sig_bytes(png, 8);
	png_read_info(png, info);
	png_get_IHDR(png, info, &width, &height, &depth, &color, NULL, NULL, NULL);

	if (pinfo != NULL) {
		pinfo->Width  = width;
		pinfo->Height = height;
		pinfo->Depth  = depth;
	}

	if (MaxTextureSize == 0)
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);

	#ifdef SUPPORTS_PALETTE_EXT
	#ifdef _WIN32
		if (PalettedTextures == -1)
			PalettedTextures = ExtSupported("GL_EXT_paletted_texture") && (strstr((const char *) glGetString(GL_VERSION), "1.1.0 3Dfx Beta") == NULL);

		if (PalettedTextures) {
			if (glColorTableEXT == NULL) {
				glColorTableEXT = (PFNGLCOLORTABLEEXTPROC) wglGetProcAddress("glColorTableEXT");
				if (glColorTableEXT == NULL)
					PalettedTextures = 0;
			}
		}
	#endif
	#endif

	if (PalettedTextures == -1)
		PalettedTextures = 0;

	if (color == PNG_COLOR_TYPE_GRAY || color == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	if (color&PNG_COLOR_MASK_ALPHA && trans != PNG_ALPHA) {
		png_set_strip_alpha(png);
		color &= ~PNG_COLOR_MASK_ALPHA;
	}

	if (!(PalettedTextures && mipmap >= 0 && trans == PNG_SOLID))
		if (color == PNG_COLOR_TYPE_PALETTE)
			png_set_expand(png);

	/*--GAMMA--*/
	checkForGammaEnv();
	if (png_get_gAMA(png, info, &fileGamma))
		png_set_gamma(png, screenGamma, fileGamma);
	else
		png_set_gamma(png, screenGamma, 1.0/2.2);

	png_read_update_info(png, info);

	/* HDG: We allocate all the png data in one linear array, thus
	   height * png_get_rowbytes() may not be > PNG_UINT_32_MAX !
	   This check fixes CVE-2010-1519. */
	if ((uint64)height * png_get_rowbytes(png, info) > PNG_UINT_32_MAX)
		goto finish;

	data = (png_bytep) malloc(png_get_rowbytes(png, info)*height);
	row_p = (png_bytep *) malloc(sizeof(png_bytep)*height);
	if (!data || !row_p)
		goto finish;

	for (i = 0; i < height; i++) {
		if (StandardOrientation)
			row_p[height - 1 - i] = &data[png_get_rowbytes(png, info)*i];
		else
			row_p[i] = &data[png_get_rowbytes(png, info)*i];
	}

	png_read_image(png, row_p);
	free(row_p);
	row_p = NULL;

	rw = SafeSize(width), rh = SafeSize(height);

	if (rw != width || rh != height) {
		const int channels = png_get_rowbytes(png, info)/width;

		data2 = (png_bytep) malloc(rw*rh*channels);
		if (!data2)
			goto finish;

 		/* Doesn't work on certain sizes */
/* 		if (gluScaleImage(glformat, width, height, GL_UNSIGNED_BYTE, data, rw, rh, GL_UNSIGNED_BYTE, data2) != 0)
			goto finish;
*/
		Resize(channels, data, width, height, data2, rw, rh);

		width = rw, height = rh;
		free(data);
		data = data2;
		data2 = NULL;
	}

	{ /* OpenGL stuff */
		glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		#ifdef SUPPORTS_PALETTE_EXT
		if (PalettedTextures && mipmap >= 0 && trans == PNG_SOLID && color == PNG_COLOR_TYPE_PALETTE) {
			png_colorp pal;
			int cols;
			GLint intf;

			if (pinfo != NULL) pinfo->Alpha = 0;
			png_get_PLTE(png, info, &pal, &cols);

			switch (cols) {
				case 1<<1:  intf = GL_COLOR_INDEX1_EXT;  break;
				case 1<<2:  intf = GL_COLOR_INDEX2_EXT;  break;
				case 1<<4:  intf = GL_COLOR_INDEX4_EXT;  break;
				case 1<<8:  intf = GL_COLOR_INDEX8_EXT;  break;
				case 1<<12: intf = GL_COLOR_INDEX12_EXT; break;
				case 1<<16: intf = GL_COLOR_INDEX16_EXT; break;
				default:
					/*printf("Warning: Colour depth %i not recognised\n", cols);*/
					goto finish;
			}
			glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, cols, GL_RGB, GL_UNSIGNED_BYTE, pal);
			glTexImage2D(GL_TEXTURE_2D, mipmap, intf, width, height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
		}
		else
		#endif
		if (trans == PNG_SOLID || trans == PNG_ALPHA || trans == PNG_LUMINANCEALPHA || color == PNG_COLOR_TYPE_RGB_ALPHA || color == PNG_COLOR_TYPE_GRAY_ALPHA) {
			GLenum glformat;
			GLint glcomponent;

			switch (color) {
				case PNG_COLOR_TYPE_GRAY:
				case PNG_COLOR_TYPE_RGB:
				case PNG_COLOR_TYPE_PALETTE:
					glformat = GL_RGB;
					glcomponent = 3;
					if (pinfo != NULL) pinfo->Alpha = 0;
					break;

				case PNG_COLOR_TYPE_GRAY_ALPHA:
				case PNG_COLOR_TYPE_RGB_ALPHA:
					glformat = GL_RGBA;
					glcomponent = 4;
					if (pinfo != NULL) pinfo->Alpha = 8;
					break;

				default:
					/*puts("glformat not set");*/
					goto finish;
			}

			if (trans == PNG_LUMINANCEALPHA)
				glformat = GL_LUMINANCE_ALPHA;

			if (mipmap == PNG_BUILDMIPMAPS)
				Build2DMipmaps(glcomponent, width, height, glformat, data, 1);
			else if (mipmap == PNG_SIMPLEMIPMAPS)
				Build2DMipmaps(glcomponent, width, height, glformat, data, 0);
			else
				glTexImage2D(GL_TEXTURE_2D, mipmap, glcomponent, width, height, 0, glformat, GL_UNSIGNED_BYTE, data);
		}
		else {
			png_bytep p, endp, q;
			int r, g, b, a;

			/* HDG another potential 32 bit address overflow, the
			   original png had 3 channels and we are going to
			   4 channels now! */
			if ((uint64)width * height > (PNG_UINT_32_MAX >> 2))
				goto finish;

			p = data, endp = p+width*height*3;
			q = data2 = (png_bytep) malloc(sizeof(png_byte)*width*height*4);

			if (pinfo != NULL) pinfo->Alpha = 8;

			#define FORSTART \
				do { \
					r = *p++; /*red  */ \
					g = *p++; /*green*/ \
					b = *p++; /*blue */ \
					*q++ = r; \
					*q++ = g; \
					*q++ = b;

			#define FOREND \
					q++; \
				} while (p != endp);

			#define ALPHA *q

			switch (trans) {
				case PNG_CALLBACKT:
					FORSTART
						ALPHA = AlphaCallback((unsigned char) r, (unsigned char) g, (unsigned char) b);
					FOREND
					break;

				case PNG_STENCIL:
					FORSTART
						if (r == StencilRed && g == StencilGreen && b == StencilBlue)
							ALPHA = 0;
						else
							ALPHA = 255;
					FOREND
					break;

				case PNG_BLEND1:
					FORSTART
						a = r+g+b;
						if (a > 255) ALPHA = 255; else ALPHA = a;
					FOREND
					break;

				case PNG_BLEND2:
					FORSTART
						a = r+g+b;
						if (a > 255*2) ALPHA = 255; else ALPHA = a/2;
					FOREND
					break;

				case PNG_BLEND3:
					FORSTART
						ALPHA = (r+g+b)/3;
					FOREND
					break;

				case PNG_BLEND4:
					FORSTART
						a = r*r+g*g+b*b;
						if (a > 255) ALPHA = 255; else ALPHA = a;
					FOREND
					break;

				case PNG_BLEND5:
					FORSTART
						a = r*r+g*g+b*b;
						if (a > 255*2) ALPHA = 255; else ALPHA = a/2;
					FOREND
					break;

				case PNG_BLEND6:
					FORSTART
						a = r*r+g*g+b*b;
						if (a > 255*3) ALPHA = 255; else ALPHA = a/3;
					FOREND
					break;

				case PNG_BLEND7:
					FORSTART
						a = r*r+g*g+b*b;
						if (a > 255*255) ALPHA = 255; else ALPHA = (int) sqrt(a);
					FOREND
					break;
			}

			#undef FORSTART
			#undef FOREND
			#undef ALPHA

			if (mipmap == PNG_BUILDMIPMAPS)
				Build2DMipmaps(4, width, height, GL_RGBA, data2, 1);
			else if (mipmap == PNG_SIMPLEMIPMAPS)
				Build2DMipmaps(4, width, height, GL_RGBA, data2, 0);
			else
				glTexImage2D(GL_TEXTURE_2D, mipmap, 4, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);

		}

		glPixelStorei(GL_PACK_ALIGNMENT, pack);
		glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
	} /* OpenGL end */

	png_read_end(png, endinfo);
	ret = 1;

finish:
	free(data);
	free(data2);
	free(row_p);
	return ret;
}

int pngLoad(const char *filename, int mipmap, int trans, pngInfo *pinfo) {
	int result;
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) return 0;

	result = pngLoadF(fp, mipmap, trans, pinfo);

	if (fclose(fp) != 0) return 0;

	return result;
}

int pngLoadF(FILE *fp, int mipmap, int trans, pngInfo *pinfo) {
	unsigned char header[8];
	png_structp png;
	png_infop info;
	png_infop endinfo;

	int ret = 0;

	if (fread(header, 1, 8, fp) != 8) return 0;
	if (!png_check_sig(header, 8)) return 0;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) return 0;
	info = png_create_info_struct(png);
	if (!info) return 0;
	endinfo = png_create_info_struct(png);
	if (!endinfo) return 0;

	if (!setjmp(png_jmpbuf(png))) {
		png_init_io(png, fp);
		ret = pngLoadCommon(mipmap, trans, pinfo, png, info, endinfo);
	}

	png_destroy_read_struct(&png, &info, &endinfo);
	return ret;
}

typedef struct glpng_memread_struct {
	png_bytep mem;
	png_size_t rpos;
} glpng_memread;

static void glpng_read_mem(png_structp png, png_bytep dst, png_size_t size) {
	glpng_memread *mr = (glpng_memread*) png_get_io_ptr(png);
	memcpy(dst, mr->mem + mr->rpos, size);
	mr->rpos += size;
}

int pngLoadMem(const char *mem, int size, int mipmap, int trans, pngInfo *pinfo) {
	unsigned char header[8];
	png_structp png;
	png_infop info;
	png_infop endinfo;
	glpng_memread memread;

	int ret = 0;

	if (size < 8)
		return 0; // error

	memcpy(header, mem, 8);
	if (!png_check_sig(header, 8)) return 0;

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) return 0;
	info = png_create_info_struct(png);
	if (!info) return 0;
	endinfo = png_create_info_struct(png);
	if (!endinfo) return 0;

	if (!setjmp(png_jmpbuf(png))) {
		memread.rpos = 0;
		memread.mem = ((png_bytep) mem) + 8;
		png_set_read_fn(png, &memread, glpng_read_mem);
		ret = pngLoadCommon(mipmap, trans, pinfo, png, info, endinfo);
	}

	png_destroy_read_struct(&png, &info, &endinfo);
	return ret;
}

static unsigned int SetParams(int wrapst, int magfilter, int minfilter) {
	unsigned int id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapst);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapst);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);

	return id;
}

unsigned int pngBind(const char *filename, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter) {
	unsigned int id = SetParams(wrapst, magfilter, minfilter);

	if (id != 0 && pngLoad(filename, mipmap, trans, info))
		return id;
	return 0;
}

unsigned int pngBindF(FILE *file, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter) {
	unsigned int id = SetParams(wrapst, magfilter, minfilter);

	if (id != 0 && pngLoadF(file, mipmap, trans, info))
		return id;
	return 0;
}

unsigned int pngBindMem(const char *mem, int size, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter) {
	unsigned int id = SetParams(wrapst, magfilter, minfilter);

	if (id != 0 && pngLoadMem(mem, size, mipmap, trans, info))
		return id;
	return 0;
}

void pngSetStencil(unsigned char red, unsigned char green, unsigned char blue) {
	StencilRed = red, StencilGreen = green, StencilBlue = blue;
}

void pngSetAlphaCallback(unsigned char (*callback)(unsigned char red, unsigned char green, unsigned char blue)) {
	if (callback == NULL)
		AlphaCallback = DefaultAlphaCallback;
	else
		AlphaCallback = callback;
}

void pngSetViewingGamma(double viewingGamma) {
	if(viewingGamma > 0) {
		gammaExplicit = 1;
		screenGamma = 2.2/viewingGamma;
	}
	else {
		gammaExplicit = 0;
		screenGamma = 2.2;
	}
}

void pngSetStandardOrientation(int standardorientation) {
	StandardOrientation = standardorientation;
}
