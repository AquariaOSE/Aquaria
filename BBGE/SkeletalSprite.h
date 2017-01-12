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
	AC_RESET_PASS,
};

class ParticleEffect;
class SkeletalSprite;

class Bone : public Quad
{
public:
	Bone();
	void setAnimated(int a);

	enum {
		ANIM_NONE		= 0,
		ANIM_POS		= 1,
		ANIM_ROT		= 2,
		ANIM_ALL		= 10
	};
	void createStrip(bool vert, int num);
	Quad* addFrame(const std::string &gfx);
	void showFrame(int i);
	void destroy();
	std::string gfx;
	std::string name;
	int boneIdx, pidx, rbp;
	std::map<int, ParticleEffect*> emitters;
	std::string prt;
	std::vector<Vector> changeStrip;

	bool generateCollisionMask;
	int animated;
	Vector originalScale;

	void addSegment(Bone *b);

	int segmentChain;

	void updateSegments();
	void updateSegment(Bone *b, const Vector &diff);

	SkeletalSprite *skeleton;


	void setSegmentProps(int minDist, int maxDist, bool reverse);
	Vector segmentOffset;

	bool fileRenderQuad;
	bool selectable;
	int originalRenderPass; // stores the render pass originally set in the XML file. For AC_RESET_PASS.

protected:
	int minDist, maxDist, reverse;
	std::vector<Bone*> segments;
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
	int idx, x, y, rot;
	float sx, sy;
	bool doScale;
	std::vector<Vector> strip;
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
	BoneKeyframe *getBoneKeyframe(int idx);
	std::string cmd;
	std::vector<BoneCommand> commands;

	void copyAllButTime(SkeletalKeyframe *copy);
};

class Animation
{
public:
	Animation();
	std::string name;
	typedef std::vector <SkeletalKeyframe> Keyframes;
	Keyframes keyframes;
	SkeletalKeyframe *getKeyframe(int key);
	SkeletalKeyframe *getLastKeyframe();
	SkeletalKeyframe *getFirstKeyframe();
	SkeletalKeyframe *getPrevKeyframe(float t);
	SkeletalKeyframe *getNextKeyframe(float t);
	void cloneKey(int key, float toffset);
	void deleteKey(int key);
	void reorderKeyframes();
	float getAnimationLength();
	int getSkeletalKeyframeIndex(SkeletalKeyframe *skey);
	int getNumKeyframes();
	void reverse();
	bool resetPassOnEnd;
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
	bool createTransitionAnimation(const std::string& anim, float time);
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
	int currentAnimation;
	bool animating;


};

class SkeletalSprite : public RenderObject
{
public:

	SkeletalSprite();
	void loadSkeletal(const std::string &fn);
	bool saveSkeletal(const std::string &fn);
	void loadSkin(const std::string &fn);

	Bone *getBoneByIdx(int idx);
	Bone *getBoneByName(const std::string &name);
	void animate(const std::string &animation, int loop = 0, int layer=0);



	void setTime(float time, int layer=0);

	void updateBones();
	void playCurrentAnimation(int loop=0, int layer=0);
	void stopAnimation(int layer=0);
	void stopAllAnimations();

	float transitionAnimate(const std::string& anim, float time, int loop=0, int layer=0);

	bool isAnimating(int layer=0);

	void setTimeMultiplier(float t, int layer=0);

	Bone* getSelectedBone(bool mouseBased = true);
	Animation *getCurrentAnimation(int layer=0);


	void nextAnimation();
	void prevAnimation();
	void lastAnimation();
	void firstAnimation();
	void updateSelectedBoneColor();


	void setFreeze(bool f);



	Animation *getAnimation(const std::string& anim);

	std::vector<Animation> animations;
	std::vector<Bone*> bones;

	inline int getSelectedBoneIdx(void) { return selectedBone; }
	void setSelectedBone(int b);
	void selectPrevBone();
	void selectNextBone();

	bool isLoaded();
	int getNumAnimLayers() const { return animLayers.size(); }

	AnimationLayer* getAnimationLayer(int l);
	int getBoneIdx(Bone *b);
	void toggleBone(int idx, int v);

	void setAnimationKeyNotify(RenderObject *r);

	std::string filenameLoaded;

	static std::string animationPath, skinPath, secondaryAnimationPath;
	static void clearCache();

protected:
	bool frozen;
	RenderObject *animKeyNotify;
	bool loaded;
	int selectedBone;
	friend class AnimationLayer;
	std::vector<AnimationLayer> animLayers;
	Bone* initBone(int idx, std::string gfx, int pidx, int rbp=0, std::string name="", float cr=0, bool fh=false, bool fv=false);
	void deleteBones();
	void onUpdate(float dt);
};

#endif
