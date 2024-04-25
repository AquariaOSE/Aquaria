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
#include "DarkLayer.h"
#include "Core.h"
#include "RenderBase.h"

DarkLayer::DarkLayer()
{
	quality = 0;
	active = false;
	texture = 0;
	useFrameBuffer = false;
	layer = -1;
	beginLayer = -1;
	lastLayer = -1;
}

void DarkLayer::unloadDevice()
{
	if (useFrameBuffer)
		frameBuffer.unloadDevice();
	else
	{
		if (texture)
			glDeleteTextures(1, &texture);
	}
}

void DarkLayer::reloadDevice()
{
	if (useFrameBuffer)
		frameBuffer.reloadDevice();
	else
		texture = generateEmptyTexture(quality);
}

bool DarkLayer::isUsed() const
{
	return active;
}

bool DarkLayer::shouldRenderLayer(int lr) const
{
	if(!active)
		return true;
	return useFrameBuffer || (lr < beginLayer || lr > lastLayer);
}

void DarkLayer::init(int quality, bool useFrameBufferParam)
{
	useFrameBuffer = useFrameBufferParam;

	this->quality = quality;

	if (useFrameBuffer)
	{
		if (!frameBuffer.init(quality, quality, 1))
			useFrameBuffer = false;
		else
			debugLog("Dark Layer: using framebuffer");
	}
	if (!useFrameBuffer)
	{
		debugLog("Dark Layer: using generated texture");
		texture = generateEmptyTexture(quality);
	}
}

void DarkLayer::toggle(bool on)
{
	this->active = on;
}

void DarkLayer::preRender()
{
	if(!useFrameBuffer)
	{
		glViewport(0, 0, quality, quality);
		glClearColor(1,1,1,0);
		glClear(GL_COLOR_BUFFER_BIT);
		core->renderInternal(beginLayer, lastLayer, false);
		glBindTexture(GL_TEXTURE_2D, texture);
		glReadBuffer(GL_BACK);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, quality, quality, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void DarkLayer::beginCapture()
{
	if(useFrameBuffer)
	{
		frameBuffer.pushCapture(0);
		glClearColor(1,1,1,0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

void DarkLayer::endCapture()
{
	if(useFrameBuffer)
	{
		frameBuffer.popCapture();
	}
}

void DarkLayer::render(const RenderState& rs) const
{
	glPushMatrix();
	glLoadIdentity();


	glEnable(GL_TEXTURE_2D);
	if (useFrameBuffer)
		frameBuffer.bindTexture(0);
	else
		glBindTexture(GL_TEXTURE_2D,texture);

	rs.gpu.setBlend(BLEND_MULT);

	glColor4f(1,1,1,1);

	const float width  =  core->getWindowWidth();
	const float height =  core->getWindowHeight();
	const float offX   = -(core->getVirtualOffX() * width / core->getVirtualWidth());
	const float offY   = -(core->getVirtualOffY() * height / core->getVirtualHeight());
	const float stretch = 4;

	glBegin(GL_QUADS);

		glTexCoord2f(0,1);
		glVertex2f(offX-stretch, offY-stretch);

		glTexCoord2f(0,0);
		glVertex2f(offX-stretch, height+offY+stretch);

		glTexCoord2f(1,0);
		glVertex2f(width+offX+stretch, height+offY+stretch);

		glTexCoord2f(1,1);
		glVertex2f(width+offX+stretch, offY-stretch);

	glEnd();

	glPopMatrix();

	RenderObject::lastTextureApplied = 0;
	glBindTexture(GL_TEXTURE_2D, 0);
}
