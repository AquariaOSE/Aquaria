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
#include "Core.h"
#include "TTFFont.h"


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
    ttvfs::VFSFile *vf = core->vfs.GetFile(str.c_str());
    if(!vf)
    {
        font = new FTGLTextureFont(str.c_str()); // file not in VFS, just pretend nothing happened
        font->FaceSize(sz);
        return;
    }

    const unsigned char *buf = (const unsigned char*)vf->getBuf();
    create(buf, vf->size(), sz); // this copies the buffer internally
    core->addVFSFileForDrop(vf); // so we can delete our own
}

void TTFFont::create(const unsigned char *data, unsigned long datalen, int sz)
{
	font = new FTGLTextureFont(data, datalen);
	font->FaceSize(sz);
}

TTFText::TTFText(TTFFont *font) : RenderObject(), font(font)
{
	align = ALIGN_LEFT;
	hw = 0;
	h = 0;
	width = 0;
	shadow = false;
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

float TTFText::getWidth()
{
	float llx, lly, llz, urx, ury, urz;
	font->font->BBox(originalText.c_str(), llx, lly, llz, urx, ury, urz);
	return urx - llx;
}

float TTFText::getHeight()
{
	float llx, lly, llz, urx, ury, urz;
	font->font->BBox(originalText.c_str(), llx, lly, llz, urx, ury, urz);
	return ury - lly;
}

float TTFText::getFullHeight()
{
	/*
	float llx, lly, llz, urx, ury, urz;
	font->font->BBox(originalText.c_str(), llx, lly, llz, urx, ury, urz);
	float diff = ury - lly;
	*/
	return text.size()*lineHeight;
}

void TTFText::setWidth(int width)
{
	this->width = width;

	updateAlign();
	updateFormatting();
}

void TTFText::updateFormatting()
{
	int start = 0, lastSpace = -1;
	text.clear();
	int i=0;
	int sz = originalText.size();
	for (i = 0; i < sz; i++)
	{
		if (originalText[i] == '\n')
		{
			text.push_back(originalText.substr(start, i-start));
			start = i+1;
		}
		else
		{
			if (originalText[i] == ' ')
			{
				lastSpace = i;
			}
			float llx, lly, llz, urx, ury, urz;
			font->font->BBox(originalText.substr(start, i-start).c_str(), llx, lly, llz, urx, ury, urz);
			int w = urx - llx;
			if (width != 0 && w >= width)
			{
				if (lastSpace != -1) {
					text.push_back(originalText.substr(start, lastSpace-start));
					i = lastSpace+1;
					lastSpace = -1;
					start = i;
				}
				else {
					text.push_back(originalText.substr(start, i-start));
				}
			}
		}
	}
	if (i == sz)
	{
		text.push_back(originalText.substr(start, i-start));
	}
	lineHeight = font->font->LineHeight();
}

void TTFText::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
}

int TTFText::getLineHeight()
{
	return lineHeight;
}

int TTFText::findLine(const std::string &label)
{
	for (int i = 0; i < text.size(); i++)
	{
		if (text[i].find(label) != std::string::npos)
		{
			return i;
		}
	}
	return 0;
}

void TTFText::onRender()
{
	/*
	glColor4f(0,0,0,0.5);
	glBegin(GL_QUADS);
	glVertex2f(-hw, h/2);
	glVertex2f(hw, h/2);
	glVertex2f(hw, -h/2);
	glVertex2f(-hw, -h/2);
	glEnd();
	*/

	for (int i = 0; i < text.size(); i++)
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


	/*
	glBindTexture(GL_TEXTURE_2D, 0);
	glLineWidth(2);
	glBegin(GL_LINES);
	glVertex2f(-hw, 5);
	glVertex2f(hw, 5);
	glEnd();
	*/
}
