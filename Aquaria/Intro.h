#ifndef AQUARIA_INTRO_H
#define AQUARIA_INTRO_H

#include "StateManager.h"
#include "Precacher.h"

class Quad;

class Intro : public StateObject
{
public:
	Intro();
	void applyState();
	void removeState();
	void update(float dt);

	void endIntro();
	bool waitQuit(float t);
protected:
	void createMeteor(int layer, Vector pos, Vector off, Vector sz);
	void clearMeteors();

	std::vector<Quad*>meteors;

	bool done;

	Precacher cachy;

};

#endif
