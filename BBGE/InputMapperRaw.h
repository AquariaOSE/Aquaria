#ifndef BBGE_INPUTMAPPERRAW_H
#define BBGE_INPUTMAPPERRAW_H

#include "InputMapper.h"
#include "GameKeys.h"

// Legacy mapper: Raw keys to game actions. Required by e.g. editor.
// Used by legacy parts of ActionMapper.
class InputMapperRaw : public IInputMapper
{
public:
	InputMapperRaw();
	virtual ~InputMapperRaw();
	virtual void input(const RawInput *inp);
	void update();

	bool isKeyPressed(unsigned k) const;
	bool isKeyChanged(unsigned k) const;

private:
	// actually bools
	unsigned char keyState[ACTION_BUTTON_ENUM_SIZE];
	unsigned char keyChanged[ACTION_BUTTON_ENUM_SIZE];
};


#endif
