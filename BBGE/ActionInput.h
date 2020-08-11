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
#ifndef ACTIONINPUT_H
#define ACTIONINPUT_H

#include <string>
#include "InputSystem.h"

// Slots for each input/key type
enum ActionInputSize
{
	INP_MSESIZE = 1,
	INP_KEYSIZE = 2,
	INP_JOYSIZE = 1,
	INP_NUMFIELDS = INP_MSESIZE + INP_KEYSIZE + INP_JOYSIZE
};

std::string getInputCodeToUserString(unsigned int k, size_t joystickID);

// Mapping for one input action ("Cook", "SwimLeft", etc)
class ActionInput
{
public: // only public because some static funcs in the cpp file need this. Don't use elsewhere!
	struct JoyData
	{
		JoyData();
		InputControlType ctrlType;
		unsigned ctrlID; // 0 = not set, otherwise actual id + 1
		int x, y; // each [-1..+1]: (x,y) for hat direction, (x) for axis direction
	};
	inline const std::string& getName() const { return name; }
private:
	unsigned mse[INP_MSESIZE]; // stores button+1, 0 = not set
	unsigned key[INP_KEYSIZE]; // 0 = no key
	JoyData joy[INP_JOYSIZE];
	std::string name;
public:

	ActionInput(const std::string& name_);

	// indexing:
	// slot -> per-category (start at 0 for each INP_*)
	// field: absolute, [0 ..INP_NUMFIELDS)

	static unsigned GetField(InputDeviceType dev, unsigned slot);

	bool Import(const RawInput& inp, unsigned slot); // autodetects type, slot is per-category
	bool ImportField(const RawInput& inp, unsigned field); // returns false if field and type of inp don't match
	bool Export(RawInput& inp, unsigned field) const; // returns false if no mapping present
	
	// for checking whether a key/button/etc is configured
	bool hasEntry(unsigned field) const;
	void clearEntry(unsigned field);

	// for config (de-)serialization
	std::string toString() const;
	void fromString(const std::string &read);

	// for the UI (field is in [0..INP_COMBINED_SIZE])
	std::string prettyPrintField(unsigned field, int joystickID = -1) const;
};

#endif
