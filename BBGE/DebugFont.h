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

#include "BaseText.h"
#include "Event.h"

class DebugFont : public BaseText
{
public:
	DebugFont(int initFontSize=0, const std::string &initText="");
	void setText(const std::string &text) OVERRIDE;
	void setWidth(float width) OVERRIDE;
	void setFontSize(float sz) OVERRIDE;
	size_t getNumLines() const OVERRIDE { return lines.size(); }
	virtual void setAlign(Align align) OVERRIDE;
	virtual float getHeight() const OVERRIDE;
	virtual float getLineHeight() const OVERRIDE;
	virtual float getStringWidth(const std::string& text) const OVERRIDE;
	virtual float getActualWidth() const OVERRIDE;
protected:
	float fontDrawSize, textWidth;
	void formatText();
	void onRender(const RenderState& rs) const OVERRIDE;
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
	DebugButton(int buttonID=-1, DebugButtonReceiver *receiver=0, int bgWidth=0, int fsize=0);
	DebugFont *label;
	EventPtr event;
	int buttonID;
	float activeAlpha;
	Vector activeColor;
	float inactiveAlpha;
	Vector inactiveColor;

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
