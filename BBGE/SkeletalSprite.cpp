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
#include "SkeletalSprite.h"
#include "Core.h"
#include "Particles.h"
#include "MathFunctions.h"
#include "SimpleIStringStream.h"
#include "ReadXML.h"
#include "RenderBase.h"
#include "SplineGrid.h"
#include "RenderGrid.h"

#include <tinyxml2.h>
using namespace tinyxml2;

std::string SkeletalSprite::animationPath				= "data/animations/";
std::string SkeletalSprite::skinPath					= "skins/";

std::string SkeletalSprite::secondaryAnimationPath		= "";

static std::map<std::string, XMLDocument*> skelCache;

static XMLDocument *_retrieveSkeletalXML(const std::string& name, bool keepEmpty)
{
	std::map<std::string, XMLDocument*>::iterator it = skelCache.find(name);
	if(it != skelCache.end())
		return it->second;

	XMLDocument *doc = readXML(name, NULL, keepEmpty);
	if(doc)
		skelCache[name] = doc;

	return doc;
}

void SkeletalSprite::clearCache()
{
	for(std::map<std::string, XMLDocument*>::iterator it = skelCache.begin(); it != skelCache.end(); ++it)
		delete it->second;
	skelCache.clear();
}


void SkeletalKeyframe::copyAllButTime(SkeletalKeyframe *copy)
{
	if (!copy) return;

	float t = this->t;
	(*this) = (*copy);
	this->t = t;
}

Bone::Bone() : CollideQuad()
{
	addType(SCO_BONE);
	fileRenderQuad = true;
	skeleton = 0;
	generateCollisionMask = true;
	enableCollision = true;
	animated = ANIM_ALL;
	originalScale = Vector(1,1);
	boneIdx = pidx = -1;
	rbp = false;
	segmentChain = 0;
	collisionMaskRadius = 0;

	minDist = maxDist = 128;
	reverse = false;
	selectable = true;
	originalRenderPass = 0;
	stripVert = false;
}

Bone::~Bone()
{
}

ParticleEffect *Bone::getEmitter(unsigned slot) const
{
	return slot < emitters.size() ? emitters[slot] : NULL;
}

void Bone::destroy()
{
	Quad::destroy();

	for (size_t i = 0; i < segments.size(); i++)
	{
		segments[i]->setLife(1.0);
		segments[i]->setDecayRate(10);
		segments[i]->alpha = 0;
	}
	segments.clear();
}

bool Bone::canCollide() const
{
	return this->enableCollision && this->alpha.x == 1 && this->renderQuad && (!this->collisionMask.empty() || this->collideRadius);
}

void Bone::addSegment(Bone *b)
{
	segments.push_back(b);

	b->segmentChain = 2;

	skeleton->removeChild(b);

	core->getTopStateData()->addRenderObject(b, skeleton->getTopLayer());
	b->position = this->getWorldPosition();
}

void Bone::createStrip(bool vert, int num)
{
	RenderGrid *grid = vert ? createGrid(2, num) : createGrid(num, 2);
	stripVert = vert;
	grid->gridType = GRID_STRIP;
	changeStrip.resize(num);
}


Quad* Bone::addFrame(const std::string &gfx)
{
	renderQuad = false;
	Quad *q = new Quad();
	q->setTexture(gfx);
	q->renderBeforeParent = 1;
	addChild(q, PM_POINTER);
	return q;
}

void Bone::showFrame(int idx)
{

	int c = 0;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		RenderObject *r = (*i);
		if (idx == c)
		{
			if (r->alpha == 0)
			{
				r->alpha = 1;

				// add option to turn on alpha fading
				//r->alpha.interpolateTo(1, t);
			}
			else
			{
				r->alpha = 1;
			}
		}
		else
		{
			if (r->alpha == 1)
			{
				r->alpha = 0;
				//r->alpha.interpolateTo(0, t*2);
			}
			else
			{
				r->alpha = 0;
			}
		}
		c++;
	}
}


void Bone::setAnimated(int b)
{


	animated = b;
}


void Bone::setSegmentProps(int minDist, int maxDist, bool reverse)
{
	this->minDist = minDist;
	this->maxDist = maxDist;
	this->reverse = reverse;
}

void Bone::updateSegment(Bone *b, const Vector &diff)
{


	float angle = -1;
	if (diff.getSquaredLength2D() > sqr(maxDist))
	{
		Vector useDiff = diff;
		useDiff.setLength2D(maxDist);
		Vector reallyUseDiff = diff - useDiff;
		b->position += reallyUseDiff;

		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), diff, angle);
	}
	else if (diff.getSquaredLength2D() > sqr(minDist))
	{
		b->position += diff*0.05f;

		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), diff, angle);


	}
	if (angle != -1)
	{


		if (b->rotation.z >= 270 && angle < 90)
		{
			b->rotation.stop();
			b->rotation.z -= 360;
		}

		if (b->rotation.z <= 90 && angle > 270)
		{
			b->rotation.stop();
			b->rotation.z += 360;
		}


		b->rotation.interpolateTo(Vector(0,0,angle),0.2f);
	}

}

void Bone::updateSegments()
{
	if (segmentChain>0 && !segments.empty())
	{



		if (!reverse)
		{
			for (size_t i = 0; i < segments.size(); i++)
			{
				Vector diff;
				if (i == 0)
				{
					Vector world = getWorldCollidePosition(segmentOffset);
					diff = world - segments[i]->getWorldPosition();
				}
				else
					diff = segments[i-1]->getWorldPosition() - segments[i]->getWorldPosition();

				updateSegment(segments[i], diff);
			}
		}
		else
		{
			int top = segments.size()-1;
			for (int i = top; i >= 0; i--)
			{
				Vector diff;
				if (i == top)
				{
					Vector world = getWorldCollidePosition(segmentOffset);
					diff = world - segments[i]->getWorldPosition();
				}
				else
					diff = segments[i+1]->getWorldPosition() - segments[i]->getWorldPosition();

				updateSegment(segments[i], diff);
			}
		}
	}
}

void Bone::spawnParticlesFromCollisionMask(const char *p, unsigned intv, int layer, float rotz)
{
	for (size_t j = 0; j < this->collisionMask.size(); j+=intv)
	{
		Vector pos = this->getWorldCollidePosition(this->collisionMask[j]);
		core->createParticleEffect(p, pos, layer, rotz);
	}
}

void Bone::renderCollision(const RenderState& rs) const
{
	if (!collisionMask.empty())
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);

		glLoadIdentity();
		core->setupRenderPositionAndScale();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glColor4f(1,1,0,0.5);

		for (size_t i = 0; i < transformedCollisionMask.size(); i++)
		{
			Vector collide = this->transformedCollisionMask[i];



			glTranslatef(collide.x, collide.y, 0);
			RenderObject *parent = this->getTopParent();
			if (parent)
				drawCircle(collideRadius*parent->scale.x, 45);
			glTranslatef(-collide.x, -collide.y, 0);
		}


		glDisable(GL_BLEND);
		glPopMatrix();
		glPopAttrib();
	}
	else
		CollideQuad::renderCollision(rs);
}

Vector Bone::getCollisionMaskNormal(Vector pos, float dist) const
{
	Vector sum;
	for (size_t i = 0; i < this->transformedCollisionMask.size(); i++)
	{
		Vector diff = pos - transformedCollisionMask[i];
		if (diff.isLength2DIn(dist))
			sum += diff;
	}
	sum.normalize2D();
	return sum;
}


bool BoneCommand::parse(Bone *b, SimpleIStringStream &is)
{
	std::string type;
	is >> type;
	this->b = b;
	if (type=="AC_PRT_LOAD")
	{
		command = AC_PRT_LOAD;
		is >> slot >> file;
	}
	else if (type=="AC_SND_PLAY")
	{
		command = AC_SND_PLAY;
		is >> file;
	}
	else if (type=="AC_FRM_SHOW")
	{
		command = AC_FRM_SHOW;
		is >> slot;
	}
	else if (type=="AC_PRT_START")
	{
		command = AC_PRT_START;
		is >> slot;

	}
	else if (type=="AC_PRT_STOP")
	{
		command = AC_PRT_STOP;
		is >> slot;

	}
	else if (type=="AC_SEGS_START")
		command = AC_SEGS_START;
	else if (type=="AC_SEGS_STOP")
		command = AC_SEGS_STOP;
	else if (type == "AC_SET_PASS")
	{
		command = AC_SET_PASS;
		is >> slot;
	}
	else if(type == "AC_RESET_PASS")
		command = AC_RESET_PASS;
	else // fail
	{
		std::ostringstream os;
		os << "Failed to parse bone command string: invalid command: " << type;
		errorLog(os.str());
	}

	return true;
}

void BoneCommand::run()
{

	switch(command)
	{
	case AC_SND_PLAY:
	{
		core->sound->playSfx(file);
	}
	break;
	case AC_FRM_SHOW:
	{
		b->showFrame(slot);
	}
	break;
	case AC_PRT_LOAD:
	{
		ParticleEffect *e = b->getEmitter(slot);
		if (e)
		{
			e->load(file);
		}
	}
	break;
	case AC_PRT_START:
	{
		ParticleEffect *e = b->getEmitter(slot);
		if (e)
			e->start();
	}
	break;
	case AC_PRT_STOP:
	{
		ParticleEffect *e = b->getEmitter(slot);
		if (e)
			e->stop();
	}
	break;
	case AC_SET_PASS:
		b->setRenderPass(slot);
	break;
	case AC_RESET_PASS:
		b->setRenderPass(b->originalRenderPass);
	break;
	case AC_SEGS_START:
	case AC_SEGS_STOP:
		break;
	}
}


AnimationLayer::AnimationLayer()
{
	lastNewKey = 0;
	fallThru= 0;

	timer = 0;
	loop = 0;
	enqueuedAnimationLoop = 0;
	timeMultiplier = 1;
	animationLength = 0;
	currentAnimation = 0;
	animating = false;
	fallThruSpeed = 0;
	s = 0;
}

void AnimationLayer::setTimeMultiplier(float t)
{
	timeMultiplier = t;
}

void AnimationLayer::playCurrentAnimation(int loop)
{
	playAnimation(currentAnimation, loop);
}

void AnimationLayer::animate(const std::string &a, int loop)
{
	std::string animation = a;
	stringToLower(animation);

	bool played = false;
	for (size_t i = 0; i < s->animations.size(); i++)
	{
		if (s->animations[i].name == animation)
		{
			playAnimation(i, loop);
			played = true;
			break;
		}
	}
	if (!played)
	{
		std::ostringstream os;
		os << "Could not find animation: " << animation;
		debugLog(os.str());
	}
}

void AnimationLayer::playAnimation(int idx, int loop)
{
	if (!(&s->animLayers[0] == this))
	{
		fallThru = 1;
		fallThruSpeed = 10;
	}
	timeMultiplier = 1;

	currentAnimation = idx;
	timer = 0;
	animating = true;

	this->loop = loop;

	animationLength = getCurrentAnimation()->getAnimationLength();

}

void AnimationLayer::enqueueAnimation(const std::string& anim, int loop)
{
	enqueuedAnimation = anim;
	enqueuedAnimationLoop = loop;
	stringToLower(enqueuedAnimation);
}

float AnimationLayer::transitionAnimate(std::string anim, float time, int loop)
{
	stringToLower(anim);
	float totalTime =0;
	if(Animation *a = this->s->getAnimation(anim))
	{
		if (time <= 0) // no transition?
		{
			animate(anim, loop);
		}
		else
		{
			createTransitionAnimation(*a, time);
			timeMultiplier = 1;

			currentAnimation = -1;
			this->loop = 0;
			timer = 0;
			animating = 1;
			animationLength = getCurrentAnimation()->getAnimationLength();
			enqueueAnimation(anim, loop);
		}
		if (loop > -1)
			totalTime = a->getAnimationLength()*(loop+1) + time;
		else
			totalTime = a->getAnimationLength() + time;
	}
	return totalTime;
}

void AnimationLayer::setSkeletalSprite(SkeletalSprite *s)
{
	this->s = s;
}

Animation* AnimationLayer::getCurrentAnimation()
{
	if (currentAnimation == -1)
		return &blendAnimation;
	if (currentAnimation >= s->animations.size())
	{
		std::ostringstream os;
		os << "skel: " << s->filenameLoaded << " currentAnimation: " << currentAnimation << " is out of range\n error in anim file?";
		exit_error(os.str());
		return 0;
	}
	return &s->animations[currentAnimation];
}

void AnimationLayer::createTransitionAnimation(Animation& to, float time)
{
	blendAnimation.keyframes.clear();
	SkeletalKeyframe k;
	k.t = 0;
	for (size_t i = 0; i < s->bones.size(); i++)
	{
		BoneKeyframe b;
		b.idx = s->bones[i]->boneIdx;
		b.x = s->bones[i]->position.x;
		b.y = s->bones[i]->position.y;
		b.rot = s->bones[i]->rotation.z;
		b.sx = s->bones[i]->scale.x;
		b.sy = s->bones[i]->scale.y;
		k.keyframes.push_back(b);
	}
	blendAnimation.keyframes.push_back(k);

	SkeletalKeyframe k2;
	k2 = *to.getKeyframe(0);
	k2.t = time;
	blendAnimation.keyframes.push_back(k2);

	blendAnimation.name = to.name;
}


void AnimationLayer::stopAnimation()
{
	if(s->loaded && getCurrentAnimation()->resetPassOnEnd)
		resetPass();
	animating = false;
	if (!enqueuedAnimation.empty())
	{
		animate(enqueuedAnimation, enqueuedAnimationLoop);
		enqueuedAnimation = "";
		enqueuedAnimationLoop = 0;
	}
}

bool AnimationLayer::isAnimating()
{
	return animating;
}

float AnimationLayer::getAnimationLength()
{
	return animationLength;
}

Animation::Animation()
: resetPassOnEnd(false)
{
}

size_t Animation::getNumKeyframes()
{
	return keyframes.size();
}

SkeletalKeyframe *Animation::getKeyframe(size_t key)
{
	if (key >= keyframes.size()) return 0;
	return &keyframes[key];
}

void Animation::reverse()
{
	Keyframes copy = keyframes;
	Keyframes copy2 = keyframes;
	keyframes.clear();
	int sz = copy.size()-1;
	for (int i = sz; i >= 0; i--)
	{
		keyframes.push_back(copy[i]);
		keyframes[keyframes.size()-1].t = copy2[sz-i].t;
	}
	reorderKeyframes();
}

float Animation::getAnimationLength()
{
	return getLastKeyframe()->t;
}

SkeletalKeyframe *Animation::getLastKeyframe()
{
	if (!keyframes.empty())
		return &keyframes[keyframes.size()-1];
	return 0;
}

SkeletalKeyframe *Animation::getFirstKeyframe()
{
	if (!keyframes.empty())
		return &keyframes[0];
	return 0;
}

void Animation::reorderKeyframes()
{

	for (size_t i = 0; i < keyframes.size(); i++)
	{
		for (size_t j = 0; j < keyframes.size()-1; j++)
		{
			if (keyframes[j].t > keyframes[j+1].t)
			{
				SkeletalKeyframe temp = keyframes[j+1];
				keyframes[j+1] = keyframes[j];
				keyframes[j] = temp;
			}
		}
	}
}

void Animation::cloneKey(size_t key, float toffset)
{
	std::vector<SkeletalKeyframe> copy = this->keyframes;
	keyframes.clear();
	size_t i = 0;
	for (i = 0; i <= key; i++)
		keyframes.push_back(copy[i]);
	for (i = key; i < copy.size(); i++)
		keyframes.push_back(copy[i]);
	keyframes[key+1].t += toffset;
}

void Animation::deleteKey(size_t key)
{
	std::vector<SkeletalKeyframe> copy = this->keyframes;
	keyframes.clear();
	size_t i = 0;
	for (i = 0; i < key; i++)
		keyframes.push_back(copy[i]);
	for (i = key+1; i < copy.size(); i++)
		keyframes.push_back(copy[i]);
}

size_t Animation::getSkeletalKeyframeIndex(SkeletalKeyframe *skey)
{
	for (size_t i = 0; i < keyframes.size(); i++)
	{
		if (&keyframes[i] == skey)
			return i;
	}
	return -1;
}

BoneGridInterpolator * Animation::getBoneGridInterpolator(size_t boneIdx)
{
	for(size_t i = 0; i < interpolators.size(); ++i)
	{
		BoneGridInterpolator& bgip = interpolators[i];
		if(bgip.idx == boneIdx)
		{
			return &bgip;
		}
	}
	return 0;
}


BoneKeyframe *SkeletalKeyframe::getBoneKeyframe(size_t idx)
{
	for (size_t i = 0; i < keyframes.size(); i++)
	{
		if (keyframes[i].idx == idx)
		{
			return &keyframes[i];
		}
	}
	return 0;
}

SkeletalKeyframe *Animation::getPrevKeyframe(float t)
{
	size_t kf = -1;
	for (size_t i = keyframes.size(); i-- > 0; )
	{
		if (t >= keyframes[i].t)
		{
			kf = i;
			break;
		}
	}
	if (kf == -1)
		return 0;
	if (kf >= keyframes.size())
		kf = keyframes.size()-1;
	return &keyframes[kf];
}

SkeletalKeyframe *Animation::getNextKeyframe(float t)
{
	size_t kf = -1;
	for (size_t i = 0; i < keyframes.size(); i++)
	{
		if (t <= keyframes[i].t)
		{
			kf = i;
			break;
		}
	}

	if (kf == -1)
		return 0;
	if (kf >= keyframes.size())
		kf = keyframes.size()-1;
	return &keyframes[kf];
}

SkeletalSprite::SkeletalSprite() : RenderObject()
{
	frozen = false;
	animKeyNotify = 0;
	loaded = false;
	animLayers.resize(10);
	for (size_t i = 0; i < animLayers.size(); i++)
		animLayers[i].setSkeletalSprite(this);
	selectedBone = -1;
}

SkeletalSprite::~SkeletalSprite()
{
}

void SkeletalSprite::destroy()
{
	bones.clear(); // they are added as children too, so the next call will do the actual deletion
	RenderObject::destroy();
}

void SkeletalSprite::setAnimationKeyNotify(RenderObject *r)
{
	animKeyNotify = r;
}

void SkeletalSprite::animate(const std::string &animation, int loop, int layer)
{
	animLayers[layer].animate(animation, loop);
}

float SkeletalSprite::transitionAnimate(const std::string& anim, float time, int loop, int layer)
{
	AnimationLayer *animLayer = getAnimationLayer(layer);
	if (animLayer)
		return animLayer->transitionAnimate(anim, time, loop);

	std::ostringstream os;
	os << "playing animation on invalid layer: " << layer;
	errorLog(os.str());
	return 0;
}

AnimationLayer* SkeletalSprite::getAnimationLayer(size_t l)
{
	if (l < animLayers.size())
	{
		return &animLayers[l];
	}
	std::ostringstream os;
	os << "couldn't get animLayer: " << l;
	debugLog(os.str());
	return 0;
}

bool SkeletalSprite::isLoaded()
{
	return loaded;
}

void SkeletalSprite::onUpdate(float dt)
{
	if (frozen) return;
	RenderObject::onUpdate(dt);

	size_t i = 0;

	for (i = 0; i < bones.size(); i++)
	{
		Bone *b = bones[i];
		if (b && !b->collisionMask.empty())
		{
			if (b->collisionMask.size() != b->transformedCollisionMask.size())
			{
				b->transformedCollisionMask.resize(b->collisionMask.size());
			}
			for (size_t i = 0; i < b->collisionMask.size(); i++)
			{
				b->transformedCollisionMask[i] = b->getWorldCollidePosition(b->collisionMask[i]);
			}
		}
	}


	for (i = 0; i < animLayers.size(); i++)
	{
		animLayers[i].update(dt);
	}

}

void AnimationLayer::update(float dt)
{
	timeMultiplier.update(dt);
	if (animating)
	{
		timer += dt*timeMultiplier.x;

		if (timer >= animationLength)
		{
			float leftover;
			if (animationLength > 0)
				leftover = fmodf(timer, animationLength);
			else
				leftover = 0;
			timer = animationLength;
			if (loop==-1 || loop > 0)
			{
				playAnimation(this->currentAnimation, loop);
				if (loop > 0)
					loop --;
				timer = leftover;
			}
			else
			{
				stopAnimation();
			}
		}
		updateBones();
	}
	else if (!animating)
	{
		if (fallThru > 0)
		{
			fallThru -= dt * fallThruSpeed;
			if (fallThru < 0)
				fallThru = 0;
			updateBones();
		}
	}
}

bool SkeletalSprite::saveSkeletal(const std::string &fn)
{
	std::string file, filename=fn;
	stringToLower(filename);

	if (!secondaryAnimationPath.empty())
	{
		createDir(secondaryAnimationPath);
		file = secondaryAnimationPath + filename + ".xml";
	}
	else
	{
		file = animationPath + filename + ".xml";
	}

	size_t i = 0;
	XMLDocument *xml = _retrieveSkeletalXML(file, true);
	xml->Clear();

	XMLElement *animationLayers = xml->NewElement("AnimationLayers");
	for (i = 0; i < animLayers.size(); i++)
	{
		XMLElement *animationLayer = xml->NewElement("AnimationLayer");
		if (animLayers[i].ignoreBones.size() > 0)
		{
			std::ostringstream os;
			for (size_t j = 0; j < animLayers[i].ignoreBones.size(); j++)
			{
				os << animLayers[i].ignoreBones[j] << " ";
			}
			animationLayer->SetAttribute("ignore", os.str().c_str());
		}
		if (animLayers[i].includeBones.size() > 0)
		{
			std::ostringstream os;
			for (size_t j = 0; j < animLayers[i].includeBones.size(); j++)
			{
				os << animLayers[i].includeBones[j] << " ";
			}
			animationLayer->SetAttribute("include", os.str().c_str());
		}
		if (!animLayers[i].name.empty())
		{
			animationLayer->SetAttribute("name", animLayers[i].name.c_str());
		}

		animationLayers->InsertEndChild(animationLayer);
	}
	xml->InsertEndChild(animationLayers);


	XMLElement *bones = xml->NewElement("Bones");
	for (i = 0; i < this->bones.size(); i++)
	{
		const RenderGrid * const grid = this->bones[i]->getGrid();
		XMLElement *bone = xml->NewElement("Bone");
		bone->SetAttribute("idx", (unsigned int) this->bones[i]->boneIdx);
		bone->SetAttribute("gfx", this->bones[i]->gfx.c_str());
		bone->SetAttribute("pidx", this->bones[i]->pidx);
		bone->SetAttribute("name", this->bones[i]->name.c_str());
		bone->SetAttribute("fh", this->bones[i]->isfh());
		bone->SetAttribute("fv", this->bones[i]->isfv());
		bone->SetAttribute("gc", this->bones[i]->generateCollisionMask);
		bone->SetAttribute("cr", this->bones[i]->collideRadius);
		if(!this->bones[i]->enableCollision)
			bone->SetAttribute("c", this->bones[i]->enableCollision);
		if (!this->bones[i]->fileRenderQuad)
		{
			bone->SetAttribute("rq", this->bones[i]->fileRenderQuad);
		}
		if (!this->bones[i]->selectable)
		{
			bone->SetAttribute("sel", this->bones[i]->selectable);
		}
		if (this->bones[i]->rbp)
			bone->SetAttribute("rbp", (int)this->bones[i]->rbp);
		if (this->bones[i]->originalRenderPass)
			bone->SetAttribute("pass", this->bones[i]->originalRenderPass);
		if (this->bones[i]->offset.x)
			bone->SetAttribute("offx", this->bones[i]->offset.x);
		if (this->bones[i]->offset.y)
			bone->SetAttribute("offy", this->bones[i]->offset.y);
		if (!this->bones[i]->prt.empty())
			bone->SetAttribute("prt", this->bones[i]->prt.c_str());
		if(grid && grid->gridType != GRID_STRIP)
		{
			std::ostringstream os;
			os << grid->width() << " " << grid->height();
			bone->SetAttribute("grid", os.str().c_str());
		}
		else if (!this->bones[i]->changeStrip.empty())
		{
			std::ostringstream os;
			os << this->bones[i]->stripVert << " " << this->bones[i]->changeStrip.size();
			bone->SetAttribute("strip", os.str().c_str());
		}
		if (!this->bones[i]->internalOffset.isZero())
		{
			std::ostringstream os;
			os << this->bones[i]->internalOffset.x << " " << this->bones[i]->internalOffset.y;
			bone->SetAttribute("io", os.str().c_str());
		}
		if (this->bones[i]->isRepeatingTextureToFill())
		{
			bone->SetAttribute("rt", 1);
		}
		if (this->bones[i]->originalScale.x != 1 || this->bones[i]->originalScale.y != 1)
		{
			std::ostringstream os;
			os << this->bones[i]->originalScale.x << " " << this->bones[i]->originalScale.y;
			bone->SetAttribute("sz", os.str().c_str());
		}

		if(grid && grid->drawOrder != GRID_DRAW_DEFAULT)
		{
			bone->SetAttribute("gridDrawOrder", (int)grid->drawOrder);
		}


		for (Children::iterator j = this->bones[i]->children.begin(); j != this->bones[i]->children.end(); j++)
		{
			Bone *b = dynamic_cast<Bone*>(*j);
			Quad *q = dynamic_cast<Quad*>(*j);
			Particle *p = dynamic_cast<Particle*>(*j);
			if (q && !b && !p)
			{
				XMLElement *frame = xml->NewElement("Frame");
				frame->SetAttribute("gfx", q->texture->name.c_str());
				if (q->getRenderPass() != 0)
				{
					frame->SetAttribute("pass", q->getRenderPass());
				}
				bone->InsertEndChild(frame);
			}
		}
		bones->InsertEndChild(bone);
	}
	xml->InsertEndChild(bones);

	XMLElement *animations = xml->NewElement("Animations");
	for (i = 0; i < this->animations.size(); i++)
	{
		Animation *a = &this->animations[i];
		XMLElement *animation = xml->NewElement("Animation");
		animation->SetAttribute("name", a->name.c_str());
		if(a->resetPassOnEnd)
			animation->SetAttribute("resetPassOnEnd", a->resetPassOnEnd);

		for (size_t j = 0; j < a->interpolators.size(); ++j)
		{
			const BoneGridInterpolator& bgip = a->interpolators[j];
			XMLElement *interp = xml->NewElement("Interpolator");
			Bone *bone = this->getBoneByIdx(bgip.idx);
			assert(bone->gridType == Quad::GRID_INTERP);
			if(bgip.storeBoneByIdx)
				interp->SetAttribute("bone", (int)bone->boneIdx);
			else
				interp->SetAttribute("bone", bone->name.c_str());

			{
				std::ostringstream osty;
				osty << "bspline"
					<< " " <<bgip.bsp.ctrlX()
					<< " " <<bgip.bsp.ctrlY()
					<< " " <<bgip.bsp.degX()
					<< " " <<bgip.bsp.degY();
				interp->SetAttribute("type", osty.str().c_str());
			}
			{
				std::ostringstream osd;
				for (size_t k = 0; k < a->keyframes.size(); k++)
				{
					SkeletalKeyframe& sk = a->keyframes[k];
					BoneKeyframe *bk = sk.getBoneKeyframe(bgip.idx);

					assert(bk->controlpoints.size() == bgip.bsp.ctrlX() * bgip.bsp.ctrlY());
					osd << bgip.bsp.ctrlX() << " " << bgip.bsp.ctrlY();
					for(size_t p = 0; p < bk->controlpoints.size(); ++p)
						osd << " " << bk->controlpoints[p].x << " " << bk->controlpoints[p].y;
					osd << " ";
				}
				interp->SetAttribute("data", osd.str().c_str());
			}
			animation->InsertEndChild(interp);

		}
		for (size_t j = 0; j < a->keyframes.size(); j++)
		{
			XMLElement *key = xml->NewElement("Key");
			if (!a->keyframes[j].sound.empty())
				key->SetAttribute("sound", a->keyframes[j].sound.c_str());
			if (!a->keyframes[j].cmd.empty())
			{
				key->SetAttribute("cmd", a->keyframes[j].cmd.c_str());
			}
			if (a->keyframes[j].lerpType != 0)
			{
				key->SetAttribute("lerp", a->keyframes[j].lerpType);
			}
			std::ostringstream os;
			os << a->keyframes[j].t << " ";
			std::ostringstream szos;
			for (size_t k = 0; k < a->keyframes[j].keyframes.size(); k++)
			{
				BoneKeyframe *b = &a->keyframes[j].keyframes[k];
				Bone *bone = this->getBoneByIdx(b->idx);
				if(bone)
				{
					const RenderGrid * const bgrid = bone->getGrid();
					os << b->idx << " " << b->x << " " << b->y << " " << b->rot << " ";
					// don't want to store grid points if they can be regenerated automatically
					size_t usedGridSize = (!bgrid || bgrid->gridType == GRID_INTERP) ? 0 : b->grid.size();
					os << usedGridSize << " ";
					if(usedGridSize)
						for (size_t i = 0; i < usedGridSize; i++)
							os << b->grid[i].x << " " << b->grid[i].y << " ";
					if (b->doScale)
						szos << b->idx << " " << b->sx << " " << b->sy << " ";
				}
			}
			std::string szoss = szos.str();
			if (!szoss.empty())
				key->SetAttribute("sz", szoss.c_str());

			key->SetAttribute("e", os.str().c_str());

			animation->InsertEndChild(key);
		}
		animations->InsertEndChild(animation);
	}

	xml->InsertEndChild(animations);
	return xml->SaveFile(file.c_str()) == XML_SUCCESS;
}

size_t SkeletalSprite::getBoneIdx(Bone *b)
{
	for (size_t i = 0; i < bones.size(); i++)
	{
		if (bones[i] == b)
			return i;
	}
	return -1;
}

void SkeletalSprite::toggleBone(size_t idx, int v)
{
	if (idx < bones.size())
	{
		bones[idx]->alpha.x = v;
	}
}

Bone *SkeletalSprite::getBoneByName(const std::string &name)
{
	for (size_t i = 0; i < bones.size(); i++)
	{
		if (bones[i]->name == name)
			return bones[i];
	}
	return 0;
}

Bone *SkeletalSprite::getBoneByIdx(size_t idx)
{
	for (size_t i = 0; i < bones.size(); i++)
	{
		if (bones[i]->boneIdx == idx)
			return bones[i];
	}
	return 0;
}

Bone *SkeletalSprite::initBone(int idx, std::string gfx, int pidx, bool rbp, std::string name, float cr, bool fh, bool fv)
{
	Bone *b = new Bone;
	b->boneIdx = idx;
	b->setTexture(gfx);
	b->skeleton = this;
	b->gfx = gfx;
	b->rbp = rbp;
	b->renderBeforeParent = rbp;
	b->pidx = pidx;
	b->collideRadius = cr;
	b->name = name;

	if (fh)
		b->flipHorizontal();
	if (fv)
		b->flipVertical();
	bones.push_back(b);
	return b;
}

void SkeletalSprite::firstAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation = 0;
}

bool SkeletalSprite::selectAnimation(const char* name)
{
	for(size_t i = 0; i < animations.size(); ++i)
	{
		if(animations[i].name == name)
		{
			stopAnimation();
			animLayers[0].currentAnimation = i;
			return true;
		}
	}
	return false;
}

void SkeletalSprite::lastAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation = animations.size()-1;
}

void SkeletalSprite::nextAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation++;
	if (animLayers[0].currentAnimation >= animations.size())
		animLayers[0].currentAnimation = 0;
}

void SkeletalSprite::prevAnimation()
{
	stopAnimation();
	animLayers[0].currentAnimation--;
	if (animLayers[0].currentAnimation >= animations.size())
		animLayers[0].currentAnimation = animations.size()-1;
}

void SkeletalSprite::deleteBones()
{
	bones.clear();
	for(Children::iterator it = children.begin(); it != children.end(); ++it)
	{
		(*it)->safeKill();
	}
}

Animation *SkeletalSprite::getAnimation(const std::string& anim)
{
	for (size_t i = 0; i < animations.size(); i++)
	{
		if (animations[i].name == anim)
			return &animations[i];
	}
	return 0;
}

void SkeletalSprite::loadSkin(const std::string &fn)
{
	std::string file;

	if (!secondaryAnimationPath.empty())
	{
		file = secondaryAnimationPath + skinPath + fn + ".xml";
	}

	if (file.empty() || !exists(file, false))
	{
		file = animationPath + skinPath + fn + ".xml";
	}

	file = adjustFilenameCase(file);

	if (!exists(file))
	{
		errorLog("Could not load skin[" + file + "] - File not found.");
		return;
	}
	XMLDocument *d = _retrieveSkeletalXML(file, false);
	if(!d)
	{
		errorLog("Could not load skin[" + file + "] - Malformed XML.");
		return;
	}

	XMLElement *bonesXml = d->FirstChildElement("Bones");
	if (bonesXml)
	{
		XMLElement *boneXml = bonesXml->FirstChildElement("Bone");
		while (boneXml)
		{
			int idx = atoi(boneXml->Attribute("idx"));
			Bone *b = getBoneByIdx(idx);
			if (b)
			{
				if (boneXml->Attribute("rq"))
				{
					int rq = atoi(boneXml->Attribute("rq"));
					b->renderQuad = !!rq;
				}

				std::string gfx;
				if (boneXml->Attribute("gfx"))
				{
					gfx = boneXml->Attribute("gfx");
					if (!gfx.empty())
					{
						b->gfx = gfx;
						b->setTexture(b->gfx);
						b->renderQuad = true;
					}
				}

				if (gfx.empty())
				{
					b->renderQuad = false;
				}

				if (boneXml->Attribute("fh"))
				{
					int fh = atoi(boneXml->Attribute("fh"));
					if (fh)
						b->flipHorizontal();
				}
				if (boneXml->Attribute("fv"))
				{
					int fv = atoi(boneXml->Attribute("fv"));
					if (fv)
						b->flipVertical();
				}



			}
			else
			{
				std::ostringstream os;
				os << "SkinLoad: Could not find idx[" << idx << "]";
				debugLog(os.str());
			}
			boneXml = boneXml->NextSiblingElement("Bone");
		}
	}
}

void SkeletalSprite::stopAnimation(int layer)
{
	if(size_t(layer) < animLayers.size())
		animLayers[layer].stopAnimation();
}

void SkeletalSprite::stopAllAnimations()
{
	for (size_t i = 0; i < animLayers.size(); i++)
	{
		animLayers[i].stopAnimation();
	}
}

void SkeletalSprite::playCurrentAnimation(int loop, int layer)
{
	if(size_t(layer) < animLayers.size())
		animLayers[layer].playCurrentAnimation(loop);
}

void SkeletalSprite::loadSkeletal(const std::string &fn)
{
	filenameLoaded = "";
	loaded = false;
	stopAnimation();
	animLayers.clear();
	deleteBones();


	filenameLoaded = fn;
	stringToLower(filenameLoaded);

	std::string file;

	if (!secondaryAnimationPath.empty())
	{
		file = secondaryAnimationPath + filenameLoaded + ".xml";
	}

	if (file.empty() || !exists(file, false))
	{
		file = animationPath + filenameLoaded + ".xml";
	}

	if (!exists(file))
	{
		filenameLoaded = "";
		errorLog("Could not load skeletal[" + file + "] - File not found.");
		return;
	}

	file = adjustFilenameCase(file);

	XMLDocument *xml = _retrieveSkeletalXML(file, false);
	if(!xml)
	{
		filenameLoaded = "";
		errorLog("Could not load skeletal[" + file + "] - Malformed XML.");
		return;
	}

	loaded = true;

	XMLElement *bones = xml->FirstChildElement("Bones");
	if (bones)
	{
		if (bones->Attribute("scale"))
		{
			SimpleIStringStream is(bones->Attribute("scale"), SimpleIStringStream::REUSE);
			is >> scale.x >> scale.y;
		}

		XMLElement *bone = bones->FirstChildElement("Bone");
		while(bone)
		{
			int idx = atoi(bone->Attribute("idx"));
			int pidx = -1, rbp=0, cr=0, fh=0, fv=0;

			std::string name;
			if (bone->Attribute("pidx"))
				pidx = atoi(bone->Attribute("pidx"));
			if (bone->Attribute("rbp"))
				rbp = atoi(bone->Attribute("rbp"));

			if (bone->Attribute("name"))
				name = bone->Attribute("name");
			if (bone->Attribute("cr"))
				cr = atoi(bone->Attribute("cr"));
			if (bone->Attribute("fh"))
				fh = atoi(bone->Attribute("fh"));
			if (bone->Attribute("fv"))
				fv = atoi(bone->Attribute("fv"));

			std::string gfx = bone->Attribute("gfx");
			Bone *newb = initBone(idx, gfx, pidx, rbp, name, cr, fh, fv);
			if (bone->Attribute("offx"))
				newb->offset.x = atoi(bone->Attribute("offx"));
			if (bone->Attribute("offy"))
				newb->offset.y = atoi(bone->Attribute("offy"));

			if (bone->Attribute("prt"))
			{
				newb->prt = bone->Attribute("prt");
				SimpleIStringStream is(newb->prt.c_str(), SimpleIStringStream::REUSE);
				int slot;
				std::string pfile;
				while (is >> slot)
				{
					if(slot < 0)
					{
						errorLog("particle slot < 0");
						break;
					}
					is >> pfile;
					// add particle system + load
					ParticleEffect *e = new ParticleEffect;
					if(newb->emitters.size() <= (size_t)slot)
						newb->emitters.resize(slot+4, NULL);
					newb->emitters[slot] = e;
					newb->addChild(e, PM_POINTER);
					e->load(pfile);
				}
			}
			XMLElement *fr=0;
			fr = bone->FirstChildElement("Frame");
			int frc=0;
			while(fr)
			{
				Quad *q=0;
				std::string gfx;
				if (fr->Attribute("gfx"))
				{
					gfx = fr->Attribute("gfx");
					q = newb->addFrame(gfx);
				}
				if (fr->Attribute("pass"))
				{
					if (q)
					{
						q->setRenderPass(atoi(fr->Attribute("pass")));
					}
				}
				fr = fr->NextSiblingElement("Frame");
				frc++;
			}
			if (frc)
			{
				newb->showFrame(0);
			}
			if (bone->Attribute("pass"))
			{
				int pass = atoi(bone->Attribute("pass"));
				newb->originalRenderPass = pass;
				newb->setRenderPass(pass);
			}
			if (bone->Attribute("gc"))
			{
				newb->generateCollisionMask = atoi(bone->Attribute("gc"));
			}
			if (bone->Attribute("c"))
			{
				newb->enableCollision = atoi(bone->Attribute("c"));
			}
			if (bone->Attribute("rq"))
			{
				newb->renderQuad = newb->fileRenderQuad = atoi(bone->Attribute("rq"));
			}
			if (bone->Attribute("io"))
			{
				SimpleIStringStream is(bone->Attribute("io"), SimpleIStringStream::REUSE);
				is >> newb->internalOffset.x >> newb->internalOffset.y;
			}

			if (bone->Attribute("strip"))
			{
				SimpleIStringStream is(bone->Attribute("strip"), SimpleIStringStream::REUSE);
				bool vert;
				int num;
				is >> vert >> num;
				newb->createStrip(vert, num);
			}
			if (bone->Attribute("sz"))
			{
				float sx, sy;
				SimpleIStringStream is(bone->Attribute("sz"), SimpleIStringStream::REUSE);
				is >> sx >> sy;

				newb->scale = newb->originalScale = Vector(sx,sy);
			}
			if (bone->Attribute("rt"))
			{
				newb->repeatTextureToFill(true);
			}

			if (bone->Attribute("blend"))
			{
				//if (bone->Attribute("blend")=="add")
				newb->setBlendType(BLEND_ADD);
				//this->setBlendType(BLEND_ADD); // FIXME: seems wrong to do this here -- fg
			}

			if (bone->Attribute("alpha"))
			{
				float a=1.0;
				SimpleIStringStream is(bone->Attribute("alpha"), SimpleIStringStream::REUSE);
				is >> a;
				newb->alpha = a;
			}

			if (bone->Attribute("alphaMod"))
			{
				float a=1.0;
				SimpleIStringStream is(bone->Attribute("alphaMod"), SimpleIStringStream::REUSE);
				is >> a;
				newb->alphaMod = a;
			}

			if (bone->Attribute("segs"))
			{
				int x, y;
				float dgox, dgoy, dgmx, dgmy, dgtm;
				bool dgo;
				SimpleIStringStream is(bone->Attribute("segs"), SimpleIStringStream::REUSE);
				is >> x >> y >> dgox >> dgoy >> dgmx >> dgmy >> dgtm >> dgo;
				newb->setSegs(x, y, dgox, dgoy, dgmx, dgmy, dgtm, dgo);
			}

			if (bone->Attribute("color"))
			{
				SimpleIStringStream in(bone->Attribute("color"), SimpleIStringStream::REUSE);
				in >> newb->color.x >> newb->color.y >> newb->color.z;
			}
			if (bone->Attribute("sel"))
			{
				newb->selectable = bone->BoolAttribute("sel");
			}
			if (bone->Attribute("grid"))
			{
				RenderGrid *grid = newb->getGrid();
				if(!grid)
				{
					SimpleIStringStream is(bone->Attribute("grid"), SimpleIStringStream::REUSE);
					int x, y;
					is >> x >> y;
					grid = newb->createGrid(x, y);
				}
				else
				{
					std::ostringstream os;
					os << "Bone idx " << newb->idx << " already has a DrawGrid, ignoring \"grid\" attribute";
					errorLog(os.str());
				}
				if(const char *gdo = bone->Attribute("gridDrawOrder"))
				{
					int ord = atoi(gdo);
					grid->drawOrder = (GridDrawOrder)ord;
				}
			}
			bone = bone->NextSiblingElement("Bone");
		}
		// attach bones
		for (size_t i = 0; i < this->bones.size(); i++)
		{
			Bone *b = this->bones[i];
			if (b->pidx != -1)
			{
				Bone *pb = getBoneByIdx(b->pidx);
				if (!pb)
				{
					std::ostringstream os;
					os << "Parent bone not found, index: " << b->pidx << " from bone idx: " << b->getIdx();
					debugLog(os.str());
				}
				else if(b == pb) // self-loop would crash
				{
					std::ostringstream os;
					os << "Bone index " << b->pidx << " has itself as parent, this is bad, ignoring";
					errorLog(os.str());
				}
				else
				{
					pb->addChild(b, PM_POINTER);
				}
			}
			else
				addChild(b, PM_POINTER);
		}
	}

	animLayers.clear();
	XMLElement *animationLayers = xml->FirstChildElement("AnimationLayers");
	if (animationLayers)
	{
		XMLElement *animationLayer = animationLayers->FirstChildElement("AnimationLayer");
		while (animationLayer)
		{
			AnimationLayer newAnimationLayer;
			if (animationLayer->Attribute("ignore"))
			{
				SimpleIStringStream is(animationLayer->Attribute("ignore"), SimpleIStringStream::REUSE);
				int t;
				while (is >> t)
				{
					newAnimationLayer.ignoreBones.push_back(t);
				}
			}
			if (animationLayer->Attribute("include"))
			{
				SimpleIStringStream is(animationLayer->Attribute("include"), SimpleIStringStream::REUSE);
				int t;
				while (is >> t)
				{
					newAnimationLayer.includeBones.push_back(t);
				}
			}
			if (animationLayer->Attribute("name"))
			{
				newAnimationLayer.name = animationLayer->Attribute("name");
			}
			newAnimationLayer.setSkeletalSprite(this);
			animLayers.push_back(newAnimationLayer);
			animationLayer = animationLayer->NextSiblingElement("AnimationLayer");
		}
	}

	animations.clear();
	XMLElement *animations = xml->FirstChildElement("Animations");
	if (animations)
	{
		XMLElement *animation = animations->FirstChildElement("Animation");
		while(animation)
		{
			Animation newAnimation;
			newAnimation.name = animation->Attribute("name");
			newAnimation.resetPassOnEnd = animation->BoolAttribute("resetPassOnEnd");
			stringToLower(newAnimation.name);

			XMLElement *key = animation->FirstChildElement("Key");
			while (key)
			{
				SkeletalKeyframe newSkeletalKeyframe;
				if (key->Attribute("e"))
				{
					float time;
					SimpleIStringStream is(key->Attribute("e"), SimpleIStringStream::REUSE);
					is >> time;
					int idx, x, y, rot, strip;
					newSkeletalKeyframe.t = time;
					if (key->Attribute("sound"))
					{
						newSkeletalKeyframe.sound = key->Attribute("sound");
					}
					if (key->Attribute("lerp"))
					{
						newSkeletalKeyframe.lerpType = atoi(key->Attribute("lerp"));
					}
					while (is >> idx)
					{
						BoneKeyframe b;
						is >> x >> y >> rot >> strip;
						b.idx = idx;
						b.x = x;
						b.y = y;
						b.rot = rot;
						if (strip>0)
						{
							b.grid.resize(strip);
							for (size_t i = 0; i < b.grid.size(); i++)
							{
								is >> b.grid[i].x >> b.grid[i].y;

							}
						}
						if (key->Attribute("sz"))
						{
							SimpleIStringStream is2(key->Attribute("sz"), SimpleIStringStream::REUSE);
							int midx;
							float bsx, bsy;
							while (is2 >> midx)
							{
								is2 >> bsx >> bsy;
								if (midx == idx)
								{
									b.doScale = true;
									b.sx = bsx;
									b.sy = bsy;
									break;
								}
							}
						}
						newSkeletalKeyframe.keyframes.push_back(b);
					}

				}
				if (key->Attribute("d"))
				{
					float time;
					SimpleIStringStream is(key->Attribute("d"), SimpleIStringStream::REUSE);
					is >> time;
					int idx, x, y, rot;

					newSkeletalKeyframe.t = time;
					if (key->Attribute("sound"))
					{
						newSkeletalKeyframe.sound = key->Attribute("sound");
					}
					while (is >> idx)
					{
						is >> x >> y >> rot;
						BoneKeyframe b;
						b.idx = idx;
						b.x = x;
						b.y = y;
						b.rot = rot;
						newSkeletalKeyframe.keyframes.push_back(b);
					}
				}
				if (key->Attribute("cmd"))
				{
					newSkeletalKeyframe.cmd = key->Attribute("cmd");
					SimpleIStringStream is(newSkeletalKeyframe.cmd.c_str(), SimpleIStringStream::REUSE);
					int bidx;
					while (is >> bidx)
					{
						Bone *b = this->getBoneByIdx(bidx);
						if (b)
						{
							BoneCommand bcmd;
							if(!bcmd.parse(b, is))
								break;
							newSkeletalKeyframe.commands.push_back(bcmd);
						}
						else
						{
							std::ostringstream os;
							os << "SkeletalSprite::loadSkeletal: File " << fn << " anim " << newAnimation.name << " specifies non-existing bone idx " << bidx;
							errorLog(os.str());
						}
					}
				}
				// generate empty bone keys
				for (size_t i = 0; i < this->bones.size(); i++)
				{
					Bone *bone = this->bones[i];
					BoneKeyframe *bk = newSkeletalKeyframe.getBoneKeyframe(bone->boneIdx);
					if(!bk)
					{
						BoneKeyframe b;
						b.idx = bone->boneIdx;
						newSkeletalKeyframe.keyframes.push_back(b);
					}
				}
				newAnimation.keyframes.push_back(newSkeletalKeyframe);
				key = key->NextSiblingElement("Key");
			}

			// <Interpolator bone="name or idx" type="TYPE config and params" data="controlpoints; aded by editor" />
			XMLElement *interp = animation->FirstChildElement("Interpolator");
			for( ; interp; interp = interp->NextSiblingElement("Interpolator"))
			{
				Bone *bi = NULL;
				const char *sbone = interp->Attribute("bone");
				bool boneByIdx = false;
				if(sbone)
				{
					bi = getBoneByName(sbone);
					if(!bi)
					{
						bi = getBoneByIdx(atoi(sbone));
						boneByIdx = true;
					}
				}
				if(!bi)
				{
					std::ostringstream os;
					os << "Interpolator specifies non-existing bone [" << (sbone ? sbone : "(null)") << "]";
					debugLog(os.str());
					continue;
				}
				RenderGrid *grid = bi->getGrid();
				if(!grid)
				{
					std::ostringstream os;
					os << "Interpolator specifies bone [" << bi->boneIdx << "] that has no grid";
					debugLog(os.str());
					continue;
				}

				SplineType spline = SPLINE_BSPLINE;
				unsigned cx = 3, cy = 3, degx = 3, degy = 3;
				if(const char *stype = interp->Attribute("type"))
				{
					SimpleIStringStream is(stype, SimpleIStringStream::REUSE);
					std::string ty;
					is >> ty;
					BoneGridInterpolator bgip;
					if(ty == "bspline")
					{
						spline = SPLINE_BSPLINE;
						if(!(is >> cx >> cy >> degx >> degy))
						{
							if(!degx)
								degx = 1;
							if(!degy)
								degy = 1;
						}
						if(cx < 2)
							cx = 2;
						if(cy < 2)
							cy = 2;
					}
					else
					{
						errorLog("Unknown interpolator spline type [" + ty + "]");
						continue;
					}
				}

				grid->gridType = GRID_INTERP;
				// bone grid should have been created via <Bone grid=... /> earlier

				const char *idata = interp->Attribute("data");
				newAnimation.interpolators.push_back(BoneGridInterpolator());
				BoneGridInterpolator& bgip = newAnimation.interpolators.back();
				//bgip.type = spline;
				bgip.idx = bi->boneIdx;
				bgip.storeBoneByIdx = boneByIdx;


				// ---- bspline -----
				bgip.bsp.resize(cx, cy, degx, degy);

				const size_t numcp = size_t(cx) * size_t(cy);
				const size_t numgridp = grid->linearsize();

				// data format: "W H [x y x y ... (W*H times)] W H x y x y ..."
				//               ^- start of 1st keyframe  ^- 2nd keyframe
				SimpleIStringStream is(idata ? idata : "",  SimpleIStringStream::REUSE);

				// fixup keyframes and recalc spline points
				for(size_t k = 0; k < newAnimation.keyframes.size(); ++k)
				{
					SkeletalKeyframe& kf = newAnimation.keyframes[k];
					BoneKeyframe *bk = kf.getBoneKeyframe(bgip.idx);

					bk->controlpoints.resize(numcp);
					bgip.bsp.reset(&bk->controlpoints[0]);

					unsigned w = 0, h = 0;
					Vector cp;
					cp.z = 1; // we want all grid points at full alpha

					if((is >> w >> h))
						for(unsigned y = 0; y < h; ++y)
							for(unsigned x = 0; x < w; ++x)
								if((is >> cp.x >> cp.y))
									if(x < cx && y < cy)
										bk->controlpoints[y*size_t(cx) + x] = cp;

					bk->grid.resize(numgridp);
					bgip.updateGridOnly(*bk, bi);
				}
				// ---- end bspline -----
			}

			animation = animation->NextSiblingElement("Animation");
			this->animations.push_back(newAnimation);
		}
	}
}

Animation *SkeletalSprite::getCurrentAnimation(size_t layer)
{
	return layer < animLayers.size() ? animLayers[layer].getCurrentAnimation() : NULL;
}

void SkeletalSprite::setTime(float time, size_t layer)
{
	if(layer < animLayers.size())
		animLayers[layer].timer = time;
}

void AnimationLayer::resetPass()
{
	for (size_t i = 0; i < s->bones.size(); i++)
	{
		Bone *b = s->bones[i];
		if (contains(b))
			b->setRenderPass(b->originalRenderPass);
	}
}

bool AnimationLayer::contains(const Bone *b) const
{
	const int idx = b->boneIdx;
	if (!ignoreBones.empty())
	{
		for (size_t j = 0; j < ignoreBones.size(); j++)
			if (idx == ignoreBones[j])
				return false;
	}
	else if (!includeBones.empty())
	{
		for (size_t j = 0; j < includeBones.size(); j++)
			if (idx == includeBones[j])
				return true;
		return false;
	}

	return true;
}

void AnimationLayer::updateBones()
{
	if (!animating && !(&s->animLayers[0] == this) && fallThru == 0) return;

	SkeletalKeyframe *key1 = getCurrentAnimation()->getPrevKeyframe(timer);
	SkeletalKeyframe *key2 = getCurrentAnimation()->getNextKeyframe(timer);
	if (!key1 || !key2) return;
	float t1 = key1->t;
	float t2 = key2->t;



	float diff = t2-t1;
	float dt;
	if (diff != 0)
		dt = (timer - t1)/(t2-t1);
	else
		dt = 0;

	if (lastNewKey != key2)
	{
		if (!key2->sound.empty())
		{
			core->sound->playSfx(key2->sound);
		}
		if (!key2->commands.empty())
		{
			for (size_t i = 0; i < key2->commands.size(); i++)
			{
				key2->commands[i].run();
			}
		}
		if (s->animKeyNotify)
		{
			s->animKeyNotify->onAnimationKeyPassed(getCurrentAnimation()->getSkeletalKeyframeIndex(lastNewKey));
		}
	}
	lastNewKey = key2;

	for (size_t i = 0; i < s->bones.size(); i++)
	{
		Bone *b = s->bones[i];

		if (b->segmentChain == 1)
		{
			b->updateSegments();
		}
		if (b->segmentChain < 2)
		{
			if (b->animated != Bone::ANIM_NONE && contains(b))
			{
				int idx = b->boneIdx;
				BoneKeyframe *bkey1 = key1->getBoneKeyframe(idx);
				BoneKeyframe *bkey2 = key2->getBoneKeyframe(idx);
				if (bkey1 && bkey2)
				{
					if (!animating && fallThru > 0)
					{
						//HACK: TODO: fix this up nice like below
						Vector p = Vector((bkey2->x-bkey1->x)*dt+bkey1->x, (bkey2->y-bkey1->y)*dt+bkey1->y);
						float rot = (bkey2->rot - bkey1->rot)*dt + bkey1->rot;
						p = Vector((p.x-b->position.x)*fallThru+b->position.x, (p.y-b->position.y)*fallThru+b->position.y);
						rot = (rot-b->rotation.z)*fallThru + b->rotation.z;
						if (b->animated & Bone::ANIM_POS)
							b->position = p;
						if (b->animated & Bone::ANIM_ROT)
							b->rotation.z = rot;
					}
					else
					{
						int lerpType = key2->lerpType;
						//k(0)×(2u3-3u2+1) + k(1)×(3u2-2u3)
						if (b->animated & Bone::ANIM_POS)
						{
							b->position = Vector(lerp(bkey1->x, bkey2->x, dt, lerpType), lerp(bkey1->y, bkey2->y, dt, lerpType));
						}
						if (b->animated & Bone::ANIM_ROT)
						{
							b->rotation.z = lerp(bkey1->rot, bkey2->rot, dt, lerpType);
						}
						if (b->animated==Bone::ANIM_ALL && (bkey1->doScale || bkey2->doScale))
						{
							b->scale.x = lerp(bkey1->sx, bkey2->sx, dt, lerpType);
							b->scale.y = lerp(bkey1->sy, bkey2->sy, dt, lerpType);
						}
						RenderGrid *grid = b->getGrid();
						if (grid && b->animated==Bone::ANIM_ALL && !b->changeStrip.empty() &&  grid->gridType == GRID_STRIP)
						{
							if (bkey2->grid.size() < b->changeStrip.size())
								bkey2->grid.resize(b->changeStrip.size());
							if (bkey1->grid.size() < b->changeStrip.size())
								bkey1->grid.resize(b->changeStrip.size());
							for (size_t i = 0; i < b->changeStrip.size(); i++)
							{
								b->changeStrip[i] = Vector(lerp(bkey1->grid[i].x, bkey2->grid[i].x, dt, lerpType), lerp(bkey1->grid[i].y, bkey2->grid[i].y, dt, lerpType));
							}
							b->setStripPoints(b->stripVert, &b->changeStrip[0], b->changeStrip.size());
						}
						if (grid && b->animated==Bone::ANIM_ALL && grid->gridType == GRID_INTERP)
						{
							const size_t N = grid->linearsize();
							if(bkey1->grid.size() < N)
							{
								bkey1->grid.resize(N);
								RenderGrid::ResetWithAlpha(&bkey1->grid[0], grid->width(), grid->height(), 1.0f);
							}
							if(bkey2->grid.size() < N)
							{
								bkey2->grid.resize(N);
								RenderGrid::ResetWithAlpha(&bkey2->grid[0], grid->width(), grid->height(), 1.0f);
							}

							Vector *dst = grid->data();
							for(size_t i = 0; i < N; ++i)
							{
								dst[i].x = lerp(bkey1->grid[i].x, bkey2->grid[i].x, dt, lerpType);
								dst[i].y = lerp(bkey1->grid[i].y, bkey2->grid[i].y, dt, lerpType);
							}
						}
					}
				}
			}
		}
	}
}

void SkeletalSprite::setFreeze(bool f)
{
	frozen = f;
}

void SkeletalSprite::updateBones()
{
	if (!frozen)
	{
		for (size_t i = 0; i < animLayers.size(); i++)
		{
			animLayers[i].updateBones();
		}
	}


}

bool SkeletalSprite::isAnimating(int layer)
{
	return animLayers[layer].animating;
}

void SkeletalSprite::setTimeMultiplier(float t, int layer)
{
	animLayers[layer].timeMultiplier = t;
}

Bone* SkeletalSprite::getSelectedBone(bool mouseBased)
{
	if (!loaded) return 0;
	if (mouseBased)
	{
		float closestDist = HUGE_VALF;
		Bone *b = 0;
		Vector p = core->mouse.position;
		for (size_t i = 0; i < bones.size(); i++)
		{
			if (bones[i]->renderQuad || core->getShiftState())
			{
				bones[i]->color = Vector(1,1,1);
				if (bones[i]->selectable && bones[i]->renderQuad && bones[i]->isCoordinateInsideWorld(p))
				{
					float dist = (bones[i]->getWorldPosition() - p).getSquaredLength2D();
					if (dist <= closestDist)
					{
						closestDist = dist;
						b = bones[i];
						selectedBone = i;
					}
				}
			}
		}
		if (b)
		{
			b->color = Vector(1,0,0);
		}
		return b;
	}
	// else
	if (!bones.empty() && selectedBone < bones.size())
		return bones[selectedBone];

	return 0;
}


void SkeletalSprite::updateSelectedBoneColor()
{
	for (size_t i = 0; i < bones.size(); i++)
	{
		bones[i]->color = Vector(1,1,1);
	}
	Bone *b = bones[selectedBone];
	if (b)
		b->color = Vector(0.5,0.5,1);
}

void SkeletalSprite::setSelectedBone(int b)
{
	selectedBone = b;
	updateSelectedBoneColor();
}

void SkeletalSprite::selectPrevBone()
{
	const size_t oldsel = selectedBone;
	do
	{
		selectedBone++;
		if(selectedBone == oldsel)
			break;
		if (selectedBone >= bones.size())
			selectedBone = 0;
	}
	while (!bones[selectedBone]->selectable);
	updateSelectedBoneColor();
}

void SkeletalSprite::selectNextBone()
{
	const size_t oldsel = selectedBone;
	do
	{
		selectedBone--;
		if(selectedBone == oldsel)
			break;
		if (selectedBone >= bones.size())
			selectedBone = bones.size()-1;
	}
	while (!bones[selectedBone]->selectable);
	updateSelectedBoneColor();
}

void BoneGridInterpolator::updateGridOnly(BoneKeyframe& bk, const Bone *bone)
{
	const RenderGrid *grid = bone->getGrid();
	assert(bone->boneIdx == bk.idx);
	assert(bk.grid.size() == grid->linearsize());
	bsp.recalc(&bk.grid[0], grid->width(), grid->height(), &bk.controlpoints[0]);

}

void BoneGridInterpolator::updateGridAndBone(BoneKeyframe& bk, Bone *bone)
{
	updateGridOnly(bk, bone);
	Vector *dst = bone->getGrid()->data();
	std::copy(bk.grid.begin(), bk.grid.end(), dst);
}
