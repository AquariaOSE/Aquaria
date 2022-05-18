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
#include "RenderObject.h"
#include "Core.h"
#include "MathFunctions.h"
#include "RenderBase.h"

#include <assert.h>
#include <algorithm>

#ifdef BBGE_USE_GLM
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#endif

bool	RenderObject::renderCollisionShape			= false;
size_t	RenderObject::lastTextureApplied			= 0;
bool	RenderObject::lastTextureRepeat				= false;
bool	RenderObject::renderPaths					= false;

RenderObjectLayer *RenderObject::rlayer				= 0;

void RenderObject::toggleAlpha(float t)
{
	if (alpha.x < 0.5f)
		alpha.interpolateTo(1,t);
	else
		alpha.interpolateTo(0,t);
}

int RenderObject::getTopLayer()
{
	if (parent)
	{
		return parent->getTopLayer();
	}
	return layer;
}

void RenderObject::applyBlendType()
{
	if (blendEnabled)
	{
		glEnable(GL_BLEND);
		switch (blendType)
		{
		case BLEND_DEFAULT:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
		case BLEND_ADD:
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		break;
		case BLEND_SUB:
			glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		break;
		case BLEND_MULT:
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
		}
	}
	else
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}
}

void RenderObject::setColorMult(const Vector &color, const float alpha)
{
	if (colorIsSaved)
	{
		debugLog("setColorMult() WARNING: can't do nested multiplies");
		return;
	}
	this->colorIsSaved = true;
	this->savedColor.x = this->color.x;
	this->savedColor.y = this->color.y;
	this->savedColor.z = this->color.z;
	this->savedAlpha = this->alpha.x;
	this->color *= color;
	this->alpha.x *= alpha;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->setColorMult(color, alpha);
	}
}

void RenderObject::clearColorMult()
{
	if (!colorIsSaved)
	{
		debugLog("clearColorMult() WARNING: no saved color to restore");
		return;
	}
	this->color.x = this->savedColor.x;
	this->color.y = this->savedColor.y;
	this->color.z = this->savedColor.z;
	this->alpha.x = this->savedAlpha;
	this->colorIsSaved = false;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->clearColorMult();
	}
}

RenderObject::RenderObject()
{
	addType(SCO_RENDEROBJECT);
	useOldDT = false;

	updateAfterParent = false;
	ignoreUpdate = false;
	overrideRenderPass = OVERRIDE_NONE;
	renderPass = 0;
	overrideCullRadiusSqr = 0;
	repeatTexture = false;
	alphaMod = 1;
	collideRadius = 0;
	motionBlurTransition = false;
	motionBlurFrameOffsetCounter = 0;
	motionBlurFrameOffset = 0;
	motionBlur = false;
	idx = -1;
	_fv = false;
	_fh = false;
	updateCull = -1;

	layer = LR_NONE;
	cull = true;

	pm = PM_NONE;

	blendEnabled = true;
	texture = 0;
	width = 0;
	height = 0;
	scale = Vector(1,1,1);
	color = Vector(1,1,1);
	alpha.x = 1;

	life = maxLife = 1;
	decayRate = 0;
	_dead = false;
	_hidden = false;
	fadeAlphaWithLife = false;
	blendType = BLEND_DEFAULT;

	followCamera = 0;
	stateData = 0;
	parent = 0;

	renderBeforeParent = false;


	colorIsSaved = false;
	shareAlphaWithChildren = false;
	shareColorWithChildren = false;
	motionBlurTransitionTimer = 0;
}

RenderObject::~RenderObject()
{
}

Vector RenderObject::getWorldPosition()
{
	return getWorldCollidePosition();
}

RenderObject* RenderObject::getTopParent()
{
	RenderObject *p = parent;
	RenderObject *lastp=0;
	while (p)
	{
		lastp = p;
		p = p->parent;
	}
	return lastp;
}

bool RenderObject::isPieceFlippedHorizontal()
{
	RenderObject *p = getTopParent();
	if (p)
		return p->isfh();
	return isfh();
}


Vector RenderObject::getInvRotPosition(const Vector &vec)
{
	glPushMatrix();
	glLoadIdentity();

	std::vector<RenderObject*>chain;
	RenderObject *p = this;
	while(p)
	{
		chain.push_back(p);
		p = p->parent;
	}

	for (int i = chain.size()-1; i >= 0; i--)
	{
		glRotatef(-(chain[i]->rotation.z+chain[i]->rotationOffset.z), 0, 0, 1);

		if (chain[i]->isfh())
		{

			glRotatef(180, 0, 1, 0);
		}
	}

	if (vec.x != 0 || vec.y != 0)
	{

		glTranslatef(vec.x, vec.y, 0);
	}

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float x = m[12];
	float y = m[13];
	float z = m[14];

	glPopMatrix();
	return Vector(x,y,z);
}

#ifdef BBGE_USE_GLM
static glm::mat4 matrixChain(const RenderObject *ro)
{
	glm::mat4 tranformMatrix = glm::scale(
		glm::translate(
			glm::rotate(
				glm::translate(
					ro->getParent() ? matrixChain(ro->getParent()) : glm::mat4(1.0f),
					glm::vec3(ro->position.x+ro->offset.x, ro->position.y+ro->offset.y, 0)
				),
				ro->rotation.z + ro->rotationOffset.z,
				glm::vec3(0, 0, 1)
			),
			glm::vec3(ro->beforeScaleOffset.x, ro->beforeScaleOffset.y, 0.0f)
		),
		glm::vec3(ro->scale.x, ro->scale.y, 0.0f)
	);

	if (ro->isfh())
		tranformMatrix *= glm::rotate(180.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	tranformMatrix *= glm::translate(glm::vec3(ro->internalOffset.x, ro->internalOffset.y, 0.0f));
	return tranformMatrix;
}
#else
static void matrixChain(RenderObject *ro)
{
	if (RenderObject *parent = ro->getParent())
		matrixChain(parent);

	glTranslatef(ro->position.x+ro->offset.x, ro->position.y+ro->offset.y, 0);
	glRotatef(ro->rotation.z+ro->rotationOffset.z, 0, 0, 1);
	glTranslatef(ro->beforeScaleOffset.x, ro->beforeScaleOffset.y, 0);
	glScalef(ro->scale.x, ro->scale.y, 0);
	if (ro->isfh())
	{

		glRotatef(180, 0, 1, 0);
	}
	glTranslatef(ro->internalOffset.x, ro->internalOffset.y, 0);
}
#endif

float RenderObject::getWorldRotation()
{
	Vector up = getWorldCollidePosition(Vector(0,1));
	Vector orig = getWorldPosition();
	float rot = 0;
	MathFunctions::calculateAngleBetweenVectorsInDegrees(orig, up, rot);
	return rot;
}

Vector RenderObject::getWorldPositionAndRotation()
{
	Vector up = getWorldCollidePosition(Vector(0,1));
	Vector orig = getWorldPosition();
	MathFunctions::calculateAngleBetweenVectorsInDegrees(orig, up, orig.z);
	return orig;
}

Vector RenderObject::getWorldCollidePosition(const Vector &vec)
{
#ifdef BBGE_USE_GLM
	glm::mat4 transformMatrix = glm::translate(
		matrixChain(this),
		glm::vec3(vec.x, vec.y, 0.0f)
	);

	return Vector(transformMatrix[3][0], transformMatrix[3][1], 0);
#else
	glPushMatrix();
	glLoadIdentity();

	matrixChain(this);
	glTranslatef(vec.x, vec.y, 0);

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float x = m[12];
	float y = m[13];

	glPopMatrix();
	return Vector(x,y,0);
#endif
}

void RenderObject::fhTo(bool fh)
{
	if ((fh && !_fh) || (!fh && _fh))
	{
		flipHorizontal();
	}
}

void RenderObject::flipHorizontal()
{
	bool wasFlippedHorizontal = _fh;

	_fh = !_fh;

	if (wasFlippedHorizontal != _fh)
	{
		onFH();
	}

}

void RenderObject::flipVertical()
{

	_fv = !_fv;

}

void RenderObject::destroy()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		// must do this first
		// otherwise child will try to remove THIS
		(*i)->parent = 0;
		switch ((*i)->pm)
		{
		case PM_STATIC:
			(*i)->destroy();
			break;
		case PM_POINTER:
			(*i)->destroy();
			delete (*i);
			break;
		}
	}
	children.clear();

	if (parent)
	{
		parent->removeChild(this);
		parent = 0;
	}

	texture = NULL;
}

const RenderObject &RenderObject::operator=(const RenderObject &r)
{
	errorLog("Operator= not defined for RenderObject. Use 'copyProperties'");
	return *this;
}

Vector RenderObject::getRealPosition()
{
	if (parent)
	{
		return position + offset + parent->getRealPosition();
	}
	return position + offset;
}

Vector RenderObject::getRealScale()
{
	if (parent)
	{
		return scale * parent->getRealScale();
	}
	return scale;
}

void RenderObject::setStateDataObject(StateData *state)
{
	stateData = state;
}


void RenderObject::toggleCull(bool value)
{
	cull = value;
}

void RenderObject::moveToFront()
{
	if(RenderObject *p = parent)
	{
		if(p->children.size() && p->children[p->children.size()-1] != this)
		{
			p->removeChild(this);
			p->addChild(this, (ParentManaged)this->pm, RBP_NONE, CHILD_BACK); // To back of list -> rendered on top
		}
	}
	else if (layer != -1)
		core->renderObjectLayers[this->layer].moveToFront(this);
}

void RenderObject::moveToBack()
{
	if(RenderObject *p = parent)
	{
		if(p->children.size() && p->children[0] != this)
		{
			p->removeChild(this);
			p->addChild(this, (ParentManaged)this->pm, RBP_NONE, CHILD_FRONT); // To front of list -> rendered first, below everything else
		}
	}
	else if (layer != -1)
		core->renderObjectLayers[this->layer].moveToBack(this);
}

void RenderObject::enableMotionBlur(int sz, int off)
{
	motionBlur = true;
	motionBlurPositions.resize(sz);
	motionBlurFrameOffsetCounter = 0;
	motionBlurFrameOffset = off;
	for (size_t i = 0; i < motionBlurPositions.size(); i++)
	{
		motionBlurPositions[i].position = position;
		motionBlurPositions[i].rotz = rotation.z;
	}
}

void RenderObject::disableMotionBlur()
{
	motionBlurTransition = true;
	motionBlurTransitionTimer = 1.0;
	motionBlur = false;
}

bool RenderObject::isfhr()
{
	RenderObject *p = this;
	bool fh = false;
	do
		if (p->isfh())
			fh = !fh;
	while ((p = p->parent));
	return fh;

}

bool RenderObject::isfvr()
{
	RenderObject *p = this;
	bool fv = false;
	do
		if (p->isfv())
			fv = !fv;
	while ((p = p->parent));
	return fv;

}

bool RenderObject::hasRenderPass(const int pass)
{
	if (pass == renderPass)
		return true;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && (*i)->hasRenderPass(pass))
			return true;
	}
	return false;
}

void RenderObject::render()
{
	if (isHidden()) return;

	/// new (breaks anything?)
	if (alpha.x == 0 || alphaMod == 0) return;

	if (core->currentLayerPass != RENDER_ALL && renderPass != RENDER_ALL)
	{
		RenderObject *top = getTopParent();
		if (top == NULL && this->overrideRenderPass != OVERRIDE_NONE)
		{
			// FIXME: overrideRenderPass is not applied to the
			// node itself in the original check (below); is
			// that intentional?  Doing the same thing here
			// for the time being.  --achurch
			if (core->currentLayerPass != this->renderPass
			 && core->currentLayerPass != this->overrideRenderPass)
				return;
		}
		else if (top != NULL && top->overrideRenderPass != OVERRIDE_NONE)
		{
			if (core->currentLayerPass != top->overrideRenderPass)
				return;
		}
		else
		{
			if (!hasRenderPass(core->currentLayerPass))
				return;
		}
	}

	if (motionBlur || motionBlurTransition)
	{
		Vector oldPos = position;
		float oldAlpha = alpha.x;
		float oldRotZ = rotation.z;
		for (size_t i = 0; i < motionBlurPositions.size(); i++)
		{
			position = motionBlurPositions[i].position;
			rotation.z = motionBlurPositions[i].rotz;
			alpha = 1.0f-(float(i)/float(motionBlurPositions.size()));
			alpha *= 0.5f;
			if (motionBlurTransition)
			{
				alpha *= motionBlurTransitionTimer;
			}
			renderCall();
		}
		position = oldPos;
		alpha.x = oldAlpha;
		rotation.z = oldRotZ;

		renderCall();
	}
	else
		renderCall();
}

void RenderObject::renderCall()
{
	position += offset;

	glPushMatrix();


	if (layer != LR_NONE)
	{
		RenderObjectLayer *l = &core->renderObjectLayers[layer];
		if (l->followCamera != NO_FOLLOW_CAMERA)
		{
			followCamera = l->followCamera;
		}
	}
	if (followCamera!=0 && !parent)
	{
		if (followCamera == 1)
		{
			glLoadIdentity();
			glScalef(core->globalResolutionScale.x, core->globalResolutionScale.y,0);
			glTranslatef(position.x, position.y, position.z);
			if (isfh())
			{

				glRotatef(180, 0, 1, 0);
			}

			glRotatef(rotation.z+rotationOffset.z, 0, 0, 1);
		}
		else
		{
			Vector pos = getFollowCameraPosition();

			glTranslatef(pos.x, pos.y, pos.z);
			if (isfh())
			{

				glRotatef(180, 0, 1, 0);
			}
			glRotatef(rotation.z+rotationOffset.z, 0, 0, 1);
		}
	}
	else
	{

		glTranslatef(position.x, position.y, position.z);

		if (RenderObject::renderPaths && position.data && position.data->path.getNumPathNodes() > 0)
		{
			glLineWidth(4);
			glEnable(GL_BLEND);

			size_t i = 0;
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
			glBindTexture(GL_TEXTURE_2D, 0);

			glBegin(GL_LINES);
			for (i = 0; i < position.data->path.getNumPathNodes()-1; i++)
			{
				glVertex2f(position.data->path.getPathNode(i)->value.x-position.x, position.data->path.getPathNode(i)->value.y-position.y);
				glVertex2f(position.data->path.getPathNode(i+1)->value.x-position.x, position.data->path.getPathNode(i+1)->value.y-position.y);
			}
			glEnd();

			glPointSize(20);
			glBegin(GL_POINTS);
			glColor4f(0.5,0.5,1,1);
			for (i = 0; i < position.data->path.getNumPathNodes(); i++)
			{
				glVertex2f(position.data->path.getPathNode(i)->value.x-position.x, position.data->path.getPathNode(i)->value.y-position.y);
			}
			glEnd();
		}

		glRotatef(rotation.z+rotationOffset.z, 0, 0, 1);
		if (isfh())
		{

			glRotatef(180, 0, 1, 0);
		}
	}

	glTranslatef(beforeScaleOffset.x, beforeScaleOffset.y, beforeScaleOffset.z);
	glScalef(scale.x, scale.y, 1);
	glTranslatef(internalOffset.x, internalOffset.y, internalOffset.z);


	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && (*i)->renderBeforeParent)
			(*i)->render();
	}


	{
		Vector col = color;
		if (rlayer)
			col *= rlayer->color;

		glColor4f(col.x, col.y, col.z, alpha.x*alphaMod);
	}

	if (texture)
	{

		if (texture->textures[0] != lastTextureApplied || repeatTexture != lastTextureRepeat)
		{
			texture->apply(repeatTexture);
			lastTextureRepeat = repeatTexture;
			lastTextureApplied = texture->textures[0];
		}
	}
	else
	{
		if (lastTextureApplied != 0 || repeatTexture != lastTextureRepeat)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			lastTextureApplied = 0;
			lastTextureRepeat = repeatTexture;
		}
	}

	applyBlendType();


	bool doRender = true;
	int pass = renderPass;
	if (core->currentLayerPass != RENDER_ALL && renderPass != RENDER_ALL)
	{
		RenderObject *top = getTopParent();
		if (top)
		{
			if (top->overrideRenderPass != OVERRIDE_NONE)
				pass = top->overrideRenderPass;
		}

		doRender = (core->currentLayerPass == pass);
	}

	if (renderCollisionShape)
		renderCollision();

	if (doRender)
		onRender();


	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && !(*i)->renderBeforeParent)
			(*i)->render();
	}


	glPopMatrix();


	position -= offset;
}

void RenderObject::renderCollision()
{
	if (collideRadius > 0)
	{
		glPushMatrix();
		glLoadIdentity();
		core->setupRenderPositionAndScale();
		glBindTexture(GL_TEXTURE_2D, 0);
		glTranslatef(position.x+offset.x, position.y+offset.y, 0);

		glTranslatef(internalOffset.x, internalOffset.y, 0);
		glEnable(GL_BLEND);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1,0,0,0.5);
		drawCircle(collideRadius, 8);
		glDisable(GL_BLEND);
		glTranslatef(offset.x, offset.y,0);
		glPopMatrix();
	}
}

void RenderObject::addDeathNotify(RenderObject *r)
{
	deathNotifications.remove(r);
	deathNotifications.push_back(r);
}

void RenderObject::deathNotify(RenderObject *r)
{
	deathNotifications.remove(r);
}

void RenderObject::lookAt(const Vector &pos, float t, float minAngle, float maxAngle, float offset)
{
	Vector myPos = this->getWorldPosition();
	float angle = 0;

	if (myPos.x == pos.x && myPos.y == pos.y)
	{
		return;
	}
	MathFunctions::calculateAngleBetweenVectorsInDegrees(myPos, pos, angle);

	RenderObject *p = parent;
	while (p)
	{
		angle -= p->rotation.z;
		p = p->parent;
	}

	if (isPieceFlippedHorizontal())
	{
		angle = 180-angle;



		offset = -offset;
	}
	angle += offset;
	if (angle < minAngle)
		angle = minAngle;
	if (angle > maxAngle)
		angle = maxAngle;

	int amt = 10;
	if (isPieceFlippedHorizontal())
	{
		if (pos.x < myPos.x-amt)
		{
			angle = 0;
		}
	}
	else
	{
		if (pos.x > myPos.x+amt)
		{
			angle = 0;
		}
	}

	rotation.interpolateTo(Vector(0,0,angle), t);
}

void RenderObject::update(float dt)
{
	if (ignoreUpdate)
	{
		return;
	}
	if (useOldDT)
	{
		dt = core->get_old_dt();
	}
	if (!isDead())
	{

		onUpdate(dt);

		if (isHidden())
			return;

		for (Children::iterator i = children.begin(); i != children.end(); i++)
		{
			if ((*i)->updateAfterParent && (((*i)->pm == PM_POINTER) || ((*i)->pm == PM_STATIC)))
			{
				(*i)->update(dt);
			}
		}
	}
}

void RenderObject::removeChild(RenderObject *r)
{
	r->parent = 0;
	Children::iterator oldend = children.end();
	Children::iterator newend = std::remove(children.begin(), oldend, r);
	if(oldend != newend)
	{
		children.resize(std::distance(children.begin(), newend));
		return;
	}

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->removeChild(r);
	}
}

void RenderObject::enqueueChildDeletion(RenderObject *r)
{
	if (r->parent == this)
	{
		// Don't garbage a child more than once
		for (size_t i = 0; i < childGarbage.size(); ++i)
			if(childGarbage[i] == r)
				return;
		childGarbage.push_back(r);
	}
}

void RenderObject::safeKill()
{
	alpha = 0;
	life = 0;
	onEndOfLife();

	for (RenderObjectList::iterator i = deathNotifications.begin(); i != deathNotifications.end(); i++)
	{
		(*i)->deathNotify(this);
	}

	if (this->parent)
	{
		parent->enqueueChildDeletion(this);

	}
	else
	{
		if (stateData)
			stateData->removeRenderObject(this);
		else
			core->enqueueRenderObjectDeletion(this);
	}
}

Vector RenderObject::getNormal()
{
	float a = MathFunctions::toRadians(getAbsoluteRotation().z);
	return Vector(sinf(a),cosf(a));
}

// HACK: this is probably a slow implementation
Vector RenderObject::getForward()
{
	Vector v = getWorldCollidePosition(Vector(0,-1, 0));
	Vector r = v - getWorldCollidePosition();
	r.normalize2D();


	return r;
}

Vector RenderObject::getAbsoluteRotation()
{
	Vector r = rotation;
	if (parent)
	{
		return parent->getAbsoluteRotation() + r;
	}
	return r;
}

void RenderObject::onUpdate(float dt)
{
	if (isDead()) return;

	updateLife(dt);

	// FIXME: We might not need to do lifetime checks either; I just
	// left that above for safety since I'm not certain.  --achurch
	if (isHidden()) return;

	position += velocity * dt;
	velocity += gravity * dt;
	position.update(dt);
	velocity.update(dt);
	scale.update(dt);
	rotation.update(dt);
	color.update(dt);
	alpha.update(dt);
	offset.update(dt);
	internalOffset.update(dt);
	beforeScaleOffset.update(dt);
	rotationOffset.update(dt);

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (shareAlphaWithChildren)
			(*i)->alpha.x = this->alpha.x;
		if (shareColorWithChildren)
			(*i)->color = this->color;

		if (!(*i)->updateAfterParent && (((*i)->pm == PM_POINTER) || ((*i)->pm == PM_STATIC)))
		{
			(*i)->update(dt);
		}
	}

	if (!childGarbage.empty())
	{
		for (Children::iterator i = childGarbage.begin(); i != childGarbage.end(); i++)
		{
			removeChild(*i);
			(*i)->destroy();
			delete (*i);
		}
		childGarbage.clear();
	}

	if (motionBlur)
	{
		if (motionBlurFrameOffsetCounter >= motionBlurFrameOffset)
		{
			motionBlurFrameOffsetCounter = 0;
			motionBlurPositions[0].position = position;
			motionBlurPositions[0].rotz = rotation.z;
			for (int i = motionBlurPositions.size()-1; i > 0; i--)
			{
				motionBlurPositions[i] = motionBlurPositions[i-1];
			}
		}
		else
			motionBlurFrameOffsetCounter ++;
	}
	if (motionBlurTransition)
	{
		motionBlurTransitionTimer -= dt*2;
		if (motionBlurTransitionTimer <= 0)
		{
			motionBlur = motionBlurTransition = false;
			motionBlurTransitionTimer = 0;
		}
	}


}

void RenderObject::unloadDevice()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->unloadDevice();
	}
}

void RenderObject::reloadDevice()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->reloadDevice();
	}
}

bool RenderObject::setTexture(const std::string &n)
{
	std::string name = n;
	stringToLowerUserData(name);

	if (name.empty())
	{
		setTexturePointer(NULL);
		return false;
	}

	if(texture && texture->getLoadResult() == TEX_SUCCESS && name == texture->name)
		return true; // no texture change

	CountedPtr<Texture> tex = core->addTexture(name);
	setTexturePointer(tex);
	return tex && tex->getLoadResult() == TEX_SUCCESS;
}

void RenderObject::addChild(RenderObject *r, ParentManaged pm, RenderBeforeParent rbp, ChildOrder order)
{
	if (r->parent)
	{
		errorLog("Engine does not support multiple parents");
		return;
	}

	if (order == CHILD_BACK)
		children.push_back(r);
	else
		children.insert(children.begin(), r);

	r->pm = pm;

	if (rbp == RBP_OFF)
		r->renderBeforeParent = 0;
	else if (rbp == RBP_ON)
		r->renderBeforeParent = 1;

	r->parent = this;
}

StateData *RenderObject::getStateData()
{
	if (parent)
	{
		return parent->getStateData();
	}
	else
		return stateData;
}

void RenderObject::setOverrideCullRadius(float ovr)
{
	overrideCullRadiusSqr = ovr * ovr;
}

bool RenderObject::isCoordinateInRadius(const Vector &pos, float r)
{
	Vector d = pos-getRealPosition();

	return (d.getSquaredLength2D() < r*r);
}
