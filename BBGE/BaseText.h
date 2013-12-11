#ifndef BASETEXT_H
#define BASETEXT_H

#include "RenderObject.h"


class BaseText : public RenderObject
{
public:
	BaseText() { addType(SCO_TEXT); }
	virtual ~BaseText() {}
	virtual void setText(const std::string& text) = 0;
	virtual void setWidth(int width) = 0;
	virtual void setFontSize(int sz) = 0;
	virtual void setAlign(Align a) = 0;

};

#endif
