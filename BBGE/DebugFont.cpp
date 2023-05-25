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
#include "DebugFont.h"
#include "RenderBase.h"
#include "Core.h"

DebugFont::DebugFont(int initSz, const std::string &initText)
{
	align = ALIGN_LEFT;
	followCamera = 1;
	fontDrawSize = 5;
	textWidth = 800;
	maxW = 0;
	if (initSz)
	{
		setFontSize(initSz);
	}

	if (!initText.empty())
	{
		setText(initText);
	}
}

void DebugFont::setWidth(float width)
{
	textWidth = width;
}

void DebugFont::setFontSize(float sz)
{
	fontDrawSize = sz;
}

float DebugFont::getHeight() const
{
	return fontDrawSize * lines.size() * 1.5f; // vspc in render()
}

float DebugFont::getLineHeight() const
{
	return fontDrawSize * 1.5f; // vspc in render()
}

float DebugFont::getStringWidth(const std::string& text) const
{
	int maxchars = 0;
	int c = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		if(text[i] == '\n')
		{
			maxchars = std::max(maxchars, c);
			c = 0;
		}
		else
			++c;
	}
	maxchars = std::max(maxchars, c);
	return fontDrawSize * maxchars * (1.4f * 0.75f);
}

float DebugFont::getActualWidth() const
{
	return maxW * (1.4f * 0.75f); // numbers taken from onRender()
}

void DebugFont::formatText()
{
	std::string text;
	text = this->text;
	lines.clear();
	std::string currentLine;
	int lastSpace = -1;
	float currentWidth = 0;
	maxW = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		currentWidth += fontDrawSize;

		if (currentWidth > textWidth || text[i] == '\n')
		{
			if (text[i] == '\n')
			{
				lastSpace = i;
			}
			lines.push_back(text.substr(0, lastSpace));
			int tsz = text.size();
			text = text.substr(lastSpace+1, tsz);
			i = 0;
			maxW = std::max(maxW, currentWidth);
			currentWidth = 0;
			lastSpace = 0;
			continue;
		}

		if (text[i] == ' ')
		{
			lastSpace = i;
		}
	}
	maxW = std::max(maxW, currentWidth);
	if (!text.empty() && (text.size() > 1 || text[0] != ' '))
	{
		lines.push_back(text);
	}
}

void DebugFont::setText(const std::string &text)
{
	this->text = text;
	formatText();
}

void DebugFont::onRender(const RenderState& rs) const
{
	const float vspc = 1.5;

	for (size_t i = 0; i < lines.size(); i++)
	{

		float width = (lines[i].size()) * fontDrawSize * 1.4f * 0.75f;
		if (align == ALIGN_CENTER)
		{
			glTranslatef(-width*0.5f, 0, 0);
		}

		glColor4f(0,0,0,alpha.x*alphaMod);
		core->print(1, (i * vspc*fontDrawSize)+1, (char*)(lines[i].c_str()), fontDrawSize);
		glColor4f(color.x,color.y,color.z,alpha.x*alphaMod);
		core->print(0, (i * vspc*fontDrawSize), (char*)(lines[i].c_str()), fontDrawSize);

		if (align == ALIGN_CENTER)
		{
			glTranslatef(width*0.5f, 0, 0);
		}
	}
}

void DebugFont::setAlign(Align align)
{
	this->align = align;
}


#include "../BBGE/Quad.h"

DebugButton::DebugButton(int buttonID, DebugButtonReceiver *receiver, int bgWidth, int fsize)
	: RenderObject()
	, label(0)
	, buttonID(buttonID)
	, activeAlpha(0.5f)
	, activeColor(1,1,1)
	, inactiveAlpha(0.5f)
	, inactiveColor(0,0,0)
	, highlight(0)
	, receiver(receiver)
{
	if (bgWidth == 0)
		bgWidth = 150;
	if (fsize == 0)
		fsize = 22;
	int szw = bgWidth;

	highlight = new Quad();
	highlight->setWidthHeight(szw, fsize);
	highlight->position = Vector(szw*0.5f, 0);
	highlight->alpha = inactiveAlpha;
	highlight->color = inactiveColor;
	addChild(highlight, PM_POINTER);

	label = new DebugFont(float(fsize)/3.0f, "DebugButton");
	label->position = Vector(20, 0);
	addChild(label, PM_POINTER);

	md = false;

	followCamera = 1;
}

void DebugButton::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (alpha.x < 1)
		return;

	bool doit = false;


	if (highlight->isCoordinateInsideWorld(core->mouse.position) && ((!md) || (md && !core->mouse.buttons.left)))
	{
		highlight->color.interpolateTo(activeColor, 0.1f);
		highlight->alpha.interpolateTo(activeAlpha, 0.1f);

		if (core->mouse.buttons.left && !md)
			md = true;
		else if (md && !core->mouse.buttons.left)
		{
			doit = true;
			md = false;
		}

		if (doit)
		{
			event.call();
			if (receiver)
				receiver->buttonPress(this);
		}
	}
	else
	{
		if (md && !core->mouse.buttons.left)
		{
			md = false;
		}
		highlight->color.interpolateTo(inactiveColor, 0.1f);
		highlight->alpha.interpolateTo(inactiveAlpha, 0.1f);
	}


}

