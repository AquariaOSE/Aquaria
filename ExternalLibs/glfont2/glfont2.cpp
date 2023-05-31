//*******************************************************************
//glfont2.cpp -- glFont Version 2.0 implementation
//Copyright (c) 1998-2002 Brad Fish
//See glfont.html for terms of use
//May 14, 2002
//*******************************************************************

//STL headers
#include <string>
#include "ttvfs_stdio.h"
#include "ByteBuffer.h"
using namespace std;


#ifdef _WIN32 /* Stupid Windows needs to include windows.h before gl.h */
#undef FAR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetCharWidth
#endif
#include <GL/gl.h>

//glFont header
#include "glfont2.h"
using namespace glfont;

//*******************************************************************
//GLFont Class Implementation
//*******************************************************************
GLFont::GLFont ()
{
	//Initialize header to safe state
	header.tex = -1;
	header.tex_width = 0;
	header.tex_height = 0;
	header.start_char = 0;
	header.end_char = 0;
	header.chars = NULL;
}
//*******************************************************************
GLFont::~GLFont ()
{
	//Destroy the font
	Destroy();
}
//*******************************************************************
bool GLFont::Create (const char *file_name, int tex, bool loadTexture)
{
	ByteBuffer::uint32 num_chars, num_tex_bytes;
	char *tex_bytes;

	//Destroy the old font if there was one, just to be safe
	Destroy();


	VFILE *fh = vfopen(file_name, "rb");
	if (!fh)
		return false;

	size_t sz = 0;
	if(vfsize(fh, &sz) < 0)
	{
		vfclose(fh);
		return false;
	}

	ByteBuffer bb(sz);
	bb.resize(sz);
	vfread(bb.contents(), 1, sz, fh);
	vfclose(fh);

	// Read the header from file
	header.tex = tex;
	bb.skipRead(4); // skip tex field
	header.tex_width = bb.read<ByteBuffer::uint32>();
	header.tex_height = bb.read<ByteBuffer::uint32>();
	header.start_char = bb.read<ByteBuffer::uint32>();
	header.end_char = bb.read<ByteBuffer::uint32>();
	bb.skipRead(4); // skip chars field

	//Allocate space for character array
	num_chars = header.end_char - header.start_char + 1;
	if ((header.chars = new GLFontChar[num_chars]) == NULL)
		return false;

	//Read character array
	for (unsigned int i = 0; i < num_chars; i++)
	{
		bb >> header.chars[i].dx;
		bb >> header.chars[i].dy;
		bb >> header.chars[i].tx1;
		bb >> header.chars[i].ty1;
		bb >> header.chars[i].tx2;
		bb >> header.chars[i].ty2;
	}

	//Read texture pixel data
	num_tex_bytes = header.tex_width * header.tex_height * 2;
	tex_bytes = new char[num_tex_bytes];
	// HACK: Aquaria uses override textures, so we can live with the truncation.
	bb.read(tex_bytes, std::min(num_tex_bytes, bb.readable()));

	//Build2DMipmaps(3, header.tex_width, header.tex_height, GL_UNSIGNED_BYTE, tex_bytes, 1);

	if (loadTexture)
	{
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D(GL_TEXTURE_2D, 0, 2, header.tex_width,
			header.tex_height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
			(void *)tex_bytes);
		//gluBuild2DMipmaps(GL_TEXTURE_2D, 2, header.tex_width, header.tex_height, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (void*)tex_bytes);
		//Build2DMipmaps(3, header.tex_width, header.tex_height, GL_LUMINANCE_ALPHA, tex_bytes, 1);
		//Create OpenGL texture
		/*
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		*/
	}

	//Free texture pixels memory
	delete[] tex_bytes;

	//Return successfully
	return true;
}
//*******************************************************************
bool GLFont::Create (const std::string &file_name, int tex, bool loadTexture)
{
	return Create(file_name.c_str(), tex);
}
//*******************************************************************
void GLFont::Destroy (void)
{
	//Delete the character array if necessary
	if (header.chars)
	{
		delete[] header.chars;
		header.chars = NULL;
	}
}
//*******************************************************************
void GLFont::GetTexSize (std::pair<int, int> *size) const
{
	//Retrieve texture size
	size->first = header.tex_width;
	size->second = header.tex_height;
}
//*******************************************************************
int GLFont::GetTexWidth (void) const
{
	//Return texture width
	return header.tex_width;
}
//*******************************************************************
int GLFont::GetTexHeight (void) const
{
	//Return texture height
	return header.tex_height;
}
//*******************************************************************
void GLFont::GetCharInterval (std::pair<int, int> *interval) const
{
	//Retrieve character interval
	interval->first = header.start_char;
	interval->second = header.end_char;
}
//*******************************************************************
int GLFont::GetStartChar (void) const
{
	//Return start character
	return header.start_char;
}
//*******************************************************************
int GLFont::GetEndChar (void) const
{
	//Return end character
	return header.end_char;
}
//*******************************************************************
void GLFont::GetCharSize (unsigned char c, std::pair<int, int> *size) const
{
	//Make sure character is in range
	if (c < header.start_char || c > header.end_char)
	{
		//Not a valid character, so it obviously has no size
		size->first = 0;
		size->second = 0;
	}
	else
	{
		GLFontChar *glfont_char;

		//Retrieve character size
		glfont_char = &header.chars[c - header.start_char];
		size->first = (int)(glfont_char->dx * header.tex_width);
		size->second = (int)(glfont_char->dy *
			header.tex_height);
	}
}
//*******************************************************************
int GLFont::GetCharWidth (unsigned char c) const
{
	//Make sure in range
	if (c < header.start_char || c > header.end_char)
		return 0;
	else
	{
		GLFontChar *glfont_char;

		//Retrieve character width
		glfont_char = &header.chars[c - header.start_char];

		// hack to fix empty spaces
		if (c == ' ' && glfont_char->dx <= 0)
		{
			GLFontChar *glfont_a = &header.chars['a' - header.start_char];
			glfont_char->dx = glfont_a->dx*0.75f;
			glfont_char->dy = glfont_a->dy;
		}

		return (int)(glfont_char->dx * header.tex_width);
	}
}
//*******************************************************************
int GLFont::GetCharHeight (unsigned char c) const
{
	//Make sure in range
	if (c < header.start_char || c > header.end_char)
		return 0;
	else
	{
		GLFontChar *glfont_char;

		//Retrieve character height
		glfont_char = &header.chars[c - header.start_char];
		return (int)(glfont_char->dy * header.tex_height);
	}
}
//*******************************************************************
void GLFont::Begin (void) const
{
	//Bind to font texture
	glBindTexture(GL_TEXTURE_2D, header.tex);
}
//*******************************************************************
void GLFont::GetStringSize (const std::string &text, std::pair<int, int> *size) const
{
	unsigned int i;
	unsigned int c;
	GLFontChar *glfont_char;
	float width;

	//debugLog("size->second");
	//Height is the same for now...might change in future
	size->second = (int)(header.chars[header.start_char].dy *
		header.tex_height);

	//Calculate width of string
	width = 0.0F;
	for (i = 0; i < text.size(); i++)
	{
		//Make sure character is in range
		c = (unsigned char)text[i];

		if (c < header.start_char || c > header.end_char)
			continue;

		//Get pointer to glFont character
		glfont_char = &header.chars[c - header.start_char];

		//Get width and height
		width += glfont_char->dx * header.tex_width;
	}

	//Save width
	//debugLog("size first");
	size->first = (int)width;

	//debugLog("done");
}

//End of file


