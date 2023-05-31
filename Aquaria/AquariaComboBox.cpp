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
#include "AquariaMenuItem.h"
#include "DSQ.h"

#define AQUARIACOMBOBOXITEM_DOWN	-2
#define	AQUARIACOMBOBOXITEM_UP		-3

#define SCROLL_DELAY				0.1f
#define SCROLL_DELAY_FIRST			0.4f

AquariaComboBox::AquariaComboBox(Vector textscale) : RenderObject()
{

	bar = new Quad("gui/combo-drop", Vector(0,0));
	addChild(bar, PM_POINTER);

	scrollBtnUp = new Quad("gui/combo-button-up", Vector(70,0));
	scrollBtnUp->alpha = 0;
	scrollBtnUp->flipVertical();
	addChild(scrollBtnUp, PM_POINTER);

	scrollBtnDown = new Quad("gui/combo-button-up", Vector(88,0));
	scrollBtnDown->alpha = 0;
	addChild(scrollBtnDown, PM_POINTER);

	selectedItemLabel = new BitmapText(dsq->smallFont);
	selectedItemLabel->setAlign(ALIGN_LEFT);
	selectedItemLabel->setFontSize(8);
	selectedItemLabel->offset.y = -10;
	selectedItemLabel->position.x = -50;
	selectedItemLabel->scale = textscale;
	addChild(selectedItemLabel, PM_POINTER);

	numDrops = 8;

	mb = false;
	isopen = false;

	setSelectedItem(0);

	scroll = 0;

	enqueuedSelectItem = -1;

	scrollDelay = 0;
	firstScroll = 0;

	this->textscale = textscale;
}

void AquariaComboBox::enqueueSelectItem(size_t index)
{
	enqueuedSelectItem = index;
}

void AquariaComboBox::setScroll(size_t sc)
{
	scroll = sc;
}

std::string AquariaComboBox::getSelectedItemString()
{
	if (selectedItem < items.size())
		return items[selectedItem];
	return "";
}

void AquariaComboBox::doScroll(int t)
{
	if (t == 0)
	{
		if(scroll > 0) {
			scroll--;
			close(0);
			open(0);
		}
	}
	else
	{
		scroll++;
		if (scroll+numDrops > items.size()) {
			scroll = items.size() - numDrops;
		}
		else
		{
			close(0);
			open(0);
		}
	}
}

void AquariaComboBox::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (alpha.x < 1)
		return;



	if (enqueuedSelectItem != -1)
	{
		setSelectedItem(enqueuedSelectItem);
		enqueuedSelectItem = -1;
	}

	scrollDelay -= dt;
	if (scrollDelay < 0) scrollDelay = 0;

	bool clickedInside = false;

	if (isopen)
	{
		if (!core->mouse.buttons.left)
		{
			firstScroll = 1;
			scrollDelay = 0;
		}

		if (core->mouse.buttons.left && scrollBtnDown->isCoordinateInsideWorldRect(core->mouse.position, 20, 32))
		{
			clickedInside = true;
			if (scrollDelay == 0)
			{
				doScroll(1);

				if (firstScroll)
				{
					scrollDelay = SCROLL_DELAY_FIRST;
					firstScroll = 0;
				}
				else
				{
					scrollDelay = SCROLL_DELAY;
				}
			}
			scrollBtnDown->setTexture("gui/combo-button-down");
		}
		else
		{
			scrollBtnDown->setTexture("gui/combo-button-up");
		}

		if (core->mouse.buttons.left && scrollBtnUp->isCoordinateInsideWorldRect(core->mouse.position, 20, 32))
		{
			clickedInside = true;
			if (scrollDelay == 0)
			{
				doScroll(0);

				if (firstScroll)
				{
					scrollDelay = SCROLL_DELAY_FIRST;
					firstScroll = 0;
				}
				else
				{
					scrollDelay = SCROLL_DELAY;
				}
			}
			scrollBtnUp->setTexture("gui/combo-button-down");
		}
		else
		{
			scrollBtnUp->setTexture("gui/combo-button-up");
		}
	}



	if (bar->isCoordinateInsideWorld(core->mouse.position))
	{
		clickedInside = true;
		if (!mb && core->mouse.buttons.left)
		{
			mb = true;
		}
		else if (mb && !core->mouse.buttons.left)
		{
			mb = false;

			core->sound->playSfx("click");

			if (isopen)
				close();
			else
				open();
		}
	}
	else
	{
		mb = false;
	}

	if (isopen)
	{
		if (core->mouse.scrollWheelChange > 0)
		{
			doScroll(0);
		}
		else if (core->mouse.scrollWheelChange < 0)
		{
			doScroll(1);
		}

		if(!clickedInside && core->mouse.buttons.left)
		{
			for(size_t i = 0; i < shownItems.size(); ++i)
				if(shownItems[i]->mb)
				{
					clickedInside = true;
					break;
				}
			if(!clickedInside)
				close();
		}
	}

}

void AquariaComboBox::open(float t)
{
	shownItems.clear();

	for (size_t i = scroll; i < scroll + numDrops; i++)
	{
		if (i < items.size())
		{
			AquariaComboBoxItem *a = new AquariaComboBoxItem(items[i], i, this, textscale);
			a->alpha = 0;
			a->alpha.interpolateTo(1, t);
			a->position.y = (a->getHeight()+2) * ((i-scroll)+1);
			addChild(a, PM_POINTER);
			shownItems.push_back(a);
		}
	}

	scrollBtnDown->alpha.interpolateTo(1, t);
	scrollBtnUp->alpha.interpolateTo(1, t);

	isopen = true;
}

void AquariaComboBox::close(float t)
{
	if (!isopen) return;

	isopen = false;

	for (size_t i = 0; i < shownItems.size(); i++)
	{
		shownItems[i]->alpha.interpolateTo(0, t);
	}

	if (t>0)
		dsq->run(t, true);

	for(size_t i = 0; i < shownItems.size(); i++)
	{
		removeChild(shownItems[i]);
		shownItems[i]->destroy();
		delete shownItems[i];
	}

	scrollBtnDown->alpha.interpolateTo(0, t);
	scrollBtnUp->alpha.interpolateTo(0, t);

	shownItems.clear();
}

bool AquariaComboBox::setSelectedItem(const std::string &item)
{
	for (size_t i = 0; i < items.size(); i++)
	{
		if (items[i] == item)
		{
			setSelectedItem(i);
			return true;
		}
	}
	return false;
}

void AquariaComboBox::setSelectedItem(size_t index)
{
	if (isopen)
		close();

	if (index == AQUARIACOMBOBOXITEM_UP)
	{
		doScroll(1);
	}
	else if (index == AQUARIACOMBOBOXITEM_DOWN)
	{
		doScroll(0);
	}
	else if(index < items.size())
	{
		selectedItem = index;
		selectedItemLabel->setText(items[index]);
		scroll = index;
		if (scroll + numDrops > items.size())
		{
			if (items.size() < numDrops)
				scroll = 0;
			else
				scroll = items.size() - numDrops;
		}
}
}

size_t AquariaComboBox::getSelectedItem()
{
	return selectedItem;
}

size_t AquariaComboBox::addItem(const std::string &n)
{
	items.push_back(n);

	if (items.size() == 1)
	{
		setSelectedItem(0);
	}

	return items.size()-1;
}

Vector unselectedColor(0.7f, 0.7f, 0.7f);
Vector selectedColor(1,1,1);

AquariaComboBoxItem::AquariaComboBoxItem(const std::string &str, size_t idx, AquariaComboBox *combo, Vector textscale) : Quad()
{
	this->combo = combo;
	index = idx;

	setTexture("gui/combo-drop");

	label = new BitmapText(dsq->smallFont);
	label->setAlign(ALIGN_LEFT);
	label->setFontSize(8);
	label->setText(str);
	label->offset.y = -10;
	label->position.x = -50;
	label->scale = textscale;
	addChild(label, PM_POINTER);

	color = unselectedColor;
	label->color = unselectedColor;

	shareAlphaWithChildren = 1;

	mb = false;
}

void AquariaComboBoxItem::onUpdate(float dt)
{
	Quad::onUpdate(dt);

	if (label)
	{
		label->alpha.x = alpha.x;
	}

	if (this->isCoordinateInsideWorld(core->mouse.position))
	{
		color = selectedColor;
		label->color = selectedColor;

		if (!mb && core->mouse.buttons.left)
		{
			mb = true;
		}
		else if (mb && !core->mouse.buttons.left)
		{
			mb = false;

			core->sound->playSfx("click");

			if (combo)
				combo->enqueueSelectItem(index);
		}
	}
	else
	{
		color = unselectedColor;
		label->color = unselectedColor;

		mb = false;

	}
}
