#include "InputMapper.h"
#include "InputSystem.h"
#include "ActionMapper.h"
#include <assert.h>
#include <math.h>
#include <algorithm>

std::vector<ActionMapper*> IInputMapper::s_actionmappers;

GameControlState::GameControlState(size_t numbuttons, size_t numaxes)
: buttons(numbuttons), axes(numaxes)
, mouseX(0), mouseY(0), wheelDelta(0)
, lastDevice(INP_DEV_NODEVICE)
{
}

void GameControlState::clear()
{
	std::fill(axes.begin(), axes.end(), 0.0f);
	std::fill(buttons.begin(), buttons.end(), 0);
}

// -----------------

void IInputMapper::RegisterActionMapper(ActionMapper *mapper)
{
	s_actionmappers.push_back(mapper);
}

void IInputMapper::UnregisterActionMapper(ActionMapper *mapper)
{
	s_actionmappers.erase(std::remove(s_actionmappers.begin(), s_actionmappers.end(), mapper), s_actionmappers.end());
}

void IInputMapper::ForwardAction(unsigned idx, bool state, int playerID, InputDeviceType dev)
{
	const size_t N = s_actionmappers.size();
	for(size_t i = 0; i < N; ++i)
		s_actionmappers[i]->recvAction(idx, state, playerID, dev);
}

void IInputMapper::ForwardDirectInput(unsigned k, bool state)
{
	const size_t N = s_actionmappers.size();
	for(size_t i = 0; i < N; ++i)
		s_actionmappers[i]->recvDirectInput(k, state);
}


// ------------------


IInputMapper::IInputMapper()
{
	InputSystem::addMapper(this);
}

IInputMapper::~IInputMapper()
{
	InputSystem::addMapper(this);
}

// ------------------

InputMapper::InputMapper(int playerID)
: acceptMouse(true), acceptMouseID(-1), playerID(playerID)
{
}

InputMapper::~InputMapper()
{
	InputSystem::removeMapper(this);
}

static float rescale(float t, float lower, float upper, float rangeMin, float rangeMax)
{
	if(upper == lower)
		return rangeMin;

	return (((t - lower) / (upper - lower)) * (rangeMax - rangeMin)) + rangeMin;
}

static float clamp(float x, float a, float b)
{
	return std::max(a, std::min(x, b));
}

static float rescaleClamp(float t, float lower, float upper, float rangeMin, float rangeMax)
{
	return clamp(rescale(t, lower, upper, rangeMin, rangeMax), std::min(rangeMin, rangeMax), std::max(rangeMin, rangeMax));
}

unsigned char InputMapper::MapToButton(const Mapping& m)
{
	assert(m.buttonOrAxis > 0);

	switch(m.raw.src.ctrlType)
	{
		case INP_CTRL_BUTTON:
			return m.raw.u.pressed;

		case INP_CTRL_AXIS: // when axis is beyond threshold, register as button press
			return m.val.axis > 0
				? m.raw.u.axis > m.val.axis
				: m.raw.u.axis < m.val.axis;

		case INP_CTRL_HAT:
			return !memcmp(&m.raw.u.ivec, &m.val.ivec, sizeof(m.raw.u.ivec));

		default:
			;
	}

	return 0;
}

float InputMapper::MapToAxis(const Mapping& m)
{
	assert(m.buttonOrAxis < 0);

	switch(m.raw.src.ctrlType)
	{
		case INP_CTRL_BUTTON:
			return m.raw.u.pressed ? m.val.axis : 0.0f;

		case INP_CTRL_WHEEL:
			return m.raw.u.axis * m.val.axis;

		case INP_CTRL_AXIS:
			return m.raw.u.axis < 0
				? rescaleClamp(m.raw.u.axis,   m.val.axis,  1.0f,   0.0f,  1.0f)
				: rescaleClamp(m.raw.u.axis,  -m.val.axis, -1.0f,   0.0f, -1.0f);

		case INP_CTRL_HAT: // hat to axis; one hat direction should be 0
			return m.val.axis * m.raw.u.ivec.x + m.val.axis * m.raw.u.ivec.y;

		default:
			;
	}

	return 0.0f;
}

void InputMapper::accumulate(GameControlState *ctrl)
{
	ctrl->wheelDelta = wheelDelta;
	ctrl->mouseX = mouseX;
	ctrl->mouseY = mouseY;

	// walk over all inputs, check the values, apply to state
	const size_t N = mappings.size();
	for(size_t i = 0; i < N; ++i)
	{
		const Mapping& m = mappings[i];
		if(m.buttonOrAxis > 0)
			ctrl->buttons[m.buttonOrAxis - 1] += MapToButton(m);
		if(m.buttonOrAxis < 0)
			ctrl->axes[-m.buttonOrAxis - 1] += MapToAxis(m);

		ctrl->lastDevice = m.raw.src.deviceType;
	}

	wheelDelta = 0;
}

// TODO controllerfixup: need "follow-up" mapping
// means "axis A -> mapped axis 0 ==> if mapped axis 0 > thresh -> trigger action
// same for buttons: ACTION_SWIMLEFT -> ACTION_MENULEFT

void InputMapper::input(const RawInput *inp)
{
	size_t N = mappings.size();
	if(inp->src.deviceType == INP_DEV_MOUSE && acceptMouse && (acceptMouseID < 0 || inp->src.deviceID == acceptMouseID))
	{
		switch(inp->src.ctrlType)
		{
			case INP_CTRL_POSITION:
				mouseX = inp->u.ivec.x;
				mouseY = inp->u.ivec.y;
				break;

			case INP_CTRL_WHEEL:
				wheelDelta += inp->u.axis; // accumulate deltas until fetched
				break;
		}
	}

	for(size_t i = 0; i < N; ++i)
	{
		Mapping& m = mappings[i];
		if(!memcmp(&m.raw.src, &inp->src, sizeof(m.raw.src)))
		{
			memcpy(&m.raw.u, &inp->u, sizeof(m.raw.u)); // store raw inputs if source matches

			if(m.buttonOrAxis > 0) // Mapped to an action?
			{
				unsigned char state =  MapToButton(m); // Does this count as a button press?
				if(m.buttonState != state) // Did it actually change? (filter Axis movements that don't actually "release" the button)
					ForwardAction(m.buttonOrAxis - 1, state, playerID, inp->src.deviceType);
			}
		}
	}
}

// don't trigger if we just released a button or wiggled an analog stick by 1/10th millimeter.
bool InputMapper::CleanupForMapping(InputControlType c, InputMapper::MapType mt, InputValue& val)
{
	switch(c)
	{
		case INP_CTRL_BUTTON:
			if(!val.pressed)
				return false;
			val.pressed = 1;
			return true;

		case INP_CTRL_AXIS:
			//if(fabsf(val.axis) < 0.6f)
			//	return false;
			if(mt == TO_AXIS && val.axis < 0)
				val.axis = -val.axis;
			return true;

		case INP_CTRL_HAT:
			if(!!val.ivec.x + !!val.ivec.y != 1) // exactly one hat axis, not centered, not diagonal
				return false;
			if(mt == TO_AXIS)
			{
				// make sure the axis goes in the right direction
				if(val.ivec.x < 0)
					val.ivec.x = -val.ivec.x;
				if(val.ivec.y < 0)
					val.ivec.y = -val.ivec.y;
			}
			return true;

		case INP_CTRL_POSITION:
			return false;

		case INP_CTRL_WHEEL:
			if(!val.axis)
				return false;
			if(mt == TO_AXIS && val.axis < 0)
				val.axis = -val.axis;
	}

	return false;
}


bool InputMapper::addMapping(MapType mt, const RawInput& inp, unsigned targetID)
{
	Mapping m;
	m.buttonOrAxis = 1 + (mt == TO_BUTTON ? int(targetID) : -int(targetID));
	m.raw = inp;
	m.val = inp.u;

	if(!CleanupForMapping(inp.src.ctrlType, mt, m.val))
		return false;

	mappings.push_back(m);
	return true;
}

void InputMapper::clearMapping()
{
	mappings.clear();
}
