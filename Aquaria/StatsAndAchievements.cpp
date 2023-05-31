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
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "StatsAndAchievements.h"
#include "ttvfs_stdio.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof (x) / sizeof ((x)[0]))
#endif

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }

static Achievement g_rgAchievements[] =
{
	_ACH_ID( ACH_DISCOVER_ALL_RECIPES,	"Experienced Chef" ),		// verified
	_ACH_ID( ACH_MASS_TRANSIT,			"Mass Transit" ),			// verified
	_ACH_ID( ACH_EXPLORER,				"Explorer" ),				// verified
	_ACH_ID( ACH_HEALTHY,				"Healthy" ),				// verified (really make sure)

	_ACH_ID( ACH_AQUIRE_ALL_SONGS,		"Songstress" ),				// verified
	_ACH_ID( ACH_DEFEAT_PRIESTS,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_DEFEAT_OCTOMUN,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_DEFEAT_ROCKCRAB,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_DEFEAT_MANTISSHRIMP,	"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_DEFEAT_KINGJELLY,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_REACHED_OPEN_WATERS,	"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_REACHED_SPRITE_CAVE,	"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_REACHED_THE_VEIL,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_ROMANCE,				"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_BELLY_OF_THE_WHALE,	"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_ALEK_AND_DAREC,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_THE_FROZEN_VEIL,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_MOMMY_AND_DADDY,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_RESCUED_ALL_SOULS,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_BUCKING_BRONCO,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_COMBO_EATER,			"xxxxxxxxx" ),				// verified (adjust # of devours?)
	_ACH_ID( ACH_ATE_A_PARROT,			"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_FLUNG_A_MONKEY,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_KILLED_THE_COWARD,		"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_HIGH_DIVE,				"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_SPEED_RACER,			"xxxxxxxxx" ),				// verified
	_ACH_ID( ACH_DEFEAT_MERGOG,			"xxxxxxxxx" ),				// verified
};


// temp record
bool	killedParrotWithBite		= false;
bool	flungMonkey		= false;
bool	killedCoward				= false;

bool	highDiveIsHigh				= false;
bool	highDiveIsDone				= false;

float	ridingEkkritTime			= 0.0f;
float	ridingEkkritTimeMax			= 60.0f;
bool	rodeEkkritToTheStars		= false;

int		biteDeathComboNum			= 0;
float	biteDeathComboTime			= 0.5f;
float	biteDeathComboCounter		= 0.0f;
const int biteDeathComboMax			= 6;

const int seahorseRaceAchievementTimeMin = 59;


const int SUNKENCITY_BOSSDONE				= 16;

const int FLAG_SUNKENCITY_PUZZLE			= 113;

const int FLAG_SPIRIT_ERULIAN				= 124;
const int FLAG_SPIRIT_KROTITE				= 125;
const int FLAG_SPIRIT_DRASK					= 126;
const int FLAG_SPIRIT_DRUNIAD				= 127;

const int FLAG_TRANSTURTLE_VEIL01			= 130;
//const int FLAG_TRANSTURTLE_OPENWATER06		= 131;
const int FLAG_TRANSTURTLE_FOREST04			= 132;
const int FLAG_TRANSTURTLE_OPENWATER03		= 133;
const int FLAG_TRANSTURTLE_FOREST05			= 134;
const int FLAG_TRANSTURTLE_MAINAREA			= 135;
const int FLAG_TRANSTURTLE_SEAHORSE			= 136;
const int FLAG_TRANSTURTLE_VEIL02			= 137;
const int FLAG_TRANSTURTLE_ABYSS03			= 138;
const int FLAG_TRANSTURTLE_FINALBOSS		= 139;

const int FLAG_SEAHORSEBESTTIME				= 247;

//const int FLAG_MINIBOSS_START				= 700;
//const int FLAG_MINIBOSS_NAUTILUSPRIME		= 700;
const int FLAG_MINIBOSS_KINGJELLY			= 701;
const int FLAG_MINIBOSS_MERGOG				= 702;
const int FLAG_MINIBOSS_CRAB				= 703;
const int FLAG_MINIBOSS_OCTOMUN				= 704;
const int FLAG_MINIBOSS_MANTISSHRIMP		= 705;
//const int FLAG_MINIBOSS_PRIESTS				= 706;
//const int FLAG_MINIBOSS_END					= 720;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
#pragma warning( push )
//  warning C4355: 'this' : used in base member initializer list
//  This is OK because it's warning on setting up the Steam callbacks, they won't use this until after construction is done
#pragma warning( disable : 4355 )
#endif
StatsAndAchievements::StatsAndAchievements()
{


#ifndef BBGE_BUILD_STEAMWORKS
	unlockedDisplayTimestamp = -1.0f;
#endif

	requestedStats = false;
	statsValid = false;
	storeStats = false;


}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

//-----------------------------------------------------------------------------
// Purpose: Run a frame for the CStatsAndAchievements
//-----------------------------------------------------------------------------
void StatsAndAchievements::RunFrame()
{
#ifndef BBGE_BUILD_STEAMWORKS
	if ( !requestedStats )
	{
		requestedStats = true;

		const size_t max_achievements = ARRAYSIZE(g_rgAchievements);
		VFILE *io = NULL;

		// Get generic achievement data...
		std::string fname = localisePath("data/achievements.txt");
		io = vfopen(fname.c_str(), "r");
		char line[1024];
		for (size_t i = 0; i < max_achievements; i++)
		{
			if (!io || (vfgets(line, sizeof (line), io) == NULL))
				snprintf(line, sizeof (line), "Achievement #%d", (int) i);
			else
			{
				for (char *ptr = (line + strlen(line)) - 1; (ptr >= line) && ((*ptr == '\r') || (*ptr == '\n')); ptr--)
					*ptr = '\0';
			}
			line[sizeof (g_rgAchievements[i].name) - 1] = '\0';  // just in case.
			strcpy(g_rgAchievements[i].name, line);

			if (!io || (vfgets(line, sizeof (line), io) == NULL))
				snprintf(line, sizeof (line), "[Description of Achievement #%d is missing!]", (int) i);
			else
			{
				for (char *ptr = (line + strlen(line)) - 1; (ptr >= line) && ((*ptr == '\r') || (*ptr == '\n')); ptr--)
					*ptr = '\0';
			}
			line[sizeof (g_rgAchievements[i].desc) - 1] = '\0';  // just in case.
			strcpy(g_rgAchievements[i].desc, line);

			// unsupported at the moment.
			g_rgAchievements[i].iconImage = 0;
		}

		if (io != NULL)
			vfclose(io);

		// See what this specific player has achieved...

		unsigned char *buf = new unsigned char[max_achievements];
		size_t br = 0;
		fname = (core->getUserDataFolder() + "/achievements.bin");
		FILE *u = fopen(fname.c_str(), "rb");
		if (u == NULL)
			statsValid = true;  // nothing to report.
		else
		{
			br = fread(buf, sizeof (buf[0]), max_achievements, u);
			fclose(u);
		}

		if (br == max_achievements)
		{
			statsValid = true;  // but we'll reset if there's a problem.
			for (size_t i = 0; statsValid && (i < max_achievements); i++)
			{
				const int val = ((int) (buf[i] ^ 0xFF)) - ((int)i);
				statsValid = ((val == 0) || (val == 1));
				g_rgAchievements[i].achieved = (val == 1);
			}
		}
		delete[] buf;
	}
#endif

	if ( !statsValid ) {
		debugLog("stats not valid");
		return;
	}

	// Get info from sources

	// Evaluate achievements

	// but only if we're not in a mod
	if (!dsq->mod.isActive())
	{
		for (size_t iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch )
		{
			EvaluateAchievement( g_rgAchievements[iAch] );
		}
	}


	// Store stats
	StoreStatsIfNecessary();
}

void StatsAndAchievements::appendStringData(std::string &data)
{
	if (!statsValid)
	{
		data += "(Sorry, achievement data is apparently invalid.)\n\n";
		return;
	}

	int count;

	count = 0;
	data += "Unlocked:\n\n";
	for (size_t iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch )
	{
		const Achievement &ach = g_rgAchievements[iAch];
		if (!ach.achieved)
			continue;
		count++;
		data += "  ";
		data += ach.name;
		data += ": ";
		data += ach.desc;
		data += "\n";
	}

	if (count == 0)
		data += "  (none!)\n";
	data += "\n";

	count = 0;
	data += "Locked:\n\n";
	for (size_t iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch )
	{
		const Achievement &ach = g_rgAchievements[iAch];
		if (ach.achieved)
			continue;
		count++;
		data += "  ";
		data += ach.name;
		data += ": ";
		data += ach.desc;
		data += "\n";
	}

	if (count == 0)
		data += "  (none!)\n";
	data += "\n";
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate distance traveled
//-----------------------------------------------------------------------------
/*
void StatsAndAchievements::AddDistanceTraveled( float flDistance )
{
	m_flGameFeetTraveled += SpaceWarClient()->PixelsToFeet( flDistance );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Game state has changed
//-----------------------------------------------------------------------------
/*
void StatsAndAchievements::OnGameStateChange( EClientGameState eNewState )
{
	if ( !m_bStatsValid )
		return;

	switch ( eNewState )
	{
	case k_EClientStatsAchievements:
	case k_EClientGameStartServer:
	case k_EClientGameMenu:
	case k_EClientGameQuitMenu:
	case k_EClientGameExiting:
	case k_EClientGameInstructions:
	case k_EClientGameConnecting:
	case k_EClientGameConnectionFailure:
	default:
		break;
	case k_EClientGameActive:
		// Reset per-game stats
		m_flGameFeetTraveled = 0;
		m_ulTickCountGameStart = m_pGameEngine->GetGameTickCount();
		break;
	case k_EClientFindInternetServers:
		break;
	case k_EClientGameWinner:
		if ( SpaceWarClient()->BLocalPlayerWonLastGame() )
			m_nTotalNumWins++;
		else
			m_nTotalNumLosses++;
		// fall through
	case k_EClientGameDraw:

		// Tally games
		m_nTotalGamesPlayed++;

		// Accumulate distances
		m_flTotalFeetTraveled += m_flGameFeetTraveled;

		// New max?
		if ( m_flGameFeetTraveled > m_flMaxFeetTraveled )
			m_flMaxFeetTraveled = m_flGameFeetTraveled;

		// Calc game duration
		m_flGameDurationSeconds = ( m_pGameEngine->GetGameTickCount() - m_ulTickCountGameStart ) / 1000.0;

		// We want to update stats the next frame.
		storeStats = true;

		break;
	}
}
*/


//-----------------------------------------------------------------------------
// Purpose: see if we should unlock this achievement
//-----------------------------------------------------------------------------
void StatsAndAchievements::EvaluateAchievement( Achievement &achievement )
{


	// Already have it?
	if ( achievement.achieved ) {

		return;
	}

	switch ( achievement.achievementID )
	{
	// real evals:
	case ACH_DISCOVER_ALL_RECIPES:
		{
			bool knowAll=true;

			// this code is part of what avoids duplicate recipes being required
			// in the case of veggie soup there are two ways to make it
			// i figure that finding one way is enough for the achievement
			bool didLeafPoultice = false;
			bool didPoisonLoaf = false;
			bool didVeggieSoup = false;

			for (size_t i = 0; i < dsq->continuity.recipes.size(); i++ )
			{
				if (dsq->continuity.recipes[i].isKnown())
				{
					if (dsq->continuity.recipes[i].result == "LeafPoultice") {
						didLeafPoultice = true;
					} else if (dsq->continuity.recipes[i].result == "PoisonLoaf") {
						didPoisonLoaf = true;
					} else if (dsq->continuity.recipes[i].result == "VeggieSoup") {
						didVeggieSoup = true;
					}
				}
			}
			for (size_t i = 0; i < dsq->continuity.recipes.size(); i++ )
			{
				if (!dsq->continuity.recipes[i].isKnown())
				{
					if ((dsq->continuity.recipes[i].result == "LeafPoultice" && didLeafPoultice)
						|| (dsq->continuity.recipes[i].result == "PoisonLoaf" && didPoisonLoaf)
						|| (dsq->continuity.recipes[i].result == "VeggieSoup" && didVeggieSoup))
					{}
					else {

						knowAll = false;
					}
				}
			}

			if (knowAll)
			{
				UnlockAchievement(achievement);
			}
		}
		break;

	case ACH_MASS_TRANSIT:
		{


			if (dsq->continuity.getFlag(FLAG_TRANSTURTLE_VEIL01) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_VEIL02) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_OPENWATER03) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_FOREST04) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_FOREST05) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_MAINAREA) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_SEAHORSE) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_ABYSS03) > 0
				&& dsq->continuity.getFlag(FLAG_TRANSTURTLE_FINALBOSS) > 0)

			{
				UnlockAchievement(achievement);
			}
		}
		break;

	case ACH_EXPLORER:
		{
			// check world map data somehow
			bool hasAllMap = true;
			for (size_t i = 0; i < dsq->continuity.worldMap.getNumWorldMapTiles(); i++)
			{
				WorldMapTile *tile = dsq->continuity.worldMap.getWorldMapTile(i);
				if (!tile->revealed && (nocasecmp(tile->name, "thirteenlair") != 0)) {

					hasAllMap = false;
					break;
				}
			}
			if (hasAllMap)
			{
				UnlockAchievement(achievement);
			}
		}
		break;

	case ACH_HEALTHY:
		// is it really 10??
		if ( dsq->continuity.maxHealth >= 10 )
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_AQUIRE_ALL_SONGS:

		if (dsq->continuity.hasSong(SONG_BIND)
			&& dsq->continuity.hasSong(SONG_SHIELDAURA)
			&& dsq->continuity.hasSong(SONG_LI)
			&& dsq->continuity.hasSong(SONG_ENERGYFORM)
			&& dsq->continuity.hasSong(SONG_BEASTFORM)
			&& dsq->continuity.hasSong(SONG_NATUREFORM)
			&& dsq->continuity.hasSong(SONG_SUNFORM)
			&& dsq->continuity.hasSong(SONG_DUALFORM)
			&& dsq->continuity.hasSong(SONG_FISHFORM)
			&& dsq->continuity.hasSong(SONG_SPIRITFORM))
		{

			UnlockAchievement(achievement);
		}
		break;

	// gameplay
	case ACH_DEFEAT_PRIESTS:

		if (dsq->continuity.hasSong(SONG_SPIRITFORM))
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_DEFEAT_OCTOMUN:
		if (dsq->continuity.getFlag(FLAG_MINIBOSS_OCTOMUN) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_DEFEAT_ROCKCRAB:
		if (dsq->continuity.getFlag(FLAG_MINIBOSS_CRAB) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_DEFEAT_MERGOG:
		if (dsq->continuity.getFlag(FLAG_MINIBOSS_MERGOG) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_DEFEAT_MANTISSHRIMP:
		if (dsq->continuity.getFlag(FLAG_MINIBOSS_MANTISSHRIMP) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_DEFEAT_KINGJELLY:
		if (dsq->continuity.getFlag(FLAG_MINIBOSS_KINGJELLY) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_REACHED_OPEN_WATERS:
		if (game->sceneName == "openwater02")
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_REACHED_SPRITE_CAVE:
		if (game->sceneName == "forestspritecave")
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_REACHED_THE_VEIL:
		// when Naija jumps through the veil
		if (dsq->continuity.getFlag("leftWater")!=0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_ROMANCE:
		if (dsq->continuity.getFlag(FLAG_LI) >= 100)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_BELLY_OF_THE_WHALE:
		if (game->sceneName == "whale")
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_ALEK_AND_DAREC:
		if (game->sceneName == "weirdcave")
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_THE_FROZEN_VEIL:
		if (game->sceneName == "frozenveil")
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_MOMMY_AND_DADDY:


		if (dsq->continuity.getFlag(FLAG_SUNKENCITY_PUZZLE) >= SUNKENCITY_BOSSDONE)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_RESCUED_ALL_SOULS:
		if (dsq->continuity.getFlag(FLAG_SPIRIT_ERULIAN) > 0
			&& dsq->continuity.getFlag(FLAG_SPIRIT_KROTITE) > 0
			&& dsq->continuity.getFlag(FLAG_SPIRIT_DRASK) > 0
			&& dsq->continuity.getFlag(FLAG_SPIRIT_DRUNIAD) > 0)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_BUCKING_BRONCO:
		// ride ekkrit for a minute
		if (rodeEkkritToTheStars)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_COMBO_EATER:
		// eat n=12 things in a row
		if (biteDeathComboNum >= biteDeathComboMax)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_ATE_A_PARROT:
		// eat one parrot
		if (killedParrotWithBite)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_FLUNG_A_MONKEY:
		// monkey gets hit with nature form
		if (flungMonkey)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_KILLED_THE_COWARD:
		// coward dies
		if (killedCoward)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_HIGH_DIVE:
		// fall from bird nest to water
		if (highDiveIsDone)
		{
			UnlockAchievement(achievement);
		}
		break;

	case ACH_SPEED_RACER:
		// get really low time in seahorse race
		int bestTime = dsq->continuity.getFlag(FLAG_SEAHORSEBESTTIME);
		if (bestTime > 0 && bestTime <= seahorseRaceAchievementTimeMin)
		{
			UnlockAchievement(achievement);
		}
		break;
	}
}

void StatsAndAchievements::flingMonkey(Entity *e)
{
	flungMonkey = true;
}

void StatsAndAchievements::entityDied(Entity *eDead)
{
	if (eDead->name == "parrot" && eDead->lastDamage.damageType == DT_AVATAR_BITE) {
		killedParrotWithBite = true;
	}

	if (eDead->name == "coward") {
		killedCoward = true;
	}

	if (eDead->lastDamage.damageType == DT_AVATAR_BITE) {
		// an enemy got bit to death
		// how much time from the last death?
		if (biteDeathComboCounter > biteDeathComboTime) {
			// then we missed out on a combo, suck a nut
			biteDeathComboNum = 0;
		} else {
			// we made it!!
			biteDeathComboNum ++;
		}
		biteDeathComboCounter = 0;
	}



}

void StatsAndAchievements::update(float dt)
{
	//debugLog("update stats and achievements");
	Avatar *avatar = 0;
	if (game && game->avatar) {
		avatar = game->avatar;
	}

	if (avatar) {
		BoneLock *b = avatar->getBoneLock();

		if (!rodeEkkritToTheStars) {
			if (!game->isPaused() && b->on) {

				if (b->entity->name == "ekkrit") {
					ridingEkkritTime += dt;



					if (ridingEkkritTime >= ridingEkkritTimeMax) {
						rodeEkkritToTheStars = true;
					}
				}
			}
		}

		// TODO: verify sceneName is correct
		if (game->sceneName == "veil02") {
			if (avatar->position.x > 18056 && avatar->position.x < 21238 && avatar->position.y < 4185) {
				debugLog("highDiveIsHigh!");
				highDiveIsHigh = true;
			} else if (highDiveIsHigh && avatar->isUnderWater()) {
				// fell into water from right place, must have been the right jump (lol, we hope)
				highDiveIsDone = true;
				debugLog("highDiveIsDone!");
			}
		} else {
			highDiveIsHigh = false;
			highDiveIsDone = false;
		}
	}

#ifndef BBGE_BUILD_STEAMWORKS
	// change no state if we're still fading in/out.
	if (!dsq->achievement_box->alpha.isInterpolating())
	{
		const float maxUnlockDisplayTime = 5.0f;
		// still displaying an unlock notification?
		if (unlockedDisplayTimestamp > 0.0f)
		{
			unlockedDisplayTimestamp -= dt;
			if (unlockedDisplayTimestamp <= 0.0f)
			{
				unlockedDisplayTimestamp = -1.0f;
				dsq->achievement_text->alpha.interpolateTo(0, 1);
				dsq->achievement_box->alpha.interpolateTo(0, 1.2f);
			}
		}

		// more achievements to display?
		else if (unlockedToBeDisplayed.size() > 0)
		{
			const std::string &name = unlockedToBeDisplayed.front();
			unlockedDisplayTimestamp = maxUnlockDisplayTime;
			std::string text("Achievement Unlocked:\n");
			text += name;
			unlockedToBeDisplayed.pop_front();
			dsq->achievement_text->setText(text);
			dsq->achievement_text->alpha.interpolateTo(1, 1);
			dsq->achievement_box->alpha.interpolateTo(1, 0.1f);
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Unlock this achievement
//-----------------------------------------------------------------------------
void StatsAndAchievements::UnlockAchievement( Achievement &achievement )
{
	if ((achievement.achieved) || (!statsValid))
		return;

	achievement.achieved = true;

	// the icon may change once it's unlocked
	achievement.iconImage = 0;

#ifndef BBGE_BUILD_STEAMWORKS
	unlockedToBeDisplayed.push_back(achievement.name);
#endif

	// Store stats end of frame
	storeStats = true;
}

//-----------------------------------------------------------------------------
// Purpose: Store stats in the Steam database
//-----------------------------------------------------------------------------
void StatsAndAchievements::StoreStatsIfNecessary()
{
	if ( storeStats )
	{
		// already set any achievements in UnlockAchievement

#ifndef BBGE_BUILD_STEAMWORKS
		storeStats = false;  // only ever try once.

		// FIXME: We should use a temporary file to ensure that data
		// isn't lost if the filesystem gets full.  The canonical
		// method is to write to a new file, then call
		// rename("new.file", "existing.file") after the new file has
		// been successfully written; POSIX specifies that such a call
		// atomically replaces "existing.file" with "new.file".
		// However, I've heard that Windows doesn't allow this sort of
		// file replacement.  Will this work on Windows and MacOS?
		// Please advise.  --achurch

		const std::string fname(core->getUserDataFolder() + "/achievements.bin");
		FILE *io = fopen(fname.c_str(), "wb");
		if (io == NULL)
			return;

		const size_t max_achievements = ARRAYSIZE(g_rgAchievements);
		unsigned char *buf = new unsigned char[max_achievements];

		for (size_t i = 0; i < max_achievements; i++)
		{
			int val = g_rgAchievements[i].achieved ? 1 : 0;
			buf[i] = ((unsigned char) (val + ((int)i))) ^ 0xFF;
		}

		if (fwrite(buf, sizeof (buf[0]), max_achievements, io) != max_achievements)
			debugLog("Failed to write achievements 1");
		delete[] buf;

		char cruft[101];
		for (size_t i = 0; i < sizeof (cruft); i++)
			cruft[i] = (char) rand();
		if (fwrite(cruft, sizeof (cruft[0]), ARRAYSIZE(cruft), io) != ARRAYSIZE(cruft))
			debugLog("Failed to write achievements 2");

		fclose(io);
#endif
	}
}

#ifdef BBGE_BUILD_STEAMWORKS
#error Someone with access to the Steamworks SDK actually has to implement this!
#endif
