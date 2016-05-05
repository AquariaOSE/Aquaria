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
#include <assert.h>

#include "Effects.h"
#include "Core.h"

PostProcessingFX::PostProcessingFX()
{
	blendType = 0;
	layer = renderLayer = 0;
	intensity = 0.1;
	blurTimes = 12;
	radialBlurColor = Vector(1,1,1);
	for (int i = 0; i < FXT_MAX; i++)
		enabled[i] = false;
}

void PostProcessingFX::init()
{
}


void PostProcessingFX::update(float dt)
{
}

void PostProcessingFX::preRender()
{
}

void PostProcessingFX::toggle(FXTypes type)
{
	enabled[int(type)] = !enabled[int(type)];
}

void PostProcessingFX::enable(FXTypes type)
{
	enabled[int(type)] = true;
}

bool PostProcessingFX::isEnabled(FXTypes type)
{
	return enabled[int(type)];
}

void PostProcessingFX::disable(FXTypes type)
{
	enabled[int(type)] = false;
}

void PostProcessingFX::render()
{
	if(!core->frameBuffer.isEnabled())
		return;

	for (int i = 0; i < FXT_MAX; i++)
	{
		if (enabled[i])
		{
			glPushMatrix();
			FXTypes type = (FXTypes)i;
			switch(type)
			{
			case FXT_RADIALBLUR:

				float windowW = core->getWindowWidth();
				float windowH = core->getWindowHeight();
				float textureW = core->frameBuffer.getWidth();
				float textureH = core->frameBuffer.getHeight();

				float alpha = intensity;

				float offX   = -(core->getVirtualOffX() * windowW / core->getVirtualWidth());
				float offY   = -(core->getVirtualOffY() * windowH / core->getVirtualHeight());

				float width2 = windowW / 2;
				float height2 = windowH / 2;

				float pw = float(windowW)/float(textureW);
				float ph = float(windowH)/float(textureH);

				glLoadIdentity();


				glTranslatef(width2 + offX, height2 + offY, 0);

				glEnable(GL_TEXTURE_2D);

				core->frameBuffer.bindTexture();

				glEnable(GL_BLEND);

				if (blendType == 1)
					glBlendFunc(GL_SRC_ALPHA,GL_ONE);
				else
					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

				float percentX = pw, percentY = ph;

				float inc = 0.01;
				float spost = 0.0f;											// Starting Texture Coordinate Offset
				float alphadec = alpha / blurTimes;

				glBegin(GL_QUADS);											// Begin Drawing Quads
					for (int num = 0;num < blurTimes; num++)						// Number Of Times To Render Blur
					{
						glColor4f(radialBlurColor.x, radialBlurColor.y, radialBlurColor.z, alpha);					// Set The Alpha Value (Starts At 0.2)

						glTexCoord2d(spost, spost);
						glVertex3f(-width2, height2,  0.0);

						glTexCoord2d(percentX-spost, spost);
						glVertex3f( width2, height2,  0.0);

						glTexCoord2d(percentX-spost, percentY-spost);
						glVertex3f( width2,  -height2,  0.0);

						glTexCoord2d(spost, percentY-spost);
						glVertex3f(-width2,  -height2,  0.0);

						spost += inc;										// Gradually Increase spost (Zooming Closer To Texture Center)
						alpha -= alphadec;							// Gradually Decrease alpha (Gradually Fading Image Out)
					}
				glEnd();


				glColor4f(1,1,1,1);
				glBindTexture(GL_TEXTURE_2D, 0);
				RenderObject::lastTextureApplied = 0;


			break;
			}
			glPopMatrix();

		}
	}
}

/*
GLuint		blurTexture;
GLuint emptyTexture()											// Create An Empty Texture
{
	GLuint txtnumber;											// Texture ID
	unsigned int* data;											// Stored Data

	// Create Storage Space For Texture Data (128x128x4)
	data = (unsigned int*)new GLuint[((128 * 128)* 4 * sizeof(unsigned int))];
	ZeroMemory(data,((128 * 128)* 4 * sizeof(unsigned int)));	// Clear Storage Memory

	glGenTextures(1, &txtnumber);								// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, txtnumber);					// Bind The Texture
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 128, 128, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);						// Build Texture Using Information In data
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	delete [] data;												// Release data

	return txtnumber;											// Return The Texture ID
}

PostProcessingFX::PostProcessingFX()
{

}

void PostProcessingFX::init(FXTypes type)
{
	if (type == FXT_RADIALBLUR)
	{
		blurTexture = emptyTexture();
	}
	enabled[(int)type] = true;
}

void PostProcessingFX::shutdown(FXTypes type)
{
	enabled[int(type)] = false;
}

void PostProcessingFX::preRender()
{
	for (int i = 0; i < FXT_MAX; i++)
	{
		if (enabled[i])
		{
			FXTType type = (FXType)i;
			switch(type)
			{
			case FXT_RADIALBLUR:
				glViewport(0,0,128,128);									// Set Our Viewport (Match Texture Size;
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And Depth Buffer
				core->render();
				glBindTexture(GL_TEXTURE_2D,BlurTexture);					// Bind To The Blur Texture

				// Copy Our ViewPort To The Blur Texture (From 0,0 To 128,128... No Border)
				glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 0, 0, 128, 128, 0);

				glClearColor(0.0f, 0.0f, 0.5f, 0.5);						// Set The Clear Color To Medium Blue
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And Depth Buffer

				glViewport(0, 0, 800, 600);
			break;
			}
		}
	}
}

void PostProcessingFX::render()
{
	for (int i = 0; i < FXT_MAX; i++)
	{
		if (enabled[i])
		{
			FXTType type = (FXType)i;
			switch(type)
			{
			case FXT_RADIALBLUR:
				glBegin(GL_QUADS);
					
				glEnd();
			break;
			}
		}
	}
}
*/
