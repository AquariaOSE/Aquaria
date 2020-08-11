#ifndef BBGE_INPUTMAPPER_H
#define BBGE_INPUTMAPPER_H

#include "InputSystem.h"
#include <vector>

class ActionMapper;

// This is what the engine sees to handle the current state.
// Changes are not tracked here!
struct GameControlState
{
	GameControlState(size_t numbuttons, size_t numaxes);
	int mouseX, mouseY;
	float wheelDelta;
	InputDeviceType lastDevice;
	std::vector<unsigned char> buttons; // actually bool
	std::vector<float> axes; // uncalibrated floats, -1 .. +1

	void clear(); // Call before InputMapper::accumulate()
};

class IInputMapper
{
public:
	IInputMapper();
	virtual void input(const RawInput *inp) = 0;

	// Called by ActionMapper ctor/dtor to register itself
	static void RegisterActionMapper(ActionMapper *mapper);
	static void UnregisterActionMapper(ActionMapper *mapper);

	// Forward actions to all registered input mappers
	static void ForwardAction(unsigned idx, bool state, int playerID, InputDeviceType dev);
	static void ForwardDirectInput(unsigned k, bool state);

protected:
	virtual ~IInputMapper();

private:
	// shared between all InputMapper instances
	static std::vector<ActionMapper*> s_actionmappers;
};

// Maps raw device input to player input.
// One mapper per player.
class InputMapper : public IInputMapper
{
public:
	InputMapper(int playerID);
	virtual ~InputMapper();

	bool acceptMouse;
	int acceptMouseID; // -1: accept all, otherwise: that id
	const int playerID;

	// raw data go in
	virtual void input(const RawInput *inp);

	// output: apply changes to ctrl
	void accumulate(GameControlState *ctrl);

	enum MapType
	{
		TO_BUTTON,
		TO_AXIS
	};
	bool addMapping(MapType mt, const RawInput& inp, unsigned targetID);
	void clearMapping();

private:
	struct Mapping
	{
		RawInput raw; // last input that came in, also contains src to check for
		int buttonOrAxis; // >0: button+1, <0: (-axis-1), never 0
		InputValue val; // used as min. axis value to trigger, deadzone, hat direction, etc
		unsigned char buttonState; // for buttons: consider this pressed?
	};
	const GameControlState *target;
	float wheelDelta;
	int mouseX, mouseY;
	std::vector<Mapping> mappings;

	static unsigned char MapToButton(const InputMapper::Mapping& m);
	static float MapToAxis(const InputMapper::Mapping& m);
	static bool CleanupForMapping(InputControlType c, InputMapper::MapType mt, InputValue& val); // modifies val, returns whether acceptable
};

#endif
