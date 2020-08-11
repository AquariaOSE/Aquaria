#ifndef GAMEINPUT_H
#define GAMEINPUT_H

#include "Vector.h"
#include "InputMapper.h"
#include "GameEnums.h"

class GameInput : public GameControlState
{
public:
	GameInput();
	void update();
	Vector getStick1() const;
	Vector getStick2() const;

private:
	void emulateAction(unsigned ac, bool on);
	bool _emuActions[ACTION_SIZE];
};

#endif
