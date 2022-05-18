/*
Copyright (C) 2007, 2012 - Bit-Blot

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
#ifndef SCRIPTOBJECT_H
#define SCRIPTOBJECT_H

enum ScriptObjectType
{
	SCO_NONE              = 0x0000,

	// If you change this enum, do not forget to adjust the string array in the cpp,
	// and to add additional compile time assertions to ScriptInterface.cpp as necessary!
	SCO_RENDEROBJECT      = 0x0001,
	SCO_ENTITY            = 0x0002,
	SCO_INGREDIENT        = 0x0004,
	SCO_COLLIDE_ENTITY    = 0x0008,
	SCO_SCRIPTED_ENTITY   = 0x0010,
	SCO_BEAM              = 0x0020,
	SCO_SHOT              = 0x0040,
	SCO_WEB               = 0x0080,
	SCO_BONE              = 0x0100,
	SCO_PATH              = 0x0200,
	SCO_QUAD              = 0x0400,
	SCO_TEXT              = 0x0800,
	SCO_PAUSEQUAD         = 0x1000,
	SCO_SHADER            = 0x2000,
	SCO_PARTICLE_EFFECT   = 0x4000,
	SCO_QUAD_GRID         = 0x8000,
	SCO_COLLIDE_QUAD      = 0x10000,

	SCO_FORCE_32BIT = 0xFFFFFFFF
};


class ScriptObject
{
public:

	ScriptObject()
		: _objtype(SCO_NONE)
	{
	}

	virtual ~ScriptObject() {}

	inline void addType(ScriptObjectType ty)
	{
		_objtype  = ScriptObjectType(int(ty) | int(_objtype)); // prevent the compiler from crying
	}

	inline bool isType(ScriptObjectType bt) const
	{
		return (_objtype & bt) == bt;
	}

	inline bool isExactType(ScriptObjectType bt) const
	{
		return _objtype  == bt;
	}

	inline std::string getTypeString() const
	{
		return getTypeString(_objtype);
	}

	static std::string getTypeString(unsigned int ty);

	// public to allow the static compile check in ScriptInterface.cpp to work
	ScriptObjectType _objtype;
};

#endif
