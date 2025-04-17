//*******************************************************************
//glfont2.h -- Header for glfont2.cpp
//Copyright (c) 1998-2002 Brad Fish
//See glfont.html for terms of use
//May 14, 2002
//*******************************************************************

#ifndef GLFONT2_H
#define GLFONT2_H

#include <assert.h>

// HACK: use engine functions because i can't be bothered to
// write pure opengl functions anymore until this all gets ripped
// out anyway
#include "../BBGE/VertexBuffer.h"

//*******************************************************************
//GLFont Interface
//*******************************************************************

//glFont namespace
namespace glfont
{
	class GLFont;
}

//glFont class
class glfont::GLFont
{
private:

	//glFont character structure
	typedef struct
	{
		float dx, dy;
		float tx1, ty1;
		float tx2, ty2;
	} GLFontChar;

	//glFont header structure
	struct
	{
		unsigned int tex;
		unsigned int tex_width, tex_height;
		unsigned int start_char, end_char;
		GLFontChar *chars;
	} header;

	mutable DynamicGPUBuffer vbo;

public:

	//Constructor
	GLFont ();

	//Destructor
	~GLFont ();

public:

	//Creates the glFont
	bool Create (const char *file_name, int tex, bool loadTexture=true);
	bool Create (const std::string &file_name, int tex, bool loadTexture=true);

	//Destroys the glFont
	void Destroy (void);

	//Texture size retrieval methods
	void GetTexSize (std::pair<int, int> *size) const;
	int GetTexWidth (void) const;
	int GetTexHeight (void) const;

	//Character interval retrieval methods
	void GetCharInterval (std::pair<int, int> *interval) const;
	int GetStartChar (void) const;
	int GetEndChar (void) const;

	//Character size retrieval methods
	void GetCharSize (unsigned char c, std::pair<int, int> *size) const;
	int GetCharWidth (unsigned char c) const;
	int GetCharHeight (unsigned char c) const;

	void GetStringSize (const std::string &text, std::pair<int, int> *size) const;


	//Begins text output with this font
	void Begin (void) const;

	//Template function to output a scaled, colored std::basic_string
	template<class T> void DrawString (
		const std::basic_string<T> &text, float scalar, float x,
		float y, const float *top_color, const float *bottom_color, float alpha, float lastAlpha) const
	{
		const size_t sz = text.size();
		const size_t bytes = sz * 4 * (2+2+4) * sizeof(float);

		const float tw = header.tex_width * scalar;
		const float th = header.tex_height * scalar;

		do
		{
			float *p = (float*)vbo.beginWrite(GPUBUFTYPE_VEC2_TC_RGBA, bytes, GPUACCESS_DEFAULT);

			for(size_t i = 0; i < text.size(); ++i)
			{
				//Make sure character is in range
				unsigned int c = (unsigned int)text[i];
				if (c < header.start_char || c > header.end_char)
					continue;

				const GLFontChar &fc = header.chars[c - header.start_char];

				const float width = fc.dx * tw;
				const float height = fc.dy * th;

				float a = alpha;
				if (i == (sz-1))
					a *= lastAlpha;

				*p++ = x; // x
				*p++ = y; // y
				*p++ = fc.tx1; // u
				*p++ = fc.ty1; // v
				*p++ = top_color[0];
				*p++ = top_color[1];
				*p++ = top_color[2];
				*p++ = a;

				*p++ = x + width; // x
				*p++ = y; // y
				*p++ = fc.tx2; // u
				*p++ = fc.ty1; // v
				*p++ = top_color[0];
				*p++ = top_color[1];
				*p++ = top_color[2];
				*p++ = a;

				*p++ = x + width; // x
				*p++ = y + height; // y
				*p++ = fc.tx2; // u
				*p++ = fc.ty2; // v
				*p++ = bottom_color[0];
				*p++ = bottom_color[1];
				*p++ = bottom_color[2];
				*p++ = a;

				*p++ = x; // x
				*p++ = y + height; // y
				*p++ = fc.tx1; // u
				*p++ = fc.ty2; // v
				*p++ = bottom_color[0];
				*p++ = bottom_color[1];
				*p++ = bottom_color[2];
				*p++ = a;

				x += width;
			}
		}
		while(!vbo.commitWrite());

		vbo.apply();

		size_t last = sz * 4;
		for(size_t i = 0; i < last; i += 4)
			glDrawArrays(GL_TRIANGLE_FAN, i, 4);
	}
};

//*******************************************************************

#endif

//End of file


