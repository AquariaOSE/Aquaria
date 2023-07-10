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
#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include "Base.h"
#include "EngineEnums.h"
#include "Texture.h"
#include "ScriptObject.h"
#include "RenderState.h"
#include <list>

class Core;
class StateData;
class Texture;

enum RenderObjectFlags
{
	RO_CLEAR			= 0x00,
	RO_RENDERBORDERS	= 0x01,
	RO_NEXT				= 0x02,
	RO_MOTIONBLUR		= 0x04
};

enum AutoSize
{
	AUTO_VIRTUALWIDTH		= -101,
	AUTO_VIRTUALHEIGHT		= -102
};

enum ParentManaged
{
	PM_NONE					= 0, // child is destroyed with parent, but not deleted. The childs' update() is NOT called.
	PM_POINTER				= 1, // child is deleted together with parent. update() is called.
	PM_STATIC				= 2  // child is destroyed with parent, but not deleted. update() is called.
};

enum ChildOrder
{
	CHILD_BACK				= 0,
	CHILD_FRONT				= 1
};

enum RenderBeforeParent
{
	RBP_NONE				= -1,
	RBP_OFF					= 0,
	RBP_ON					= 1
};

struct MotionBlurFrame
{
	Vector position;
	float rotz;
};

struct MotionBlurData
{
	MotionBlurData();
	bool transition;
	unsigned frameOffsetCounter, frameOffset;
	float transitionTimer;
	std::vector<MotionBlurFrame> positions;
};

class RenderObjectLayer;

class RenderObject : public ScriptObject
{
public:
	friend class Core;
	RenderObject();
	virtual ~RenderObject();
	virtual void render(const RenderState& rs) const;

	void setTexturePointer(CountedPtr<Texture> t)
	{
		this->texture = t;
		onSetTexture();
	}

	void setStateDataObject(StateData *state);
	bool setTexture(const std::string &name);

	void toggleAlpha(float t = 0.2f);

	virtual void update(float dt);
	bool isDead() const {return _dead;}
	bool isHidden() const {return _hidden || (parent && parent->isHidden());}

	bool shouldTryToRender() const; // somewhat expensive
	bool isVisibleInPass(int pass) const;

	// Set whether the object is hidden.  If hidden, no updates (except
	// lifetime checks) or render operations will be performed, and no
	// child objects will be updated or rendered.
	void setHidden(bool hidden) {_hidden = hidden;}

	void setLife(float newlife)
	{
		maxLife = this->life = newlife;
	}
	void setDecayRate(float newdecayRate)
	{
		this->decayRate = newdecayRate;
	}
	void setBlendType (BlendType bt)
	{
		_blendType = bt;
	}
	inline BlendType getBlendType() const
	{
		return (BlendType)_blendType;
	}


	virtual void destroy();

	virtual void flipHorizontal();
	virtual void flipVertical();

	bool isfh() const { return _fh; }
	bool isfv() const { return _fv; }

	// recursive
	bool isfhr() const;
	bool isfvr() const;

	size_t getIdx() const { return idx; }
	void setIdx(size_t newidx) { this->idx = newidx; }
	void moveToFront();
	void moveToBack();

	inline float getCullRadiusSqr() const
	{
		if (overrideCullRadiusSqr != 0)
			return overrideCullRadiusSqr;
		if (width == 0 || height == 0)
			return 0;
		const float w = width*scale.x;
		const float h = height*scale.y;
		return w*w + h*h;
	}

	int getTopLayer() const;

	void enableMotionBlur(int sz=10, int off=5);
	void disableMotionBlur();

	void addChild(RenderObject *r, ParentManaged pm, RenderBeforeParent rbp = RBP_NONE, ChildOrder order = CHILD_BACK);
	void removeChild(RenderObject *r);

	Vector getRealPosition() const;
	Vector getRealScale() const;

	StateData *getStateData() const;

	// HACK: This is defined in RenderObject_inline.h because it needs
	// the class Core definition.  --achurch
	inline bool isOnScreen() const;

	bool isCoordinateInRadius(const Vector &pos, float r) const;

	void toggleCull(bool value);

	void safeKill();

	Vector getWorldPosition() const;
	Vector getWorldCollidePosition(const Vector &vec=Vector(0,0,0)) const;

	RenderObject *getTopParent() const;

	virtual void onAnimationKeyPassed(int key){}

	Vector getAbsoluteRotation() const;
	float getWorldRotation() const;
	Vector getWorldPositionAndRotation() const; // more efficient shortcut, returns rotation in vector z component
	Vector getNormal() const;
	Vector getForward() const;
	void setOverrideCullRadius(float ovr);
	void setRenderPass(int pass) { renderPass = pass; }
	int getRenderPass() const { return renderPass; }

	// TODO: remove this once the render loop is split into a per-pass object collection phase
	// and an actual rendering phase
	enum { RENDER_ALL=999 };

	// Defined in RenderObject_inline.h
	inline Vector getFollowCameraPosition() const;
	inline Vector getFollowCameraPosition(const Vector& pos) const;

	void lookAt(const Vector &pos, float t, float minAngle, float maxAngle, float offset=0);
	inline RenderObject *getParent() const {return parent;}
	void fhTo(bool fh);
	void addDeathNotify(RenderObject *r);
	virtual void unloadDevice();
	virtual void reloadDevice();

	MotionBlurData *ensureMotionBlur();
	void freeMotionBlur();

	//-------------------------------- Methods above, fields below

	static bool renderCollisionShape;
	static bool renderPaths;
	static size_t lastTextureApplied;
	static bool lastTextureRepeat;

	//--------------------------

	// fields ordered by hotness

	// TODO: this should be a bitmask
	bool fadeAlphaWithLife;
	bool renderBeforeParent;
	bool shareAlphaWithChildren;
	bool shareColorWithChildren;
	bool cull;
	bool ignoreUpdate;
	bool useOldDT;
	bool repeatTexture;
	bool _dead;
	bool _hidden;
	bool _fv, _fh;
	bool _markedForDelete;

	unsigned char pm;  // unsigned char to save space

	char _blendType;


	InterpolatedVector position;
	InterpolatedVector scale;
	InterpolatedVector color, alpha;
	InterpolatedVector rotation;
	InterpolatedVector offset, rotationOffset, internalOffset, beforeScaleOffset;
	InterpolatedVector velocity, gravity;

	CountedPtr<Texture> texture;

	float life;

	// if 0: use value from RenderLayer. If still 0, render normally.
	// UI elements have this == 1, ie. are totally unaffected by camera movement
	// and stay always on the same place on the screen.
	// Any value > 0 and < 1 is parallax scrolling, where closer to 0 moves slower
	// (looks like further away).
	float followCamera;

	float alphaMod;
	float updateCull;
	int layer;

	float decayRate;
	float maxLife;

	// In layers that have multi-pass rendering enabled, the object will only be rendered
	// in this pass (single-pass layers always render, regardless of this setting).
	int renderPass;


	float overrideCullRadiusSqr;


	float width, height;  // Only used by Quads, but stored here for getCullRadius()

	// ----------------------

	typedef std::vector<RenderObject*> Children;
	Children children;

protected:
	RenderObject *parent;

	virtual void onFH(){}
	virtual void onFV(){}
	virtual void onSetTexture(){}
	virtual void onRender(const RenderState& rs) const {}
	virtual void onUpdate(float dt);
	virtual void deathNotify(RenderObject *r);
	virtual void onEndOfLife() {}

	bool updateLife(float dt);

	// Is this object or any of its children rendered in pass "pass"?
	bool hasRenderPass(const int pass) const;

	inline void renderCall(const RenderState& rs, const Vector& renderAt, float renderRotation) const;
	virtual void renderCollision(const RenderState& rs) const;
	void debugRenderPaths() const;

	typedef std::list<RenderObject*> RenderObjectList;
	RenderObjectList deathNotifications;

	size_t idx; // index in layer
	StateData *stateData;
	MotionBlurData *motionBlur;

private:
	const RenderObject &operator=(const RenderObject &r); // undefined
};

#endif
