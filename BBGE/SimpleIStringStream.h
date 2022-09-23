/*
Copyright (C) 2010 Andrew Church <achurch@achurch.org>

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
#ifndef SIMPLEISTRINGSTREAM_H
#define SIMPLEISTRINGSTREAM_H

/*
 * This class implements a lightweight version of the std::istringstream
 * class used for parsing strings.  std::istringstream (like many STL
 * classes) requires a significant amount of overhead to implement its
 * flexibility, and in some environments, the simple act of reading a
 * floating-point value:
 *
 *     std::istringstream is; float f; is >> f;
 *
 * can be 20-30 times (not a typo!) slower than the C equivalent:
 *
 *     float f = strtof(s, &s);
 *
 * This class eschews most of std::istringstream's flexibility in favor of
 * a more efficient implementation to improve performance when reading
 * text-format data files.
 *
 * To create a new SimpleIStringStream, initialize it with or assign to it
 * the string to be parsed:
 *
 *     SimpleIStringStream is("123 4.5 -6.78e9");   // Alternative 1
 *     SimpleIStringStream is = "123 4.5 -6.78e9";  // Alternative 2
 *
 * If the string is a "char *" or "const char *" that is guaranteed to
 * exist throughout the lifetime of the SimpleIStringStream instance, you
 * can also use this more efficient constructor:
 *
 *     SimpleIStringStream is("123 4.5 -6.78e9", SimpleIStringStream::REUSE);
 *
 * which avoids making a separate copy of the string buffer.  (It is then
 * incumbent upon the caller to ensure that the string actually remains
 * valid throughout the SimpleIStringStream's lifetime.)  Alternatively, if
 * you have a dynamically-allocated "char *" buffer that you want the
 * SimpleIStringStream to free for you when you're done with it, construct
 * the object like this:
 *
 *     SimpleIStringStream is(buffer, SimpleIStringStream::TAKE_OVER);
 *
 * In either of the latter two cases, you must not modify the string buffer
 * while the SimpleIStringStream object exists.
 *
 * While it is possible to create a SimpleIStringStream using a std::string
 * object, the object's contents will always be copied on creation for
 * performance reasons; it is not possible to use the REUSE or TAKE_OVER
 * flags with STL strings.
 *
 * The only operators (other than assignment) implemented by
 * SimpleIStringStream are "bool()", for testing end-of-stream (in the same
 * manner as std::istringstream), and ">>", the extraction operator.  All
 * arithmetic, std::string, and character extractors supported by
 * std::istringstream except "long double" are also supported here, but not
 * the C string or function extractors.
 */

#include "Base.h"

/*************************************************************************/

/**
 * SISS_VERIFY:  Verify the behavior of SimpleIStringStream by creating an
 * istringstream alongside the local buffer and ensuring that it always
 * extracts the same values returned by the SimpleIStringStream.
 */
// #define SISS_VERIFY


#ifdef SISS_VERIFY
#  include <sstream>
#endif

/*************************************************************************/
/*************************** Class definition ****************************/
/*************************************************************************/

class SimpleIStringStream {

  public:
	/* Reuse flag passed to StringStream(char *,int). */
	enum {
		/* Make a copy of the buffer (default action). */
		COPY,
		/* Use the passed-in string pointer as is.  Requires the string
		 * pointer to remain valid over the life of this object. */
		REUSE,
		/* Take over the passed-in string buffer, which must have been
		 * allocated with new[].  The buffer will be deleted on object
		 * destruction. */
		TAKE_OVER
	};

	/*-------------------------------------------------------------------*/

	/**
	 * SimpleIStringStream():  Basic object constructor.  No extraction is
	 * possible until a string has been assigned.
	 */
	inline SimpleIStringStream();

	/**
	 * SimpleIStringStream(...):  Create a new SimpleIStringStream from a
	 * C-style or STL string.  Each of these constructors is exactly
	 * equivalent to creating a SimpleIString with the default constructor
	 * and then calling setString() with the same parameters.
	 */
	inline SimpleIStringStream(char *string, int reuse_flag = COPY);
	inline SimpleIStringStream(const char *string, int reuse_flag = COPY);
	inline SimpleIStringStream(std::string string);

	/**
	 * ~SimpleIStringStream():  Object destructor.
	 */
	inline ~SimpleIStringStream();

	/*------------------------------*/

	/**
	 * setString(...):  Set the string from which data will be extracted,
	 * replacing any existing string.
	 *
	 * [Parameters]
	 *         string: String to use for extraction.
	 *     reuse_flag: Flag indicating buffer reuse semantics for "char *"
	 *                    string buffers.
	 * [Return value]
	 *     None
	 */
	inline void setString(char *string, int reuse_flag = COPY);
	inline void setString(const char *string, int reuse_flag = COPY);
	inline void setString(const std::string &string);

	/**
	 * operator=(char *), operator=(const char *), operator=(std::string):
	 * Set the string from which data will be extracted.  Exactly
	 * equivalent to setString(string).
	 *
	 * [Parameters]
	 *     string: Assignment source
	 * [Return value]
	 *     this
	 */
	inline SimpleIStringStream &operator=(char *string);
	inline SimpleIStringStream &operator=(const char *string);
	inline SimpleIStringStream &operator=(const std::string &string);

	/**
	 * operator=(SimpleIStringStream):  Make a copy of an existing stream.
	 * The input string buffer (if any) will always be copied, regardless
	 * of any reuse_flag passed to the original stream.
	 *
	 * [Parameters]
	 *     stream: Assignment source
	 * [Return value]
	 *     this
	 */
	inline SimpleIStringStream &operator=(const SimpleIStringStream &stream);

	/*------------------------------*/

	/**
	 * operator void*:  Evaluate the stream as a boolean.  Returns NULL if
	 * an error has occurred, this otherwise.  (This allows the stream to
	 * be used in a while loop like "while (stream >> var)", in the same
	 * way as ordinary std::istringstream objects.)
	 *
	 * [Parameters]
	 *     None
	 * [Return value]
	 *     False if an error has occurred, else true
	 */
	inline operator void*() const;

	/**
	 * operator>>:  Extract a value from a stream.  String extraction skips
	 * leading whitespace and stops at the first following whitespace
	 * character.
	 *
	 * [Parameters]
	 *     target: Target for storing extracted value (right-hand operand)
	 * [Return value]
	 *     this
	 */
	inline SimpleIStringStream &operator>>(bool &target);
	inline SimpleIStringStream &operator>>(short &target);
	inline SimpleIStringStream &operator>>(unsigned short &target);
	inline SimpleIStringStream &operator>>(int &target);
	inline SimpleIStringStream &operator>>(unsigned int &target);
	inline SimpleIStringStream &operator>>(long &target);
	inline SimpleIStringStream &operator>>(unsigned long &target);
	inline SimpleIStringStream &operator>>(float &target);
	inline SimpleIStringStream &operator>>(double &target);
	inline SimpleIStringStream &operator>>(char &target);
	inline SimpleIStringStream &operator>>(signed char &target);
	inline SimpleIStringStream &operator>>(unsigned char &target);
	inline SimpleIStringStream &operator>>(std::string &target);

	/*-------------------------------------------------------------------*/

  private:
	char *buffer;       // The buffer we're parsing.
	char *position;     // Our current position in the buffer.
	bool freeOnDestroy; // Should we free the buffer when we're destroyed?
	bool error;         // Current error status.

#ifdef SISS_VERIFY
	std::istringstream std_is;
	unsigned long ispos;
#endif

	/* Is the given character a whitespace character? */
	inline bool my_isspace(char c) const {
		return c==' ' || c=='\t' || c=='\r' || c=='\n' || c=='\v';
	}

	/* Skip over leading whitespace.  Assumes "position" is valid. */
	inline void skip_spaces() {
		while (my_isspace(*position)) {
			position++;
		}
	}

};  // class SimpleIStringStream

/*************************************************************************/
/*********************** Inline method definitions ***********************/
/*************************************************************************/

/* Helper macros used with SISS_VERIFY to verify result values.  "type" is
 * the type of data being read, and "def_value" is the default value to
 * test against when the stream reports an error. */

#ifdef SISS_VERIFY

extern void debugLog(const std::string &s);

#define VERIFY_SETUP \
	char *old_position = position;

#define VERIFY(type,def_value)  do {                                    \
	type test;                                                          \
	std_is >> test;                                                     \
	ispos += std_is.gcount();                                           \
	if (std_is.fail()) {                                                \
		test = def_value;                                               \
	}                                                                   \
	if (test != target) {                                               \
		std::ostringstream os_target; os_target   << target;            \
		std::ostringstream os_test;   os_test     << test;              \
		std::ostringstream os_offset; os_offset << (position - buffer); \
		debugLog(std::string("SimpleIStringStream >> " #type ": MISMATCH") \
		         + " (us=" + os_target.str()                            \
		         + " STL=" + (std_is.fail() ? "<<FAIL>>" : os_test.str()) \
		         + ") at: [" + std::string(old_position).substr(0,100)  \
		         + "] ([" + std::string(buffer).substr(0,100) + "] + "  \
		         + os_offset.str() + ")");                              \
	}                                                                   \
} while (0)

#else

#define VERIFY_SETUP            /*nothing*/
#define VERIFY(type,def_value)  /*nothing*/

#endif

/*************************************************************************/

inline SimpleIStringStream::SimpleIStringStream()
{
	buffer = NULL;
	position = NULL;
	freeOnDestroy = false;
	error = false;
}

inline SimpleIStringStream::SimpleIStringStream(char *string, int reuse_flag)
{
	freeOnDestroy = false;  // Don't try to free the uninitialized pointer.
	setString(string, reuse_flag);
}

inline SimpleIStringStream::SimpleIStringStream(const char *string, int reuse_flag)
{
	freeOnDestroy = false;
	setString(string, reuse_flag);
}

inline SimpleIStringStream::SimpleIStringStream(std::string string)
{
	freeOnDestroy = false;
	setString(string);
}

inline SimpleIStringStream::~SimpleIStringStream()
{
	if (freeOnDestroy) {
		delete[] buffer;
	}
}

/*-----------------------------------------------------------------------*/

inline void SimpleIStringStream::setString(char *string, int reuse_flag)
{
	if (freeOnDestroy) {
		delete[] buffer;
	}

	if (reuse_flag == REUSE) {
		buffer = string;
		freeOnDestroy = false;
	} else if (reuse_flag == TAKE_OVER) {
		buffer = string;
		freeOnDestroy = true;
	} else {
		const size_t size = strlen(string) + 1;
		buffer = new char[size];
		freeOnDestroy = true;
		if (buffer) {
			memcpy(buffer, string, size);
		}
	}
	position = buffer;
	error = false;
#ifdef SISS_VERIFY
	if (buffer) {
		std_is.str(buffer);
		std_is.clear();
		ispos = 0;
	}
#endif
}

inline void SimpleIStringStream::setString(const char *string, int reuse_flag)
{
	if (freeOnDestroy) {
		delete[] buffer;
	}

	if (reuse_flag == REUSE) {
		/* We never actually write to the buffer, and we won't attempt to
		 * free it, so this cast is safe. */
		buffer = const_cast<char *>(string);
		freeOnDestroy = false;
	} else {
		/* It makes no sense to TAKE_OVER a const char *, since we could
		 * never free it, so we treat TAKE_OVER like COPY. */
		const size_t size = strlen(string) + 1;
		buffer = new char[size];
		freeOnDestroy = true;
		if (buffer) {
			memcpy(buffer, string, size);
		}
	}
	position = buffer;
	error = false;
#ifdef SISS_VERIFY
	if (buffer) {
		std_is.str(buffer);
		std_is.clear();
		ispos = 0;
	}
#endif
}

inline void SimpleIStringStream::setString(const std::string &string)
{
	setString(string.c_str(), COPY);
}

inline SimpleIStringStream &SimpleIStringStream::operator=(char *string)
{
	setString(string);
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator=(const char *string)
{
	setString(string);
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator=(
	const std::string &string)
{
	setString(string);
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator=(
	const SimpleIStringStream &stream)
{
	/* Watch out that we don't try to assign ourselves, or we'll raise a
	 * use-after-free bug. */
	if (&stream == this) {
		return *this;
	}

	setString(stream.buffer, COPY);
	return *this;
}

/*-----------------------------------------------------------------------*/

inline SimpleIStringStream::operator void*() const
{
#ifdef SISS_VERIFY
	if (!error != bool(std_is)) {
		std::ostringstream os_offset; os_offset << (position - buffer);
		debugLog(std::string("SimpleIStringStream bool MISMATCH: us=")
		         + (!error ? "true" : "false") + " STL="
		         + (std_is ? "true" : "false") + " at: ["
		         + std::string(position).substr(0,100) + "] (["
		         + std::string(buffer).substr(0,100) + "] + "
		         + os_offset.str() + ")");
	}
#endif
	return error ? NULL : (void*)this;
}

/*-----------------------------------------------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(bool &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = false;
		} else {
			skip_spaces();
			unsigned long longval = strtoul(position, &position, 10);
			target = (longval == 1);
			error = (position == old_position || *position == 0);
		}
		VERIFY(bool, false);
	} else {
		target = false;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(short &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = static_cast<short>(strtol(position, &position, 0));
			error = (position == old_position || *position == 0);
		}
		VERIFY(short, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(unsigned short &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = static_cast<unsigned short>(strtoul(position, &position, 0));
			error = (position == old_position || *position == 0);
		}
		VERIFY(unsigned short, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(int &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = static_cast<int>(strtol(position, &position, 0));
			error = (position == old_position || *position == 0);
		}
		VERIFY(int, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(unsigned int &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = static_cast<unsigned int>(strtoul(position, &position, 0));
			error = (position == old_position || *position == 0);
		}
		VERIFY(unsigned int, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(long &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = strtol(position, &position, 0);
			error = (position == old_position || *position == 0);
		}
		VERIFY(long, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(unsigned long &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = strtoul(position, &position, 0);
			error = (position == old_position || *position == 0);
		}
		VERIFY(unsigned long, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(float &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = strtof(position, &position);
			error = (position == old_position || *position == 0);
		}
		VERIFY(float, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(double &target)
{
	if (position) {
		char *old_position = position;
		if (error) {
			target = 0;
		} else {
			skip_spaces();
			target = strtod(position, &position);
			error = (position == old_position || *position == 0);
		}
		VERIFY(double, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(char &target)
{
	if (position) {
#ifdef SISS_VERIFY
		char *old_position = position;
#endif
		if (error) {
			target = 0;
		} else {
			target = *position;
			if (*position) {
				position++;
				error = (*position == 0);
			} else {
				error = true;
			}
		}
		VERIFY(char, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(signed char &target)
{
	if (position) {
#ifdef SISS_VERIFY
		char *old_position = position;
#endif
		if (error) {
			target = 0;
		} else {
			target = *position;
			if (*position) {
				position++;
				error = (*position == 0);
			} else {
				error = true;
			}
		}
		VERIFY(signed char, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

inline SimpleIStringStream &SimpleIStringStream::operator>>(unsigned char &target)
{
	if (position) {
#ifdef SISS_VERIFY
		char *old_position = position;
#endif
		if (error) {
			target = 0;
		} else {
			target = static_cast<unsigned char>(*position);
			if (*position) {
				position++;
				error = (*position == 0);
			} else {
				error = true;
			}
		}
		VERIFY(unsigned char, 0);
	} else {
		target = 0;
		error = true;
	}
	return *this;
}

/*----------------------------------*/

inline SimpleIStringStream &SimpleIStringStream::operator>>(std::string &target)
{
	if (position) {
#ifdef SISS_VERIFY
		char *old_position = position;
#endif
		if (error) {
			target = "";
		} else {
			skip_spaces();
			if (!*position) {
				target = "";
				error = true;
			} else {
				char *start = position;
				while (*position && !my_isspace(*position)) {
					position++;
				}
				target.assign(start, static_cast<size_t>(position - start));
				error = (*position == 0);
			}
		}
		VERIFY(std::string, "");
	} else {
		target = "";
		error = true;
	}
	return *this;
}

/*************************************************************************/

#undef VERIFY_SETUP
#undef VERIFY

#endif // SIMPLEISTRINGSTREAM_H

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   indent-tabs-mode: t
 *   tab-width: 4
 * End:
 *
 * vim: shiftwidth=4:
 */
