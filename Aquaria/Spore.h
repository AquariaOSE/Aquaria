#ifndef SPORE_H
#define SPORE_H

#include "CollideEntity.h"
#include <list>

class Spore : public CollideEntity
{
public:
	Spore(const Vector &position);
	typedef std::list<Spore*> Spores;
	static Spores spores;
	static void killAllSpores();
	static bool isPositionClear(const Vector &position);
	void destroy();
protected:
	void onEnterState(int state);
	void onUpdate(float dt);
	void onEndOfLife();
};


#endif
