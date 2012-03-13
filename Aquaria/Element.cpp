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
#include "Element.h"
#include "Game.h"
#include "Avatar.h"

Element::Element() : Quad()
{
	elementFlag = EF_NONE;
	wavyFlip = false;
	elementEffectIndex = -1;
	elementActive = true;
	bgLayer = 0;

	wavyAngleOffset=0;
	wavyMagnitude=0;
	wavyLerpIn=0;
	wavyWaving=false;
	wavyFlip=false;

	elementEffectType = 0;
	wavyRadius = 0;
	wavyMin = 0;
	wavyMax = 0;
	templateIdx = -1;

	setStatic(true);
}

void Element::wavyPull(int to, int from, float dt)
{
	Vector diff = wavy[to] - wavy[from];
	if (!diff.isZero())
	{
		diff.capLength2D(wavyMax);
		if (diff.isLength2DIn(wavyMin))
		{
			diff.setLength2D(wavyMin);
		}
		wavy[to] = wavy[from] + diff;
	}
}

void Element::updateEffects(float dt)
{
	switch (elementEffectType)
	{
	case EFX_ALPHA:
		alpha.update(dt);
	break;
	case EFX_WAVY:
		//debugLog("EXF_WAVY update");
		/// check player position
	{
		// if a big wavy doesn't work, this is probably why
		//if ((position - dsq->game->avatar->position).isLength2DIn(1024))
		{
			int touchIdx = -1;
			Vector pos = position;
			pos.y = position.y - (height*scale.y)/2;

			float hitPerc=0;
			Vector p = dsq->game->avatar->position;// + Vector(200,0);
			if (p.x > position.x-16 && p.x < position.x+16)
			{
				float h2 = (height*scale.y)/2.0f;
				if (p.y < position.y+h2 && p.y > position.y-h2)
				{
					touchIdx = 0;
					hitPerc = pos.y - p.y;
					hitPerc /= float(height*scale.y);
					hitPerc = (1.0f-hitPerc)-1.0f;
					
					
					//std::cout << "hit!\n";
					/*
					std::ostringstream os;
					os << "hit perc: " << hitPerc;
					debugLog(os.str());
					*/
				}
			}
			/*
			for (int i = 0; i < wavy.size()-1; i++)
			{
				if (isTouchingLine(wavy[0]+pos, wavy[i+1]+pos, dsq->game->avatar->position, wavyRadius))
				{
					//wavy[i+1] = dsq->game->avatar->position;
					touchIdx = i+1;
					break;
				}
			}
			*/

			if (touchIdx != -1)
			{
				// start pull
				wavyWaving = true;
				wavyAngleOffset = 0;
				float ramp = dsq->game->avatar->vel.getLength2D()/800.0f;
				if (ramp < 0)	ramp = 0;
				if (ramp > 1)	ramp = 1;

				wavyMagnitude = 100 * ramp + 16;

				if (dsq->game->avatar->vel.x < 0)
					wavyMagnitude = -wavyMagnitude;

				/*
				if (hitPerc > 0.35f)
					wavyMagnitude = -wavyMagnitude;
				*/

				wavyAngleOffset = (hitPerc-0.5f)*PI;

				wavySave = wavy;
				wavyLerpIn = 0;
			}

			if (wavyWaving)
			{
				/*
				float wavyMagnitude = wavyMagnitude;
				if (dsq->continuity.form == FORM_FISH)
					wavyMagnitude *= 0.1f;
				*/
				float wavyMagMult = 1;
				
				if (dsq->continuity.form == FORM_FISH)
					wavyMagMult = 0.4;

				float spd = PI*1.1f;
				float magRedSpd = 48;
				float lerpSpd = 5.0;
				for (int i = 0; i < wavy.size(); i++)
				{
					float weight = float(i)/float(wavy.size());
					if (wavyFlip)
						weight = 1.0f-weight;
					if (weight < 0.125f)
						weight *= 0.5f;
					wavy[i].x = sinf(wavyAngleOffset + (float(i)/float(wavy.size()))*PI)*float(wavyMagnitude*wavyMagMult)*weight;
					if (!wavySave.empty())
					{
						if (wavyLerpIn < 1)
							wavy[i].x = wavy[i].x*wavyLerpIn + (wavySave[i].x*(1.0f-wavyLerpIn));
					}
				}
				
				if (wavyLerpIn < 1)
				{
					wavyLerpIn += dt*lerpSpd;
					if (wavyLerpIn > 1)
						wavyLerpIn = 1;
				}
				wavyAngleOffset += dt*spd;
				if (wavyMagnitude > 0)
				{
					wavyMagnitude -= magRedSpd*dt;
					if (wavyMagnitude < 0)
						wavyMagnitude = 0;
				}
				else
				{
					wavyMagnitude += magRedSpd*dt;
					if (wavyMagnitude > 0)
						wavyMagnitude = 0;
				}

				//std::cout << "setting grid from wav w/ wavyWaving\n";
				setGridFromWavy();
				
			}
			else
			{
				//std::cout << "not waving";
				setGridFromWavy();
			}
			/*
			for (int i = touchIdx; i < wavy.size()-1; i++)
			{
				wavyPull(i, i+1, dt);
			}
			for (int i = touchIdx; i >= 0; i--)
			{
				wavyPull(i, i-1, dt);
			}
			*/

			// normal down pull
			/*
			for (int i = 0; i < wavy.size()-1; i++)
			{
				wavyPull(i, i+1, dt);
			}
			*/
		}
	}
	break;
	}
}

void Element::update(float dt)
{
	BBGE_PROF(Element_update);
	if (!core->particlesPaused)
	{
		updateLife(dt);
		updateEffects(dt);
		if (drawGrid)
			updateGrid(dt);
	}

//	updateCullVariables();
}

Element::~Element()
{
}

void Element::destroy()
{
	Quad::destroy();
}

int Element::getElementEffectIndex()
{
	return elementEffectIndex;
}

void Element::setGridFromWavy()
{
	if (drawGrid)
	{
		//std::cout << "set grid from wavy (" << xDivs << ", " << yDivs << ")\n"
		
		for (int x = 0; x < xDivs-1; x++)
		{
			for (int y = 0; y < yDivs; y++)
			{
				int wavy_y = (yDivs - y)-1;
				if (wavy_y < wavy.size())
				{
					drawGrid[x][y].x = (wavy[wavy_y].x/float(getWidth()) - 0.5f);
					drawGrid[x+1][y].x = (wavy[wavy_y].x/float(getWidth()) + 0.5f);
				}
			}
		}
	}
	else
	{
		//std::cout << "no drawgrid...\n";
	}
}

void Element::setElementEffectByIndex(int eidx)
{
	deleteGrid();

	setBlendType(RenderObject::BLEND_DEFAULT);
	alpha.stop();
	alpha = 1;

	elementEffectIndex = eidx;

	ElementEffect e = dsq->getElementEffectByIndex(eidx);

	switch(e.type)
	{
	case EFX_SEGS:
	{
		setSegs(e.segsx, e.segsy, e.segs_dgox, e.segs_dgoy, e.segs_dgmx, e.segs_dgmy, e.segs_dgtm, e.segs_dgo);
		setStatic(false);
	}
	break;
	case EFX_ALPHA:
	{
		setBlendType(e.blendType);
		alpha = e.alpha;
		setStatic(false);
	}
	break;
	case EFX_WAVY:
	{
		/*
		char buf[256];
		sprintf(buf, "setting wavy segsy: %d radius: %d min: %d max: %d", e.segsy, e.wavy_radius, e.wavy_min, e.wavy_max);
		debugLog(buf);
		*/
		wavy.resize(e.segsy);
		float bity = float(getHeight())/float(e.segsy);
		for (int i = 0; i < wavy.size(); i++)
		{
			wavy[i] = Vector(0, -(i*bity));
		}
		//wavySave = wavy;
		wavyRadius = e.wavy_radius;
		wavyFlip = e.wavy_flip;
		wavyMin = bity;
		wavyMax = bity*1.2f;

		//wavyRadius = 8;

		createGrid(2, e.segsy);

		setGridFromWavy();

		//createGrid(8,8);
		/*
		wavyMin = e.wavy_min;
		wavyMax = e.wavy_max;
		*/
		setStatic(false);
	}
	break;
	default:
		setStatic(true);
	break;
	}
	elementEffectType = e.type;
}

void Element::render()
{
	if (!elementActive) return;
#ifdef AQUARIA_BUILD_SCENEEDITOR
	if (dsq->game->isSceneEditorActive() && this->bgLayer == dsq->game->sceneEditor.bgLayer
		&& dsq->game->sceneEditor.editType == ET_ELEMENTS)
	{
		renderBorderColor = Vector(0.5,0.5,0.5);
		if (!dsq->game->sceneEditor.selectedElements.empty())
		{
			for (int i = 0; i < dsq->game->sceneEditor.selectedElements.size(); i++)
			{
				if (this == dsq->game->sceneEditor.selectedElements[i])
					renderBorderColor = Vector(1,1,1);
			}
		}
		else
		{
			if (dsq->game->sceneEditor.editingElement == this)
				renderBorderColor = Vector(1,1,1);
		}
		renderBorder = true;
		//errorLog("!^!^$");
	}
#endif
	
	if (this->elementEffectType == EFX_WAVY)
	{
		//debugLog("rendering efx_wavy");
	}
	
	Quad::render();

	/*
	if (!wavy.empty())
	{
		
		glDisable(GL_BLEND);
		Vector pos = position;
		pos.y = position.y + (getHeight()*scale.y)/2.0f;
		glBegin(GL_LINES);
			for (int i = 0; i < wavy.size()-1; i++)
			{
				glColor4f(1, 0, 0, 1);
				glVertex3f(wavy[i].x+pos.x, wavy[i].y+pos.y, 0);
				glVertex3f(wavy[i+1].x+pos.x, wavy[i+1].y+pos.y, 0);
			}
		glEnd();
		glEnable(GL_BLEND);
	}
	*/

	renderBorder = false;
}

void Element::fillGrid()
{
	if (life == 1 && elementActive)
	{
		if (elementFlag == EF_SOLID)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLE, true);
		}
		else if (elementFlag == EF_HURT)
		{
			dsq->game->fillGridFromQuad(this, OT_HURT, false);
		}
		else if (elementFlag == EF_SOLID2)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLE, false); 
		}
		else if (elementFlag == EF_SOLID3)
		{
			dsq->game->fillGridFromQuad(this, OT_INVISIBLEIN, false); 
		}
	}
}

// override this functionality as needed
bool Element::canSeeAvatar(Avatar *avatar) 
{
	return false;
}

bool Element::isActive()
{
	return true;
}

float Element::getSortDepth()
{
	return Quad::getSortDepth() - bgLayer*0.01f;
}

