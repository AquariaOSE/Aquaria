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
	components = 0;
#ifdef BBGE_BUILD_OPENGL
	textures[0] = 0;
#endif
#ifdef BBGE_BUILD_DIRECTX
	d3dTexture = 0;
#endif
	width = height = 0;

	repeat = false;
	pngSetStandardOrientation(0);
	imageData = 0;
	layer = 0;

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
	float w, h, c;
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &c);// assume 4
	int size = w*h*c;
	unsigned char *data=0;
	data = (unsigned char*)malloc(size*sizeof(char));
	if (c == 4)
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	/*
	else if (c == 3)
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	*/
	else
	{
		if (data)
			free(data);
		return 0;
	}

	int smallestx = -1, largestx = -1;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			int p = (y*w*c) + x*c;
			if (data[p+3] >= 254)
			{
				if (smallestx == -1 || x < smallestx)
					smallestx = x;
				if (largestx == -1 || x > largestx)
					largestx = x;
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	free(data);
	return largestx - smallestx;
#elif defined(BBGE_BUILD_DIRECTX)
	return 0;
#endif
}

int Texture::getPixelHeight()
{
#ifdef BBGE_BUILD_OPENGL
	float w, h, c;
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
	glGetTexLevelParameterfv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &c);// assume 4
	int size = w*h*c;
	unsigned char *data=0;
	data = (unsigned char*)malloc(size*sizeof(char));
	if (c == 4)
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	/*
	else if (c == 3)
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	*/
	else
	{
		if (data)
			free(data);
		return 0;
	}
	int smallesty = -1, largesty = -1;
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			int p = (y*w*c) + x*c;
			if (data[p+3] >= 254)
			{
				if (smallesty == -1 || y < smallesty)
					smallesty = y;
				if (largesty == -1 || y > largesty)
					largesty = y;
			}
		}
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	if (data)
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
	if (true)
	{
		unload();
		load(loadName);

		if (ow != -1 && oh != -1)
		{
			width = ow;
			height = oh;
		}
	}
	else
	{
		debugLog("name was too short, didn't load");
	}
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

	size_t pos = file.find_last_of('.');

	if ((pos != std::string::npos) && (pos >= 0))
	{
		// make sure this didn't catch the '.' in /home/username/.Aquaria/*  --ryan.
		const std::string userdata = core->getUserDataFolder();
		const size_t len = userdata.length();
		if (pos < len)
			pos = std::string::npos;
	}

	if (core->debugLogTextures)
	{
		std::ostringstream os;
		os << "pos [" << pos << "], file :" << file;
		debugLog(os.str());
	}

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
			if (core->getUserDataFolder().empty())
			{
				unpackFile(file, "poot.tmp");
				loadTGA("poot.tmp");
				remove("poot.tmp");
			}
			else
			{
				unpackFile(file, core->getUserDataFolder() + "/poot.tmp");
				loadTGA(core->getUserDataFolder() + "/poot.tmp");
				remove((core->getUserDataFolder() + "/poot.tmp").c_str());
			}

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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
#endif
#ifdef BBGE_BUILD_DIRECTX
	core->getD3DDevice()->SetTexture(0, d3dTexture);

#endif
}

void Texture::unbind()
{
}

void Texture::setLayer(int layer)
{
	this->layer = layer;
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

	if (filter == GL_NEAREST)
	{
		textures[0] = pngBind(file.c_str(), PNG_NOMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, filter, filter);
	}
	else
	{
		textures[0] = pngBind(file.c_str(), PNG_BUILDMIPMAPS, pngType, &info, GL_CLAMP_TO_EDGE, GL_LINEAR_MIPMAP_LINEAR, filter);
	}


	if (info.Alpha)
		components = 4;
	else
		components = 3;
	/*
	pngRawInfo rawinfo;
	bool success = pngLoadRaw(file.c_str(), &rawinfo);
	glBindTexture(GL_TEXTURE_2D, id);
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, rawinfo.Width, rawinfo.Height,
                GL_RGB, GL_UNSIGNED_BYTE, rawinfo.Data);
	*/
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
#ifdef BBGE_BUILD_GLFW
	GLFWimage image;
	glfwReadImage(file.c_str(), &image, 0);
	width = image.Width;
	height = image.Height;
	glfwFreeImage(&image);


	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glfwLoadTexture2D(file.c_str(), 0);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);	// Linear Filtering
#endif


	/*
	glfwLoadTexture2D(file.c_str(), 0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);	// Linear Filtering
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);	// Linear Filtering
	width = imageTGA->sizeX;
	height = imageTGA->sizeY;
	*/

#ifdef BBGE_BUILD_SDL
	ImageTGA *imageTGA;

	if ((imageTGA = TGAload(file.c_str())) != 0)
	{
		glGenTextures(1, &textures[0]);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter);	// Linear Filtering
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);	// Linear Filtering

		if (imageTGA->channels==3)
			glTexImage2D(GL_TEXTURE_2D, 0, 3, imageTGA->sizeX, imageTGA->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, imageTGA->data);
		else if (imageTGA->channels==4)
		{
			//errorLog("4 channels");
			glTexImage2D(GL_TEXTURE_2D, 0, 4,imageTGA->sizeX, imageTGA->sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageTGA->data);
		}
		width = imageTGA->sizeX;
		height = imageTGA->sizeY;
	}
	if (imageTGA)
	{
		if (imageTGA->data)
			delete[] (imageTGA->data);
		free (imageTGA);
	}
#endif
}


#define TGA_RGB		 2		// This tells us it's a normal RGB (really BGR) file
#define TGA_A		 3		// This tells us it's a ALPHA file
#define TGA_RLE		10		// This tells us that the targa is Run-Length Encoded (RLE)

#if defined(BBGE_BUILD_UNIX)
typedef uint8_t byte;
typedef uint16_t WORD;
#endif


static int fread_int(FILE *file, int size)
{
	int buffer;
	
	//input.read((char *)&buffer, 4);
	if (fread(&buffer, size, 1, file) != 1)
		return 0;
#ifdef BBGE_BUILD_SDL
	return SDL_SwapLE32(buffer);
#else
	return buffer;
#endif
}

#ifdef BBGE_BUILD_WINDOWS
	#define byte char
#endif

ImageTGA *Texture::TGAload(const char *filename)
{
/*
	//HACK: function isn't macosx friendly
	return 0;
*/
	ImageTGA *pImageData = NULL;		// This stores our important image data
	WORD width = 0, height = 0;			// The dimensions of the image
	byte length = 0;					// The length in bytes to the pixels
	byte imageType = 0;					// The image type (RLE, RGB, Alpha...)
	byte bits = 0;						// The bits per pixel for the image (16, 24, 32)
	FILE *pFile = NULL;					// The file pointer
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

	// Open a file pointer to the targa file and check if it was found and opened

	if((pFile = fopen(core->adjustFilenameCase(filename).c_str(), "rb")) == NULL)  //, "rb" // openRead(fn)
	{
		// Display an error message saying the file was not found, then return NULL
		debugLog("Unable to load TGA File!");
		return NULL;
	}

	// Allocate the structure that will hold our eventual image data (must free it!)
	pImageData = (ImageTGA*)malloc(sizeof(ImageTGA));

	// Read in the length in bytes from the header to the pixel data
	//fread(&length, sizeof(byte), 1, pFile);
	length = fread_int(pFile, sizeof(byte));

	// Jump over one byte
	fseek(pFile,1,SEEK_CUR);

	// Read in the imageType (RLE, RGB, etc...)
	//fread(&imageType, sizeof(byte), 1, pFile);
	imageType = fread_int(pFile, sizeof(byte));

	// Skip past general information we don't care about
	fseek(pFile, 9, SEEK_CUR);

	// Read the width, height and bits per pixel (16, 24 or 32)
	/*
	fread(&width,  sizeof(WORD), 1, pFile);
	fread(&height, sizeof(WORD), 1, pFile);
	fread(&bits,   sizeof(byte), 1, pFile);
	*/
	width = fread_int(pFile, sizeof(WORD));
	height = fread_int(pFile, sizeof(WORD));
	bits = fread_int(pFile, sizeof(byte));
	
	/*
	std::ostringstream os;
	os << "TGALoad: width: " << width << " height: " << height << " bits: " << bits;
	debugLog(os.str());
	*/

	// Now we move the file pointer to the pixel data
	fseek(pFile, length + 1, SEEK_CUR);

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
				if (fread(pLine, stride, 1, pFile) != 1)
					break;

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
				if (fread(&pixels, sizeof(unsigned short), 1, pFile) != 1)
					break;

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
			if (fread(&rleID, sizeof(byte), 1, pFile) != 1)
				break;

			// Check if we don't have an encoded string of colors
			if(rleID < 128)
			{
				// Increase the count by 1
				rleID++;

				// Go through and read all the unique colors found
				while(rleID)
				{
					// Read in the current color
					if (fread(pColors, sizeof(byte) * channels, 1, pFile) != 1)
						break;

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
				if (fread(pColors, sizeof(byte) * channels, 1, pFile) != 1)
					break;

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

	// Close the file pointer that opened the file
	fclose(pFile);

	// Fill in our tImageTGA structure to pass back
	pImageData->channels = channels;
	pImageData->sizeX    = width;
	pImageData->sizeY    = height;

	// Return the TGA data (remember, you must free this data after you are done)
	return pImageData;
}

