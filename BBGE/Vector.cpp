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
#include "Vector.h"
#include "MathFunctions.h"
#include "Base.h"
#include <float.h>

/*************************************************************************/

void Vector::rotate2D360(int angle)
{
	//float len = this->getLength2D();
	float a = MathFunctions::toRadians(angle);
	float oldx = x, oldy = y;
	x = cosf(a)*oldx - sinf(a)*oldy;
	y = -(sinf(a)*oldx + cosf(a)*oldy);
}

void Vector::rotate2DRad(float rad)
{
	float ox=x,oy=y;
	x = cosf(rad)*ox - sinf(rad)*oy;
	y = sinf(rad)*ox + cosf(rad)*oy;
}


Vector getRotatedVector(const Vector &vec, float rot)
{
#ifdef BBGE_BUILD_OPENGL
	glPushMatrix();
	glLoadIdentity();

	glRotatef(rot, 0, 0, 1);

	if (vec.x != 0 || vec.y != 0)
	{
		//glRotatef(this->rotation.z, 0,0,1,this->rotation.z);
		glTranslatef(vec.x, vec.y, 0);
	}

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float x = m[12];
	float y = m[13];
	float z = m[14];

	glPopMatrix();
	return Vector(x,y,z);
#elif defined(BBGE_BUILD_DIRECTX)
	return vec;
#endif
}

// note update this from float lerp
Vector lerp(const Vector &v1, const Vector &v2, float dt, int lerpType)
{
	switch(lerpType)
	{
		case LERP_EASE:
		{
			// ease in and out
			return v1*(2*(dt*dt*dt)-3*sqr(dt)+1) + v2*(3*sqr(dt) - 2*(dt*dt*dt));
		}
		case LERP_EASEIN:
		{
			float lerpAvg = 1.0f-dt;
			return (v2-v1)*(sinf(dt*PI_HALF)*(1.0f-lerpAvg)+dt*lerpAvg)+v1;
		}
		case LERP_EASEOUT:
		{
			return (v2-v1)*-sinf(-dt*PI_HALF)+v1;
		}
	}

	return (v2-v1)*dt+v1;
}

/*************************************************************************/

float Bias( float x, float biasAmt )
{
	// WARNING: not thread safe
	static float lastAmt = -1;
	static float lastExponent = 0;
	if( lastAmt != biasAmt )
	{
		lastExponent = logf( biasAmt ) * -1.4427f; // (-1.4427 = 1 / log(0.5))
	}
	return powf( x, lastExponent );
}


float Gain( float x, float biasAmt )
{
	// WARNING: not thread safe
	if( x < 0.5f )
		return 0.5f * Bias(2*x, 1-biasAmt);
	else
		return 1 - 0.5f * Bias(2 - 2*x, 1-biasAmt);
}


float SmoothCurve( float x )
{
	return (1 - cosf( x * PI )) * 0.5f;
}


inline float MovePeak( float x, float flPeakPos )
{
	// Todo: make this higher-order?
	if( x < flPeakPos )
		return x * 0.5f / flPeakPos;
	else
		return 0.5f + 0.5f * (x - flPeakPos) / (1 - flPeakPos);
}


float SmoothCurve_Tweak( float x, float flPeakPos, float flPeakSharpness )
{
	float flMovedPeak = MovePeak( x, flPeakPos );
	float flSharpened = Gain( flMovedPeak, flPeakSharpness );
	return SmoothCurve( flSharpened );
}

float SimpleSpline( float value )
{
	float valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return (3 * valueSquared - 2 * valueSquared * value);
}


void VectorPath::addPathNode(Vector v, float p)
{
	VectorPathNode node;
	node.value = v;
	node.percent = p;
	pathNodes.push_back(node);
}

void VectorPath::flip()
{
	std::vector<VectorPathNode> copyNodes;
	copyNodes = pathNodes;
	pathNodes.clear();
	for (int i = copyNodes.size()-1; i >=0; i--)
	{
		copyNodes[i].percent = 1 - copyNodes[i].percent;
		pathNodes.push_back(copyNodes[i]);
	}
}

void VectorPath::realPercentageCalc()
{
	float totalLen = getLength();
	float len = 0;
	for (int i = 1; i < pathNodes.size(); i++)
	{
		Vector diff = pathNodes[i].value - pathNodes[i-1].value;
		len += diff.getLength2D();

		pathNodes[i].percent = len/totalLen;
	}
}

float VectorPath::getSubSectionLength(int startIncl, int endIncl)
{
	float len = 0;
	for (int i = startIncl+1; i <= endIncl; i++)
	{
		Vector diff = pathNodes[i].value - pathNodes[i-1].value;
		len += diff.getLength2D();
	}
	return len;
}

float VectorPath::getLength()
{
	float len = 0;
	for (int i = 1; i < pathNodes.size(); i++)
	{
		Vector diff = pathNodes[i].value - pathNodes[i-1].value;
		len += diff.getLength2D();
	}
	return len;
}

void VectorPath::clear()
{
	pathNodes.clear();
}

void VectorPath::splice(const VectorPath &path, int sz)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	int i = 0;
	for (i = 0; i < path.pathNodes.size(); i++)
		pathNodes.push_back(path.pathNodes[i]);
	for (i = sz+1; i < copy.size(); i++)
		pathNodes.push_back(copy[i]);
	for (i = 0; i < pathNodes.size(); i++)
	{
		pathNodes[i].percent = i/float(pathNodes.size());
	}
}

void VectorPath::removeNodes(int startInclusive, int endInclusive)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	for (int i = 0; i < copy.size(); i++)
	{
		if (i < startInclusive || i > endInclusive)
		{
			pathNodes.push_back(copy[i]);
		}
	}
}

void VectorPath::prepend(const VectorPath &path)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	int i = 0;
	for (i = 0; i < path.pathNodes.size(); i++)
		pathNodes.push_back(path.pathNodes[i]);
	for (i = 0; i < copy.size(); i++)
		pathNodes.push_back(copy[i]);
}

void VectorPath::calculatePercentages()
{
	for (int i = 0; i < pathNodes.size(); i++)
	{
		pathNodes[i].percent = i/float(pathNodes.size());
	}
}

void VectorPath::append(const VectorPath &path)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	int i = 0;
	for (i = 0; i < copy.size(); i++)
		pathNodes.push_back(copy[i]);
	for (i = 0; i < path.pathNodes.size(); i++)
		pathNodes.push_back(path.pathNodes[i]);
}

void VectorPath::subdivide()
{
	/*
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	for (int i = 0; i < copy.size(); i++)
	{
		if (i < 4)
		pathNodes.push_back(i);
	}
	*/
}

void VectorPath::cut(int n)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	for (int i = 0; i < copy.size(); i+=n)
	{
		pathNodes.push_back(copy[i]);
	}
}

void VectorPath::removeNode(int t)
{
	std::vector<VectorPathNode> copy = pathNodes;
	pathNodes.clear();
	for (int i = 0; i < copy.size(); i++)
	{
		if (i != t)
			pathNodes.push_back(copy[i]);
	}
}

Vector VectorPath::getValue(float percent)
{
	if (pathNodes.empty())
	{
		debugLog("Vector path nodes empty");
		return Vector(0,0,0);
	}

	float usePercent = percent;
	VectorPathNode *from = 0, *target = 0;
	from = &pathNodes[0];
	int i = 0;
	for (i = 0; i < pathNodes.size(); i++)
	{
		if (pathNodes[i].percent >= usePercent)
		{
			target = &pathNodes[i];
			break;
		}
		from = &pathNodes[i];
	}

	if (!from && !target)
	{
		msg ("returning first value");
		return pathNodes[0].value;
	}
	else if (!from && target)
	{
		msg("Unexpected Path node result (UPDATE: Could use current value as from?)");
	}
	else if (from && !target)
	{
		// Should only happen at end
//		msg ("returning just a value");
		return from->value;
	}
	else if (from && target && from==target)
	{
		return from->value;
	}
	else if (from && target)
	{
		//bool smoothing = false;
		Vector v;
		float perc=0;
		perc = ((usePercent - from->percent)/(target->percent-from->percent));
		//perc = Gain(perc, 0.8);
		Vector targetValue = target->value;
		Vector fromValue = from->value;
	
		/*
		int nexti = i + 1;
		int previ = i - 1;
		if (perc > 0.5f && nexti < pathNodes.size())
		{
			float scale = ((perc-0.5f)/0.5f) * 0.1f;
			targetValue = targetValue * (1.0f-scale) + pathNodes[nexti].value * scale;
		}
		else if (perc < 0.5f && previ > 0)
		{
			float scale = (1.0f-(perc/0.5f)) * 0.1f;
			targetValue = targetValue * (1.0f-scale) + pathNodes[previ].value * scale;
		}
		*/
		
		v = (targetValue - fromValue) * (perc);
		v += fromValue;
		return v;
		/*
		int nexti = i + 1;
		int previ = i - 1;
		if (smoothing && perc >= 0.5f && nexti < pathNodes.size() && nexti >= 0)
		{
			VectorPathNode *next = &pathNodes[nexti];
			float nextPerc = perc - 0.5f;
			v = (target->value - from->value) * (perc-nextPerc);
			Vector v2 = (next->value - from->value) * nextPerc;
			v = v+v2;
			v += from->value;
		}
		else if (smoothing && perc <= 0.5f && previ < pathNodes.size() && previ >= 0)
		{
			VectorPathNode *prev = &pathNodes[previ];
			float prevPerc = perc + 0.5f;
			v = (target->value - from->value) * (perc-prevPerc);
			Vector v2 = (from->value - prev->value) * prevPerc;
			//v = (v + v2)/2.0f;
			v = v+v2;
			v += from->value;
		}
		else
		{			
			v = (target->value - from->value) * (perc);
			v += from->value;
		}
		*/
		/*
		int nexti = i + 1;
		Vector perp;
		if (smoothing && nexti < pathNodes.size() && nexti >= 0)
		{			
			VectorPathNode *next = &pathNodes[nexti];
			Vector perp = (next->value - from->value);			
			perp = perp.getPerpendicularLeft();
			Vector p = getNearestPointOnLine(from->value, next->value, target->value);
			float dist = (target->value - p).getLength2D();
			if (dist > 0)
			{
				float bulge = sinf(perc * PI);
				perp |= dist;
				perp *= bulge;
			}
		}
		*/

		


		
	}
	return Vector(0,0,0);
}

/*************************************************************************/

float InterpolatedVector::interpolateTo(Vector vec, float timePeriod, int loopType, bool pingPong, bool ease, InterpolateToFlag flag)
{
	if (timePeriod == 0)
	{
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
		return 0;
	}

	InterpolatedVectorData *data = ensureData();

	data->ease = ease;
	data->timePassed = 0;
	//data->fakeTimePassed = 0;
	if (timePeriod < 0)
	{
		timePeriod = -timePeriod;
		timePeriod = (vec-Vector(x,y,z)).getLength3D() / timePeriod;
		/*
		std::ostringstream os;
		os << "calced: " << timePeriod;
		debugLog(os.str());
		*/
	}
	data->timePeriod = timePeriod;
	data->from = Vector (this->x, this->y, this->z);
	data->target = vec;
	
	data->loopType = loopType;
	data->pingPong = pingPong;

	
	if (!data->trigger)
	{
		if (flag != IS_LOOPING)
		{
			data->startOfInterpolationEvent.call();
			data->endOfInterpolationEvent.set(0);
		}
		data->interpolating = true;
	}
	else
		data->pendingInterpolation = true;

	return data->timePeriod;
}

void InterpolatedVector::setInterpolationTrigger(InterpolatedVector *trigger, bool triggerFlag)
{
	InterpolatedVectorData *data = ensureData();
	data->trigger = trigger;
	data->triggerFlag = triggerFlag;
}
void InterpolatedVector::stop()
{
	if (data)
		data->interpolating = false;
}

void InterpolatedVector::startPath(float time, float ease)
{
	InterpolatedVectorData *data = ensureData();

	if (data->path.getNumPathNodes()==0) return;
	data->pathTimer = 0;
	data->pathTime = time;
	data->followingPath = true;
	data->loopType = 0;
	data->pingPong = false;
	data->speedPath = false;
	data->endOfPathEvent.set(0);
	// get the right values to start off with
	updatePath(0);
	data->timeSpeedEase = ease;
	if (ease > 0)
	{		
		data->timeSpeedMultiplier = 0;
	}
	else
	{
		data->timeSpeedMultiplier = 1;
	}
}

void InterpolatedVector::startSpeedPath(float speed)
{
	InterpolatedVectorData *data = ensureData();

	data->ease = false;
	data->currentPathNode = 0;
	data->pathTimer = 0;
	data->pathSpeed = speed;
	data->followingPath = true;
	data->loopType = 0;	
	data->pingPong = false;
	data->speedPath = true;
	updatePath(0);
}

void InterpolatedVector::stopPath()
{
	if (data)
		data->followingPath = false;
}

void InterpolatedVector::resumePath()
{
	InterpolatedVectorData *data = ensureData();
	data->followingPath = true;
}

void InterpolatedVector::updatePath(float dt)
{
	InterpolatedVectorData *data = ensureData();

	if (!data->speedPath)
	{
		if (data->pathTimer > data->pathTime)
		{
			Vector value = data->path.getPathNode(data->path.getNumPathNodes()-1)->value;
			this->x = value.x;
			this->y = value.y;
			this->z = value.z;
			if (data->loopType != 0)
			{
	    			if (data->loopType > 0)
    					data->loopType -= 1;

				int oldLoopType = data->loopType;
				
				if (data->pingPong)
				{
					// flip path
					data->path.flip();
					startPath(data->pathTime);
					data->loopType = oldLoopType;
				}
				else
				{
					startPath(data->pathTime);
					data->loopType = oldLoopType;
				}
			}
			else
			{
				stopPath();
				data->endOfPathEvent.call();
			}
		}
		else
		{
			data->pathTimer += dt * data->pathTimeMultiplier;
				
			//	;//dt*data->timeSpeedMultiplier;
			float perc = data->pathTimer/data->pathTime;
			Vector value = data->path.getValue(perc);
			this->x = value.x;
			this->y = value.y;
			this->z = value.z;

			

			/*
			std::ostringstream os;
			os << "nodes: " << data->path.getNumPathNodes() << " pathTimer: " << data->pathTimer << " pathTime: " << data->pathTime << " perc: " << perc << " p(" << x << ", " << y << ")";
			debugLog(os.str());
			*/
			/*
			float diff = data->pathTime - data->pathTimer;
			if (data->timeSpeedEase > 0)
			{
				float secs = 1.0f/data->timeSpeedEase;
				if (diff <= secs)
				{
					data->timeSpeedMultiplier -= dt*data->timeSpeedEase;
					if (data->timeSpeedMultiplier < 0.1f)
						data->timeSpeedMultiplier = 0.1f;
				}
			}
			if (data->timeSpeedMultiplier < 1)
			{
				data->timeSpeedMultiplier += dt*data->timeSpeedEase;
				if (data->timeSpeedMultiplier >= 1)
					data->timeSpeedMultiplier = 1;
			}
			*/
			
		}
	}
	else
	{
		if (!isInterpolating())
		{
			data->currentPathNode++;
			VectorPathNode *node = data->path.getPathNode(data->currentPathNode);
			/*
			if (node)
			{
				
			}
			else
			{
				stopPath();
				data->endOfPathEvent.call();
			}
			*/
			if (node)
			{
				interpolateTo(node->value, (node->value - Vector(this->x, this->y, this->z)).getLength3D()*(1.0f/data->pathSpeed));
			}
			else
			{
				// handle looping etc
				stopPath();
				data->endOfPathEvent.call();
			}
		}
	}
}

float InterpolatedVector::getPercentDone()
{
	InterpolatedVectorData *data = ensureData();
	return data->timePassed/data->timePeriod;
}

void InterpolatedVector::doInterpolate(float dt)
{
	InterpolatedVectorData *data = ensureData();

	//errorLog ("gothere");
	/*
	// old method
	if (data->ease)
	{
		float diff = data->timePassed / data->timePeriod;
		if (diff > 0.5f)
			diff = 1.0f - diff;
		diff /= 0.5f;
		diff *= 2;
		//diff += 0.5f;
		data->fakeTimePassed += dt*diff;
	}
	*/
	data->timePassed += dt;
 	if (data->timePassed >= data->timePeriod)
	{
	        this->x = data->target.x;
		this->y = data->target.y;
		this->z = data->target.z;
		data->interpolating = false;

		if (data->loopType != 0)
		{
			if (data->loopType > 0)
				data->loopType -= 1;

			if (data->pingPong)
			{
				interpolateTo (data->from, data->timePeriod, data->loopType, data->pingPong, data->ease, IS_LOOPING);
			}
			else
			{
				this->x = data->from.x;
				this->y = data->from.y;
				this->z = data->from.z;
				interpolateTo (data->target, data->timePeriod, data->loopType, data->pingPong, data->ease, IS_LOOPING);
			}
		}
		else
		{
			data->endOfInterpolationEvent.call();
			data->endOfInterpolationEvent.set(0);
		}

	}
	else
	{
		Vector v;

		/*
		// old method
		if (data->ease)
		{
			v = lerp(data->from, data->target, (data->timePassed / data->timePeriod), data->ease);
			//v = (data->target - data->from) * 
			//v = (data->target - data->from) * (data->fakeTimePassed / data->timePeriod);
		}
		else
		{
			float perc = data->timePassed / data->timePeriod;
			v = (data->target - data->from) * perc;
		}

		v += data->from;
		*/

		v = lerp(data->from, data->target, (data->timePassed / data->timePeriod), data->ease ? LERP_EASE : LERP_LINEAR);

		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		//*updatee += data->from;
	}
}
