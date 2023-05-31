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
#include "ToolTip.h"
#include "DSQ.h"
#include "Game.h"
#include "InGameMenu.h"

bool ToolTip::toolTipsOn = true;

ToolTip::ToolTip() : RenderObject()
{
	followCamera = 1;

	back = new Quad();
	back->alphaMod = 0.8f;
	back->setWidthHeight(50,50);
	back->color = 0;
	back->renderBorder = true;
	back->borderAlpha = 0.2f;
	back->renderCenter = false;
	addChild(back, PM_POINTER);

	text = new BitmapText(dsq->smallFont);
	text->alpha = 0.9f;
	text->setAlign(ALIGN_LEFT);
	addChild(text, PM_POINTER);

	text->alpha = back->alpha = 0;
	areaType = 0;
	areaWidth = areaHeight = 0;

	required = false;
}

void ToolTip::setText(const std::string &t, const Vector &center, int width)
{
	int height = 0;

	back->position = center;

	text->setWidth(width - 40);

	text->setText(t);
	height = text->getHeight()+20;

	back->setWidthHeight(width, height);

	text->position = center - Vector(width,height)*0.5f + Vector(5, 5);
}

void ToolTip::setArea(const Vector &p1, const Vector &p2)
{
	areaType = 0;
	areaCenter = p1;
	areaWidth = p2.x - p1.x;
	areaHeight = p2.y - p1.y;
}

void ToolTip::setAreaFromCenter(const Vector &center, int width, int height)
{
	areaType = 0;
	areaCenter = center - Vector(width*0.5f, height*0.5f);
	areaWidth = width;
	areaHeight = height;
}

void ToolTip::setCircularAreaFromCenter(const Vector &center, int diameter)
{
	areaType = 1;
	areaCenter = center;
	areaWidth = diameter*0.5f;
}

void ToolTip::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (alpha.x == 1)
	{
		bool in = false;
		switch(areaType)
		{
		case 0:
			if (core->mouse.position.x > areaCenter.x && core->mouse.position.x < areaCenter.x + areaWidth)
			{
				if ((core->mouse.position.y > areaCenter.y && core->mouse.position.y < areaCenter.y + areaHeight))
				{
					in = true;
				}
			}
		break;
		case 1:
			if ((core->mouse.position - areaCenter).isLength2DIn(areaWidth))
			{
				in = true;
			}
		break;
		}

		const float t = 0.0;
		if (in && (required || dsq->user.control.toolTipsOn))
		{
			back->alpha.interpolateTo(1, t);
			text->alpha.interpolateTo(1, t);
		}
		else
		{
			back->alpha.interpolateTo(0, t);
			text->alpha.interpolateTo(0, t);
		}
	}
}

void ToolTip::render(const RenderState& rs) const
{
	if (!game->getInGameMenu()->recipeMenu.on && toolTipsOn)
	{
		RenderObject::render(rs);
	}
}
