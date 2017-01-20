#ifndef AQUARIA_DAMAGE_H
#define AQUARIA_DAMAGE_H

#include "Vector.h"
#include "GameEnums.h"

class Shot;
class Bone;
class Entity;

enum DamageType
{
	DT_NONE					= -1,
	DT_ENEMY				= 0,
	DT_ENEMY_ENERGYBLAST	= 1,
	DT_ENEMY_SHOCK			= 2,
	DT_ENEMY_BITE			= 3,
	DT_ENEMY_TRAP			= 4,
	DT_ENEMY_WEB			= 5,
	DT_ENEMY_BEAM			= 6,
	DT_ENEMY_GAS			= 7,
	DT_ENEMY_INK			= 8,
	DT_ENEMY_POISON			= 9,
	DT_ENEMY_ACTIVEPOISON	= 10,
	DT_ENEMY_CREATOR		= 11,
	DT_ENEMY_MANTISBOMB		= 12,
	DT_ENEMY_REALMAX		= 13,
	DT_ENEMY_MAX			= 13,

	DT_AVATAR				= 1000,
	DT_AVATAR_ENERGYBLAST	= 1001,
	DT_AVATAR_SHOCK			= 1002,
	DT_AVATAR_BITE			= 1003,
	DT_AVATAR_VOMIT			= 1004,
	DT_AVATAR_ACID			= 1005,
	DT_AVATAR_SPORECHILD	= 1006,
	DT_AVATAR_LIZAP			= 1007,
	DT_AVATAR_NATURE		= 1008,
	DT_AVATAR_ENERGYROLL	= 1009,
	DT_AVATAR_VINE			= 1010,
	DT_AVATAR_EAT			= 1011,
	DT_AVATAR_EAT_BASICSHOT	= 1011,
	DT_AVATAR_EAT_MAX		= 1012,
	DT_AVATAR_LANCEATTACH	= 1013,
	DT_AVATAR_LANCE			= 1014,
	DT_AVATAR_CREATORSHOT	= 1015,
	DT_AVATAR_DUALFORMLI	= 1016,
	DT_AVATAR_DUALFORMNAIJA = 1017,
	DT_AVATAR_BUBBLE		= 1018,
	DT_AVATAR_SEED			= 1019,
	DT_AVATAR_PET			= 1020,
	DT_AVATAR_PETNAUTILUS	= 1021,
	DT_AVATAR_PETBITE		= 1022,
	DT_AVATAR_REALMAX		,
	DT_AVATAR_MAX			= 1030,
	DT_TOUCH				= 1031,
	DT_CRUSH				= 1032,
	DT_SPIKES				= 1033,
	DT_STEAM				= 1034,
	DT_WALLHURT				= 1035,
	DT_REALMAX
};


struct DamageData
{
	DamageData()
	{
		damage = 0;
		attacker = 0;
		bone = 0;
		damageType = DT_TOUCH;
		form = (FormType)0;
		shot = 0;
		effectTime = 0;
		useTimer = true;
	}
	FormType form;
	DamageType damageType;
	Entity *attacker;
	Bone *bone;
	float damage;
	Vector hitPos;
	Shot *shot;
	float effectTime;
	bool useTimer;
};



#endif
