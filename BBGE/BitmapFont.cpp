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
#include "BitmapFont.h"
#include "RenderBase.h"
#include "Core.h"

#include "../ExternalLibs/glfont2/glfont2.h"

using namespace glfont;

BmpFont::BmpFont()
: font(new GLFont)
{
	scale = 1;
	loaded = false;
	overrideTexture = 0;
	fontTopColor = Vector(1,1,1);
	fontBtmColor = Vector(1,1,1);
}

BmpFont::~BmpFont()
{
	delete font;
	destroy();
}

void BmpFont::destroy()
{
	if (loaded)
	{
		font->Destroy();
		loaded = false;
	}

	overrideTexture = NULL;
}

void BmpFont::load(const std::string &file, float scale, bool loadTexture)
{
	if (loaded)
		font->Destroy();

	this->scale = scale;

	GLuint id=0;
	glGenTextures(1, &id);

	if (!font->Create(file.c_str(), id, loadTexture))
		return;


	loaded = true;
}

BitmapText::BitmapText(const BmpFont& bmpFont)
	: bmpFont(bmpFont)
{
	currentScrollLine = currentScrollChar = 0;
	scrollDelay = 0;
	scrolling = false;
	align = ALIGN_CENTER;
	textWidth = 600;
	this->fontDrawSize = 24;

	cull = false;


	alignWidth = 0;


}

int BitmapText::getWidthOnScreen()
{
	return text.size()*(fontDrawSize/2);
}

void BitmapText::setAlign(Align align)
{
	this->align = align;
}

const std::string& BitmapText::getText()
{
	return this->text;
}

void BitmapText::setText(const std::string &text)
{
	this->text = text;
	formatText();
}

void BitmapText::setWidth(float width)
{
	textWidth = width;
}

float BitmapText::getSetWidth()
{
	return textWidth;
}

float BitmapText::getHeight() const
{
	float sz = bmpFont.font->GetCharHeight('A') * bmpFont.scale;
	return lines.size()*sz;
}

float BitmapText::getLineHeight() const
{
	return bmpFont.font->GetCharHeight('A') * bmpFont.scale;
}

void BitmapText::formatText()
{
	std::string text;
	text = this->text;
	lines.clear();
	std::string currentLine;
	size_t lastSpace = size_t(-1);
	float currentWidth = 0;
	alignWidth = 0;
	maxW = 0;
	for (size_t i = 0; i < text.size(); i++)
	{

		float sz = bmpFont.font->GetCharWidth(text[i])*bmpFont.scale;
		currentWidth += sz;

		if (currentWidth+sz >= textWidth || text[i] == '\n')
		{
			if (text[i] == '\n')
			{
				lastSpace = i;
			}
			lines.push_back(text.substr(0, lastSpace));
			size_t tsz = text.size();
			text = text.substr(lastSpace+1, tsz);
			i = 0;
			alignWidth = currentWidth;
			maxW = std::max(currentWidth, maxW);
			currentWidth = 0;
			lastSpace = 0;
			continue;
		}

		if (text[i] == ' ')
		{
			lastSpace = i;
		}
	}
	maxW = std::max(currentWidth, maxW);
	if (alignWidth == 0)
		alignWidth = currentWidth;
	if (!text.empty() && (text.size() > 1 || text[0] != ' '))
	{
		lines.push_back(text);
	}
}

bool BitmapText::isEmpty()
{
	return lines.empty();
}

void BitmapText::scrollText(const std::string &text, float scrollSpeed)
{
	if (text.empty()) return;
	this->scrollSpeed = scrollSpeed;
	scrollDelay = scrollSpeed;
	this->text = text;
	scrolling = true;
	formatText();
	currentScrollLine = 0;
	currentScrollChar = 0;
}

void BitmapText::setFontSize(float sz)
{
	this->fontDrawSize = sz;
}

void BitmapText::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	if (scrollDelay > 0 && scrolling)
	{
		if (lines.empty())
		{
			currentScrollLine = 0;
			scrolling = false;
			scrollDelay = 0;
		}
		else
		{
			scrollDelay -= dt;
			while (scrollDelay <= 0)
			{
				float diff = scrollDelay;
				scrollDelay = scrollSpeed;
				scrollDelay += diff;
				currentScrollChar ++;
				if (currentScrollChar >= lines[currentScrollLine].size())
				{
					currentScrollLine++;
					currentScrollChar = 0;
					if (currentScrollLine >= lines.size())
					{
						currentScrollLine = 0;
						scrolling = false;
					}
				}
			}
		}
	}
}

void BitmapText::onRender(const RenderState& rs) const
{
	const Vector top = bmpFont.fontTopColor * color;
	const Vector btm = bmpFont.fontBtmColor * color;

	glEnable(GL_TEXTURE_2D);


	const glfont::GLFont * const font = bmpFont.font;
	font->Begin();

	if (bmpFont.overrideTexture) bmpFont.overrideTexture->apply();

	const float scale = bmpFont.scale;
	float y=0;
	float x=0;

	float adj = font->GetCharHeight('A') * scale;

	if (scrolling)
	{
		for (size_t i = 0; i <= currentScrollLine; i++)
		{
			std::string theLine = lines[i];
			if (i == currentScrollLine)
				theLine = theLine.substr(0, currentScrollChar);

			x=0;
			if (align == ALIGN_CENTER)
			{
				std::pair<int, int> sz;
				font->GetStringSize(lines[i], &sz);
				x = -sz.first*0.5f*scale;
			}
			float la = 1.0f-(scrollDelay/scrollSpeed);


			font->DrawString(theLine, scale, x, y, &top.x, &btm.x, alpha.x, la);
			y += adj;
		}
	}
	else
	{
		for (size_t i = 0; i < lines.size(); i++)
		{
			x=0;
			if (align == ALIGN_CENTER)
			{
				std::pair<int, int> sz;
				font->GetStringSize(lines[i], &sz);
				x = -sz.first*0.5f*scale;
			}
			font->DrawString(lines[i], scale, x, y, &top.x, &btm.x, alpha.x, 1);
			y += adj;
		}
	}


	glBindTexture(GL_TEXTURE_2D, 0);

}

void BitmapText::unloadDevice()
{
	RenderObject::unloadDevice();
}

void BitmapText::reloadDevice()
{
	RenderObject::reloadDevice();
	setText(this->text);
}

void BitmapText::stopScrollingText()
{
	scrolling = false;
}

bool BitmapText::isScrollingText()
{
	return scrolling;
}

size_t BitmapText::getNumLines() const
{
	return lines.size();
}

float BitmapText::getStringWidth(const std::string& text) const
{
	std::string tmp;
	int maxsize = 0;
	tmp.reserve(text.length());
	for (size_t i = 0; i < text.size(); i++)
	{
		if(text[i] == '\n')
		{
			std::pair<int, int> dim;
			bmpFont.font->GetStringSize(tmp, &dim);
			maxsize = std::max(maxsize, dim.first);
			tmp.resize(0);
		}
		else
			tmp += text[i];
	}
	std::pair<int, int> dim;
	bmpFont.font->GetStringSize(tmp, &dim);
	maxsize = std::max(maxsize, dim.first);

	return maxsize * bmpFont.scale;
}


