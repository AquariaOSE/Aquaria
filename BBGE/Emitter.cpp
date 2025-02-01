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
#include "Particles.h"
#include "RenderBase.h"


SpawnParticleData::SpawnParticleData()
{
	suckIndex = -1;
	suckStr = 0;

	randomScale1 = 1;
	randomScale2 = 1;
	randomAlphaMod1 = 1;
	randomAlphaMod2 = 1;
	influenced = 0;
	spawnLocal = false;
	life = 1;
	blendType = BLEND_DEFAULT;
	scale = Vector(1,1,1);
	width = 64;
	height = 64;
	color = Vector(1,1,1);
	alpha = 1;
	randomSpawnRadius = 0;
	lastDTDifference = 0;

	randomRotationRange = 0;
	number = 1;

	randomSpawnRadiusRange = 0;
	randomSpawnMod = Vector(1,1);

	randomVelocityMagnitude = 0;

	copyParentRotation = 0;
	justOne = didOne = false;
	flipH = flipV = 0;
	spawnTimeOffset = 0;
	pauseLevel = 0;
	copyParentFlip = 0;
	inheritColor = false;
	inheritAlpha = false;
}

Emitter::Emitter(ParticleEffect *pe) : Quad(), pe(pe)
{
	//HACK:
	cull = false;
	hasRot = false;
}

void Emitter::destroy()
{
	for (Particles::iterator i = particles.begin(); i != particles.end(); i++)
	{
		(*i)->active = false;
		(*i)->emitter = 0;
	}
	particles.clear();
	Quad::destroy();

}

void Emitter::spawnParticle(float perc)
{
	Particle *p = particleManager->getFreeParticle(this);

	p->active = true;

	p->life = data.life;
	setBlendType(data.blendType);

	width = data.width;
	height = data.height;

	p->color = data.color;
	p->alpha = data.alpha;

	p->vel += data.initialVelocity;
	p->gvy = data.gravity;
	p->scale = data.scale;

	p->rot = data.rotation;

	p->pos = lastSpawn + ((currentSpawn - lastSpawn) * perc);

	float finalRadius = data.randomSpawnRadius + rng.f01() * data.randomSpawnRadiusRange;

	{
		float a = randAngle();
		p->pos += Vector(sinf(a)*finalRadius * data.randomSpawnMod.x, cosf(a)*finalRadius * data.randomSpawnMod.y);
	}

	{
		float sz = lerp(data.randomScale1, data.randomScale2, rng.f01());
		p->scale *= sz;
		if(p->scale.data)
			p->scale.data->target *= sz;
	}

	if (data.randomRotationRange > 0)
	{
		p->rot.z = rng.f01() * data.randomRotationRange;
		if(p->rot.data)
			p->rot.data->target.z += p->rot.z;
	}

	if (data.randomVelocityMagnitude > 0)
	{
		float a = randAngle();
		Vector v = Vector(sinf(a)*data.randomVelocityMagnitude, cosf(a)*data.randomVelocityMagnitude);
		p->vel += v;
	}

	if (data.copyParentRotation)
	{
		p->rot.z = getAbsoluteRotation().z;
	}
}

float Emitter::randAngle()
{
	return 2 * PI * rng.f01();
}

Vector Emitter::getSpawnPosition()
{
	if (!data.spawnLocal)
		return pe->getWorldPosition();
	return Vector(0,0);
}

void Emitter::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if(!(pe->isRunning() && core->particlesPaused <= data.pauseLevel))
		return;


	if (data.spawnTimeOffset > 0)
	{
		data.spawnTimeOffset -= dt;
		if (data.spawnTimeOffset > 0)
			return;
	}

	int spawnCount;
	float spawnPerc;
	if (data.justOne)
	{
		if (data.didOne)
			spawnCount = 0;
		else
			spawnCount = data.justOne;
		spawnPerc = 1;
		data.didOne = 1;
	}
	else
	{
		float num = data.number.x * dt;
		num += data.lastDTDifference;
		spawnCount = int(num);
		data.lastDTDifference = num - float(spawnCount);
		if (spawnCount > 0)
			spawnPerc = 1.0f / float(spawnCount);
	}

	if (spawnCount > 0)
	{
		// Avoid calling this until we know we actually need it for
		// generating a particle (it has to apply the matrix chain,
		// which is slow).
		currentSpawn = getSpawnPosition();
		if (lastSpawn.isZero())
			lastSpawn = currentSpawn;

		for (; spawnCount > 0; spawnCount--)
		{
			spawnParticle(spawnPerc);
		}

		lastSpawn = currentSpawn;
	}

	data.number.update(dt);
}

void Emitter::start()
{
	data.didOne = 0;
	lastSpawn = getSpawnPosition();
}

void Emitter::stop()
{
}

void Emitter::addParticle(Particle *p)
{
	particles.push_front(p);
}

void Emitter::removeParticle(Particle *p)
{
	if (particles.back() == p)
	{
		particles.pop_back();
	}
	else
		particles.remove(p);

}

void Emitter::onRender(const RenderState& rs) const
{
	if (particles.empty()) return;

	if (!data.spawnLocal)
	{
		glLoadIdentity();

		core->setupRenderPositionAndScale();
	}

	float w2 = width*0.5f;
	float h2 = height*0.5f;

	if (texture)
		texture->apply();


	const bool flip = data.flipH != (data.copyParentFlip && pe->isfhr());
	if (flip || hasRot)
	{
		Vector colorMult = data.inheritColor ? pe->color : Vector(1, 1, 1);
		float alphaMult = data.inheritAlpha ? pe->alpha.x : 1;

		for (Particles::const_iterator i = particles.begin(); i != particles.end(); i++)
		{
			Particle *p = *i;
			if (p->active)
			{
				const float dx = w2 * p->scale.x;
				const float dy = h2 * p->scale.y;

				Vector col = p->color * colorMult;
				glColor4f(col.x, col.y, col.z, p->alpha.x * alphaMult);


				if (flip || p->rot.z != 0 || p->rot.isInterpolating())
				{
					glPushMatrix();

						glTranslatef(p->pos.x, p->pos.y,0);

						glRotatef(p->rot.z, 0, 0, 1);

						if (flip)
						{
							glRotatef(180, 0, 1, 0);
						}



						glBegin(GL_QUADS);
							glTexCoord2f(0,1);
							glVertex2f(-dx, +dy);

							glTexCoord2f(1,1);
							glVertex2f(+dx, +dy);

							glTexCoord2f(1,0);
							glVertex2f(+dx, -dy);

							glTexCoord2f(0,0);
							glVertex2f(-dx, -dy);
						glEnd();

					glPopMatrix();
				}
				else
				{
					const float x = p->pos.x;
					const float y = p->pos.y;

					glBegin(GL_QUADS);
						glTexCoord2f(0,1);
						glVertex2f(x-dx, y+dy);

						glTexCoord2f(1,1);
						glVertex2f(x+dx, y+dy);

						glTexCoord2f(1,0);
						glVertex2f(x+dx, y-dy);

						glTexCoord2f(0,0);
						glVertex2f(x-dx, y-dy);
					glEnd();
				}
			}
		}
	}
	else
	{
		glBegin(GL_QUADS);
		for (Particles::const_iterator i = particles.begin(); i != particles.end(); i++)
		{
			Particle *p = *i;
			if (p->active)
			{
				const float x = p->pos.x;
				const float y = p->pos.y;
				const float dx = w2 * p->scale.x;
				const float dy = h2 * p->scale.y;

				glColor4f(p->color.x, p->color.y, p->color.z, p->alpha.x);

				glTexCoord2f(0,1);
				glVertex2f(x-dx, y+dy);

				glTexCoord2f(1,1);
				glVertex2f(x+dx, y+dy);

				glTexCoord2f(1,0);
				glVertex2f(x+dx, y-dy);

				glTexCoord2f(0,0);
				glVertex2f(x-dx, y-dy);
			}
		}
		glEnd();
	}



}
