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
#ifndef AQUARIAMENUITEM_H
#define AQUARIAMENUITEM_H

#include "../BBGE/BitmapFont.h"
#include "../BBGE/Quad.h"
#include "../BBGE/ActionMapper.h"
#include "../BBGE/Slider.h"
#include "../BBGE/DebugFont.h"
#include "../BBGE/TTFFont.h"
#include "../BBGE/RoundedRect.h"

namespace tinyxml2 { class XMLDocument; }

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
	static AquariaGuiElement *currentFocus;
	static AquariaGuiElement *getClosestGuiElement(const Vector& pos);
protected:
	typedef std::list<AquariaGuiElement*> GuiElements;
	static GuiElements guiElements;
	static float guiMoveTimer;
	void updateMovement(float dt);
	bool hasFocus() const;
	bool canDirMove;
	AquariaGuiElement *dirMove[DIR_MAX];
	static AquariaGuiElement *FindClosestTo(AquariaGuiElement *cur, Vector pos, Direction dir);
	static Direction GetDirection();
	static AquariaGuiElement *FocusClosestToMouse(Direction dir);
public:
	static void UpdateGlobalFocus(float dt);
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
	Quad *glow, *quad;
	bool useQuad(const std::string &tex);
	void useGlow(const std::string &tex, int w, int h);
	void useSound(const std::string &tex);
	virtual void action(int actionID, int state, int source, InputDevice device) {}

	virtual bool isCursorInMenuItem();
	Vector getGuiPosition();
	bool isGuiVisible();
	int shareAlpha;

protected:

	std::string useSfx;
	bool highlighted;
	void toggleHighlight(bool v);
	void onClick();
	virtual void onUpdate(float dt);
	virtual void onToggleHighlight(bool on) {}
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

	static std::string getSaveDescription(const tinyxml2::XMLDocument &doc);

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

	void setActionSetIndex(size_t idx);
	void setRejectJoyAxis(bool b);

protected:
	void toggleEnterKey(int on);

	void onUpdate(float dt);

	bool keyDown;


	std::string actionInputName;
	InputSetType inputSetType;
	int inputIdx;
	TTFText *keyConfigFont;
	Quad *bg, *bg2;
	size_t actionSetIndex;
	bool rejectJoyAxis;
};

class AquariaComboBox;

class AquariaComboBoxItem : public Quad
{
	friend class AquariaComboBox;
public:
	AquariaComboBoxItem(const std::string &str, size_t idx, AquariaComboBox *combo, Vector textscale);

protected:
	void onUpdate(float dt);

	size_t index;
	AquariaComboBox *combo;

	BitmapText *label;
	bool mb;
};

class AquariaComboBox : public RenderObject
{
public:
	AquariaComboBox(Vector textscale = Vector(1, 1));
	size_t addItem(const std::string &n);
	void open(float t=0.1f);
	void close(float t=0.1f);
	void setSelectedItem(size_t index);
	bool setSelectedItem(const std::string &item);
	size_t getSelectedItem();
	void enqueueSelectItem(size_t index);
	void setScroll(size_t sc);
	std::string getSelectedItemString();
	void doScroll(int dir);
	bool isOpen() const { return isopen; }
protected:
	void onUpdate(float dt);

	size_t numDrops;
	bool mb, isopen;

	size_t scroll;
	size_t enqueuedSelectItem;

	std::vector<std::string> items;

	Quad *bar, *scrollBtnUp, *scrollBtnDown;

	BitmapText *selectedItemLabel;
	size_t selectedItem;
	float scrollDelay;
	bool firstScroll;
	Vector textscale;

	std::vector<AquariaComboBoxItem*> shownItems;
};

class AquariaButton : public AquariaMenuItem
{
public:
	AquariaButton(const std::string texbase, TTFFont *font);

	void setButtonLabel(const std::string& s);
	void goDown();
	void goUp();

	Vector activeColor;
	float activeAlpha;
	Vector inactiveColor;
	float inactiveAlpha;

	TTFText * const buttonlabel;

protected:

	virtual void action(int actionID, int state, int source, InputDevice device);
	virtual void onUpdate(float dt);
	virtual void onToggleHighlight(bool on);

	std::string _texbase;
	int pressed;
	int lastpressed;
};


#endif
