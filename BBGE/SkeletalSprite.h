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
#ifndef SKELETALSPRITE_H
#define SKELETALSPRITE_H

#include "Quad.h"
#include "SimpleIStringStream.h"
#include <map>
#include "Interpolators.h"
// for 2d system only

enum AnimationCommand
{
	AC_PRT_LOAD		=0,
	AC_PRT_START		,
	AC_PRT_STOP			,
	AC_SEGS_START		,
	AC_FRM_SHOW			,
	AC_SND_PLAY			,
	AC_SEGS_STOP,
	AC_SET_PASS,
	AC_RESET_PASS
};

class ParticleEffect;
class SkeletalSprite;

class Bone : public CollideQuad
{
	friend class SkeletalSprite;
public:
	Bone();
	virtual ~Bone();
	void setAnimated(int a);

	enum {
		ANIM_NONE		= 0x00,
		ANIM_POS		= 0x01,
		ANIM_ROT		= 0x02,
		ANIM_ALL		= ANIM_POS | ANIM_ROT
	};
	void createStrip(bool vert, int num);
	Quad* addFrame(const std::string &gfx);
	void showFrame(int i);
	void destroy() OVERRIDE;
	std::string gfx;
	std::string name;
	size_t boneIdx;
	int pidx, rbp;
	std::string prt;
	std::vector<Vector> changeStrip;

	bool generateCollisionMask;
	bool enableCollision;
	int animated;
	Vector originalScale;

	bool canCollide() const;

	void addSegment(Bone *b);
	ParticleEffect *getEmitter(unsigned slot) const;

	int segmentChain;

	void updateSegments();
	void updateSegment(Bone *b, const Vector &diff);

	SkeletalSprite *skeleton;


	void setSegmentProps(int minDist, int maxDist, bool reverse);
	Vector segmentOffset;

	bool stripVert;
	bool fileRenderQuad;
	bool selectable;
	int originalRenderPass; // stores the render pass originally set in the XML file. For AC_RESET_PASS.

	void spawnParticlesFromCollisionMask(const char *p, unsigned intv, int layer, float rotz = 0);
	Vector getCollisionMaskNormal(Vector pos, float dist) const;

	virtual void renderCollision(const RenderState& rs) const OVERRIDE;

protected:
	std::vector<ParticleEffect*> emitters;
	int minDist, maxDist, reverse;
	std::vector<Bone*> segments;
public:
	std::vector<Vector> collisionMask;
	std::vector<Vector> transformedCollisionMask;
	float collisionMaskRadius;
};

class BoneCommand
{
public:
	bool parse(Bone *b, SimpleIStringStream &is);
	void run();
	AnimationCommand command;
	Bone *b;

	int slot;
	std::string file;
};

class BoneKeyframe
{
public:
	BoneKeyframe() : idx(0), x(0), y(0), rot(0), sx(1), sy(1), doScale(0) {}
	size_t idx;
	int x, y, rot;
	float sx, sy;
	bool doScale;
	std::vector<Vector> grid;
	std::vector<Vector> controlpoints;
};

class SkeletalKeyframe
{
public:
	SkeletalKeyframe()
	{
		lerpType = 0;
		t = 0;
	}
	int lerpType;
	float t;
	std::string sound;
	std::vector<BoneKeyframe> keyframes;
	BoneKeyframe *getBoneKeyframe(size_t idx);
	std::string cmd;
	std::vector<BoneCommand> commands;

	void copyAllButTime(SkeletalKeyframe *copy);
};

class BoneGridInterpolator
{
public:
	size_t idx;
	BSpline2D bsp;
	bool storeBoneByIdx;
	void updateGridOnly(BoneKeyframe& bk, const Bone *bone);
	void updateGridAndBone(BoneKeyframe& bk, Bone *bone);
};

class Animation
{
public:
	Animation();
	std::string name;
	typedef std::vector <SkeletalKeyframe> Keyframes;
	Keyframes keyframes;
	SkeletalKeyframe *getKeyframe(size_t key);
	SkeletalKeyframe *getLastKeyframe();
	SkeletalKeyframe *getFirstKeyframe();
	SkeletalKeyframe *getPrevKeyframe(float t);
	SkeletalKeyframe *getNextKeyframe(float t);
	void cloneKey(size_t key, float toffset);
	void deleteKey(size_t key);
	void reorderKeyframes();
	float getAnimationLength();
	size_t getSkeletalKeyframeIndex(SkeletalKeyframe *skey);
	size_t getNumKeyframes();
	void reverse();
	bool resetPassOnEnd;

	BoneGridInterpolator *getBoneGridInterpolator(size_t boneIdx);
	typedef std::vector <BoneGridInterpolator> Interpolators;
	Interpolators interpolators;
};

class SkeletalSprite;

class AnimationLayer
{
public:

	//----
	AnimationLayer();
	void setSkeletalSprite(SkeletalSprite *s);
	Animation *getCurrentAnimation();
	void animate(const std::string &animation, int loop);
	void update(float dt);
	void updateBones();
	void stopAnimation();
	float getAnimationLength();
	void createTransitionAnimation(Animation& to, float time);
	void playAnimation(int idx, int loop);
	void playCurrentAnimation(int loop);
	void enqueueAnimation(const std::string& anim, int loop);
	float transitionAnimate(std::string anim, float time, int loop);
	void setTimeMultiplier(float t);
	bool isAnimating();
	bool contains(const Bone *b) const;
	void resetPass();

	//----
	float fallThru;
	float fallThruSpeed;
	std::string name;
	std::vector<int> ignoreBones;
	std::vector<int> includeBones;
	SkeletalSprite *s;

	SkeletalKeyframe *lastNewKey;

	float timer;
	int loop;
	Animation blendAnimation;
	std::string enqueuedAnimation;
	int enqueuedAnimationLoop;
	//float timeMultiplier;
	//HACK: should be a lerped float
	InterpolatedVector timeMultiplier;
	float animationLength;
	size_t currentAnimation;
	bool animating;


};

class SkeletalSprite : public RenderObject
{
public:

	SkeletalSprite();
	virtual ~SkeletalSprite();
	virtual void destroy();

	void loadSkeletal(const std::string &fn);
	bool saveSkeletal(const std::string &fn);
	void loadSkin(const std::string &fn);

	Bone *getBoneByIdx(size_t idx);
	Bone *getBoneByName(const std::string &name);
	void animate(const std::string &animation, int loop = 0, int layer=0);



	void setTime(float time, size_t layer=0);

	void updateBones();
	void playCurrentAnimation(int loop=0, int layer=0);
	void stopAnimation(int layer=0);
	void stopAllAnimations();

	float transitionAnimate(const std::string& anim, float time, int loop=0, int layer=0);

	bool isAnimating(int layer=0);

	void setTimeMultiplier(float t, int layer=0);

	Bone* getSelectedBone(bool mouseBased = true);
	Animation *getCurrentAnimation(size_t layer=0);


	void nextAnimation();
	void prevAnimation();
	void lastAnimation();
	void firstAnimation();
	bool selectAnimation(const char *name);
	void updateSelectedBoneColor();


	void setFreeze(bool f);



	Animation *getAnimation(const std::string& anim);

	std::vector<Animation> animations;
	std::vector<Bone*> bones;

	inline size_t getSelectedBoneIdx(void) { return selectedBone; }
	void setSelectedBone(int b);
	void selectPrevBone();
	void selectNextBone();

	bool isLoaded();
	size_t getNumAnimLayers() const { return animLayers.size(); }

	AnimationLayer* getAnimationLayer(size_t l);
	size_t getBoneIdx(Bone *b);
	void toggleBone(size_t idx, int v);

	void setAnimationKeyNotify(RenderObject *r);

	std::string filenameLoaded;

	static std::string animationPath, skinPath, secondaryAnimationPath;
	static void clearCache();

protected:
	bool frozen;
	RenderObject *animKeyNotify;
	bool loaded;
	size_t selectedBone;
	friend class AnimationLayer;
	std::vector<AnimationLayer> animLayers;
	Bone* initBone(int idx, std::string gfx, int pidx, bool rbp=false, std::string name="", float cr=0, bool fh=false, bool fv=false);
	void deleteBones();
	void onUpdate(float dt);
};

#endif
