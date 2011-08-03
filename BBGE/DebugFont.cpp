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

DebugFont::DebugFont(int initSz, const std::string &initText) : RenderObject()
{
	align = ALIGN_LEFT;
	followCamera = 1;
	fontDrawSize = 5;
	textWidth = 800;
	if (initSz)
	{
		setFontSize(initSz);
	}

	if (!initText.empty())
	{
		setText(initText);
	}
}

void DebugFont::setWidth(int width)
{
	textWidth = width;
}

void DebugFont::setFontSize(int sz)
{
	fontDrawSize = sz;
}

void DebugFont::formatText()
{
	std::string text;
	text = this->text;
	lines.clear();
	std::string currentLine;
	int lastSpace = -1;
	int currentWidth = 0;
	int alignWidth = 0;
	for (int i = 0; i < text.size(); i++)
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
			alignWidth = currentWidth;
			currentWidth = 0;
			lastSpace = 0;
			continue;
		}

		if (text[i] == ' ')
		{
			lastSpace = i;
		}
	}
	if (alignWidth == 0)
		alignWidth = currentWidth;
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

void DebugFont::onRender()
{
	const float vspc = 1.5;

#ifdef BBGE_BUILD_OPENGL
	for (int i = 0; i < lines.size(); i++)
	{
		//float width = (lines[i].size()-1) * fontDrawSize * 1.4f * 0.75f;
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
#endif
}

void DebugFont::setAlign(Align align)
{
	this->align = align;
}


#include "../BBGE/Quad.h"

DebugButton::DebugButton(int buttonID, DebugButtonReceiver *receiver, int bgWidth, int fsize, bool quitMain)
 : RenderObject(), label(0), highlight(0), quitMain(quitMain), receiver(receiver), buttonID(buttonID)
{
	if (bgWidth == 0)
		bgWidth = 150;
	if (fsize == 0)
		fsize = 22;
	int szw = bgWidth;

	highlight = new Quad();
	highlight->setWidthHeight(szw, fsize);
	highlight->position = Vector(szw*0.5f, 0);
	highlight->alpha = 0.5;
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
		highlight->color.interpolateTo(Vector(1, 1, 1), 0.1);

		if (core->mouse.buttons.left && !md)
			md = true;
		else if (md && !core->mouse.buttons.left)
		{
			doit = true;
			md = false;
		}

		if (doit)
		{
			if (quitMain)
				core->quitNestedMain();
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
		highlight->color.interpolateTo(Vector(0,0,0), 0.1);
	}


}

