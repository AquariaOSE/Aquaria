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
#include "Shot.h"
#include "Game.h"

const float pSpawnRate = 1.0;
GasCloud::GasCloud(Entity *source, const Vector &position, const std::string &particles, const Vector &color, int radius, float life, float damage, bool isMoney, float poisonTime) : Entity()
{
	sourceEntity = source;

	this->poisonTime = poisonTime;

	this->particles = particles;
	//this->gfx = gfx;
	//this->isMoney = isMoney;
	this->position = position;
	this->damage = damage;
	this->radius = radius;
	pTimer = 0;
	setLife(life);
	setDecayRate(1);
	renderQuad = false;
	this->color = color;
	pTimer = pSpawnRate*10;
	vel = Vector(0,-100);

	emitter = new ParticleEffect;
	emitter->load(particles);
	emitter->start();
	emitter->setDie(true);
	//emitter->parentManagedStatic = true;
	//addChild(&emitter);
	dsq->game->addRenderObject(emitter, LR_PARTICLES);
	//setDamageTarget(DT_ENEMY_GAS, false)
	setEntityType(ET_NEUTRAL);

	deathSound = "";
}

void GasCloud::onUpdate(float dt)
{
	Entity::onUpdate(dt);
	if (emitter)
		emitter->position = this->position;
	if (life > 0.1f)
	{
		if (damage > 0)
		{
			FOR_ENTITIES(i)
			{
				Entity *e = (*i);
				if (e != this && (e->getEntityType() == ET_ENEMY || e->getEntityType() == ET_AVATAR) && e->isDamageTarget(DT_ENEMY_GAS) && (e->position - position).isLength2DIn(radius + e->collideRadius))
				{
					bool doit = (sourceEntity != e);
					if (doit && sourceEntity)
					{
						if (sourceEntity->getEntityType() == ET_AVATAR)
							doit = ((e->getEntityType() == ET_ENEMY) && doit);
					}

					if (doit)
					{
						DamageData d;
						d.attacker = 0;
						d.damageType = DT_ENEMY_GAS;
						d.damage = this->damage;
						e->damage(d);

						if (poisonTime > 0)
						{
							if (e->isDamageTarget(DT_ENEMY_POISON))
							{
								e->setPoison(1, poisonTime);
							}
						}
					}
				}
			}

		}
		/*
		pTimer += dt;
		while (pTimer > pSpawnRate)
		{
			int a = rand()%360;
			int r = rand()%radius;
			Vector pos = position;
			pos += Vector(sinf(a)*r, cosf(a)*r);
			Quad *quad = new Quad;
			{
				quad->rotation.z = rand()%360;
				quad->position = pos;
				quad->color = color;
				quad->setTexture(gfx);
				quad->alpha.path.addPathNode(0, 0);
				quad->alpha.path.addPathNode(0.5, 0.2);
				quad->alpha.path.addPathNode(0.5, 0.8);
				quad->alpha.path.addPathNode(0, 1);
				quad->alpha.startPath(3);
				quad->scale = Vector(0.5, 0.5);
				quad->scale.interpolateTo(Vector(2,2),4);
			}
			dsq->getTopStateData()->addRenderObject(quad, LR_PARTICLES);
			pTimer -= pSpawnRate;
		}
		*/
	}
	else
	{
		if (emitter && emitter->isRunning())
		{
			emitter->stop();
			emitter = 0;
		}
	}
}
