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

int quality = 64;
GLuint		blurTexture;
GLuint emptyTexture()											// Create An Empty Texture
{
	GLuint txtnumber=0;											// Texture ID
	unsigned int* data;											// Stored Data

	// Create Storage Space For Texture Data (128x128x4)
	data = (unsigned int*)new GLuint[((quality * quality)* 4 * sizeof(unsigned int))];
#ifdef BBGE_BUILD_WINDOWS
	ZeroMemory(data,((quality * quality)* 4 * sizeof(unsigned int)));	// Clear Storage Memory
#endif

#ifdef BBGE_BUILD_OPENGL
	glGenTextures(1, &txtnumber);								// Create 1 Texture
	glBindTexture(GL_TEXTURE_2D, txtnumber);					// Bind The Texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, quality, quality, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);						// Build Texture Using Information In data
#endif

	delete [] data;												// Release data

	return txtnumber;											// Return The Texture ID
}

PostProcessingFX::PostProcessingFX()
{
	renderLayerStart = 1;
	renderLayerEnd = 19;

#ifdef BBGE_BUILD_OPENGL
	format = GL_LUMINANCE;
#endif
	//GL_INTENSITY
	//GL_RGB
	//GL_LUMINANCE
	blendType = 0;
	layer = renderLayer = 0;
	intensity = 0.1;
	radialBlurColor = Vector(1,1,1);
	for (int i = 0; i < FXT_MAX; i++)
		enabled[i] = false;
}

void PostProcessingFX::init(FXTypes type)
{
	enabled[(int)type] = true;
	if (type == FXT_RADIALBLUR)
		blurTexture = emptyTexture();
}

void PostProcessingFX::update(float dt)
{
}

void PostProcessingFX::setRenderLayerRange(int start, int end)
{
	renderLayerStart = start;
	renderLayerEnd = end;
}

void PostProcessingFX::preRender()
{
	for (int i = 0; i < FXT_MAX; i++)
	{
		if (enabled[i])
		{
			
			FXTypes type = (FXTypes)i;
			switch(type)
			{
			case FXT_RADIALBLUR:
				if (core->frameBuffer.isInited())
				{
				}
				else
				{
#ifdef BBGE_BUILD_OPENGL
					glViewport(0,0,quality,quality);									// Set Our Viewport (Match Texture Size;
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And Depth Buffer
					core->render(renderLayerStart, renderLayerEnd);							
					glBindTexture(GL_TEXTURE_2D,blurTexture);
					glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, quality, quality, 0);

					glViewport(0, 0, core->width, core->height);
					

					/*
					glBindTexture(GL_TEXTURE_2D,blurTexture);					// Bind To The Blur Texture

					// Copy Our ViewPort To The Blur Texture (From 0,0 To 128,128... No Border)
					glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, quality, quality, 0);
					*/
					//GL_INTENSITY
					//GL_RGB
					//GL_LUMINANCE

					//glClearColor(0.0f, 0.0f, 0.5f, 0.5);						// Set The Clear Color To Medium Blue
					//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And Depth Buffer

#endif
				}				
			break;
			}
			
		}
	}
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
	for (int i = 0; i < FXT_MAX; i++)
	{
		if (enabled[i])
		{
#ifdef BBGE_BUILD_OPENGL
			glPushMatrix();
			FXTypes type = (FXTypes)i;
			switch(type)
			{
			case FXT_RADIALBLUR:
				/*
				float percentX = (float)core->width/(float)quality;
				float percentY = (float)core->height/(float)quality;
				*/
				/*
				float percentX = (float)quality/(float)core->width;
				float percentY = (float)quality/(float)core->height;
				*/
				float percentX = 1.0;
				float percentY = 1.0;

				/*
				std::ostringstream os;
				os << "p(" << percentX << ", " << percentY << ")";
				debugLog(os.str());
				*/

				glLoadIdentity();
				int times = 12;
				float inc = 0.01;
				float spost = 0.0f;											// Starting Texture Coordinate Offset
				float alphainc = 0.9f / times;								// Fade Speed For Alpha Blending
				//float alpha = 0.1f;											// Starting Alpha Value
				float alpha = intensity;

				glEnable(GL_TEXTURE_2D);									// Enable 2D Texture Mapping
				if (blendType == 1)
					glBlendFunc(GL_SRC_ALPHA,GL_ONE);					// Set Blending Mode
				else
					glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);					// Set Blending Mode
				glEnable(GL_BLEND);			// Enable Blending
				if (core->frameBuffer.isInited())
					core->frameBuffer.bindTexture();
				else
					glBindTexture(GL_TEXTURE_2D,blurTexture);					// Bind To The Blur Texture

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				alphainc = alpha / times;									// alphainc=0.2f / Times To Render Blur

				glBegin(GL_QUADS);											// Begin Drawing Quads
					for (int num = 0;num < times;num++)						// Number Of Times To Render Blur
					{
						glColor4f(radialBlurColor.x, radialBlurColor.y, radialBlurColor.z, alpha);					// Set The Alpha Value (Starts At 0.2)
						glTexCoord2f(0+spost,percentY-spost);						// Texture Coordinate	( 0, 1 )
						glVertex2f(0,0);									// First Vertex		(   0,   0 )

						glTexCoord2f(0+spost,0+spost);						// Texture Coordinate	( 0, 0 )
						glVertex2f(0,core->height);									// Second Vertex	(   0, 480 )

						glTexCoord2f(percentX-spost,0+spost);						// Texture Coordinate	( 1, 0 )
						glVertex2f(core->width,core->height);								// Third Vertex		( 640, 480 )

						glTexCoord2f(percentX-spost,percentY-spost);						// Texture Coordinate	( 1, 1 )
						glVertex2f(core->width,0);									// Fourth Vertex	( 640,   0 )

						spost += inc;										// Gradually Increase spost (Zooming Closer To Texture Center)
						alpha = alpha - alphainc;							// Gradually Decrease alpha (Gradually Fading Image Out)
					}
				glEnd();				

				glDisable(GL_BLEND);
				glBindTexture(GL_TEXTURE_2D,0);
				glColor4f(1,1,1,1);
				//glDisable(GL_TEXTURE_2D);

				// Done Drawing Quads
				/*
				glDisable(GL_TEXTURE_2D);									// Disable 2D Texture Mapping
				glDisable(GL_BLEND);										// Disable Blending
				glBindTexture(GL_TEXTURE_2D,0);								// Unbind The Blur Texture
				*/
			break;
			}
			glPopMatrix();

#endif
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
