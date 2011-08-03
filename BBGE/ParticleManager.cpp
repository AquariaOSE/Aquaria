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

ParticleManager *particleManager = 0;

typedef std::map<std::string, ParticleEffect*> ParticleBank;
ParticleBank particleBank;

std::string ParticleManager::particleBankPath = "";

ParticleManager::ParticleManager(int size)
{
	particleManager = this;
	particles.resize(size);
	used = 0;
	free = 0;
	oldFree = 0;


	collideFunction = 0;
	specialFunction = 0;

	numActive = 0;

	setSize(size);
}

void ParticleManager::setSize(int size)
{
	// dangerous!
	for (int i = 0; i < particles.size(); i++)
	{
		Particle *p = &particles[i];
		if (p->emitter)
		{
			p->emitter->removeParticle(p);
		}
	}

	particles.clear();

	particles.resize(size);

	this->size = size;
	this->halfSize = size*0.5f;

	free = oldFree = 0;
}

void ParticleManager::setNumSuckPositions(int num)
{
	suckPositions.resize(num);
}

void ParticleManager::setSuckPosition(int idx, const Vector &pos)
{
	if (idx < 0 || idx >= suckPositions.size()) return;
	suckPositions[idx] = pos;
}

Vector *ParticleManager::getSuckPosition(int idx)
{
	if (idx < 0 || idx >= suckPositions.size()) return 0;
	return &suckPositions[idx];
}

void ParticleManager::updateParticle(Particle *p, float dt)
{
	if (!p->active)	return;
	numActive++;
	if (p->emitter && p->emitter->data.pauseLevel < core->particlesPaused)
		return;

	p->color.update(dt);
	p->alpha.update(dt);
	p->scale.update(dt);
	p->rot.update(dt);
	p->pos += p->vel * dt;
	p->life = p->life - dt;
	p->vel += p->gvy * dt;

	if (p->emitter)
	{
		if (p->emitter->data.influenced)
		{
			ParticleInfluence *pinf=0;

			if (collideFunction)
			{
				if (collideFunction(p->pos))
				{
					const bool bounce = false;
					if (bounce)
					{
						p->pos = p->lpos;
						p->vel = -p->vel;
					}
					else
					{
						// fade out
						p->vel = 0;
						endParticle(p);
						return;
					}
				}
			}

			if (specialFunction)
			{
				specialFunction(p);
			}
			p->lpos = p->pos;
			Influences::iterator i = influences.begin();
			for (; i != influences.end(); i++)
			{

				pinf = &(*i);
				Vector pos = p->pos;
				//HACK: what? ->
				if (p->emitter->data.spawnLocal && p->emitter->getParent())
					pos += p->emitter->getParent()->position;
				if ((pinf->pos - pos).isLength2DIn(pinf->size + p->emitter->data.influenced))
				{
					Vector dir = pos - pinf->pos;
					dir.setLength2D(pinf->spd);
					if (!pinf->pull)
						p->vel += dir * dt;
					else
						p->vel -= dir * dt;
				}
			}
		}
		if (p->emitter->data.suckIndex > -1)
		{
			Vector *suckPos = getSuckPosition(p->emitter->data.suckIndex);
			if (suckPos)
			{
				Vector dir = (*suckPos) - p->emitter->getWorldCollidePosition(p->pos);
				dir.setLength2D(p->emitter->data.suckStr);
				p->vel += dir * dt;
			}
		}
		if (p->rot.z != 0 || p->rot.isInterpolating())
			p->emitter->hasRot = true;
	}

	p->lpos = p->pos;


	if (p->life <= 0)
	{
		endParticle(p);
	}
}

void ParticleManager::endParticle(Particle *p)
{
	if (!p) return;

	if (p->emitter)
	{
		if (!p->emitter->data.deathPrt.empty())
		{
			RenderObject *r = p->emitter->getTopParent();
			if (r)
				core->createParticleEffect(p->emitter->data.deathPrt, p->pos, r->layer, p->rot.z);
		}
		p->emitter->removeParticle(p);
	}
	if (p->index != -1)
	{
		/*
		// set free if the neighbours are also free
		int backupFree = free;
		int oldFree = p->index;
		free = oldFree;
		nextFree();
		if (!particles[free].active)
		{
			free = oldFree;
			prevFree();
			if (!particles[free].active)
			{
				// good to go!
			}
			else
			{
				free = backupFree;
			}
		}
		else
		{
			free = backupFree;
		}
		*/
		//if (p->index > free)
		//{
		//free = p->index;
		//}
	}
	p->reset();
}

void ParticleManager::nextFree(int jump)
{
	free+=jump;
	if (free >= size)
		free -= size;
}

void ParticleManager::prevFree(int jump)
{
	free -= jump;
	if (free < 0)
		free += size;
}

void ParticleManager::setFree(int free)
{
	if (free != -1)
	{
		this->free = free;
	}
}

const int spread = 8;
const int spreadCheck = 128;

// travel the list until you find an empty or give up
Particle *ParticleManager::stomp()
{
	int c = 0, idx = -1;
	//int bFree = free;
	Particle *p = 0;
	bool exceed = false;


	nextFree();
	do
	{
		if (c >= spreadCheck)
		{
			exceed = true;
			break;
		}

		p = &particles[free];
		idx = free;
		nextFree();
		c++;
	}
	while (p->active);

	/*
	int nFree = free;
	int pFree = free;

	nextFree();
	nFree = free;

	free = bFree;
	prevFree();
	pFree = free;

	do
	{
		free = nFree;
		p = &particles[free];
		idx = free;
		nextFree();
		nFree = free;

		if (p->active)
		{
			free = pFree;
			p = &particles[free];
			idx = free;
			prevFree();
			pFree = free;
		}
		c++;
		if (c >= spreadCheck)
		{
			exceed = true;
			break;
		}
	}
	while (p->active);
	*/
	/*
	if (p->active)
	{
		c = 0;
		free = backupFree;
		nextFree();
		do
		{
			p = &particles[free];
			idx = free;
			nextFree();
			c++;
			if (c >= 8)
			{
				exceed = true;
				break;
			}
		}
		while (p->active);
	}
	*/

	if (exceed)
	{
		//debugLog("EXCEEDED");
	}

	endParticle(p);
	p->index = idx;
	return p;
}

/*
const int FREELISTSIZE = 32;

void ParticleManager::getFreeList(int list)
{
	for (int i = 0; i < FREELISTSIZE; i++)
	{
		if (freeList[curList] != -1)
		{
			Particle *p = &particles[freeList[curList]];
			freeList[curList] = -1;
			return p;
		}
	}
	return 0;
}

void ParticleManager::addFreeList()
{
}
*/

Particle *ParticleManager::getFreeParticle(Emitter *emitter)
{
	BBGE_PROF(ParticleManager_getFreeParticle);
	if (size == 0) return 0;

	Particle *p = 0;

	p = &particles[free];

	if (p->active)
	{
		p = stomp();
	}
	else
	{
		endParticle(p);
		p->index = free;
		nextFree(spread);
	}

	/*
	static int lpstep = 0;
	if (c > lpstep)
		lpstep = c;
	std::ostringstream os;
	os << "psteps: " << c << " largest: " << lpstep;
	debugLog(os.str());
	*/


	/*
	p = &particles[free];
	nextFree();
	*/



	if (emitter)
	{
		p->emitter = emitter;

		emitter->addParticle(p);
	}

	return p;
}

void loadParticleCallback(const std::string &filename, intptr_t param)
{
	ParticleEffect *e = new ParticleEffect();

	std::string ident;
	int first = filename.find_last_of('/')+1;
	ident = filename.substr(first, filename.find_last_of('.')-first);
	stringToLower(ident);

	e->bankLoad(ident, ParticleManager::particleBankPath);

	particleBank[ident] = e;
}

void ParticleManager::loadParticleBank(const std::string &bank1, const std::string &bank2)
{
	clearParticleBank();

	particleBankPath = bank1;
	forEachFile(bank1, ".txt", loadParticleCallback, 0);

	if (!bank2.empty())
	{
		particleBankPath = bank2;
		forEachFile(bank2, ".txt", loadParticleCallback, 0);
	}

	particleBankPath = "";
}

void ParticleManager::loadParticleEffectFromBank(const std::string &name, ParticleEffect *load)
{
	std::string realName = name;
	stringToLower(realName);
	ParticleEffect *fx = particleBank[realName];

	if (fx)
		fx->transfer(load);
	else
	{
		std::ostringstream os;
		os << "Did not find PE [" << name << "] in particle bank";
		debugLog(os.str());
	}
}

void ParticleManager::clearParticleBank()
{
	for (ParticleBank::iterator i = particleBank.begin(); i != particleBank.end(); i++)
	{
		ParticleEffect *e = (*i).second;
		if (e)
		{
			e->destroy();
			delete e;
		}
	}
	particleBank.clear();
}

int ParticleManager::getSize()
{
	return size;
}

void ParticleManager::update(float dt)
{
	BBGE_PROF(ParticleManager_update);
	numActive = 0;
	for (int i = 0; i < particles.size(); i++)
	{
		if (particles[i].active)
		{
			updateParticle(&particles[i], dt);
		}
	}
}

void ParticleManager::clearInfluences()
{
	influences.clear();
}

void ParticleManager::addInfluence(ParticleInfluence inf)
{
	influences.push_back(inf);
}

