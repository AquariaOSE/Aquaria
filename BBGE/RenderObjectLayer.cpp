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
#include "Core.h"
#include "RenderBase.h"

	#define BASE_ARRAY_SIZE 100  // Size of an object array in a new layer

RenderObjectLayer::RenderObjectLayer()
	: renderObjects(BASE_ARRAY_SIZE)
{
	followCamera = 0;
	visible = true;
	startPass = endPass = 0;
	followCameraLock = FCL_NONE;
	update = true;

	const int size = renderObjects.size();
	for (int i = 0; i < size; i++)
		renderObjects[i] = 0;
	objectCount = 0;
	firstFreeIdx = 0;
}

RenderObjectLayer::~RenderObjectLayer()
{
}

void RenderObjectLayer::add(RenderObject* r)
{
	size_t size = renderObjects.size();
	if (firstFreeIdx >= size)
	{
		size += size/2;  // Increase size by 50% each time we fill up.
		renderObjects.resize(size);
	}

	renderObjects[firstFreeIdx] = r;
	objectCount++;
	r->setIdx(firstFreeIdx);

	for (; firstFreeIdx < size; firstFreeIdx++)
	{
		if (!renderObjects[firstFreeIdx])
			break;
	}
}

void RenderObjectLayer::remove(RenderObject* r)
{
	const size_t idx = r->getIdx();
	if (idx >= renderObjects.size())
	{
		errorLog("Trying to remove RenderObject with invalid index");
		return;
	}
	if (renderObjects[idx] != r)
	{
		errorLog("RenderObject pointer doesn't match array");
		return;
	}
	renderObjects[idx] = 0;
	objectCount--;
	if (idx < firstFreeIdx)
		firstFreeIdx = idx;
	r->setIdx(-1);
}

void RenderObjectLayer::moveToFront(RenderObject *r)
{
	const size_t size = renderObjects.size();
	const size_t curIdx = r->getIdx();
	size_t lastUsed;
	for (lastUsed = size-1; lastUsed > curIdx; lastUsed--)
	{
		if (renderObjects[lastUsed])
			break;
	}

	if (curIdx == lastUsed)
	{
		// Already at the front, so nothing to do.
	}
	else if (lastUsed < size-1)
	{
		const size_t newIdx = lastUsed + 1;
		renderObjects[curIdx] = 0;
		renderObjects[newIdx] = r;
		r->setIdx(newIdx);
		if (firstFreeIdx > curIdx)
			firstFreeIdx = curIdx;
	}
	else if (objectCount == size)
	{
		// Expand the array so future calls have a bit of breathing room.
		const size_t newSize = size + 10;
		renderObjects.resize(newSize);
		renderObjects[curIdx] = 0;
		renderObjects[size] = r;
		r->setIdx(size);
		for (size_t i = size+1; i < newSize; i++)
			renderObjects[i] = 0;
		if (firstFreeIdx > curIdx)
			firstFreeIdx = curIdx;
	}
	else
	{
		// Need to shift elements downward to make room for the new one.
		renderObjects[curIdx] = 0;
		size_t lastFree;
		for (lastFree = lastUsed-1; lastFree > curIdx; lastFree--)
		{
			if (!renderObjects[lastFree])
				break;
		}
		for (size_t i = lastFree + 1; i <= lastUsed; i++)
		{
			renderObjects[i-1] = renderObjects[i];
			renderObjects[i-1]->setIdx(i-1);  // Known to be non-NULL.
		}
		renderObjects[lastUsed] = r;
		r->setIdx(lastUsed);
		firstFreeIdx = 0;
		// Known to have at least one NULL-element
		while (renderObjects[firstFreeIdx])
			firstFreeIdx++;
	}
}

void RenderObjectLayer::moveToBack(RenderObject *r)
{
	const size_t size = renderObjects.size();
	const size_t curIdx = r->getIdx();
	size_t firstUsed;
	for (firstUsed = 0; firstUsed < curIdx; firstUsed++)
	{
		if (renderObjects[firstUsed])
			break;
	}

	if (curIdx == firstUsed)
	{
		// Already at the back, so nothing to do.
	}
	else if (firstUsed > 0)
	{
		const int newIdx = firstUsed - 1;
		renderObjects[curIdx] = 0;
		renderObjects[newIdx] = r;
		r->setIdx(newIdx);
		// firstFreeIdx must be 0 here; if we filled slot 0, then
		// scan forward for the next empty element.
		while (renderObjects[firstFreeIdx])
			firstFreeIdx++;
	}
	else if (objectCount == size)
	{
		const size_t newSize = size + 10;
		const size_t sizeDiff = newSize - size;
		const size_t newIdx = sizeDiff - 1;

		renderObjects.resize(newSize);
		renderObjects[curIdx] = 0;
		for (size_t i = newSize - 1; i >= sizeDiff; i--)
		{
			renderObjects[i] = renderObjects[i - sizeDiff];
			if(renderObjects[i])
				renderObjects[i]->setIdx(i);
		}
		for (size_t i = 0; i < newIdx; i++)
			renderObjects[i] = 0;
		renderObjects[newIdx] = r;
		r->setIdx(newIdx);
		firstFreeIdx = 0;
	}
	else
	{
		renderObjects[curIdx] = 0;
		if (curIdx < firstFreeIdx)
			firstFreeIdx = curIdx;
		for (int i = firstFreeIdx; i > 0; i--)
		{
			renderObjects[i] = renderObjects[i-1];
			renderObjects[i]->setIdx(i);  // Known to be non-NULL.
		}
		renderObjects[0] = r;
		r->setIdx(0);
		for (firstFreeIdx++; firstFreeIdx < size; firstFreeIdx++)
		{
			if (!renderObjects[firstFreeIdx])
				break;
		}
	}
}

void RenderObjectLayer::reloadDevice()
{
}

void RenderObjectLayer::prepareRender()
{
	toRender.clear();

	size_t n = 0;
	for (const RenderObject *robj = getFirst(); robj; robj = getNext())
	{
		++n;
		if(robj->shouldTryToRender())
			toRender.push_back(robj);
	}
	core->renderObjectCount += toRender.size();
	toRender.push_back(NULL); // terminate
	core->totalRenderObjectCount += n;

	switch(followCameraLock)
	{
		default:
		case FCL_NONE: followCameraMult = Vector(1, 1); break; // both H and V affected
		case FCL_HORZ: followCameraMult = Vector(1, 0); break; // only H affected
		case FCL_VERT: followCameraMult = Vector(0, 1); break; // only V affected
	}
}

void RenderObjectLayer::render() const
{
	if(toRender.size() <= 1)
		return;

	size_t proc = 0;
	CombinedRenderAndGPUState rs;

	if (startPass == endPass)
	{
		rs.pass = RenderObject::RENDER_ALL;
		const RenderObject * const * rlist = &toRender[0]; // known to have at least one element
		while(const RenderObject *ro = *rlist++)
			ro->render(rs);
		proc += toRender.size() - 1;
	}
	else
	{
		for (int pass = startPass; pass <= endPass; pass++)
		{
			rs.pass = pass;
			const RenderObject * const * rlist = &toRender[0]; // known to have at least one element
			while(const RenderObject *ro = *rlist++)
				if(ro->isVisibleInPass(pass))
				{
					ro->render(rs);
					++proc;
				}
		}
	}
	core->processedRenderObjectCount += proc;
}
