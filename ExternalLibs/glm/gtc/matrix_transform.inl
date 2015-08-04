///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2009-04-29
// Updated : 2009-04-29
// Licence : This source is under MIT License
// File    : glm/gtc/matrix_transform.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace glm{
namespace gtc{
namespace matrix_transform
{
    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> translate
	(
		detail::tmat4x4<T> const & m,
		detail::tvec3<T> const & v
	)
    {
		detail::tmat4x4<T> Result(m);
		Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		return Result;
    }
		
    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> rotate
	(
		detail::tmat4x4<T> const & m,
		T const & angle, 
		detail::tvec3<T> const & v
	)
    {
        T a = radians(angle);
        T c = cos(a);
        T s = sin(a);

        detail::tvec3<T> axis = normalize(v);

		detail::tvec3<T> temp = (T(1) - c) * axis;

        detail::tmat4x4<T> Rotate(detail::tmat4x4<T>::null);
		Rotate[0][0] = c + temp[0] * axis[0];
	    Rotate[0][1] = 0 + temp[0] * axis[1] + s * axis[2];
	    Rotate[0][2] = 0 + temp[0] * axis[2] - s * axis[1];

	    Rotate[1][0] = 0 + temp[1] * axis[0] - s * axis[2];
	    Rotate[1][1] = c + temp[1] * axis[1];
	    Rotate[1][2] = 0 + temp[1] * axis[2] + s * axis[0];

	    Rotate[2][0] = 0 + temp[2] * axis[0] + s * axis[1];
	    Rotate[2][1] = 0 + temp[2] * axis[1] - s * axis[0];
	    Rotate[2][2] = c + temp[2] * axis[2];

		detail::tmat4x4<T> Result(detail::tmat4x4<T>::null);
		Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
		Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
		Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
		Result[3] = m[3];
		return Result;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> scale
	(
		detail::tmat4x4<T> const & m,
		detail::tvec3<T> const & v
	)
    {
        detail::tmat4x4<T> Result(detail::tmat4x4<T>::null);
		Result[0] = m[0] * v[0];
		Result[1] = m[1] * v[1];
		Result[2] = m[2] * v[2];
		Result[3] = m[3];
		return Result;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> translate_slow
	(
		detail::tmat4x4<T> const & m,
		detail::tvec3<T> const & v
	)
    {
        detail::tmat4x4<T> Result(T(1));
        Result[3] = detail::tvec4<T>(v, T(1));
        return m * Result;

		//detail::tmat4x4<valType> Result(m);
		Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
		//Result[3][0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
		//Result[3][1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
		//Result[3][2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
		//Result[3][3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3];
		//return Result;
    }
		
    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> rotate_slow
	(
		detail::tmat4x4<T> const & m,
		T const & angle, 
		detail::tvec3<T> const & v
	)
    {
        T a = radians(angle);
        T c = cos(a);
        T s = sin(a);
        detail::tmat4x4<T> Result;

        detail::tvec3<T> axis = normalize(v);

        Result[0][0] = c + (1 - c)      * axis.x     * axis.x;
	    Result[0][1] = (1 - c) * axis.x * axis.y + s * axis.z;
	    Result[0][2] = (1 - c) * axis.x * axis.z - s * axis.y;
	    Result[0][3] = 0;

	    Result[1][0] = (1 - c) * axis.y * axis.x - s * axis.z;
	    Result[1][1] = c + (1 - c) * axis.y * axis.y;
	    Result[1][2] = (1 - c) * axis.y * axis.z + s * axis.x;
	    Result[1][3] = 0;

	    Result[2][0] = (1 - c) * axis.z * axis.x + s * axis.y;
	    Result[2][1] = (1 - c) * axis.z * axis.y - s * axis.x;
	    Result[2][2] = c + (1 - c) * axis.z * axis.z;
	    Result[2][3] = 0;

        Result[3] = detail::tvec4<T>(0, 0, 0, 1);
        return m * Result;
    }

    template <typename T> 
    GLM_FUNC_QUALIFIER detail::tmat4x4<T> scale_slow
	(
		detail::tmat4x4<T> const & m,
		detail::tvec3<T> const & v
	)
    {
        detail::tmat4x4<T> Result(T(1));
        Result[0][0] = v.x;
        Result[1][1] = v.y;
        Result[2][2] = v.z;
        return m * Result;
    }

	template <typename valType> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<valType> ortho
	(
		valType const & left, 
		valType const & right, 
		valType const & bottom, 
		valType const & top, 
		valType const & zNear, 
		valType const & zFar
	)
	{
		detail::tmat4x4<valType> Result(1);
		Result[0][0] = valType(2) / (right - left);
		Result[1][1] = valType(2) / (top - bottom);
		Result[2][2] = - valType(2) / (zFar - zNear);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);
		Result[3][2] = - (zFar + zNear) / (zFar - zNear);
		return Result;
	}

	template <typename valType> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<valType> ortho(
		valType const & left, 
		valType const & right, 
		valType const & bottom, 
		valType const & top)
	{
		detail::tmat4x4<valType> Result(1);
		Result[0][0] = valType(2) / (right - left);
		Result[1][1] = valType(2) / (top - bottom);
		Result[2][2] = - valType(1);
		Result[3][0] = - (right + left) / (right - left);
		Result[3][1] = - (top + bottom) / (top - bottom);
		return Result;
	}

	template <typename valType> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<valType> frustum
	(
		valType const & left, 
		valType const & right, 
		valType const & bottom, 
		valType const & top, 
		valType const & nearVal, 
		valType const & farVal
	)
	{
		detail::tmat4x4<valType> Result(0);
		Result[0][0] = (valType(2) * nearVal) / (right - left);
		Result[1][1] = (valType(2) * nearVal) / (top - bottom);
		Result[2][0] = (right + left) / (right - left);
		Result[2][1] = (top + bottom) / (top - bottom);
		Result[2][2] = -(farVal + nearVal) / (farVal - nearVal);
		Result[2][3] = valType(-1);
		Result[3][2] = -(valType(2) * farVal * nearVal) / (farVal - nearVal);
		return Result;
	}

	template <typename valType> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<valType> perspective
	(
		valType const & fovy, 
		valType const & aspect, 
		valType const & zNear, 
		valType const & zFar
	)
	{
		valType range = tan(radians(fovy / valType(2))) * zNear;	
		valType left = -range * aspect;
		valType right = range * aspect;
		valType bottom = -range;
		valType top = range;

		detail::tmat4x4<valType> Result(valType(0));
		Result[0][0] = (valType(2) * zNear) / (right - left);
		Result[1][1] = (valType(2) * zNear) / (top - bottom);
		Result[2][2] = - (zFar + zNear) / (zFar - zNear);
		Result[2][3] = - valType(1);
		Result[3][2] = - (valType(2) * zFar * zNear) / (zFar - zNear);
		return Result;
	}

	template <typename valType>
	GLM_FUNC_QUALIFIER detail::tmat4x4<valType> perspectiveFov
	(
		valType const & fov, 
		valType const & width, 
		valType const & height, 
		valType const & zNear, 
		valType const & zFar
	)
	{
		valType rad = glm::radians(fov);
		valType h = glm::cos(valType(0.5) * rad) / glm::sin(valType(0.5) * rad);
		valType w = h * height / width;

		detail::tmat4x4<valType> Result(valType(0));
		Result[0][0] = w;
		Result[1][1] = h;
		Result[2][2] = (zFar + zNear) / (zFar - zNear);
		Result[2][3] = valType(1);
		Result[3][2] = -(valType(2) * zFar * zNear) / (zFar - zNear);
		return Result;
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<T> infinitePerspective
	(
		T fovy, 
		T aspect, 
		T zNear
	)
	{
		T range = tan(radians(fovy / T(2))) * zNear;	
		T left = -range * aspect;
		T right = range * aspect;
		T bottom = -range;
		T top = range;

		detail::tmat4x4<T> Result(T(0));
		Result[0][0] = (T(2) * zNear) / (right - left);
		Result[1][1] = (T(2) * zNear) / (top - bottom);
		Result[2][2] = - T(1);
		Result[2][3] = - T(1);
		Result[3][2] = - T(2) * zNear;
		return Result;
	}

	template <typename T> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<T> tweakedInfinitePerspective
	(
		T fovy, 
		T aspect, 
		T zNear
	)
	{
		T range = tan(radians(fovy / T(2))) * zNear;	
		T left = -range * aspect;
		T right = range * aspect;
		T bottom = -range;
		T top = range;

		detail::tmat4x4<T> Result(T(0));
		Result[0][0] = (T(2) * zNear) / (right - left);
		Result[1][1] = (T(2) * zNear) / (top - bottom);
		Result[2][2] = T(0.0001) - T(1);
		Result[2][3] = T(-1);
		Result[3][2] = - (T(0.0001) - T(2)) * zNear;
		return Result;
	}

	template <typename T, typename U>
	GLM_FUNC_QUALIFIER detail::tvec3<T> project
	(
		detail::tvec3<T> const & obj, 
		detail::tmat4x4<T> const & model, 
		detail::tmat4x4<T> const & proj, 
		detail::tvec4<U> const & viewport
	)
	{
		detail::tvec4<T> tmp = detail::tvec4<T>(obj, T(1));
		tmp = model * tmp;
		tmp = proj * tmp;

		tmp /= tmp.w;
		tmp = tmp * T(0.5) + T(0.5);
		tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
		tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

		return detail::tvec3<T>(tmp);
	}

	template <typename T, typename U>
	GLM_FUNC_QUALIFIER detail::tvec3<T> unProject
	(
		detail::tvec3<T> const & win, 
		detail::tmat4x4<T> const & model, 
		detail::tmat4x4<T> const & proj, 
		detail::tvec4<U> const & viewport
	)
	{
		detail::tmat4x4<T> inverse = glm::inverse(proj * model);

		detail::tvec4<T> tmp = detail::tvec4<T>(win, T(1));
		tmp.x = (tmp.x - T(viewport[0])) / T(viewport[2]);
		tmp.y = (tmp.y - T(viewport[1])) / T(viewport[3]);
		tmp = tmp * T(2) - T(1);

		detail::tvec4<T> obj = inverse * tmp;
		obj /= obj.w;

		return detail::tvec3<T>(obj);
	}

	template <typename T, typename U> 
	detail::tmat4x4<T> pickMatrix
	(
		detail::tvec2<T> const & center, 
		detail::tvec2<T> const & delta, 
		detail::tvec4<U> const & viewport
	)
	{
		assert(delta.x > T(0) && delta.y > T(0));
		detail::tmat4x4<T> Result(1.0f);

		if(!(delta.x > T(0) && delta.y > T(0))) 
			return Result; // Error

		detail::tvec3<T> Temp(
			(T(viewport[2]) - T(2) * (center.x - T(viewport[0]))) / delta.x,
			(T(viewport[3]) - T(2) * (center.y - T(viewport[1]))) / delta.y,
			T(0));

		// Translate and scale the picked region to the entire window
		Result = translate(Result, Temp);
		return scale(Result, detail::tvec3<T>(T(viewport[2]) / delta.x, T(viewport[3]) / delta.y, T(1)));
	}

    template <typename T> 
	GLM_FUNC_QUALIFIER detail::tmat4x4<T> lookAt
	(
		detail::tvec3<T> const & eye,
		detail::tvec3<T> const & center,
		detail::tvec3<T> const & up
	)
    {
        detail::tvec3<T> f = normalize(center - eye);
        detail::tvec3<T> u = normalize(up);
        detail::tvec3<T> s = normalize(cross(f, u));
        u = cross(s, f);

        detail::tmat4x4<T> Result(1);
        Result[0][0] = s.x;
        Result[1][0] = s.y;
        Result[2][0] = s.z;
        Result[0][1] = u.x;
        Result[1][1] = u.y;
        Result[2][1] = u.z;
        Result[0][2] =-f.x;
        Result[1][2] =-f.y;
        Result[2][2] =-f.z;
    /*  Test this instead of translate3D
        Result[3][0] =-dot(s, eye);
        Result[3][1] =-dot(y, eye);
        Result[3][2] = dot(f, eye);
    */  
		return gtc::matrix_transform::translate(Result, -eye);
    }
}//namespace matrix_transform
}//namespace gtc
}//namespace glm
