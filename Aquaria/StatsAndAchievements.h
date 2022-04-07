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
#ifndef STATS_ACH_H
#define STATS_ACH_H

#ifndef BBGE_BUILD_STEAMWORKS
#include <list>
#endif

enum Achievements
{
	// completion
	ACH_DISCOVER_ALL_RECIPES	= 0,
	ACH_MASS_TRANSIT			= 1,
	ACH_EXPLORER				= 2,
	ACH_HEALTHY					= 3,
	ACH_AQUIRE_ALL_SONGS		= 4,
	// gameplay
	ACH_DEFEAT_PRIESTS			= 5,
	ACH_DEFEAT_OCTOMUN			= 6,
	ACH_DEFEAT_ROCKCRAB			= 7,
	ACH_DEFEAT_MANTISSHRIMP		= 8,
	ACH_DEFEAT_KINGJELLY		= 9,
	ACH_REACHED_OPEN_WATERS		= 10,
	ACH_REACHED_SPRITE_CAVE		= 11,
	ACH_REACHED_THE_VEIL		= 12,
	ACH_ROMANCE					= 13,
	ACH_BELLY_OF_THE_WHALE		= 14,
	ACH_ALEK_AND_DAREC			= 15,
	ACH_THE_FROZEN_VEIL			= 16,
	ACH_MOMMY_AND_DADDY			= 17,
	ACH_RESCUED_ALL_SOULS		= 18,
	// fun
	/*
	Bucking Bronco (ride the cute guy in front of Mithalas for a minute - why can't I remember his name?!)
	Combo Eater (eat like 12 enemies in a row)
	Eat a Parrot! (beast form, mid air)
	Fling a Monkey (with Nature seed)
	Kill the Coward (get that little yellow guy that runs away from you in the Forest)
	High Dive (jump from the Rukh's nest in the Veil into the water)
	Speed Racer (get a really fast time in the race)
	*/
	ACH_BUCKING_BRONCO			= 19,
	ACH_COMBO_EATER				= 20,
	ACH_ATE_A_PARROT			= 21,
	ACH_FLUNG_A_MONKEY			= 22,
	ACH_KILLED_THE_COWARD		= 23,
	ACH_HIGH_DIVE				= 24,
	ACH_SPEED_RACER				= 25,
	ACH_DEFEAT_MERGOG			= 26
};

struct Achievement
{
	Achievements achievementID;
	const char *chAchievementID;
	char name[128];
	char desc[256];
	bool achieved;
	int iconImage;
};

struct PlayStats
{
	// for sure
	float timePlayed;
	float timeInNormalForm;			// seconds spent in normal form
	float timeInEnergyForm;
	float timeInNatureForm;
	float timeInSunForm;
	float timeInSpiritForm;
	float timeInDualForm;

	float distanceSwam;			// distance swam in miles

	int timesSaved;				// # of times save is called
	int timesDied;				// # of times we hit the GameOver screen

	int foodConsumed;			// # of ingredients/food eaten

	// maybe
	int timesPlayed;			// # of times the game started
	int timesPoisoned;			// # of times the poison applied gets called on Naija

	int timesUsedTurtle;		// # of times trans turtle is used (how to check?)
	int timesRideSeahorse;		// # of times ride seahorse
	int timesLeptOutOfWater;	// # of times Naija goes not underwater after being underwater
	int timesBackflipped;		// # of times Naija does a backflip, check in Avatar.cpp
	float highestDive;			// ...?

	int creaturesConsumed;		// # of times swallow creatures, check in Avatar.cpp
	int sealoafsConsumed;		// # of sealoafs eaten
	int creaturesKilled;		//
	int monkeysFlung;			// check in StatsAndAchievements
	int racesRaced;				// # of races started
	int timesHugged;			// # of times Li hugs Naija
};


class ISteamUser;

class StatsAndAchievements
{
public:
	// Constructor
	StatsAndAchievements();

	// Run a frame
	void RunFrame();

	void appendStringData(std::string &data);

	// Accumulators
	//void AddDistanceTraveled( float flDistance );

	void entityDied(Entity *eDead);
	void update(float dt);
	void flingMonkey(Entity *e);
private:

	// Determine if we get this achievement now
	void EvaluateAchievement( Achievement &achievement );
	void UnlockAchievement( Achievement &achievement );

	// Store stats
	void StoreStatsIfNecessary();

#ifndef BBGE_BUILD_STEAMWORKS
	float unlockedDisplayTimestamp;
	std::list<std::string> unlockedToBeDisplayed;
#endif

	// Did we get the stats from Steam?
	bool requestedStats;
	bool statsValid;

	// Should we store stats this frame?
	bool storeStats;

	// PlayStats playStats;

	// Current Stat details
	//float m_flGameFeetTraveled;
	//uint64 m_ulTickCountGameStart;
	//double m_flGameDurationSeconds;

	// Persisted Stat details
	/*
	int m_nTotalGamesPlayed;
	int m_nTotalNumWins;
	int m_nTotalNumLosses;
	float m_flTotalFeetTraveled;
	float m_flMaxFeetTraveled;
	float m_flAverageSpeed;
	*/
};

#endif
