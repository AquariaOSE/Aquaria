/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Texture.h"
#include "Core.h"
#include "../ExternalLibs/glpng.h"
#include "ByteBuffer.h"

#include <assert.h>

#if defined(BBGE_BUILD_UNIX)
#include <stdint.h>
#endif

//#include "pngLoad.h"
//#include "jpeg/jpeglib.h"
/*
#include <il/il.h>
#include <il/ilu.h>
#include <il/ilut.h>
*/
#ifdef Z2D_J2K
//..\j2k-codec\j2k-codec.lib
	#include "..\j2k-codec\j2k-codec.h"
#endif

#ifdef BBGE_BUILD_OPENGL
	GLint Texture::filter = GL_LINEAR;

	GLint Texture::format = 0;
#endif
bool Texture::useMipMaps = true;

/*
#ifdef BBGE_BUILD_OPENGL
#include "glext/glext.h"
#endif
*/

TexErr Texture::textureError = TEXERR_OK;

Texture::Texture() : Resource()
{
#ifdef BBGE_BUILD_OPENGL
	textures[0] = 0;
#endif
#ifdef BBGE_BUILD_DIRECTX
	d3dTexture = 0;
#endif
	width = height = 0;

	repeat = false;
	repeating = false;
	pngSetStandardOrientation(0);

	ow = oh = -1;
}

Texture::~Texture()
{
	destroy();
}

void Texture::read(int tx, int ty, int w, int h, unsigned char *pixels)
{
#ifdef BBGE_BUILD_OPENGL
	if (tx == 0 && ty == 0 && w == this->width && h == this->height)
	{
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	else
	{
		std::ostringstream os;
		os << "Unable to read a texture subimage (size = "
		   << this->width << "x" << this->height << ", requested = "
		   << tx << "," << ty << "+" << w << "x" << h << ")";
		debugLog(os.str());
	}
#endif
}

void Texture::write(int tx, int ty, int w, int h, const unsigned char *pixels)
{
#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	glTexSubImage2D(GL_TEXTURE_2D, 0,
					tx,
					ty,
					w,
					h,
					GL_RGBA,
					GL_UNSIGNED_BYTE,
					pixels
					);

	glBindTexture(GL_TEXTURE_2D, 0);
	/*
	  target   Specifies the target	texture.  Must be
		   GL_TEXTURE_2D.

	  level	   Specifies the level-of-detail number.  Level	0 is
		   the base image level.  Level	n is the nth mipmap
		   reduction image.

	  xoffset  Specifies a texel offset in the x direction within
		   the texture array.

	  yoffset  Specifies a texel offset in the y direction within
		   the texture array.

	  width	   Specifies the width of the texture subimage.

	  height   Specifies the height	of the texture subimage.

	  format   Specifies the format	of the pixel data.  The
		   following symbolic values are accepted:
		   GL_COLOR_INDEX, GL_RED, GL_GREEN, GL_BLUE,
		   GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and
		   GL_LUMINANCE_ALPHA.

	  type	   Specifies the data type of the pixel	data.  The
		   following symbolic values are accepted:
		   GL_UNSIGNED_BYTE, GL_BYTE, GL_BITMAP,
		   GL_UNSIGNED_SHORT, GL_SHORT,	GL_UNSIGNED_INT,
		   GL_INT, and GL_FLOAT.

	  pixels   Specifies a pointer to the image data in memory.
	  */
#endif
}

void Texture::unload()
{
	Resource::unload();
#ifdef BBGE_BUILD_OPENGL
	if (textures[0])
	{
		ow = width;
		oh = height;

		if (core->debugLogTextures)
		{
			debugLog("UNLOADING TEXTURE: " + name);
		}


		glDeleteTextures(1, &textures[0]);
		textures[0] = 0;

		//removeRef();
	}
#endif
}

void Texture::destroy()
{
#ifdef BBGE_BUILD_OPENGL
	unload();
#endif
#ifdef BBGE_BUILD_DIRECTX
	if (d3dTexture)
	{
		d3dTexture->Release();
		d3dTexture = 0;
	}
#endif

	if (!core->isShuttingDown())
		core->removeTexture(this->name);

//	Resource::destroy();
}

int Texture::getPixelWidth()
{
#ifdef BBGE_BUILD_OPENGL
	int w = 0, h = 0;
	unsigned int size = 0;
	unsigned char *data = getBufferAndSize(&w, &h, &size);
	if (!data)
		return 0;

	int smallestx = -1, largestx = -1;
	for (unsigned int x = 0; x < unsigned(w); x++)
	{
		for (unsigned int y = 0; y < unsigned(h); y++)
		{
			unsigned int p = (y*unsigned(w)*4) + (x*4) + 3;
			if (p < size && data[p] >= 254)
			{
				if (smallestx == -1 || x < smallestx)
					smallestx = x;
				if (largestx == -1 || x > largestx)
					largestx = x;
			}
		}
	}
	free(data);
	return largestx - smallestx;
#elif defined(BBGE_BUILD_DIRECTX)
	return 0;
#endif
}

int Texture::getPixelHeight()
{
#ifdef BBGE_BUILD_OPENGL
	int w = 0, h = 0;
	unsigned int size = 0;
	unsigned char *data = getBufferAndSize(&w, &h, &size);
	if (!data)
		return 0;

	int smallesty = -1, largesty = -1;
	for (unsigned int x = 0; x < unsigned(w); x++)
	{
		for (unsigned int y = 0; y < unsigned(h); y++)
		{
			int p = (y*unsigned(w)*4) + (x*4) + 3;
			if (p < size && data[p] >= 254)
			{
				if (smallesty == -1 || y < smallesty)
					smallesty = y;
				if (largesty == -1 || y > largesty)
					largesty = y;
			}
		}
	}
	free(data);
	return largesty - smallesty;
#elif defined(BBGE_BUILD_DIRECTX)
	return 0;
#endif
}

void Texture::reload()
{
	Resource::reload();

	debugLog("RELOADING TEXTURE: " + name + " with loadName " + loadName + "...");

	unload();
	load(loadName);

	/*if (ow != -1 && oh != -1)
	{
		width = ow;
		height = oh;
	}*/
	debugLog("DONE");
}

void Texture::load(std::string file)
{
	Texture::textureError = TEXERR_OK;

	if (file.size()<4)
	{
		errorLog("Texture Name is Empty or Too Short");
		Texture::textureError = TEXERR_FILENOTFOUND;
		return;
	}

	stringToLowerUserData(file);
	file = core->adjustFilenameCase(file);

	loadName = file;
	repeating = false;

	size_t pos = file.find_last_of('.');

	if ((pos != std::string::npos) && (pos >= 0))
	{
		// make sure this didn't catch the '.' in /home/username/.Aquaria/*  --ryan.
		const std::string userdata = core->getUserDataFolder();
		const size_t len = userdata.length();
		if (pos < len)
			pos = std::string::npos;
	}

	/*if (core->debugLogTextures)
	{
		std::ostringstream os;
		os << "pos [" << pos << "], file :" << file;
		debugLog(os.str());
	}*/

	bool found = exists(file);

	if(!found && exists(file + ".png"))
	{
		found = true;
		file += ".png";
	}

	// .tga/.zga are never used as game graphics anywhere except save slot thumbnails.
	// if so, their file names are passed exact, not with a missing extension

	if (found)
	{
		file = localisePath(file);
		file = core->adjustFilenameCase(file);

		/*
		std::ostringstream os;
		os << "Loading texture [" << file << "]";
		debugLog(os.str());
		*/
		std::string post = file.substr(file.size()-3, 3);
		stringToLower(post);
		if (post == "png")
		{

#ifdef BBGE_BUILD_OPENGL
			loadPNG(file);
#endif

#ifdef BBGE_BUILD_DIRECTX
			D3DXCreateTextureFromFile(core->getD3DDevice(),  file.c_str(),	&this->d3dTexture);
			if (!d3dTexture)
			{
				errorLog ("failed to load texture");
			}
			else
			{
				D3DSURFACE_DESC desc;
				this->d3dTexture->GetLevelDesc(0,&desc);

				width = desc.Width;
				height = desc.Height;
			}
#endif
		}
		else if (post == "zga")
		{
			loadZGA(file);
		}
		else if (post == "tga")
		{
			loadTGA(file);
		}
		else
		{
			debugLog("unknown image file type: " + file);
			Texture::textureError = TEXERR_FILENOTFOUND;
			width = 64;
			height = 64;
		}
	}
	else
	{
		// load default image / leave white
		if (core->debugLogTextures)
			debugLog("***Could not find texture: " + file);
		Texture::textureError = TEXERR_FILENOTFOUND;
		width = 64;
		height = 64;
	}
}

void Texture::apply(bool repeatOverride)
{
#ifdef BBGE_BUILD_OPENGL
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	if (repeat || repeatOverride)
	{
		if (!repeating)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			repeating = true;
		}
	}
	else
	{
		if (repeating)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			repeating = false;
		}
	}
#endif
#ifdef BBGE_BUILD_DIRECTX
	core->getD3DDevice()->SetTexture(0, d3dTexture);

#endif
}

void Texture::unbind()
{
}

#ifdef BBGE_BUILD_OPENGL

void Texture::setID(int id)
{
	textures[0] = id;
}

#endif

void Texture::loadPNG(const std::string &file)
{
	if (file.empty()) return;

#ifdef BBGE_BUILD_OPENGL


	pngInfo info;

	int pngType = PNG_ALPHA;

	if (format != 0)
	{
		if (format == GL_LUMINANCE_ALPHA)
			pngType = PNG_LUMINANCEALPHA;
	}

#ifdef BBGE_BUILD_VFS
	ttvfs::VFSFile *vf = vfs.GetFile(file.c_str());
	const char *memptr = vf ? (const char*)vf->getBuf() : NULL;
	if(!memptr)
	{
		debugLog("Can't load PNG file: " + file);
		width = 64;
		height = 64;
		Texture::textureError = TEXERR_FILENOTFOUND;
		//exit(1);
		return;
	}

	int memsize = vf->size();
	if (filter == GL_NEAREST)
	{
		textures[0] = pngBindMem(memptr, memsize, PNG_NOMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, filter, filter);
	}
	else
	{
		textures[0] = pngBindMem(memptr, memsize, PNG_BUILDMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, filter);
	}
	vf->dropBuf(true);

#else
	if (filter == GL_NEAREST)
	{
		textures[0] = pngBind(file.c_str(), PNG_NOMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, filter, filter);
	}
	else
	{
		textures[0] = pngBind(file.c_str(), PNG_BUILDMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, filter);
	}
#endif

	if (textures[0] != 0)
	{
		width = info.Width;
		height = info.Height;
	}
	else
	{
		debugLog("Can't load PNG file: " + file);
		width = 64;
		height = 64;
		Texture::textureError = TEXERR_FILENOTFOUND;
		//exit(1);
	}


#endif
}

// internal load functions
void Texture::loadTGA(const std::string &file)
{
	loadTGA(TGAload(file.c_str()));
}

void Texture::loadZGA(const std::string &file)
{
	unsigned long size = 0;
	char *buf = readCompressedFile(file, &size);
	ImageTGA *tga = TGAloadMem(buf, size);
	if (!tga)
	{
		debugLog("Can't load ZGA File: " + file);
		return;
	}
	loadTGA(tga);
}

void Texture::loadTGA(ImageTGA *imageTGA)
{
	if (!imageTGA)
		return;

	glGenTextures(1, &textures[0]);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);	// Linear Filtering

	if (imageTGA->channels==3)
		glTexImage2D(GL_TEXTURE_2D, 0, 3, imageTGA->sizeX, imageTGA->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, imageTGA->data);
	else if (imageTGA->channels==4)
		glTexImage2D(GL_TEXTURE_2D, 0, 4,imageTGA->sizeX, imageTGA->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageTGA->data);

	width = imageTGA->sizeX;
	height = imageTGA->sizeY;

	if (imageTGA->data)
		delete[] (imageTGA->data);
	free (imageTGA);
}


#define TGA_RGB		 2		// This tells us it's a normal RGB (really BGR) file
#define TGA_A		 3		// This tells us it's a ALPHA file
#define TGA_RLE		10		// This tells us that the targa is Run-Length Encoded (RLE)

#if defined(BBGE_BUILD_UNIX)
typedef uint8_t byte;
typedef uint16_t WORD;
#endif


#ifdef BBGE_BUILD_WINDOWS
	#define byte char
#endif

ImageTGA *Texture::TGAload(const char *filename)
{
	unsigned long size = 0;
	char *rawbuf = readFile(filename, &size);
	ImageTGA *tga = TGAloadMem(rawbuf, size);
	if (rawbuf)
		delete [] rawbuf;
	if (!tga)
	{
		debugLog("Can't load TGA File!");
		return NULL;
	}
	return tga;
}

ImageTGA *Texture::TGAloadMem(void *mem, int size)
{
	if (!mem || size < 20)
		return NULL;

	ByteBuffer bb(mem, size, ByteBuffer::REUSE);

	ImageTGA *pImageData = NULL;		// This stores our important image data
	WORD width = 0, height = 0;			// The dimensions of the image
	byte length = 0;					// The length in bytes to the pixels
	byte imageType = 0;					// The image type (RLE, RGB, Alpha...)
	byte bits = 0;						// The bits per pixel for the image (16, 24, 32)
	int channels = 0;					// The channels of the image (3 = RGA : 4 = RGBA)
	int stride = 0;						// The stride (channels * width)
	int i = 0;							// A counter

	// This function loads in a TARGA (.TGA) file and returns its data to be
	// used as a texture or what have you.  This currently loads in a 16, 24
	// and 32-bit targa file, along with RLE compressed files.  Eventually you
	// will want to do more error checking to make it more robust.  This is
	// also a perfect start to go into a modular class for an engine.
	// Basically, how it works is, you read in the header information, then
	// move your file pointer to the pixel data.  Before reading in the pixel
	// data, we check to see the if it's an RLE compressed image.  This is because
	// we will handle it different.  If it isn't compressed, then we need another
	// check to see if we need to convert it from 16-bit to 24 bit.  24-bit and
	// 32-bit textures are very similar, so there's no need to do anything special.
	// We do, however, read in an extra bit for each color.


	// Allocate the structure that will hold our eventual image data (must free it!)
	pImageData = (ImageTGA*)malloc(sizeof(ImageTGA));

	// Read in the length in bytes from the header to the pixel data
	bb >> length;

	// Jump over one byte
	bb.skipRead(1);

	// Read in the imageType (RLE, RGB, etc...)
	//fread(&imageType, sizeof(byte), 1, pFile);
	bb >> imageType;

	// Skip past general information we don't care about
	bb.skipRead(9);

	// Read the width, height and bits per pixel (16, 24 or 32)
	bb >> width >> height >> bits;

	/*
	std::ostringstream os;
	os << "TGALoad: width: " << width << " height: " << height << " bits: " << bits;
	debugLog(os.str());
	*/

	// Now we move the file pointer to the pixel data
	bb.skipRead(length + 1);

	// Check if the image is RLE compressed or not
	if(imageType != TGA_RLE)
	{
		// Check if the image is a 24 or 32-bit image
		if(bits == 24 || bits == 32)
		{
			// Calculate the channels (3 or 4) - (use bits >> 3 for more speed).
			// Next, we calculate the stride and allocate enough memory for the pixels.
			channels = bits / 8;
			stride = channels * width;
			pImageData->data = new unsigned char[stride * height];

			// Load in all the pixel data line by line
			for(int y = 0; y < height; y++)
			{
				// Store a pointer to the current line of pixels
				unsigned char *pLine = &(pImageData->data[stride * y]);

				// Read in the current line of pixels
				if (bb.readable() < stride)
					break;
				bb.read(pLine, stride);

				// Go through all of the pixels and swap the B and R values since TGA
				// files are stored as BGR instead of RGB (or use GL_BGR_EXT verses GL_RGB)
				for(i = 0; i < stride; i += channels)
				{
					int temp     = pLine[i];
					pLine[i]     = pLine[i + 2];
					pLine[i + 2] = temp;
				}
			}
		}
		// Check if the image is a 16 bit image (RGB stored in 1 unsigned short)
		else if(bits == 16)
		{
			unsigned short pixels = 0;
			int r=0, g=0, b=0;

			// Since we convert 16-bit images to 24 bit, we hardcode the channels to 3.
			// We then calculate the stride and allocate memory for the pixels.
			channels = 3;
			stride = channels * width;
			pImageData->data = new unsigned char[stride * height];

			// Load in all the pixel data pixel by pixel
			for(int i = 0; i < width*height; i++)
			{
				// Read in the current pixel
				if (bb.readable() < sizeof(unsigned char))
					break;
				bb >> pixels;

				// To convert a 16-bit pixel into an R, G, B, we need to
				// do some masking and such to isolate each color value.
				// 0x1f = 11111 in binary, so since 5 bits are reserved in
				// each unsigned short for the R, G and B, we bit shift and mask
				// to find each value.  We then bit shift up by 3 to get the full color.
				b = (pixels & 0x1f) << 3;
				g = ((pixels >> 5) & 0x1f) << 3;
				r = ((pixels >> 10) & 0x1f) << 3;

				// This essentially assigns the color to our array and swaps the
				// B and R values at the same time.
				pImageData->data[i * 3 + 0] = r;
				pImageData->data[i * 3 + 1] = g;
				pImageData->data[i * 3 + 2] = b;
			}
		}
		// Else return a NULL for a bad or unsupported pixel format
		else
			return NULL;
	}
	// Else, it must be Run-Length Encoded (RLE)
	else
	{
		// First, let me explain real quickly what RLE is.
		// For further information, check out Paul Bourke's intro article at:
		// http://astronomy.swin.edu.au/~pbourke/dataformats/rle/
		//
		// Anyway, we know that RLE is a basic type compression.  It takes
		// colors that are next to each other and then shrinks that info down
		// into the color and a integer that tells how much of that color is used.
		// For instance:
		// aaaaabbcccccccc would turn into a5b2c8
		// Well, that's fine and dandy and all, but how is it down with RGB colors?
		// Simple, you read in an color count (rleID), and if that number is less than 128,
		// it does NOT have any optimization for those colors, so we just read the next
		// pixels normally.  Say, the color count was 28, we read in 28 colors like normal.
		// If the color count is over 128, that means that the next color is optimized and
		// we want to read in the same pixel color for a count of (colorCount - 127).
		// It's 127 because we add 1 to the color count, as you'll notice in the code.

		// Create some variables to hold the rleID, current colors read, channels, & stride.
		byte rleID = 0;
		int colorsRead = 0;
		channels = bits / 8;
		stride = channels * width;

		// Next we want to allocate the memory for the pixels and create an array,
		// depending on the channel count, to read in for each pixel.
		pImageData->data = new unsigned char[stride * height];
		byte *pColors = new byte [channels];

		// Load in all the pixel data
		while(i < width*height)
		{
			// Read in the current color count + 1
			bb >> rleID;

			// Check if we don't have an encoded string of colors
			if(rleID < 128)
			{
				// Increase the count by 1
				rleID++;

				// Go through and read all the unique colors found
				while(rleID)
				{
					// Read in the current color
					if (bb.readable() < channels)
						break;
					bb.read(pColors, channels);

					// Store the current pixel in our image array
					pImageData->data[colorsRead + 0] = pColors[2];
					pImageData->data[colorsRead + 1] = pColors[1];
					pImageData->data[colorsRead + 2] = pColors[0];

					// If we have a 4 channel 32-bit image, assign one more for the alpha
					if(bits == 32)
						pImageData->data[colorsRead + 3] = pColors[3];

					// Increase the current pixels read, decrease the amount
					// of pixels left, and increase the starting index for the next pixel.
					i++;
					rleID--;
					colorsRead += channels;
				}
			}
			// Else, let's read in a string of the same character
			else
			{
				// Minus the 128 ID + 1 (127) to get the color count that needs to be read
				rleID -= 127;

				// Read in the current color, which is the same for a while
				if (bb.readable() < channels)
					break;
				bb.read(pColors, channels);

				// Go and read as many pixels as are the same
				while(rleID)
				{
					// Assign the current pixel to the current index in our pixel array
					pImageData->data[colorsRead + 0] = pColors[2];
					pImageData->data[colorsRead + 1] = pColors[1];
					pImageData->data[colorsRead + 2] = pColors[0];

					// If we have a 4 channel 32-bit image, assign one more for the alpha
					if(bits == 32)
						pImageData->data[colorsRead + 3] = pColors[3];

					// Increase the current pixels read, decrease the amount
					// of pixels left, and increase the starting index for the next pixel.
					i++;
					rleID--;
					colorsRead += channels;
				}

			}

		}

		// Free up pColors
		delete[] pColors;
	}

	// Fill in our tImageTGA structure to pass back
	pImageData->channels = channels;
	pImageData->sizeX    = width;
	pImageData->sizeY    = height;

	// Return the TGA data (remember, you must free this data after you are done)
	return pImageData;
}

// ceil to next power of 2
static unsigned int clp2(unsigned int x)
{
	--x;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

unsigned char * Texture::getBufferAndSize(int *wparam, int *hparam, unsigned int *sizeparam)
{
	unsigned char *data = NULL;
	unsigned int size = 0;
	int tw = 0, th = 0;
	int w = 0, h = 0;

	// This can't happen. If it does we're doomed.
	if(width <= 0 || height <= 0)
		goto fail;

	glBindTexture(GL_TEXTURE_2D, textures[0]);

	// As returned by graphics driver

	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

	// As we know it - but round to nearest power of 2 - OpenGL does this internally anyways.
	tw = clp2(width); // known to be > 0.
	th = clp2(height);

	if (w != tw || h != th)
	{
		std::ostringstream os;
		os << "Texture::getBufferAndSize() WARNING: width/height disagree: ";
		os << "Driver says (" << w << ", " << h << "); ";
		os << "Texture says (" << width << ", " << height << "); ";
		os << "Rounded to (" << tw << ", " << th << ")";
		debugLog(os.str());
		// choose max. for size calculation
		w = w > tw ? w : tw;
		h = h > th ? h : th;
	}

	size = w * h * 4;
	if (!size)
		goto fail;

	data = (unsigned char*)malloc(size + 32);
	if (!data)
	{
		std::ostringstream os;
		os << "Game::fillGridFromQuad allocation failure, size = " << size;
		errorLog(os.str());
		goto fail;
	}
	memcpy(data + size, "SAFE", 5);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Not sure but this might be the case with nouveau drivers on linux... still investigating. -- fg
	if(memcmp(data + size, "SAFE", 5))
	{
		errorLog("Texture::getBufferAndSize(): Broken graphics driver! Wrote past end of buffer!");
		free(data); // in case we are here, this will most likely cause a crash.
		goto fail;
	}

	*wparam = w;
	*hparam = h;
	*sizeparam = size;
	return data;


fail:
	*wparam = 0;
	*hparam = 0;
	*sizeparam = 0;
	return NULL;
}
