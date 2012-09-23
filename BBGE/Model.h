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
#ifndef BBGE_MODEL_H
#define BBGE_MODEL_H

#include "RenderObject.h"


class CalCoreModel;
class CalModel;


class Model : public RenderObject
{
public:
	Model();
	void destroy();

	bool load(const std::string &file);

	
protected:
	void onUpdate(float dt);

	void onRender();
	void renderMesh(bool bWireframe, bool bLight);

protected:
	//int m_state;
	CalCoreModel* m_calCoreModel;
	CalModel* m_calModel;
	int m_animationId[16];
	int m_animationCount;
	int m_meshId[32];
	int m_meshCount;
	GLuint m_textureId[32];
	int m_textureCount;
	float m_motionBlend[3];
	float m_renderScale;
	float m_lodLevel;
	std::string m_path;
};

#endif
