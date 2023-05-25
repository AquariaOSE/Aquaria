#ifndef BASETEXT_H
#define BASETEXT_H

#include "RenderObject.h"


class BaseText : public RenderObject
{
public:
	BaseText() { addType(SCO_TEXT); }
	virtual ~BaseText() {}
	virtual void setText(const std::string& text) = 0;
	virtual void setWidth(float width) = 0;
	virtual void setFontSize(float sz) = 0;
	virtual void setAlign(Align a) = 0;
	virtual float getLineHeight() const = 0;
	virtual size_t getNumLines() const = 0;
	virtual float getHeight() const = 0; // total height
	virtual float getStringWidth(const std::string& text) const = 0; // width of string when not auto-wrapped
	virtual float getActualWidth() const = 0; // width of text after wrapping
};

#endif
