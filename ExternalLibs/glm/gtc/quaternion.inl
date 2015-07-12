///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-05-21
// Updated : 2010-02-04
// Licence : This source is under MIT License
// File    : glm/gtc/quaternion.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <limits>

namespace glm{
namespace detail{

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T>::tquat() : 
        x(0),
        y(0),
        z(0),
        w(1)
    {}

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T>::tquat
	(
		value_type const & s, 
		tvec3<T> const & v
	) : 
        x(v.x),
        y(v.y),
        z(v.z),
        w(s)
    {}

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T>::tquat
	(
		value_type const & w, 
		value_type const & x, 
		value_type const & y, 
		value_type const & z
	) :
        x(x),
        y(y),
        z(z),
        w(w)
    {}

    //////////////////////////////////////////////////////////////
    // tquat conversions

	//template <typename valType> 
	//GLM_FUNC_QUALIFIER tquat<valType>::tquat
	//(
	//	valType const & pitch, 
	//	valType const & yaw, 
	//	valType const & roll
	//)
	//{
	//	tvec3<valType> eulerAngle(pitch * valType(0.5), yaw * valType(0.5), roll * valType(0.5));
	//	tvec3<valType> c = glm::cos(eulerAngle * valType(0.5));
	//	tvec3<valType> s = glm::sin(eulerAngle * valType(0.5));
	//	
	//	this->w = c.x * c.y * c.z + s.x * s.y * s.z;
	//	this->x = s.x * c.y * c.z - c.x * s.y * s.z;
	//	this->y = c.x * s.y * c.z + s.x * c.y * s.z;
	//	this->z = c.x * c.y * s.z - s.x * s.y * c.z;
	//}

	template <typename T> 
	GLM_FUNC_QUALIFIER tquat<T>::tquat
	(
		tvec3<T> const & eulerAngle
	)
	{
		tvec3<T> c = glm::cos(eulerAngle * value_type(0.5));
		tvec3<T> s = glm::sin(eulerAngle * value_type(0.5));
		
		this->w = c.x * c.y * c.z + s.x * s.y * s.z;
		this->x = s.x * c.y * c.z - c.x * s.y * s.z;
		this->y = c.x * s.y * c.z + s.x * c.y * s.z;
		this->z = c.x * c.y * s.z - s.x * s.y * c.z;		
	}

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T>::tquat
	(
		tmat3x3<T> const & m
	)
    {
		*this = toQuat(m);
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T>::tquat
	(
		tmat4x4<T> const & m
	)
    {
		*this = toQuat(m);
    }

    //////////////////////////////////////////////////////////////
    // tquat<T> accesses

    template <typename T> 
	GLM_FUNC_QUALIFIER typename tquat<T>::value_type & tquat<T>::operator [] (int i)
    {
        return (&x)[i];
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER typename tquat<T>::value_type const & tquat<T>::operator [] (int i) const
    {
        return (&x)[i];
    }

    //////////////////////////////////////////////////////////////
    // tquat<valType> operators

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T> & tquat<T>::operator *=
	(
		value_type const & s
	)
    {
        this->w *= s;
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return *this;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER tquat<T> & tquat<T>::operator /=
	(
		value_type const & s
	)
    {
        this->w /= s;
        this->x /= s;
        this->y /= s;
        this->z /= s;
        return *this;
    }

    //////////////////////////////////////////////////////////////
    // tquat<valType> external operators

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tquat<T> operator- 
	(
		detail::tquat<T> const & q
	)
	{
		return detail::tquat<T>(-q.w, -q.x, -q.y, -q.z);
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tquat<T> operator*
	( 
		detail::tquat<T> const & q, 
		detail::tquat<T> const & p
	) 
	{ 
        return detail::tquat<T>(
            q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z,
	        q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y,
	        q.w * p.y + q.y * p.w + q.z * p.x - q.x * p.z,
	        q.w * p.z + q.z * p.w + q.x * p.y - q.y * p.x);
	} 

	// Transformation
	template <typename T>
	GLM_FUNC_QUALIFIER detail::tvec3<T> operator* 
	(
		detail::tquat<T> const & q, 
		detail::tvec3<T> const & v
	)
	{
		typename detail::tquat<T>::value_type Two(2);

		detail::tvec3<T> uv, uuv;
		detail::tvec3<T> QuatVector(q.x, q.y, q.z);
		uv = glm::cross(QuatVector, v);
		uuv = glm::cross(QuatVector, uv);
		uv *= (Two * q.w); 
		uuv *= Two; 

		return v + uv + uuv;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tvec3<T> operator* 
	(
		detail::tvec3<T> const & v,
		detail::tquat<T> const & q 
	)
	{
		return gtc::quaternion::inverse(q) * v;
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tvec4<T> operator* 
	(
		detail::tquat<T> const & q, 
		detail::tvec4<T> const & v
	)
	{
		return detail::tvec4<T>(q * detail::tvec3<T>(v), v.w);
	}

	template <typename T>
	GLM_FUNC_QUALIFIER detail::tvec4<T> operator* 
	(
		detail::tvec4<T> const & v,
		detail::tquat<T> const & q 
	)
	{
		return gtc::quaternion::inverse(q) * v;
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tquat<T> operator* 
	(
		detail::tquat<T> const & q, 
		typename detail::tquat<T>::value_type const & s
	)
	{
		return detail::tquat<T>(
			q.w * s, q.x * s, q.y * s, q.z * s);
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tquat<T> operator* 
	(
		typename detail::tquat<T>::value_type const & s,
		detail::tquat<T> const & q
	)
	{
		return q * s;
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tquat<T> operator/ 
	(
		detail::tquat<T> const & q, 
		typename detail::tquat<T>::value_type const & s
	)
	{
		return detail::tquat<T>(
			q.w / s, q.x / s, q.y / s, q.z / s);
	}

	//////////////////////////////////////
	// Boolean operators

	template <typename T> 
	GLM_FUNC_QUALIFIER bool operator==
	(
		detail::tquat<T> const & q1, 
		detail::tquat<T> const & q2
	)
	{
		return (q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w);
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER bool operator!=
	(
		detail::tquat<T> const & q1, 
		detail::tquat<T> const & q2
	)
	{
		return (q1.x != q2.x) || (q1.y != q2.y) || (q1.z != q2.z) || (q1.w != q2.w);
	}

}//namespace detail

namespace gtc{
namespace quaternion{

	////////////////////////////////////////////////////////
    template <typename T> 
	GLM_FUNC_QUALIFIER typename detail::tquat<T>::value_type length
	(
		detail::tquat<T> const & q
	)
    {
		return glm::sqrt(dot(q, q));
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> normalize
	(
		detail::tquat<T> const & q
	)
    {
        typename detail::tquat<T>::value_type len = length(q);
        if(len <= typename detail::tquat<T>::value_type(0)) // Problem
            return detail::tquat<T>(1, 0, 0, 0);
        typename detail::tquat<T>::value_type oneOverLen = typename detail::tquat<T>::value_type(1) / len;
        return detail::tquat<T>(q.w * oneOverLen, q.x * oneOverLen, q.y * oneOverLen, q.z * oneOverLen);
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER typename detail::tquat<T>::value_type dot
	(
		detail::tquat<T> const & q1, 
		detail::tquat<T> const & q2
	)
    {
        return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> cross
	(
		detail::tquat<T> const & q1, 
		detail::tquat<T> const & q2
	)
    {
        return detail::tquat<T>(
            q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z,
	        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
	        q1.w * q2.y + q1.y * q2.w + q1.z * q2.x - q1.x * q2.z,
	        q1.w * q2.z + q1.z * q2.w + q1.x * q2.y - q1.y * q2.x);
    }
/*
	// (x * sin(1 - a) * angle / sin(angle)) + (y * sin(a) * angle / sin(angle))
    template <typename T>
    GLM_FUNC_QUALIFIER detail::tquat<T> mix
	(
		detail::tquat<T> const & x, 
		detail::tquat<T> const & y, 
		typename detail::tquat<T>::value_type const & a
	)
    {
        if(a <= typename detail::tquat<T>::value_type(0)) return x;
        if(a >= typename detail::tquat<T>::value_type(1)) return y;

        float fCos = dot(x, y);
        detail::tquat<T> y2(y); //BUG!!! tquat<T> y2;
        if(fCos < typename detail::tquat<T>::value_type(0))
        {
            y2 = -y;
            fCos = -fCos;
        }

        //if(fCos > 1.0f) // problem
        float k0, k1;
        if(fCos > typename detail::tquat<T>::value_type(0.9999))
        {
            k0 = typename detail::tquat<T>::value_type(1) - a;
            k1 = typename detail::tquat<T>::value_type(0) + a; //BUG!!! 1.0f + a;
        }
        else
        {
            typename detail::tquat<T>::value_type fSin = sqrt(T(1) - fCos * fCos);
            typename detail::tquat<T>::value_type fAngle = atan(fSin, fCos);
            typename detail::tquat<T>::value_type fOneOverSin = T(1) / fSin;
            k0 = sin((typename detail::tquat<T>::value_type(1) - a) * fAngle) * fOneOverSin;
            k1 = sin((typename detail::tquat<T>::value_type(0) + a) * fAngle) * fOneOverSin;
        }

        return detail::tquat<T>(
            k0 * x.w + k1 * y2.w,
            k0 * x.x + k1 * y2.x,
            k0 * x.y + k1 * y2.y,
            k0 * x.z + k1 * y2.z);
    }

    template <typename T>
    GLM_FUNC_QUALIFIER detail::tquat<T> mix2
	(
		detail::tquat<T> const & x, 
		detail::tquat<T> const & y, 
		T const & a
	)
    {
        bool flip = false;
		if(a <= T(0)) return x;
        if(a >= T(1)) return y;

        T cos_t = dot(x, y);
		if(cos_t < T(0))
		{
			cos_t = -cos_t;
			flip = true;
		}

		T alpha(0), beta(0);

		if(T(1) - cos_t < 1e-7)
			beta = T(1) - alpha;
		else
		{
			T theta = acos(cos_t);
			T sin_t = sin(theta);
			beta = sin(theta * (T(1) - alpha)) / sin_t;
			alpha = sin(alpha * theta) / sin_t;
		}

		if(flip)
			alpha = -alpha;
		
        return normalize(beta * x + alpha * y);
    }
*/
    template <typename T>
    GLM_FUNC_QUALIFIER detail::tquat<T> mix
	(
		detail::tquat<T> const & x, 
		detail::tquat<T> const & y, 
		T const & a
	)
    {
		T angle = acos(dot(x, y));
		return (sin((1 - a) * angle) * x + sin(a * angle) * y) / sin(angle);
	}

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> conjugate
	(
		detail::tquat<T> const & q
	)
    {
        return detail::tquat<T>(q.w, -q.x, -q.y, -q.z);
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> inverse
	(
		detail::tquat<T> const & q
	)
    {
        return gtc::quaternion::conjugate(q) / gtc::quaternion::dot(q, q);
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> rotate
	(
		detail::tquat<T> const & q, 
		typename detail::tquat<T>::value_type const & angle, 
		detail::tvec3<T> const & v
	)
    {
		detail::tvec3<T> Tmp = v;

        // Axis of rotation must be normalised
        typename detail::tquat<T>::value_type len = glm::core::function::geometric::length(Tmp);
        if(abs(len - typename detail::tquat<T>::value_type(1)) > typename detail::tquat<T>::value_type(0.001))
        {
            T oneOverLen = T(1) / len;
            Tmp.x *= oneOverLen;
            Tmp.y *= oneOverLen;
            Tmp.z *= oneOverLen;
        }

        typename detail::tquat<T>::value_type AngleRad = radians(angle);
        typename detail::tquat<T>::value_type fSin = sin(AngleRad * T(0.5));

        return gtc::quaternion::cross(q, detail::tquat<T>(cos(AngleRad * T(0.5)), Tmp.x * fSin, Tmp.y * fSin, Tmp.z * fSin));
	}

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat3x3<T> mat3_cast
	(
		detail::tquat<T> const & q
	)
    {
        detail::tmat3x3<T> Result(typename detail::tquat<T>::value_type(1));
        Result[0][0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
        Result[0][1] = 2 * q.x * q.y + 2 * q.w * q.z;
        Result[0][2] = 2 * q.x * q.z - 2 * q.w * q.y;

        Result[1][0] = 2 * q.x * q.y - 2 * q.w * q.z;
        Result[1][1] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
        Result[1][2] = 2 * q.y * q.z + 2 * q.w * q.x;

        Result[2][0] = 2 * q.x * q.z + 2 * q.w * q.y;
        Result[2][1] = 2 * q.y * q.z - 2 * q.w * q.x;
        Result[2][2] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
        return Result;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> mat4_cast
	(
		detail::tquat<T> const & q
	)
    {
        return detail::tmat4x4<T>(mat3_cast(q));
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> quat_cast
	(
		detail::tmat3x3<T> const & m
	)
    {
        typename detail::tquat<T>::value_type fourXSquaredMinus1 = m[0][0] - m[1][1] - m[2][2];
        typename detail::tquat<T>::value_type fourYSquaredMinus1 = m[1][1] - m[0][0] - m[2][2];
        typename detail::tquat<T>::value_type fourZSquaredMinus1 = m[2][2] - m[0][0] - m[1][1];
        typename detail::tquat<T>::value_type fourWSquaredMinus1 = m[0][0] + m[1][1] + m[2][2];
        
        int biggestIndex = 0;
        typename detail::tquat<T>::value_type fourBiggestSquaredMinus1 = fourWSquaredMinus1;
        if(fourXSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourXSquaredMinus1;
            biggestIndex = 1;
        }
        if(fourYSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourYSquaredMinus1;
            biggestIndex = 2;
        }
        if(fourZSquaredMinus1 > fourBiggestSquaredMinus1)
        {
            fourBiggestSquaredMinus1 = fourZSquaredMinus1;
            biggestIndex = 3;
        }

        typename detail::tquat<T>::value_type biggestVal = sqrt(fourBiggestSquaredMinus1 + typename detail::tquat<T>::value_type(1)) * typename detail::tquat<T>::value_type(0.5);
        typename detail::tquat<T>::value_type mult = typename detail::tquat<T>::value_type(0.25) / biggestVal;

        detail::tquat<T> Result;
        switch(biggestIndex)
        {
        case 0:
            Result.w = biggestVal; 
            Result.x = (m[1][2] - m[2][1]) * mult;
            Result.y = (m[2][0] - m[0][2]) * mult;
            Result.z = (m[0][1] - m[1][0]) * mult;
            break;
        case 1:
            Result.w = (m[1][2] - m[2][1]) * mult;
            Result.x = biggestVal;
            Result.y = (m[0][1] + m[1][0]) * mult;
            Result.z = (m[2][0] + m[0][2]) * mult;
            break;
        case 2:
            Result.w = (m[2][0] - m[0][2]) * mult;
            Result.x = (m[0][1] + m[1][0]) * mult;
            Result.y = biggestVal;
            Result.z = (m[1][2] + m[2][1]) * mult;
            break;
        case 3:
            Result.w = (m[0][1] - m[1][0]) * mult;
            Result.x = (m[2][0] + m[0][2]) * mult;
            Result.y = (m[1][2] + m[2][1]) * mult;
            Result.z = biggestVal;
            break;
        }
        return Result;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tquat<T> quat_cast
	(
		detail::tmat4x4<T> const & m4
	)
    {
		return quat_cast(detail::tmat3x3<T>(m4));
    }

}//namespace quaternion
}//namespace gtc
}//namespace glm
