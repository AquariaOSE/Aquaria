//*******************************************************************
//glfont2.cpp -- glFont Version 2.0 implementation
//Copyright (c) 1998-2002 Brad Fish
//See glfont.html for terms of use
//May 14, 2002
//*******************************************************************

//STL headers
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
using namespace std;

//OpenGL headers
/*
#ifdef _WINDOWS
#include <windows.h>
#endif
#include <OpenGL/gl.h>
*/

#include "GL/gl.h"

#include "SDL_endian.h"

//glFont header
#include "glfont2.h"
using namespace glfont;

static int read_int(ifstream &input)
{
	int buffer;
	
	input.read((char *)&buffer, 4);
	return SDL_SwapLE32(buffer);
}

static float read_float(ifstream &input)
{
	union
	{
		int i;
		float f; 
	} buffer;
	
	input.read((char *)&buffer.i, 4);
	buffer.i = SDL_SwapLE32(buffer.i);
	return buffer.f;
}

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
	ifstream input;
	int num_chars, num_tex_bytes;
	char *tex_bytes;

	//Destroy the old font if there was one, just to be safe
	Destroy();

	//Open input file
	input.open(file_name, ios::in | ios::binary);
	if (!input)
		return false;

	// Read the header from file
	header.tex = tex;
	input.seekg(4, ios::cur); // skip tex field
	header.tex_width = read_int(input);
	header.tex_height = read_int(input);
	header.start_char = read_int(input);
	header.end_char = read_int(input);
	input.seekg(4, ios::cur); // skip chars field
	
	//Allocate space for character array
	num_chars = header.end_char - header.start_char + 1;
	if ((header.chars = new GLFontChar[num_chars]) == NULL)
		return false;

	//Read character array
	for (int i = 0; i < num_chars; i++)
	{
		header.chars[i].dx = read_float(input);
		header.chars[i].dy = read_float(input);
		header.chars[i].tx1 = read_float(input);
		header.chars[i].ty1 = read_float(input);
		header.chars[i].tx2 = read_float(input);
		header.chars[i].ty2 = read_float(input);
	}

	//Read texture pixel data
	num_tex_bytes = header.tex_width * header.tex_height * 2;
	tex_bytes = new char[num_tex_bytes];
	input.read(tex_bytes, num_tex_bytes);


	//Build2DMipmaps(3, header.tex_width, header.tex_height, GL_UNSIGNED_BYTE, tex_bytes, 1);

	if (loadTexture)
	{
#ifdef BBGE_BUILD_OPENGL
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
#endif
	}

	//Free texture pixels memory
	delete[] tex_bytes;

	//Close input file
	input.close();

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
void GLFont::GetTexSize (std::pair<int, int> *size)
{
	//Retrieve texture size
	size->first = header.tex_width;
	size->second = header.tex_height;
}
//*******************************************************************
int GLFont::GetTexWidth (void)
{
	//Return texture width
	return header.tex_width;
}
//*******************************************************************
int GLFont::GetTexHeight (void)
{
	//Return texture height
	return header.tex_height;
}
//*******************************************************************
void GLFont::GetCharInterval (std::pair<int, int> *interval)
{
	//Retrieve character interval
	interval->first = header.start_char;
	interval->second = header.end_char;
}
//*******************************************************************
int GLFont::GetStartChar (void)
{
	//Return start character
	return header.start_char;
}
//*******************************************************************
int GLFont::GetEndChar (void)
{
	//Return end character
	return header.end_char;
}
//*******************************************************************
void GLFont::GetCharSize (int c, std::pair<int, int> *size)
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
int GLFont::GetCharWidth (int c)
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
			glfont_char->dx = glfont_a->dx*0.75;
			glfont_char->dy = glfont_a->dy;
		}

		return (int)(glfont_char->dx * header.tex_width);
	}
}
//*******************************************************************
int GLFont::GetCharHeight (int c)
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
void GLFont::Begin (void)
{
#ifdef BBGE_BUILD_OPENGL
	//Bind to font texture
	glBindTexture(GL_TEXTURE_2D, header.tex);
#endif
}
//*******************************************************************
void GLFont::GetStringSize (const std::string &text, std::pair<int, int> *size)
{
	unsigned int i;
	char c;
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
		c = (char)text[i];
		
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


