#include "Panel.h"

Panel::Panel()
{
	cull = false;
}

bool Panel::canUpdate() const
{
	return alpha.x >= 1;
}

bool Panel::canReceiveInput() const
{
	return alpha.x >= 1;
}

void Panel::onUpdate(float dt)
{
	if (canUpdate())
		RenderObject::onUpdate(dt);
	else
		alpha.update(dt);
}
