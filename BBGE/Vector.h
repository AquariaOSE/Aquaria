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
#ifndef BBGE_VECTOR_H
#define BBGE_VECTOR_H

#include <stddef.h>
#include <cmath>
#include <float.h>
#include <vector>

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
     bool operator==(const Vector &vec) const
     {
          return ((x == vec.x) && (y == vec.y) && (z == vec.z));
     }

     // vecector inequality
     bool operator!=(const Vector &vec) const
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

	 inline Vector getPerpendicularLeft()
	 {
		 return Vector(-y, x);
	 }

	 inline Vector getPerpendicularRight()
	 {
		 return Vector(y, -x);
	 }

     // cross product
     const Vector operator^(const Vector &vec) const
     {
          return Vector(y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x);
     }

     // dot product
     inline scalar_t dot(const Vector &vec) const
     {
          return x*vec.x + y*vec.y + z*vec.z;
     }

	 inline scalar_t dot2D(const Vector &vec) const
	 {
		 return x*vec.x + y*vec.y;
	 }

     // dot product
     scalar_t operator%(const Vector &vec) const
     {
          return x*vec.x + y*vec.x + z*vec.z;
     }


     // length of vector
     inline scalar_t getLength3D() const
     {
          return static_cast<scalar_t>(sqrtf(x*x + y*y + z*z));
     }
     inline scalar_t getLength2D() const
     {
          return static_cast<scalar_t>(sqrtf(x*x + y*y));
     }

     // return the unit vector
	 inline const Vector unitVector3D() const
	 {
		return (*this) * (1/getLength3D());
	 }

     // normalize this vector
	 inline void normalize3D()
	 {
		if (x == 0 && y == 0 && z == 0)
		{

			x = y = z = 0;
		}
		else
		{
			(*this) *= 1/getLength3D();
		}
	 }
	 inline void normalize2D()
	 {
		if (x == 0 && y == 0)
		{

			x = y = z= 0;
		}
		else
		{
			(*this) *= 1/getLength2D();
		}
	 }

     scalar_t operator!() const
     {
          return sqrtf(x*x + y*y + z*z);
     }



	 inline void setLength3D(const float l)
	 {
		// IGNORE !!
		if (l == 0)
		{

		}
		else
		{
			float len = getLength3D();
			this->x *= (l/len);
			this->y *= (l/len);
			this->z *= (l/len);
		}
	 }
	 inline void setLength2D(const float l)
	 {
		float len = getLength2D();
		if (len == 0)
		{

		}
		else
		{
			this->x *= (l/len);
			this->y *= (l/len);
		}

	 }

     // return angle between two vectors
     inline scalar_t Angle(const Vector& normal) const
     {
          return acosf(*this % normal);
     }



	 inline bool isLength2DIn(float radius) const
	 {
		return (x*x + y*y) <= (radius*radius);
	 }



	 inline void setZero()
	 {
		this->x = this->y = this->z = 0;
	 }
	 inline scalar_t getSquaredLength2D() const
	 {
		return (x*x) + (y*y);
	 }
	 inline bool isZero() const
	 {
		return x==0 && y==0 && z==0;
	 }

	 inline bool isNan() const
	 {
#ifdef BBGE_BUILD_WINDOWS
		return _isnan(x) || _isnan(y) || _isnan(z);
#elif defined(BBGE_BUILD_UNIX)
		 return std::isnan(x) || std::isnan(y) || std::isnan(z);
#else
		return false;
#endif
	 }

	 inline void capLength2D(const float l)
	 {
		if (!isLength2DIn(l))	setLength2D(l);
	 }
	 inline void capRotZ360()
	 {
		while (z > 360)
			z -= 360;
		while (z < 0)
			z += 360;
	 }

	 void rotate2DRad(float rad);
	 void rotate2D360(float angle);
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
	inline size_t getNumPathNodes() { return pathNodes.size(); }
	void resizePathNodes(size_t sz) { pathNodes.resize(sz); }
	VectorPathNode *getPathNode(size_t i) { if (i<getNumPathNodes()) return &pathNodes[i]; return 0; }
	inline VectorPathNode& operator[](size_t i) { return pathNodes[i]; }
	inline const VectorPathNode& operator[](size_t i) const { return pathNodes[i]; }
	void cut(int n);
	void splice(const VectorPath &path, int sz);
	void prepend(const VectorPath &path);
	void append(const VectorPath &path);
	void removeNode(unsigned int i);
	void calculatePercentages();
	float getLength();
	void realPercentageCalc();
	void removeNodes(unsigned int startInclusive, unsigned int endInclusive);
	float getSubSectionLength(int startIncl, int endIncl);
protected:
	std::vector <VectorPathNode> pathNodes;
};

// This struct is used to keep all of the interpolation-specific data out
// of the global InterpolatedVector class, so that we don't waste memory on
// non-interpolated vectors.
struct InterpolatedVectorData
{
	InterpolatedVectorData()
	{
		interpolating = false;
		pingPong = false;
		loopType = 0;
		pathTimer = 0;
		pathTime = 0;
		pathSpeed = 1;
		pathTimeMultiplier = 1;
		timePassed = 0;
		timePeriod = 0;

		ease = false;
		followingPath = false;
	}

	Vector from;
	Vector target;

	VectorPath path;

	int loopType;

	float pathTimer, pathTime;
	float pathSpeed;
	float pathTimeMultiplier;
	float timePassed, timePeriod;

	bool interpolating;
	bool pingPong;
	bool ease;
	bool followingPath;
};

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
		if (vec.data)
		{
			if (data)
				*data = *vec.data;
			else
				data = new InterpolatedVectorData(*vec.data);
		}
		else
		{
			delete data;
			data = NULL;
		}
		return *this;
	}

	float interpolateTo (Vector vec, float timePeriod, int loopType = 0, bool pingPong = false, bool ease = false);
	void inline update(float dt)
	{
		if (!data)
			return;

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

#endif // BBGE_VECTOR_H
