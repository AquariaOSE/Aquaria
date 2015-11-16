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
#ifndef DEBUGFONT_H
#define DEBUGFONT_H

#include "Core.h"
#include "BaseText.h"

class DebugFont : public BaseText
{
public:
	DebugFont(int initFontSize=0, const std::string &initText="");
	void setText(const std::string &text);
	void setWidth(float width);
	void setFontSize(float sz);
	int getNumLines() { return lines.size(); }
	virtual void setAlign(Align align);
	virtual float getHeight();
	virtual float getLineHeight();
	virtual float getStringWidth(const std::string& text);
	virtual float getActualWidth();
protected:
	float fontDrawSize, textWidth;
	void formatText();
	void onRender();
	std::string text;
	std::vector<std::string> lines;
	Align align;
	float maxW;
};

class Quad;
class DebugButtonReceiver;

class DebugButton : public RenderObject
{
public:
	DebugButton(int buttonID=-1, DebugButtonReceiver *receiver=0, int bgWidth=0, int fsize=0, bool quitMain=false);
	DebugFont *label;
	EventPtr event;
	bool quitMain;
	int buttonID;
	
protected:
	void onUpdate(float dt);
	Quad *highlight;
	bool md;
	DebugButtonReceiver *receiver;
};

class DebugButtonReceiver
{
public:
	virtual void buttonPress(DebugButton *db){}
};

#endif
