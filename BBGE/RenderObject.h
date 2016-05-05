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
#ifndef __render_object__
#define __render_object__

#include "Base.h"
#include "Texture.h"
#include "Flags.h"
#include "ScriptObject.h"

class Core;
class StateData;

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
	PM_NONE					= 0,
	PM_POINTER				= 1,
	PM_STATIC				= 2
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

typedef std::vector<RectShape> CollideRects;

class RenderObjectLayer;

class RenderObject : public ScriptObject
{
public:
	friend class Core;
	RenderObject();
	virtual ~RenderObject();
	virtual void render();

	static RenderObjectLayer *rlayer;

	void setTexturePointer(CountedPtr<Texture> t)
	{
		this->texture = t;
		onSetTexture();
	}

	void setStateDataObject(StateData *state);
	bool setTexture(const std::string &name);

	void toggleAlpha(float t = 0.2);

	virtual void update(float dt);
	bool isDead() const {return _dead;}
	bool isHidden() const {return _hidden || (parent && parent->isHidden());}
	bool isStatic() const {return _static;}

	// Set whether the object is hidden.  If hidden, no updates (except
	// lifetime checks) or render operations will be performed, and no
	// child objects will be updated or rendered.
	void setHidden(bool hidden) {_hidden = hidden;}

	// Set whether the object is static.  If static, the object's data
	// (including position, scale, rotation, color, etc.) are assumed
	// not to change over the lifetime of the object, to allow for
	// optimized rendering.
	void setStatic(bool staticFlag) {_static = staticFlag;}

	void setLife(float life)
	{
		maxLife = this->life = life;
	}
	void setDecayRate(float decayRate)
	{
		this->decayRate = decayRate;
	}
	void setBlendType (int bt)
	{
		blendType = bt;
	}


	virtual void destroy();

	virtual void flipHorizontal();
	virtual void flipVertical();

	bool isfh() const { return _fh; }
	bool isfv() const { return _fv; }

	// recursive
	bool isfhr();
	bool isfvr();

	int getIdx() const { return idx; }
	void setIdx(int idx) { this->idx = idx; }
	void moveToFront();
	void moveToBack();

	inline float getCullRadiusSqr() const
	{
		if (overrideCullRadiusSqr)
			return overrideCullRadiusSqr;
		if (width == 0 || height == 0)
			return 0;
		const float w = width*scale.x;
		const float h = height*scale.y;
		return w*w + h*h;
	}

	int getTopLayer();

	void setColorMult(const Vector &color, const float alpha);
	void clearColorMult();

	void enableMotionBlur(int sz=10, int off=5);
	void disableMotionBlur();

	void addChild(RenderObject *r, ParentManaged pm, RenderBeforeParent rbp = RBP_NONE, ChildOrder order = CHILD_BACK);
	void removeChild(RenderObject *r);

	Vector getRealPosition();
	Vector getRealScale();

	virtual float getSortDepth();

	StateData *getStateData();

	void setPositionSnapTo(InterpolatedVector *positionSnapTo);

	// HACK: This is defined in RenderObject_inline.h because it needs
	// the class Core definition.  --achurch
	inline bool isOnScreen();

	bool isCoordinateInRadius(const Vector &pos, float r);

	void copyProperties(RenderObject *target);

	const RenderObject &operator=(const RenderObject &r);

	void toggleCull(bool value);

	void safeKill();

	void enqueueChildDeletion(RenderObject *r);

	Vector getWorldPosition();
	Vector getWorldCollidePosition(const Vector &vec=Vector(0,0,0));
	Vector getInvRotPosition(const Vector &vec);
	bool isPieceFlippedHorizontal();

	RenderObject *getTopParent();

	virtual void onAnimationKeyPassed(int key){}

	Vector getAbsoluteRotation();
	float getWorldRotation();
	Vector getWorldPositionAndRotation(); // more efficient shortcut, returns rotation in vector z component
	Vector getNormal();
	Vector getForward();
	void setOverrideCullRadius(float ovr);
	void setRenderPass(int pass) { renderPass = pass; }
	int getRenderPass() { return renderPass; }
	void setOverrideRenderPass(int pass) { overrideRenderPass = pass; }
	int getOverrideRenderPass() { return overrideRenderPass; }
	enum { RENDER_ALL=314, OVERRIDE_NONE=315 };

	// Defined in RenderObject_inline.h
	inline Vector getFollowCameraPosition() const;

	void lookAt(const Vector &pos, float t, float minAngle, float maxAngle, float offset=0);
	inline RenderObject *getParent() const {return parent;}
	void applyBlendType();
	void fhTo(bool fh);
	void addDeathNotify(RenderObject *r);
	virtual void unloadDevice();
	virtual void reloadDevice();

	Vector getCollisionMaskNormal(int index);

	//-------------------------------- Methods above, fields below

	static bool renderCollisionShape;
	static bool renderPaths;
	static int lastTextureApplied;
	static bool lastTextureRepeat;

	float width, height;  // Only used by Quads, but stored here for getCullRadius()
	InterpolatedVector position, scale, color, alpha, rotation;
	InterpolatedVector offset, rotationOffset, internalOffset, beforeScaleOffset;
	InterpolatedVector velocity, gravity;

	CountedPtr<Texture> texture;

	//int mode;

	bool fadeAlphaWithLife;

	bool blendEnabled;
	enum BlendTypes { BLEND_DEFAULT = 0, BLEND_ADD, BLEND_SUB, BLEND_MULT };
	unsigned char blendType;

	float life;

	float followCamera;


	bool renderBeforeParent;
	bool updateAfterParent;



	bool colorIsSaved;  // Used for both color and alpha
	Vector savedColor;  // Saved values from setColorMult()
	float savedAlpha;

	bool shareAlphaWithChildren;
	bool shareColorWithChildren;

	bool cull;
	float updateCull;
	int layer;

	InterpolatedVector *positionSnapTo;


	typedef std::vector<RenderObject*> Children;
	Children children, childGarbage;



	float collideRadius;
	Vector collidePosition;
	std::vector<Vector> collisionMask;
	std::vector<Vector> transformedCollisionMask;

	CollideRects collisionRects;
	float collisionMaskRadius;

	float alphaMod;

	bool ignoreUpdate;
	bool useOldDT;

protected:
	virtual void onFH(){}
	virtual void onFV(){}
	virtual void onDestroy(){}
	virtual void onSetTexture(){}
	virtual void onRender(){}
	virtual void onUpdate(float dt);
	virtual void deathNotify(RenderObject *r);
	virtual void onEndOfLife() {}

	inline void updateLife(float dt)
	{
		if (decayRate > 0)
		{
			life -= decayRate*dt;
			if (life<=0)
			{
				safeKill();
			}
		}
		if (fadeAlphaWithLife && !alpha.isInterpolating())
		{

			alpha = life/maxLife;
		}
	}

	// Is this object or any of its children rendered in pass "pass"?
	bool hasRenderPass(const int pass);

	inline void renderCall();
	void renderCollision();

	bool repeatTexture;
	unsigned char pm;  // unsigned char to save space
	typedef std::list<RenderObject*> RenderObjectList;
	RenderObjectList deathNotifications;
	int overrideRenderPass;
	int renderPass;
	float overrideCullRadiusSqr;
	float motionBlurTransitionTimer;
	int motionBlurFrameOffsetCounter, motionBlurFrameOffset;
	std::vector<MotionBlurFrame>motionBlurPositions;
	bool motionBlur, motionBlurTransition;

	bool _dead;
	bool _hidden;
	bool _static;
	bool _fv, _fh;

	int idx;
	RenderObject *parent;
	StateData *stateData;
	float decayRate;
	float maxLife;
};

#endif
