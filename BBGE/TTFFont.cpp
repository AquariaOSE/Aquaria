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

#include "TTFFont.h"

#ifdef AQUARIA_INTERNAL_FTGL
#include <ft2build.h>
#include FT_FREETYPE_H

#include "FTGL.h"
#include "FTGLTextureFont.h"
#else
#include <FTGL/ftgl.h>
#endif

#undef min
#undef max
#undef GetCharWidth

TTFFont::TTFFont()
{
	font = 0;
}

TTFFont::~TTFFont()
{
	if (font)
	{
		delete font;
		font = 0;
	}
}

void TTFFont::destroy()
{
	if (font)
	{
		delete font;
		font = 0;
	}
}

void TTFFont::load(const std::string &str, int sz)
{
	font = new FTGLTextureFont(str.c_str());
	font->FaceSize(sz);
}

void TTFFont::create(const unsigned char *data, unsigned long datalen, int sz)
{
	font = new FTGLTextureFont(data, datalen);
	font->FaceSize(sz);
}

TTFText::TTFText(TTFFont *f) : font(f)
{
	align = ALIGN_LEFT;
	hw = 0;
	h = 0;
	width = 0;
	shadow = false;
	maxW = 0;
}

void TTFText::setText(const std::string &txt)
{
	originalText = txt;
	updateAlign();
	updateFormatting();
}

void TTFText::setAlign(Align align)
{
	this->align = align;
	updateAlign();
	updateFormatting();
}

void TTFText::updateAlign()
{
	float llx, lly, llz, urx, ury, urz;
	font->font->BBox(originalText.c_str(), llx, lly, llz, urx, ury, urz);
	if (align == ALIGN_CENTER)
	{
		float w = urx - llx;
		hw = w/2;
		h = ury - lly;
	}
	else
	{
		hw = 0;
	}
}

size_t TTFText::getNumLines() const
{
	return (int)text.size();
}

float TTFText::getHeight() const
{
	return text.size()*lineHeight;
}

float TTFText::getStringWidth(const std::string& s) const
{
	float w = 0;
	std::string cp = s;
	float llx, lly, llz, urx, ury, urz;
	const char *start = cp.c_str();
	size_t begin = 0;
	for(size_t i = 0; i < cp.length(); ++i)
	{
		const char c = cp[i];
		if(c == '\n')
		{
			cp[i] = 0;
			const char *p = start + begin;
			font->font->BBox(p, llx, lly, llz, urx, ury, urz);
			w = std::max(w, urx - llx);
			cp[i] = c;
			begin = i + 1;
		}
	}
	if(begin < cp.length())
	{
		font->font->BBox(start + begin, llx, lly, llz, urx, ury, urz);
		w = std::max(w, urx - llx);
	}
	return w;
}

void TTFText::setWidth(float width)
{
	this->width = width;

	updateAlign();
	updateFormatting();
}

void TTFText::setFontSize(float)
{
}

void TTFText::updateFormatting()
{
	int start = 0, lastSpace = -1;
	text.clear();
	int i=0;
	int sz = originalText.size();
	maxW = 0;
	for (i = 0; i < sz; i++)
	{
		if (originalText[i] == '\n')
		{
			std::string part = originalText.substr(start, i-start);
			text.push_back(part);
			start = i+1;
			float llx, lly, llz, urx, ury, urz;
			font->font->BBox(part.c_str(), llx, lly, llz, urx, ury, urz);
			float w = urx - llx;
			maxW = std::max(maxW, w);
		}
		else
		{
			if (originalText[i] == ' ')
			{
				lastSpace = i;
			}
			float llx, lly, llz, urx, ury, urz;
			std::string part = originalText.substr(start, i-start);
			font->font->BBox(part.c_str(), llx, lly, llz, urx, ury, urz);
			float w = urx - llx;

			if (width != 0 && w >= width)
			{
				if (lastSpace != -1)
				{
					part = originalText.substr(start, lastSpace-start);
					i = lastSpace+1;
					lastSpace = -1;
					start = i;
				}
				else
					part = originalText.substr(start, i-start);

				text.push_back(part);
				// recalc width of remaining text after linebreak
				font->font->BBox(part.c_str(), llx, lly, llz, urx, ury, urz);
				w = urx - llx;
			}

			maxW = std::max(maxW, w);
		}
	}
	if (i == sz)
	{
		std::string part = originalText.substr(start, i-start);
		text.push_back(part);
		float llx, lly, llz, urx, ury, urz;
		font->font->BBox(part.c_str(), llx, lly, llz, urx, ury, urz);
		float w = urx - llx;
		maxW = std::max(maxW, w);
	}
	lineHeight = font->font->LineHeight();
}

float TTFText::getLineHeight() const
{
	return lineHeight;
}

int TTFText::findLine(const std::string &label)
{
	for (size_t i = 0; i < text.size(); i++)
	{
		if (text[i].find(label) != std::string::npos)
		{
			return i;
		}
	}
	return 0;
}

void TTFText::onRender(const RenderState& rs) const
{


	for (size_t i = 0; i < text.size(); i++)
	{
		if (shadow)
		{
			glColor4f(0,0,0,0.75f*alpha.x*alphaMod);
			glPushMatrix();
			glScalef(1, -1, 0);
			glTranslatef(1 -hw, -1 + (i*-lineHeight), 0);
			font->font->Render(text[i].c_str());
			glPopMatrix();
		}


		glColor4f(color.x, color.y, color.z, alpha.x*alphaMod);
		glPushMatrix();
		glScalef(1, -1, 0);
		glTranslatef(-hw, 0 + (i*-lineHeight), 0);
		font->font->Render(text[i].c_str());
		glPopMatrix();
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	RenderObject::lastTextureApplied = 0;



}
