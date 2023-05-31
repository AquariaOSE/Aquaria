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
#ifndef BITMAPFONT_H
#define BITMAPFONT_H


#include "RenderObject.h"
#include "BaseText.h"

namespace glfont { class GLFont; }

struct BmpFont
{
	BmpFont();
	~BmpFont();
	void load(const std::string &file, float scale=1, bool loadTexture=true);
	void destroy();

	glfont::GLFont * const font;
	float scale;
	bool loaded;

	Vector fontTopColor;
	Vector fontBtmColor;

	CountedPtr<Texture> overrideTexture;
};

class BitmapText : public BaseText
{
public:
	BitmapText(const BmpFont& bmpFont);
	void setText(const std::string &text) OVERRIDE;
	void setWidth(float width) OVERRIDE;
	float getSetWidth(); // get the width that was set
	void scrollText(const std::string &text, float scrollSpeed);
	void setFontSize(float sz) OVERRIDE;
	bool isScrollingText();
	void stopScrollingText();
	bool isEmpty();
	virtual void setAlign(Align align) OVERRIDE;
	std::string getText();
	int getWidthOnScreen();
	Vector getColorIndex(size_t i, size_t j);
	void updateWordColoring();
	void autoKern();
	virtual float getHeight() const OVERRIDE;
	void unloadDevice() OVERRIDE;
	void reloadDevice() OVERRIDE;
	float getStringWidth(const std::string& text) const OVERRIDE;
	float getActualWidth() const OVERRIDE { return maxW; }
	float getLineHeight() const OVERRIDE;
	size_t getNumLines() const OVERRIDE;

protected:
	const BmpFont& bmpFont;
	float scrollSpeed;
	void onUpdate(float dt) OVERRIDE;
	float scrollDelay;
	bool scrolling;
	size_t currentScrollLine;
	size_t currentScrollChar;
	Align align;
	float alignWidth;
	void formatText();
	float fontDrawSize;
	void onRender(const RenderState& rs) const OVERRIDE;
	typedef std::vector<std::string> Lines;
	Lines lines;
	typedef std::vector<Vector> ColorIndices;
	std::vector<ColorIndices> colorIndices;
	std::string text;
	float textWidth;
	float maxW;
};

#endif
