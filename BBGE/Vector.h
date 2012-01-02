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
#pragma once

#include <math.h>
#include <float.h>
#include <vector> 
#include "Event.h"

#ifdef BBGE_BUILD_DIRECTX
	#include <d3dx9.h>
#endif
typedef float scalar_t;

class Vector
{
public:
     scalar_t x;
     scalar_t y;
     scalar_t z;    // x,y,z coordinates

     Vector(scalar_t a = 0, scalar_t b = 0, scalar_t c = 0) : x(a), y(b), z(c) {}
     Vector(const Vector &vec) : x(vec.x), y(vec.y), z(vec.z) {}


     float inline *getv(float *v) const
	 {
		 v[0] = x; v[1] = y; v[2] = z;
		 return v;
	 }

	 float inline *getv4(float *v, float param) const
	 {
		 v[0] = x; v[1] = y; v[2] = z; v[3] = param;
		 return v;
	 }
		 
	 // vector assignment
     const Vector &operator=(const Vector &vec)
     {
          x = vec.x;
          y = vec.y;
          z = vec.z;

          return *this;
     }

     // vecector equality
     const bool operator==(const Vector &vec) const
     {
          return ((x == vec.x) && (y == vec.y) && (z == vec.z));
     }

     // vecector inequality
     const bool operator!=(const Vector &vec) const
     {
          return !(*this == vec);
     }

     // vector add
     const Vector operator+(const Vector &vec) const
     {
          return Vector(x + vec.x, y + vec.y, z + vec.z);
     }

     // vector add (opposite of negation)
     const Vector operator+() const
     {    
          return Vector(*this);
     }

     // vector increment
     const Vector& operator+=(const Vector& vec)
     {    x += vec.x;
          y += vec.y;
          z += vec.z;
          return *this;
     }

     // vector subtraction
     const Vector operator-(const Vector& vec) const
     {    
          return Vector(x - vec.x, y - vec.y, z - vec.z);
     }
     
     // vector negation
     const Vector operator-() const
     {    
          return Vector(-x, -y, -z);
     }
	 bool isZero()
	 {
		 return x == 0 && y == 0 && z == 0;
	 }

     // vector decrement
     const Vector &operator-=(const Vector& vec)
     {
          x -= vec.x;
          y -= vec.y;
          z -= vec.z;

          return *this;
     }

     // scalar self-multiply
     const Vector &operator*=(const scalar_t &s)
     {
          x *= s;
          y *= s;
          z *= s;
          
          return *this;
     }

     // scalar self-divecide
     const Vector &operator/=(const scalar_t &s)
     {
          const float recip = 1/s; // for speed, one divecision

          x *= recip;
          y *= recip;
          z *= recip;

          return *this;
     }

	 // vector self-divide
     const Vector &operator/=(const Vector &v)
	 {
          x /= v.x;
          y /= v.y;
          z /= v.z;

          return *this;
     }

     const Vector &operator*=(const Vector &v)
	 {
          x *= v.x;
          y *= v.y;
          z *= v.z;

          return *this;
     }


     // post multiply by scalar
     const Vector operator*(const scalar_t &s) const
     {
          return Vector(x*s, y*s, z*s);
     }

	 // post multiply by Vector
     const Vector operator*(const Vector &v) const
     {
          return Vector(x*v.x, y*v.y, z*v.z);
     }

     // pre multiply by scalar
     friend inline const Vector operator*(const scalar_t &s, const Vector &vec)
     {
          return vec*s;
     }

/*   friend inline const Vector operator*(const Vector &vec, const scalar_t &s)
     {
          return Vector(vec.x*s, vec.y*s, vec.z*s);
     }
*/
   // divecide by scalar
     const Vector operator/(scalar_t s) const
     {
          s = 1/s;

          return Vector(s*x, s*y, s*z);
     }


     // cross product
     const Vector CrossProduct(const Vector &vec) const
     {
          return Vector(y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x);
     }

	 Vector inline getPerpendicularLeft()
	 {
		 return Vector(-y, x);
	 }

 	 Vector inline getPerpendicularRight()
	 {
		 return Vector(y, -x);
	 }

     // cross product
     const Vector operator^(const Vector &vec) const
     {
          return Vector(y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x);
     }

     // dot product
     const scalar_t inline dot(const Vector &vec) const
     {
          return x*vec.x + y*vec.y + z*vec.z;
     }

	 const scalar_t inline dot2D(const Vector &vec) const
	 {
		 return x*vec.x + y*vec.y;
	 }

     // dot product
     const scalar_t operator%(const Vector &vec) const
     {
          return x*vec.x + y*vec.x + z*vec.z;
     }


     // length of vector
     const scalar_t inline getLength3D() const
     {
          return (scalar_t)sqrtf(x*x + y*y + z*z);
     }
     const scalar_t inline getLength2D() const
     {
          return (scalar_t)sqrtf(x*x + y*y);
     }

     // return the unit vector
	 const Vector inline unitVector3D() const
	 {
		return (*this) * (1/getLength3D());
	 }

     // normalize this vector
	 void inline normalize3D()
	 {
		if (x == 0 && y == 0 && z == 0)
		{
			//debugLog("Normalizing 0 vector");
			x = y = z = 0;
		}
		else
		{
			(*this) *= 1/getLength3D();
		}
	 }
	 void inline normalize2D()
	 {
		if (x == 0 && y == 0)
		{
			//debugLog("Normalizing 0 vector");
			x = y = z= 0;
		}
		else
		{
			(*this) *= 1/getLength2D();
		}
	 }

     const scalar_t operator!() const
     {
          return sqrtf(x*x + y*y + z*z);
     }

	 /*
     // return vector with specified length
     const Vector operator | (const scalar_t length) const
     {
          return *this * (length / !(*this));
     }

     // set length of vector equal to length
     const Vector& operator |= (const float length)
     {
          (*this).setLength2D(length);
		  return *this;
     }
	 */

	 void inline setLength3D(const float l)
	 {
		// IGNORE !!
		if (l == 0)
		{
			//debugLog("setLength3D divide by 0");
		}
		else
		{
			float len = getLength3D();
			this->x *= (l/len);
			this->y *= (l/len);
			this->z *= (l/len);
		}
	 }
	 void inline setLength2D(const float l)
	 {
		float len = getLength2D();
		if (len == 0)
		{
			//debugLog("divide by zero!");
		}
		else
		{
			this->x *= (l/len);
			this->y *= (l/len);
		}
		//this->z = 0;
	 }

     // return angle between two vectors
     const float inline Angle(const Vector& normal) const
     {
          return acosf(*this % normal);
     }

	 /*
	 const scalar_t inline cheatLen() const
	 {
			return (x*x + y*y + z*z);
	 }
	 const scalar_t inline cheatLen2D() const
	 {
		 return (x*x + y*y);
	 }
	 const scalar_t inline getCheatLength3D() const;
	 */

	 const bool inline isLength2DIn(float radius) const
	 {
		return (x*x + y*y) <= (radius*radius);
	 }

     // reflect this vector off surface with normal vector
	 /*
     const Vector inline Reflection(const Vector& normal) const
     {    
          const Vector vec(*this | 1);     // normalize this vector
          return (vec - normal * 2.0f * (vec % normal)) * !*this;
     }
	 */

	 const void inline setZero()
	 {
		this->x = this->y = this->z = 0;
	 }
	 const float inline getSquaredLength2D() const
	 {
		return (x*x) + (y*y);
	 }
	 const bool inline isZero() const
	 {
		return x==0 && y==0 && z==0;
	 }

	 const bool inline isNan() const
	 {
#ifdef BBGE_BUILD_WINDOWS
		return _isnan(x) || _isnan(y) || _isnan(z);
#elif defined(BBGE_BUILD_UNIX)
		return isnan(x) || isnan(y) || isnan(z);
#else
		return false;
#endif
	 }

	 void inline capLength2D(const float l)
	 {
		if (!isLength2DIn(l))	setLength2D(l);
	 }
	 void inline capRotZ360()
	 {
		while (z > 360)
			z -= 360;
		while (z < 0)
			z += 360;
	 }

#ifdef BBGE_BUILD_DIRECTX
	 const D3DCOLOR getD3DColor(float alpha)
	 {
		 return D3DCOLOR_RGBA(int(x*255), int(y*255), int(z*255), int(alpha*255));
	 }
#endif
	 void rotate2DRad(float rad);
	 void rotate2D360(int angle);
};


class VectorPathNode
{
public:
	VectorPathNode() { percent = 0; }

	Vector value;
	float percent;
};

class VectorPath
{
public:
	void flip();
	void clear();
	void addPathNode(Vector v, float p);
	Vector getValue(float percent);
	int getNumPathNodes() { return pathNodes.size(); }
	void resizePathNodes(int sz) { pathNodes.resize(sz); }
	VectorPathNode *getPathNode(int i) { if (i<getNumPathNodes() && i >= 0) return &pathNodes[i]; return 0; }
	void cut(int n);
	void splice(const VectorPath &path, int sz);
	void prepend(const VectorPath &path);
	void append(const VectorPath &path);
	void removeNode(int i);
	void calculatePercentages();
	float getLength();
	void realPercentageCalc();
	void removeNodes(int startInclusive, int endInclusive);
	float getSubSectionLength(int startIncl, int endIncl);
	void subdivide();
protected:
	std::vector <VectorPathNode> pathNodes;
};


class InterpolatedVector;
struct InterpolatedVectorData
{
	InterpolatedVectorData()
	{
		trigger = 0;
		triggerFlag = false;
		pendingInterpolation = false;
		interpolating = false;
		pingPong = false;
		loopType = 0;
		pathTimer = 0;
		pathTime = 0;
		pathSpeed = 1;
		currentPathNode = 0;
		pathTimeMultiplier = 1;
		timePassed = 0;
		timePeriod = 0;
		timeSpeedMultiplier = 1;
		timeSpeedEase = 0;
		//fakeTimePassed = 0;
		speedPath = false;
		ease = false;
		followingPath = false;
	}

	InterpolatedVector *trigger;
	bool triggerFlag;
	bool pendingInterpolation;

	bool interpolating;
	bool pingPong;
	int loopType;

	EventPtr endOfInterpolationEvent;
	EventPtr startOfInterpolationEvent;
	EventPtr endOfPathEvent;

	VectorPath path;
	float pathTimer, pathTime;
	float pathSpeed;
	int currentPathNode;
	float pathTimeMultiplier;

	float timePassed, timePeriod;
	Vector target;
	Vector from;

	float timeSpeedMultiplier, timeSpeedEase;
	//float fakeTimePassed;
	bool speedPath;
	bool ease;
	bool followingPath;
};


// This struct is used to keep all of the interpolation-specific data out
// of the global InterpolatedVector class, so that we don't waste memory on
// non-interpolated vectors.
class InterpolatedVector : public Vector
{
public:
	InterpolatedVector(scalar_t a = 0, scalar_t b = 0, scalar_t c = 0) : Vector(a,b,c), data(NULL) {}
	InterpolatedVector(const Vector &vec) : Vector(vec), data(NULL) {}
	~InterpolatedVector() {delete data;}

	InterpolatedVector(const InterpolatedVector &vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		if (vec.data)
			data = new InterpolatedVectorData(*vec.data);
		else
			data = NULL;
	}
	InterpolatedVector &operator=(const InterpolatedVector &vec)
	{
		x = vec.x;
		y = vec.y;
		z = vec.z;
		delete data;
		if (vec.data)
			data = new InterpolatedVectorData(*vec.data);
		else
			data = NULL;
		return *this;
	}

	void setInterpolationTrigger(InterpolatedVector *trigger, bool triggerFlag);
	enum InterpolateToFlag { NONE=0, IS_LOOPING };
	float interpolateTo (Vector vec, float timePeriod, int loopType = 0, bool pingPong = false, bool ease = false, InterpolateToFlag flag = NONE);
	void inline update(float dt)
	{
		if (!data)
			return;

		if (data->pendingInterpolation && data->trigger)
		{
			if (data->trigger->isInterpolating() == data->triggerFlag)
			{
				data->interpolating = true;
				data->pendingInterpolation = false;
			}
			else
				return;
		}
		if (isFollowingPath())
		{
			updatePath(dt);
		}
		if (isInterpolating())
		{
			doInterpolate(dt);
		}
	}

	void doInterpolate(float dt);

	inline bool isInterpolating() const
	{
		return data && data->interpolating;
	}

	void startPath(float time, float ease=0);
	void startSpeedPath(float speed);
	void stopPath();
	void resumePath();

	void updatePath(float dt);

	void stop();

	float getPercentDone();

	inline bool isFollowingPath() const
	{
		return data && data->followingPath;
	}

	// for faking a single value
	inline float getValue() const
	{
		return x;
	}


	// We never allocate this if the vector isn't used for
	// interpolation, which saves a _lot_ of memory.
	InterpolatedVectorData *data;

	inline InterpolatedVectorData *ensureData(void)
	{
		if (!data)
			data = new InterpolatedVectorData;
		return data;
	}
};

Vector getRotatedVector(const Vector &vec, float rot);

Vector lerp(const Vector &v1, const Vector &v2, float dt, int lerpType);
