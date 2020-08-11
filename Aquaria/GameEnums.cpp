#include "GameEnums.h"

// Action names (as seen in usersettings.xml) to action IDs and menu layout
const ActionDef GameActionDefs[] =
{
	// page 1
	{ "PrimaryAction", ACTION_PRIMARY, 2107, 0},
	{ "SecondaryAction", ACTION_SECONDARY, 2108, 0},

	{ "SwimUp",		ACTION_SWIMUP, 2109, 0},
	{ "SwimDown",		ACTION_SWIMDOWN, 2110, 0},
	{ "SwimLeft",		ACTION_SWIMLEFT, 2111, 0},
	{ "SwimRight",		ACTION_SWIMRIGHT, 2112, 0},

	{ "Roll",			ACTION_ROLL, 2113, 0},
	{ "Revert",		ACTION_REVERT, 2114, 0},
	{ "WorldMap",		ACTION_TOGGLEWORLDMAP, 2115, 0},
	{ "Look",			ACTION_LOOK, 2127, 0},

	// page 2
	{ "Escape",		ACTION_ESC, 2116, 1},
	{ "ToggleHelp",	ACTION_TOGGLEHELPSCREEN, 2128, 1},

	{ "PrevPage",		ACTION_PREVPAGE, 2121, 1},
	{ "NextPage",		ACTION_NEXTPAGE, 2122, 1},
	{ "CookFood",		ACTION_COOKFOOD, 2123, 1},
	{ "FoodLeft",		ACTION_FOODLEFT, 2124, 1},
	{ "FoodRight",		ACTION_FOODRIGHT, 2125, 1},
	{ "FoodDrop",		ACTION_FOODDROP, 2126, 1},

	{ "Screenshot",	ACTION_SCREENSHOT, 2132, 1},

	// page 3
	{ "SongSlot1",		ACTION_SONGSLOT1, 2129, 2},
	{ "SongSlot2",		ACTION_SONGSLOT2, 2129, 2},
	{ "SongSlot3",		ACTION_SONGSLOT3, 2129, 2},
	{ "SongSlot4",		ACTION_SONGSLOT4, 2129, 2},
	{ "SongSlot5",		ACTION_SONGSLOT5, 2129, 2},
	{ "SongSlot6",		ACTION_SONGSLOT6, 2129, 2},
	{ "SongSlot7",		ACTION_SONGSLOT7, 2129, 2},
	{ "SongSlot8",		ACTION_SONGSLOT8, 2129, 2},
	{ "SongSlot9",		ACTION_SONGSLOT9, 2129, 2},
	{ "SongSlot10",		ACTION_SONGSLOT10, 2129, 2},

	// terminator
	{ 0, 0, 0, 0 }
};
