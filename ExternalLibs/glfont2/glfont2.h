//*******************************************************************
//glfont2.h -- Header for glfont2.cpp
//Copyright (c) 1998-2002 Brad Fish
//See glfont.html for terms of use
//May 14, 2002
//*******************************************************************

#ifndef GLFONT2_H
#define GLFONT2_H

#include <assert.h>

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
		unsigned int i;
		GLFontChar *glfont_char;
		float width, height;

		//Begin rendering quads
		glBegin(GL_QUADS);

		unsigned int sz = text.size();

		float a = 0;
		//Loop through characters
		for (i = 0; i < sz; i++)
		{
			//Make sure character is in range
			unsigned int c = (unsigned char)text[i];
			if (c < header.start_char || c > header.end_char)
				continue;

			//Get pointer to glFont character
			glfont_char = &header.chars[c - header.start_char];

			//Get width and height
			width = (glfont_char->dx * header.tex_width) * scalar;
			height = (glfont_char->dy * header.tex_height) * scalar;

			if (i == (sz-1))
				a = alpha*lastAlpha;
			else
				a = alpha;

			//Specify colors, vertices, and texture coordinates
			glColor4f(top_color[0], top_color[1], top_color[2], a);
			glTexCoord2f(glfont_char->tx1, glfont_char->ty1);
			glVertex3f(x, y, 0.0F);
			glTexCoord2f(glfont_char->tx2, glfont_char->ty1);
			glVertex3f(x + width, y, 0.0F);
			glColor4f(bottom_color[0], bottom_color[1], bottom_color[2], a);
			glTexCoord2f(glfont_char->tx2, glfont_char->ty2);
			glVertex3f(x + width, y + height, 0.0F);
			glTexCoord2f(glfont_char->tx1, glfont_char->ty2);
			glVertex3f(x, y + height, 0.0F);

			//Move to next character
			x += width;
		}

		//Stop rendering quads
		glEnd();
	}
};

//*******************************************************************

#endif

//End of file


