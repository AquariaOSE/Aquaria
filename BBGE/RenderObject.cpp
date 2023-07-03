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

void RenderObject::toggleAlpha(float t)
{
	if (alpha.x < 0.5f)
		alpha.interpolateTo(1,t);
	else
		alpha.interpolateTo(0,t);
}

int RenderObject::getTopLayer() const
{
	if (parent)
	{
		return parent->getTopLayer();
	}
	return layer;
}

RenderObject::RenderObject()
{
	addType(SCO_RENDEROBJECT);
	useOldDT = false;

	ignoreUpdate = false;
	renderPass = 0;
	overrideCullRadiusSqr = 0;
	repeatTexture = false;
	alphaMod = 1;
	motionBlur = 0;
	idx = -1;
	_fv = false;
	_fh = false;
	_markedForDelete = false;
	updateCull = -1;

	layer = LR_NONE;
	cull = true;

	pm = PM_NONE;

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
	_blendType = BLEND_DEFAULT;

	followCamera = 0;
	stateData = 0;
	parent = 0;

	renderBeforeParent = false;

	shareAlphaWithChildren = false;
	shareColorWithChildren = false;
}

RenderObject::~RenderObject()
{
	freeMotionBlur();
	assert(children.empty()); // if this fires some objects were not deleted and will leak
}

Vector RenderObject::getWorldPosition() const
{
	return getWorldCollidePosition();
}

RenderObject* RenderObject::getTopParent() const
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
				glm::vec3(0.0f, 0.0f, 1.0f)
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

float RenderObject::getWorldRotation() const
{
	Vector up = getWorldCollidePosition(Vector(0,1));
	Vector orig = getWorldPosition();
	float rot = 0;
	MathFunctions::calculateAngleBetweenVectorsInDegrees(orig, up, rot);
	return rot;
}

Vector RenderObject::getWorldPositionAndRotation() const
{
	Vector up = getWorldCollidePosition(Vector(0,1));
	Vector orig = getWorldPosition();
	MathFunctions::calculateAngleBetweenVectorsInDegrees(orig, up, orig.z);
	return orig;
}

Vector RenderObject::getWorldCollidePosition(const Vector &vec) const
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
		(*i)->destroy();
		if((*i)->pm == PM_POINTER)
			delete (*i);
	}
	children.clear();

	if (parent)
	{
		parent->removeChild(this);
		parent = 0;
	}

	texture = NULL;
}

Vector RenderObject::getRealPosition() const
{
	if (parent)
	{
		return position + offset + parent->getRealPosition();
	}
	return position + offset;
}

Vector RenderObject::getRealScale() const
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
	MotionBlurData *mb = ensureMotionBlur();
	mb->transition = false;
	mb->positions.resize(sz);
	mb->frameOffsetCounter = 0;
	mb->frameOffset = off;
	for (size_t i = 0; i < mb->positions.size(); i++)
	{
		mb->positions[i].position = position;
		mb->positions[i].rotz = rotation.z;
	}
}

void RenderObject::disableMotionBlur()
{
	if(MotionBlurData *mb = this->motionBlur)
	{
		mb->transition = true;
		mb->transitionTimer = 1.0;
	}
}

bool RenderObject::isfhr() const
{
	const RenderObject *p = this;
	bool fh = false;
	do
		if (p->isfh())
			fh = !fh;
	while ((p = p->parent));
	return fh;

}

bool RenderObject::isfvr() const
{
	const RenderObject *p = this;
	bool fv = false;
	do
		if (p->isfv())
			fv = !fv;
	while ((p = p->parent));
	return fv;

}

bool RenderObject::hasRenderPass(const int pass) const
{
	if (pass == renderPass)
		return true;
	for (Children::const_iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && (*i)->hasRenderPass(pass))
			return true;
	}
	return false;
}

bool RenderObject::shouldTryToRender() const
{
	return !parent && !_hidden
		&& alpha.x > 0
		&& (!cull || isOnScreen());
}

bool RenderObject::isVisibleInPass(int pass) const
{
	assert(!parent); // This check should be done for root objects only
	assert(pass != RENDER_ALL); // why call this when we already know we don't do passes

	return hasRenderPass(pass);
}

void RenderObject::render(const RenderState& rs) const
{
	assert(parent || layer != LR_NONE);

	if(_hidden)
		return;

	/// new (breaks anything?)
	if (alpha.x == 0 || alphaMod == 0) return;

	if (MotionBlurData *mb = this->motionBlur)
	{
		RenderState rx(rs);
		const size_t sz = mb->positions.size();
		const float m = 1.0f / float(sz);
		const float m2 = alpha.x * 0.5f * (mb->transition ? mb->transitionTimer : 1.0f);
		for (size_t i = 0; i < sz; i++)
		{
			const Vector& renderAt = mb->positions[i].position;
			const float renderRotation = mb->positions[i].rotz;
			rx.alpha = (1.0f-(float(i) * m)) * m2;
			renderCall(rx, renderAt, renderRotation);
		}
	}

	renderCall(rs, position, rotation.z);
}

void RenderObject::renderCall(const RenderState& rs, const Vector& renderAt, float renderRotation) const
{
	const Vector renderPos = renderAt + offset;
	renderRotation += rotationOffset.z;

	glPushMatrix();

	if(!parent)
	{
		// Is root object. followCamera has an influence.
		float followCamera = this->followCamera;
		if (!followCamera)
		{
			// Not set for object. Use global layer value
			const RenderObjectLayer& rl = core->renderObjectLayers[layer];
			followCamera = rl.followCamera;
			if(followCamera == 0) // normal object on normal layer
				goto nofollow;
		}

		if (followCamera == 1) // UI overlay or similar; is independent of camera aka stays in the same spot on the screen
		{
			glLoadIdentity();
			glScalef(core->globalResolutionScale.x, core->globalResolutionScale.y,0);
			glTranslatef(renderPos.x, renderPos.y, renderPos.z);
		}
		else // parallax scrolling
		{
			Vector pos = getFollowCameraPosition();
			glTranslatef(pos.x, pos.y, pos.z);
		}
	}
	else
	{
nofollow:
		// The vast majority of objects ends up here. We're a child, or followCamera == 0 and not on a parallax layer.

		glTranslatef(renderPos.x, renderPos.y, renderPos.z);

		if (RenderObject::renderPaths) // TODO: move this to debug render
			debugRenderPaths();
	}

	// Apply rotation and flip
	glRotatef(renderRotation, 0, 0, 1);
	if (isfh())
		glRotatef(180, 0, 1, 0);

	glTranslatef(beforeScaleOffset.x, beforeScaleOffset.y, beforeScaleOffset.z);
	const Vector renderScale = scale * rs.scale;
	glScalef(renderScale.x, renderScale.y, 1);
	glTranslatef(internalOffset.x, internalOffset.y, internalOffset.z);

	for (Children::const_iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && (*i)->renderBeforeParent)
			(*i)->render(rs);
	}


	if (rs.pass == RENDER_ALL || renderPass == RENDER_ALL || rs.pass == renderPass)
	{
		if (texture)
		{
			if (texture->gltexid != lastTextureApplied || repeatTexture != lastTextureRepeat)
			{
				texture->apply(repeatTexture);
				lastTextureRepeat = repeatTexture;
				lastTextureApplied = texture->gltexid;
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

		rs.gpu.setBlend(getBlendType());

		// RenderState color applies to everything in the scene graph,
		// so that needs to be multiplied in unconditionally
		{
			Vector col = this->color * rs.color;
			glColor4f(col.x, col.y, col.z, rs.alpha*alpha.x*alphaMod);
		}

		onRender(rs);

		if (renderCollisionShape)
			renderCollision(rs);
	}


	for (Children::const_iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && !(*i)->renderBeforeParent)
			(*i)->render(rs);
	}


	glPopMatrix();
}

void RenderObject::renderCollision(const RenderState& rs) const
{
}

void RenderObject::debugRenderPaths() const
{
	if(!position.data)
		return;

	const size_t N = position.data->path.getNumPathNodes();
	if(!N)
		return;

	glLineWidth(4);
	glEnable(GL_BLEND);

	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBegin(GL_LINES);
	for (size_t i = 0; i < N-1; i++)
	{
		const VectorPathNode a = position.data->path[i];
		const VectorPathNode b = position.data->path[i+1];
		glVertex2f(a.value.x-position.x, a.value.y-position.y);
		glVertex2f(b.value.x-position.x, b.value.y-position.y);
	}
	glEnd();

	glPointSize(20);
	glBegin(GL_POINTS);
	glColor4f(0.5,0.5,1,1);
	for (size_t i = 0; i < N; i++)
	{
		const VectorPathNode& a = position.data->path[i];
		glVertex2f(a.value.x-position.x, a.value.y-position.y);
	}
	glEnd();
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

	const bool ishfh = this->isfhr();

	if (ishfh)
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
	if (ishfh)
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
		_markedForDelete = true;
	}
	else
	{
		if (stateData)
			stateData->removeRenderObject(this);
		else
			core->enqueueRenderObjectDeletion(this);
	}
}

Vector RenderObject::getNormal() const
{
	float a = MathFunctions::toRadians(getAbsoluteRotation().z);
	return Vector(sinf(a),cosf(a));
}

// HACK: this is probably a slow implementation
Vector RenderObject::getForward() const
{
	Vector v = getWorldCollidePosition(Vector(0,-1, 0));
	Vector r = v - getWorldCollidePosition();
	r.normalize2D();


	return r;
}

Vector RenderObject::getAbsoluteRotation() const
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

	if(!updateLife(dt))
		return;

	// FIXME: We might not need to do lifetime checks either; I just
	// left that above for safety since I'm not certain.  --achurch
	if (isHidden()) return;

	position += velocity * dt;
	velocity += gravity * dt;
	position.update(dt);
	velocity.update(dt);
	gravity.update(dt);
	scale.update(dt);
	rotation.update(dt);
	color.update(dt);
	alpha.update(dt);
	offset.update(dt);
	internalOffset.update(dt);
	beforeScaleOffset.update(dt);
	rotationOffset.update(dt);

	bool hasChildrenToDelete = false;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (shareAlphaWithChildren)
			(*i)->alpha.x = this->alpha.x;
		if (shareColorWithChildren)
			(*i)->color = this->color;

		if ((*i)->pm != PM_NONE)
		{
			(*i)->update(dt);
		}
		hasChildrenToDelete |= (*i)->_markedForDelete;

	}

	if (hasChildrenToDelete)
	{
		size_t w = 0;
		const size_t N = children.size();
		for (size_t i = 0; i < N; ++i)
		{
			RenderObject *ro = children[i];
			if(ro->_markedForDelete)
			{
				ro->parent = NULL;
				ro->destroy();
				if(ro->pm == PM_POINTER)
					delete ro;
			}
			else
				children[w++] = ro;
		}
		children.resize(w);
	}

	if (MotionBlurData *mb = this->motionBlur)
	{
		if(!mb->transition)
		{
			if (mb->frameOffsetCounter >= mb->frameOffset)
			{
				mb->frameOffsetCounter = 0;
				mb->positions[0].position = position;
				mb->positions[0].rotz = rotation.z;
				for (int i = mb->positions.size()-1; i > 0; i--)
				{
					mb->positions[i] = mb->positions[i-1];
				}
			}
			else
				mb->frameOffsetCounter ++;
		}
		else
		{
			mb->transitionTimer -= dt*2;
			if (mb->transitionTimer <= 0)
				freeMotionBlur();
		}
	}
}

bool RenderObject::updateLife(float dt)
{
	if (decayRate > 0)
	{
		life -= decayRate*dt;
		if (life<=0)
		{
			safeKill();
			return false;
		}
	}
	if (fadeAlphaWithLife && !alpha.isInterpolating())
	{

		alpha = life/maxLife;
	}
	return true;
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

MotionBlurData* RenderObject::ensureMotionBlur()
{
	MotionBlurData *mb = this->motionBlur;
	if(!mb)
	{
		mb = new MotionBlurData;
		this->motionBlur = mb;
	}
	return mb;
}

void RenderObject::freeMotionBlur()
{
	if(motionBlur)
	{
		delete motionBlur;
		motionBlur = NULL;
	}
}

bool RenderObject::setTexture(const std::string &n)
{
	CountedPtr<Texture> tex = core->getTexture(n);
	setTexturePointer(tex);
	return tex->success;
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

StateData *RenderObject::getStateData() const
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

bool RenderObject::isCoordinateInRadius(const Vector &pos, float r) const
{
	Vector d = pos-getRealPosition();

	return (d.getSquaredLength2D() < r*r);
}

MotionBlurData::MotionBlurData()
	: transition(false), frameOffsetCounter(0), frameOffset(0), transitionTimer(0)
{
}
