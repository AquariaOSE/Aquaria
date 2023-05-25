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
#ifndef BBGE_TTFFONT_H
#define BBGE_TTFFONT_H

#include "BaseText.h"

class FTGLTextureFont;

struct TTFFont
{
	TTFFont();
	~TTFFont();

	void destroy();
	void load(const std::string &str, int sz=24);
	void create(const unsigned char *data, unsigned long datalen, int sz=24);


	FTGLTextureFont *font;
};

class TTFText : public BaseText
{
public:
	TTFText(TTFFont *font);
	void setText(const std::string &txt) OVERRIDE;
	void setAlign(Align align) OVERRIDE;
	void setWidth(float width) OVERRIDE;
	float getHeight() const OVERRIDE;
	float getActualWidth() const OVERRIDE { return maxW; }
	void setFontSize(float) OVERRIDE; // dummy
	float getStringWidth(const std::string& s) const OVERRIDE;
	bool shadow;
	int findLine(const std::string &label);
	float getLineHeight() const OVERRIDE;
	size_t getNumLines() const OVERRIDE;
protected:
	float width;
	float lineHeight;
	void updateAlign();
	Align align;
	void onRender(const RenderState& rs) const OVERRIDE;
	void updateFormatting();

	std::string originalText;
	std::vector<std::string> text;
	TTFFont *font;
	int hw,h;
	float maxW;
};

#endif
