///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2011-03-07
// Updated : 2011-04-26
// Licence : This source is under MIT License
// File    : glm/gtx/ulp.inl
///////////////////////////////////////////////////////////////////////////////////////////////////

#include <cmath>
#include <cfloat>

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

typedef union
{
	float value;
	/* FIXME: Assumes 32 bit int.  */
	unsigned int word;
} ieee_float_shape_type;

typedef union
{
	double value;
	struct
	{
		glm::detail::int32 lsw;
		glm::detail::int32 msw;
	} parts;
} ieee_double_shape_type;

#define GLM_EXTRACT_WORDS(ix0,ix1,d)                                \
do {                                                            \
  ieee_double_shape_type ew_u;                                  \
  ew_u.value = (d);                                             \
  (ix0) = ew_u.parts.msw;                                       \
  (ix1) = ew_u.parts.lsw;                                       \
} while (0)

#define GLM_GET_FLOAT_WORD(i,d)                                     \
do {                                                            \
  ieee_float_shape_type gf_u;                                   \
  gf_u.value = (d);                                             \
  (i) = gf_u.word;                                              \
} while (0)

#define GLM_SET_FLOAT_WORD(d,i)                                     \
do {                                                            \
  ieee_float_shape_type sf_u;                                   \
  sf_u.word = (i);                                              \
  (d) = sf_u.value;                                             \
} while (0)

#define GLM_INSERT_WORDS(d,ix0,ix1)                                 \
do {                                                            \
  ieee_double_shape_type iw_u;                                  \
  iw_u.parts.msw = (ix0);                                       \
  iw_u.parts.lsw = (ix1);                                       \
  (d) = iw_u.value;                                             \
} while (0)

namespace glm{
namespace detail
{
	GLM_FUNC_QUALIFIER float nextafterf(float x, float y)
	{
		volatile float t;
		glm::detail::int32 hx, hy, ix, iy;

		GLM_GET_FLOAT_WORD(hx,x);
		GLM_GET_FLOAT_WORD(hy,y);
		ix = hx&0x7fffffff;             // |x|
		iy = hy&0x7fffffff;             // |y|

		if((ix>0x7f800000) ||   // x is nan 
			(iy>0x7f800000))     // y is nan 
			return x+y;
		if(x==y) return y;              // x=y, return y
		if(ix==0) {                             // x == 0
			GLM_SET_FLOAT_WORD(x,(hy&0x80000000)|1);// return +-minsubnormal
			t = x*x;
			if(t==x) return t; else return x;   // raise underflow flag
		}
		if(hx>=0) {                             // x > 0 
			if(hx>hy) {                         // x > y, x -= ulp
				hx -= 1;
			} else {                            // x < y, x += ulp
				hx += 1;
			}
		} else {                                // x < 0
			if(hy>=0||hx>hy){                   // x < y, x -= ulp
				hx -= 1;
			} else {                            // x > y, x += ulp
				hx += 1;
			}
		}
		hy = hx&0x7f800000;
		if(hy>=0x7f800000) return x+x;  // overflow
		if(hy<0x00800000) {             // underflow
			t = x*x;
			if(t!=x) {          // raise underflow flag
				GLM_SET_FLOAT_WORD(y,hx);
				return y;
			}
		}
		GLM_SET_FLOAT_WORD(x,hx);
		return x;
	}

	GLM_FUNC_QUALIFIER double nextafter(double x, double y)
	{
		volatile double t;
		glm::detail::int32 hx, hy, ix, iy;
		glm::detail::uint32 lx, ly;

		GLM_EXTRACT_WORDS(hx, lx, x);
		GLM_EXTRACT_WORDS(hy, ly, y);
		ix = hx & 0x7fffffff;             // |x| 
		iy = hy & 0x7fffffff;             // |y| 

		if(((ix>=0x7ff00000)&&((ix-0x7ff00000)|lx)!=0) ||   // x is nan
			((iy>=0x7ff00000)&&((iy-0x7ff00000)|ly)!=0))     // y is nan
			return x+y;
		if(x==y) return y;              // x=y, return y
		if((ix|lx)==0) {                        // x == 0 
			GLM_INSERT_WORDS(x, hy & 0x80000000, 1);    // return +-minsubnormal
			t = x*x;
			if(t==x) return t; else return x;   // raise underflow flag 
		}
		if(hx>=0) {                             // x > 0 
			if(hx>hy||((hx==hy)&&(lx>ly))) {    // x > y, x -= ulp 
				if(lx==0) hx -= 1;
				lx -= 1;
			} else {                            // x < y, x += ulp
				lx += 1;
				if(lx==0) hx += 1;
			}
		} else {                                // x < 0 
			if(hy>=0||hx>hy||((hx==hy)&&(lx>ly))){// x < y, x -= ulp
				if(lx==0) hx -= 1;
				lx -= 1;
			} else {                            // x > y, x += ulp
				lx += 1;
				if(lx==0) hx += 1;
			}
		}
		hy = hx&0x7ff00000;
		if(hy>=0x7ff00000) return x+x;  // overflow
		if(hy<0x00100000) {             // underflow
			t = x*x;
			if(t!=x) {          // raise underflow flag
				GLM_INSERT_WORDS(y,hx,lx);
				return y;
			}
		}
		GLM_INSERT_WORDS(x,hx,lx);
		return x;
	}
}//namespace detail
}//namespace glm

#if(GLM_COMPILER & GLM_COMPILER_VC)
#	define GLM_NEXT_AFTER_FLT(x, toward) glm::detail::nextafterf((x), (toward))
#   define GLM_NEXT_AFTER_DBL(x, toward) _nextafter((x), (toward))
#else
#   define GLM_NEXT_AFTER_FLT(x, toward) nextafterf((x), (toward))
#   define GLM_NEXT_AFTER_DBL(x, toward) nextafter((x), (toward))
#endif

namespace glm{
namespace gtx{
namespace ulp
{
    GLM_FUNC_QUALIFIER float next_float(float const & x)
    {
        return GLM_NEXT_AFTER_FLT(x, std::numeric_limits<float>::max());
    }

    GLM_FUNC_QUALIFIER double next_float(double const & x)
    {
        return GLM_NEXT_AFTER_DBL(x, std::numeric_limits<double>::max());
    }

    template<typename T, template<typename> class vecType>
    GLM_FUNC_QUALIFIER vecType<T> next_float(vecType<T> const & x)
    {
        vecType<T> Result;
        for(std::size_t i = 0; i < Result.length(); ++i)
            Result[i] = next_float(x[i]);
        return Result;
    }

    GLM_FUNC_QUALIFIER float prev_float(float const & x)
    {
        return GLM_NEXT_AFTER_FLT(x, std::numeric_limits<float>::min());
    }

    GLM_FUNC_QUALIFIER double prev_float(double const & x)
    {
        return GLM_NEXT_AFTER_DBL(x, std::numeric_limits<double>::min());
    }

    template<typename T, template<typename> class vecType>
    GLM_FUNC_QUALIFIER vecType<T> prev_float(vecType<T> const & x)
    {
        vecType<T> Result;
        for(std::size_t i = 0; i < Result.length(); ++i)
            Result[i] = prev_float(x[i]);
        return Result;
    }

    template <typename T>
    GLM_FUNC_QUALIFIER T next_float(T const & x, uint const & ulps)
    {
        T temp = x;
        for(std::size_t i = 0; i < ulps; ++i)
            temp = next_float(temp);
        return temp;
    }

    template<typename T, template<typename> class vecType>
    GLM_FUNC_QUALIFIER vecType<T> next_float(vecType<T> const & x, vecType<uint> const & ulps)
    {
        vecType<T> Result;
        for(std::size_t i = 0; i < Result.length(); ++i)
            Result[i] = next_float(x[i], ulps[i]);
        return Result;
    }

    template <typename T>
    GLM_FUNC_QUALIFIER T prev_float(T const & x, uint const & ulps)
    {
        T temp = x;
        for(std::size_t i = 0; i < ulps; ++i)
            temp = prev_float(temp);
        return temp;
    }

    template<typename T, template<typename> class vecType>
    GLM_FUNC_QUALIFIER vecType<T> prev_float(vecType<T> const & x, vecType<uint> const & ulps)
    {
        vecType<T> Result;
        for(std::size_t i = 0; i < Result.length(); ++i)
            Result[i] = prev_float(x[i], ulps[i]);
        return Result;
    }

    template <typename T>
    GLM_FUNC_QUALIFIER uint float_distance(T const & x, T const & y)
    {
        uint ulp = 0;

        if(x < y)
        {
            T temp = x;
            while(temp != y && ulp < std::numeric_limits<std::size_t>::max())
            {
                ++ulp;
                temp = next_float(temp);
            }
        }
        else if(y < x)
        {
            T temp = y;
            while(temp != x && ulp < std::numeric_limits<std::size_t>::max())
            {
                ++ulp;
                temp = next_float(temp);
            }
        }
        else // ==
        {

        }

        return ulp;
    }

    template<typename T, template<typename> class vecType>
    GLM_FUNC_QUALIFIER vecType<uint> float_distance(vecType<T> const & x, vecType<T> const & y)
    {
        vecType<uint> Result;
        for(std::size_t i = 0; i < Result.length(); ++i)
            Result[i] = float_distance(x[i], y[i]);
        return Result;
    }
/*
	inline std::size_t ulp
	(
		detail::thalf const & a,
		detail::thalf const & b
	)
	{
		std::size_t Count = 0;
		float TempA(a);
		float TempB(b);
		//while((TempA = _nextafterf(TempA, TempB)) != TempB)
			++Count;
		return Count;
	}

	inline std::size_t ulp
	(
		float const & a,
		float const & b
	)
	{
		std::size_t Count = 0;
		float Temp = a;
		//while((Temp = _nextafterf(Temp, b)) != b)
        {
            std::cout << Temp << " " << b << std::endl;
			++Count;
        }
		return Count;
	}

	inline std::size_t ulp
	(
		double const & a,
		double const & b
	)
	{
		std::size_t Count = 0;
		double Temp = a;
		//while((Temp = _nextafter(Temp, b)) != b)
        {
            std::cout << Temp << " " << b << std::endl;
			++Count;
        }
		return Count;
	}

	template <typename T>
	inline std::size_t ulp
	(
		detail::tvec2<T> const & a,
		detail::tvec2<T> const & b
	)
	{
        std::size_t ulps[] = 
        {
            ulp(a[0], b[0]),
            ulp(a[1], b[1])
        };

        return glm::max(ulps[0], ulps[1]);
	}

	template <typename T>
	inline std::size_t ulp
	(
		detail::tvec3<T> const & a,
		detail::tvec3<T> const & b
	)
	{
        std::size_t ulps[] = 
        {
            ulp(a[0], b[0]),
            ulp(a[1], b[1]),
            ulp(a[2], b[2])
        };

        return glm::max(glm::max(ulps[0], ulps[1]), ulps[2]);
	}

	template <typename T>
	inline std::size_t ulp
	(
		detail::tvec4<T> const & a,
		detail::tvec4<T> const & b
	)
	{
        std::size_t ulps[] = 
        {
            ulp(a[0], b[0]),
            ulp(a[1], b[1]),
            ulp(a[2], b[2]),
            ulp(a[3], b[3])
        };

        return glm::max(glm::max(ulps[0], ulps[1]), glm::max(ulps[2], ulps[3]));
	}
*/
}//namespace ulp
}//namespace gtx
}//namespace glm
