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
#include "BloomEffect.h"
#include "Core.h"

BloomEffect::BloomEffect() : RenderObject()
{
	active = false;
	followCamera = 1;
	cull = 0;
	stretch = 0;
	startLayer = endLayer = -1;
	
	texID = 0;
	format = GL_LUMINANCE;

	useFrameBuffer = true;
}

void BloomEffect::init(int quality, int startLayer, int endLayer)
{
	this->startLayer = startLayer;
	this->endLayer = endLayer;
	this->quality = quality;
	
	if (useFrameBuffer)
	{
		if (frameBuffer.init(quality, quality))
			active = true;
		else
		{
			active = false;
			useFrameBuffer = false;
		}
	}
	else
	{
		texID = generateEmptyTexture(quality);
	}
}

void BloomEffect::unloadDevice()
{
	if (frameBuffer.isInited())
		frameBuffer.unloadDevice();
}

void BloomEffect::reloadDevice()
{
	if (frameBuffer.isInited())
		frameBuffer.reloadDevice();
}

void BloomEffect::render()
{
	if (active && frameBuffer.isInited())
	{
		// get
		glViewport(0,0,quality,quality);

		frameBuffer.startCapture();

		glClearColor(0,0,0,1);
		glClear(GL_COLOR_BUFFER_BIT);
		core->render(startLayer, endLayer, false);

		if (useFrameBuffer)
		{
			frameBuffer.endCapture();
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D,texID);
			glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, quality, quality, 0);
		}

		core->setClearColor(core->getClearColor());
		glViewport(0, 0, core->width, core->height);


		// render
		glEnable(GL_TEXTURE_2D);
		if (useFrameBuffer)
			frameBuffer.bindTexture();
		else
			glBindTexture(GL_TEXTURE_2D,texID);

		glPushMatrix();
			glLoadIdentity();

			//glScalef(scale.x, scale.y, 0);

			float spost = 0.0;


			int x=0,y=0;
			x = -core->getVirtualOffX()*2;
			y = -core->getVirtualOffY()*2;
			applyBlendType();
			glColor4f(color.x, color.y, color.z, alpha.x*alphaMod);
			glBegin(GL_QUADS);											// Begin Drawing Quads

			glTexCoord2f(0,1);							// Texture Coordinate	( 0, 1 )
			glVertex2f(x-stretch,y-stretch);									// First Vertex		(   0,   0 )
			glTexCoord2f(0,0);							// Texture Coordinate	( 0, 0 )
			glVertex2f(x-stretch,core->height+stretch);							// Second Vertex	(   0, 480 )

			glTexCoord2f(1,0);							// Texture Coordinate	( 1, 0 )
			glVertex2f(core->width+stretch,core->height+stretch);				// Third Vertex		( 640, 480 )

			glTexCoord2f(1,1);							// Texture Coordinate	( 1, 1 )
			glVertex2f(core->width+stretch,y-stretch);							// Fourth Vertex	( 640,   0 )
			glEnd();

		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
		RenderObject::lastTextureApplied = 0;
	}
}

