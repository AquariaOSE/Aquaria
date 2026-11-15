#ifndef BBGE_PANEL_H
#define BBGE_PANEL_H

#include "RenderObject.h"

// Invisible UI container. Hide it by setting its alpha to 0: then neither it nor its children are rendered.
// While canUpdate() is false the children are not updated (only the panel's own alpha keeps fading),
// and while canReceiveInput() is false GUI elements inside it don't react to input.
class Panel : public RenderObject
{
public:
	Panel();

	void setVisible(bool on) { alpha = on ? 1.0f : 0.0f; }
	bool isVisible() const { return alpha.x > 0; }

	virtual bool canUpdate() const;
	virtual bool canReceiveInput() const;

protected:
	void onUpdate(float dt) OVERRIDE;
	bool childrenCanReceiveInput() const OVERRIDE { return canReceiveInput(); }
};

#endif
