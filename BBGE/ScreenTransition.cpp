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
#include "ScreenTransition.h"
#include "Core.h"

ScreenTransition::ScreenTransition() : RenderObject()
{
	createTexture();

	cull = false;
	followCamera = 1;
	alpha = 0;
}

void ScreenTransition::createTexture()
{
	width = core->getVirtualWidth();
	height = core->getVirtualHeight();
	
	windowWidth = core->getWindowWidth();
	windowHeight = core->getWindowHeight();

	textureWidth = windowWidth;
	textureHeight = windowHeight;

	sizePowerOf2Texture(textureWidth);
	sizePowerOf2Texture(textureHeight);

	/*
	if (windowWidth>1024)
	{
		textureWidth = 2048;
		textureHeight = 1024;
	}
	*/

#ifdef BBGE_BUILD_OPENGL
	//create our texture
	glGenTextures(1,&screen_texture);
	glBindTexture(GL_TEXTURE_2D, screen_texture);
	//GL_NEAREST
	// vs
	//GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);		//GL_NEAREST);		//GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);		//GL_NEAREST);		//GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,3, textureWidth, textureHeight, 0 , GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D,0);
#endif

/*
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
*/
}

void ScreenTransition::destroyTexture()
{
#ifdef BBGE_BUILD_OPENGL
	if (screen_texture)
	{
		glDeleteTextures(1, &screen_texture);
		screen_texture = 0;
	}
#endif
}

void ScreenTransition::unloadDevice()
{
	RenderObject::unloadDevice();
	destroyTexture();
}

void ScreenTransition::reloadDevice()
{
	RenderObject::reloadDevice();
	createTexture();
}

void ScreenTransition::capture()
{	
	core->render();

	/*
	std::ostringstream os;
	os << "windowWidth [" << windowWidth << "] windowHeight [" << windowHeight << "]";
	errorLog(os.str());
	*/
	
#ifdef BBGE_BUILD_OPENGL
	if (screen_texture)
	{
		glBindTexture(GL_TEXTURE_2D,screen_texture);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, windowWidth, windowHeight);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
#endif

	core->showBuffer();
}

void ScreenTransition::go(float time)
{
	capture();
	transition(time);
}

void ScreenTransition::transition(float time)
{
	core->resetTimer();
	alpha = 1;
	alpha.interpolateTo(0, time);
}

bool ScreenTransition::isGoing()
{
	return alpha.isInterpolating();
}

void ScreenTransition::onRender()
{
	if (alpha.x == 0) return;
	
#ifdef BBGE_BUILD_OPENGL
	float width2 = float(width)/2;
	float height2 = float(height)/2;
	
	const float pw = float(windowWidth)/float(textureWidth);
	const float ph = float(windowHeight)/float(textureHeight);
	
	/*
	std::ostringstream os;
	os << "wh(" << width2 << ", " << height2 << ") p(" << pw << ", " << ph << ")";
	debugLog(os.str());
	*/
	
	glBindTexture(GL_TEXTURE_2D, screen_texture);
	//glBindTexture(GL_TEXTURE_2D, 0);
	// 16/10 = 1.6
	// 16/9 = 1.77777
	// 4/3 = 1.33333
	
	// fix slight offset in 16/10 resolution
	// only on mac?

#if 0 //def BBGE_BUILD_MACOSX
	float aspect = float(core->width) / float(core->height);
	float checkAspect = 16.0f/10.0f;
	if (fabsf(aspect - checkAspect) < 0.01f)
	{
		glTranslatef(0.5f,0.0f,0.0f);
	}
#endif

	glBegin(GL_QUADS);
		//glNormal3f( 0.0f, 0.0f, 1.0f);
		//glColor4f(color.x, color.y, color.z, alpha.getValue());
		glTexCoord2d(0, 0);
		glVertex3f(-width2, height2,  0.0);
		glTexCoord2d(pw, 0);
		glVertex3f( width2, height2,  0.0);
		glTexCoord2d(pw, ph);
		glVertex3f( width2,  -height2,  0.0);
		glTexCoord2d(0, ph);
		glVertex3f(-width2,  -height2,  0.0);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	
	RenderObject::lastTextureApplied = 0;
#endif
}

