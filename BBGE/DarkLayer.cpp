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

DarkLayer::DarkLayer()
{
	quality = 0;
	active = false;
	layer = -1;
	renderLayer = -1;
	texture = 0;

	stretch = 4;
	format = GL_RGB;			//FIXED?: used to be GL_LUMINANCE, that might have been causing problems
	useFrameBuffer = true;		//BUG?: will do this even if frame buffer is off in usersettings...
}

void DarkLayer::unloadDevice()
{
	if (useFrameBuffer)
		frameBuffer.unloadDevice();
	else
	{
#ifdef BBGE_BUILD_OPENGL
		if (texture)
			glDeleteTextures(1, &texture);
#endif
	}
}

void DarkLayer::reloadDevice()
{
	if (useFrameBuffer)
		frameBuffer.reloadDevice();
	else
		texture = generateEmptyTexture(quality);
}

int DarkLayer::getRenderLayer()
{
	return renderLayer;
}

bool DarkLayer::isUsed()
{
	//HACK: disabling dark layer for temporary testing build
	// MAKE SURE TO RESTORE THIS CODE TO THE WAY IT WAS
	return layer > -1 && active;
	//return false;
}

void DarkLayer::setLayers(int layer, int rl)
{
	this->layer = layer;
	this->renderLayer = rl;
}

void DarkLayer::init(int quality, bool useFrameBufferParam)
{
	useFrameBuffer = useFrameBufferParam;

	this->quality = quality;

	if (useFrameBuffer)
	{		
		if (!frameBuffer.init(quality, quality))
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

int DarkLayer::getLayer()
{
	return layer;
}

void DarkLayer::toggle(bool on)
{
	this->active = on;
}

void DarkLayer::preRender()
{
#ifdef BBGE_BUILD_OPENGL
	bool verbose = core->coreVerboseDebug;
	if (layer != -1)
	{
		if (verbose) debugLog("viewport");

		glViewport(0,0,quality,quality);
		//core->clearBuffers();
		
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if (verbose) debugLog("startCapture");

		if (useFrameBuffer)
			frameBuffer.startCapture();

		if (verbose) debugLog("clearColor");
		
		glClearColor(1,1,1,1);
		glClear(GL_COLOR_BUFFER_BIT);

		if (verbose) debugLog("render");
		
		core->render(layer, layer, false); 

		if (verbose) debugLog("endCapture");
		
		if (useFrameBuffer)
			frameBuffer.endCapture();
		else
		{
			glBindTexture(GL_TEXTURE_2D,texture);					// Bind To The Blur Texture
			// Copy Our ViewPort To The Blur Texture (From 0,0 To q,q... No Border)
			glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, quality, quality, 0);
		}

		if (verbose) debugLog("viewport");
		
		glViewport(0, 0, core->width, core->height);
		glClearColor(0,0,0,0);
		
		if (verbose) debugLog("done");
		/*				
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		*/
		//glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, quality, quality, 0);
	}
#endif
}

void DarkLayer::render()
{
#ifdef BBGE_BUILD_OPENGL
	if (renderLayer != -1)
	{
		glPushMatrix();
		glLoadIdentity();
		//float percentX = (float)core->width/(float)quality;
		//float percentY = (float)core->height/(float)quality;
		
		glEnable(GL_TEXTURE_2D);
		if (useFrameBuffer)
			frameBuffer.bindTexture();
		else
			glBindTexture(GL_TEXTURE_2D,texture);
		
		//glDisable(GL_BLEND);	
		
		glEnable(GL_BLEND);	
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_ZERO, GL_SRC_COLOR);

		//glBlendEquation(GL_FUNC_SUBTRACT);

		// subtractive blend! (using color)
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);

		GLenum error = glGetError();
		if (error == GL_INVALID_ENUM)
		{
			debugLog("darkLayer: invalid enum");
		}
		else if (error == GL_INVALID_OPERATION)
		{
			debugLog("darkLayer: invalid operation");
		}
		
		//glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
		//glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1,1,1,1);

		float width  =  core->getWindowWidth();
		float height =  core->getWindowHeight();
		float offX   = -(core->getVirtualOffX() * width / core->getVirtualWidth());
		float offY   = -(core->getVirtualOffY() * height / core->getVirtualHeight());

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
#endif
}
