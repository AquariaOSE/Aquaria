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

	int finalRadius = data.randomSpawnRadius;
	if (data.randomSpawnRadiusRange > 0)
		finalRadius += rand()%data.randomSpawnRadiusRange;

	switch (data.spawnArea)
	{
	case SpawnParticleData::SPAWN_CIRCLE:
	{
		float a = rand()%360;
		p->pos += Vector(sinf(a)*finalRadius * data.randomSpawnMod.x, cosf(a)*finalRadius * data.randomSpawnMod.y);
	}
	break;
	case SpawnParticleData::SPAWN_LINE:
	{
		if (rand()%2 == 0)
			p->pos.x += finalRadius;
		else
			p->pos.x -= finalRadius;
	}
	break;
	}

	if (data.randomScale1 == 1 && data.randomScale1 == data.randomScale2)
	{

	}
	else
	{
		int r = rand()%(int(data.randomScale2*100) - int(data.randomScale1*100));
		float sz = data.randomScale1 + float(r)/100.0f;
		p->scale = Vector(sz,sz);
	}


	if (data.randomRotationRange > 0)
	{
		p->rot.z = rand()%int(data.randomRotationRange);
		p->rot.ensureData();
		p->rot.data->target.z += p->rot.z;
	}



	if (data.randomVelocityMagnitude > 0)
	{
		float a = rand()%data.randomVelocityRange;
		Vector v = Vector(sinf(a)*data.randomVelocityMagnitude, cosf(a)*data.randomVelocityMagnitude);
		p->vel += v;
	}

	if (data.copyParentRotation)
	{
		p->rot.z = getAbsoluteRotation().z;
	}
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



	if (pe->isRunning() && core->particlesPaused <= data.pauseLevel)
	{
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
		else if (data.useSpawnRate)
		{
			spawnCount = 0;
			spawnPerc = 1;
			data.counter += dt;
			while (data.counter > data.spawnRate.x)  // Faster than division
			{
				data.counter -= data.spawnRate.x;
				spawnCount++;
			}
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
		data.velocityMagnitude.update(dt);
		data.spawnOffset.update(dt);
	}
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



	if (hasRot)
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


				if (p->rot.z != 0 || p->rot.isInterpolating())
				{
					glPushMatrix();

						glTranslatef(p->pos.x, p->pos.y,0);

						glRotatef(p->rot.z, 0, 0, 1);

						if (data.flipH || (data.copyParentFlip && (pe->isfh() || (pe->getParent() && pe->getParent()->isfh()))))
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
