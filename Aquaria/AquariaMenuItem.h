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
#pragma once

#include "../BBGE/BitmapFont.h"
#include "../BBGE/Quad.h"
#include "../BBGE/ActionMapper.h"
#include "../ExternalLibs/tinyxml.h"
#include "../BBGE/Slider.h"
#include "../BBGE/DebugFont.h"
#include "../BBGE/TTFFont.h"
#include "../BBGE/RoundedRect.h"

class AquariaGuiElement
{
public:
	AquariaGuiElement();
	void clean();
	void setDirMove(int dir, AquariaGuiElement *item);
	void setCanDirMove(bool on);
	virtual void setFocus(bool v);
	virtual Vector getGuiPosition()=0;
	virtual bool isGuiVisible()=0;
	static bool canDirMoveGlobal;
	int guiInputLevel;
	static int currentGuiInputLevel;
	bool hasInput();
protected:
	typedef std::list<AquariaGuiElement*> GuiElements;
	static GuiElements guiElements;
	static float guiMoveTimer;
	void updateMovement(float dt);
	bool hasFocus, canDirMove;
	AquariaGuiElement *dirMove[DIR_MAX];
};

class AquariaGuiQuad : public Quad, public AquariaGuiElement
{
public:
	AquariaGuiQuad();
	void destroy();
	Vector getGuiPosition();
	bool isGuiVisible();
	void update(float dt);

protected:
	void onUpdate(float dt);
};

class AquariaMenuItem : virtual public Quad, public ActionMapper, public AquariaGuiElement
{
public:
	AquariaMenuItem();
	void destroy();
	void setLabel(const std::string &label);
	EventPtr event;
	BitmapText *font, *glowFont;
	TiXmlElement *ability, *xmlItem;
	int choice;
	Quad *glow, *quad;
	void useQuad(const std::string &tex);
	void useGlow(const std::string &tex, int w, int h);
	void useSound(const std::string &tex);
	
	bool isCursorInMenuItem();
	Vector getGuiPosition();
	bool isGuiVisible();
	int shareAlpha;
protected:

	std::string useSfx;
	bool highlighted;
	void toggleHighlight(bool v);
	void onClick();
	void onUpdate(float dt);
};

class AquariaSaveSlot : public AquariaGuiQuad
{
public:
	AquariaSaveSlot(int slot);
	bool isEmpty() { return empty; }
	int getSlotIndex() { return slotIndex; }
	void close(bool trans);
	void hide();
	void transition();

	bool isGuiVisible();

	bool mbDown;

	static std::string getSaveDescription(const TiXmlDocument &doc);

protected:
	void onUpdate(float dt);
	bool selected;
	static bool closed;
	bool done;
	
	int slotIndex;
	bool empty;
	Quad *gfx;
	Quad *box;
	Quad *screen;
	BitmapText *text1, *glowText;
};

class AquariaSlider : public Slider, public AquariaGuiElement
{
public:
	AquariaSlider();
	void destroy();

	Vector getGuiPosition();
	bool isGuiVisible();
	bool hadInput() {bool ret = _hadInput; _hadInput = false; return ret;}
protected:
	void onUpdate(float dt);
	bool doSliderInput(float dt);  // Returns whether input was detected

	float inputTimer, inputDelay;
	bool _hadInput;
};

class AquariaCheckBox : public CheckBox, public AquariaGuiElement
{
public:
	AquariaCheckBox();
	void destroy();

	Vector getGuiPosition();
	bool isGuiVisible();
protected:
	void onUpdate(float dt);
};

class AquariaKeyConfig : public AquariaGuiElement, public RenderObject
{
public:
	AquariaKeyConfig(const std::string &actionInputName, InputSetType type, int idx);
	void destroy();

	Vector getGuiPosition();
	bool isGuiVisible();

	static AquariaKeyConfig *waitingForInput;
	
	void setLock(int lock);
	
protected:
	int locked;
	void toggleEnterKey(int on);

	void onUpdate(float dt);

	bool keyDown;

	
	std::string actionInputName;
	InputSetType inputSetType;
	int inputIdx;

	//BitmapText *label;
	//DebugFont *keyConfigFont;
	TTFText *keyConfigFont;
	Quad *bg;
};

class AquariaComboBox;

class AquariaComboBoxItem : public Quad
{
public:
	AquariaComboBoxItem(const std::string &str, int idx, AquariaComboBox *combo);

protected:
	void onUpdate(float dt);

	int index;
	AquariaComboBox *combo;

	BitmapText *label;
	bool mb;
};

class AquariaComboBox : public RenderObject
{
public:
	AquariaComboBox();

	void destroy();

	int addItem(const std::string &n);
	void open(float t=0.1);
	void close(float t=0.1);
	void setSelectedItem(int index);
	bool setSelectedItem(const std::string &item);
	int getSelectedItem();
	void enqueueSelectItem(int index);
	void setScroll(int sc);
	std::string getSelectedItemString();
	void doScroll(int dir);
protected:
	void onUpdate(float dt);

	int numDrops;
	bool mb, isopen;

	int scroll;
	int enqueuedSelectItem;

	std::vector<std::string> items;
	std::vector<BitmapText*> itemTexts;

	Quad *bar, *window, *scrollBtnUp, *scrollBtnDown, *scrollBar;

	BitmapText *selectedItemLabel;
	int selectedItem;
	float scrollDelay;
	bool firstScroll;

	std::vector<AquariaComboBoxItem*> shownItems;
};

/*
class SelectionList : public RenderObject
{
public:
	SelectionList(std::string file, std::string font, int items);
	void reload();

	virtual void onSelect(int idx){}
protected:
	void load();
	std::vector<std::string> list;
};
*/

