///////////////////////////////////////////////////////////////////////////////////////////////////
// OpenGL Mathematics Copyright (c) 2005 - 2011 G-Truc Creation (www.g-truc.net)
///////////////////////////////////////////////////////////////////////////////////////////////////
// Created : 2008-08-01
// Updated : 2008-09-10
// Licence : This source is under MIT License
// File    : glm/core/func_trigonometric.hpp
///////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef glm_core_func_trigonometric
#define glm_core_func_trigonometric

namespace glm
{
	namespace test{
		void main_core_func_trigonometric();
	}//namespace test

	namespace core{
	namespace function{
	//! Define Angle and trigonometry functions 
	//! from Section 8.1 of GLSL 1.30.8 specification. 
	//! Included in glm namespace.
	namespace trigonometric{

	/// \addtogroup core_funcs
	///@{

	//! Converts degrees to radians and returns the result.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/radians.xml">GLSL radians man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType radians(genType const & degrees);

	//! Converts radians to degrees and returns the result.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/degrees.xml">GLSL degrees man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType degrees(genType const & radians);

	//! The standard trigonometric sine function. 
	//! The values returned by this function will range from [-1, 1].
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/sin.xml">GLSL sin man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType sin(genType const & angle);

	//! The standard trigonometric cosine function. 
	//! The values returned by this function will range from [-1, 1].
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/cos.xml">GLSL cos man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType cos(genType const & angle);

	//! The standard trigonometric tangent function.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/tan.xml">GLSL tan man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType tan(genType const & angle); 

	//! Arc sine. Returns an angle whose sine is x. 
	//! The range of values returned by this function is [-PI/2, PI/2]. 
	//! Results are undefined if |x| > 1.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/asin.xml">GLSL asin man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType asin(genType const & x);

	//! Arc cosine. Returns an angle whose sine is x. 
	//! The range of values returned by this function is [0, PI]. 
	//! Results are undefined if |x| > 1.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/acos.xml">GLSL acos man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType acos(genType const & x);

	//! Arc tangent. Returns an angle whose tangent is y/x. 
	//! The signs of x and y are used to determine what 
	//! quadrant the angle is in. The range of values returned 
	//! by this function is [-PI, PI]. Results are undefined 
	//! if x and y are both 0. 
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/atan.xml">GLSL atan man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType atan(genType const & y, genType const & x);

	//! Arc tangent. Returns an angle whose tangent is y_over_x. 
	//! The range of values returned by this function is [-PI/2, PI/2].
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/atan.xml">GLSL atan man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType atan(genType const & y_over_x);

	//! Returns the hyperbolic sine function, (exp(x) - exp(-x)) / 2
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/sinh.xml">GLSL sinh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType sinh(genType const & angle);

	//! Returns the hyperbolic cosine function, (exp(x) + exp(-x)) / 2
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/cosh.xml">GLSL cosh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType cosh(genType const & angle);

	//! Returns the hyperbolic tangent function, sinh(angle) / cosh(angle)
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/tanh.xml">GLSL tanh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType tanh(genType const & angle);

	//! Arc hyperbolic sine; returns the inverse of sinh.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/asinh.xml">GLSL asinh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType asinh(genType const & x);
	
	//! Arc hyperbolic cosine; returns the non-negative inverse
	//! of cosh. Results are undefined if x < 1.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/acosh.xml">GLSL acosh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType acosh(genType const & x);

	//! Arc hyperbolic tangent; returns the inverse of tanh.
	//! Results are undefined if abs(x) >= 1.
    //!
    //! \li <a href="http://www.opengl.org/sdk/docs/manglsl/xhtml/atanh.xml">GLSL atanh man page</a>
    //! \li GLSL 1.30.08 specification, section 8.1	
	template <typename genType> 
	genType atanh(genType const & x);

	///@}

	}//namespace trigonometric
	}//namespace function
	}//namespace core

	using namespace core::function::trigonometric;
}//namespace glm

#include "func_trigonometric.inl"

#endif//glm_core_func_trigonometric


