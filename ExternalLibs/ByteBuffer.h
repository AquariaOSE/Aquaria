#ifndef BYTEBUFFER_H
#define BYTEBUFFER_H

#include <stdlib.h>
#include <string.h> // for memcpy
#include <stdio.h>
#include <string>
#include <algorithm>
#include "minipstdint.h"

#if defined(__GNUC__) && __GNUC__ <= 2
#  define BB_OLD_GNUC
#endif



// ** compatibility stuff for BBGE .... **


#define BYTEBUFFER_NO_EXCEPTIONS

// from SDL headers
#if defined(__hppa__) || \
	defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
	(defined(__MIPS__) && defined(__MISPEB__)) || \
	defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
	defined(__sparc__)
#define BB_IS_BIG_ENDIAN 1
#endif

// ****


namespace ByteBufferTools
{
	template<int T> inline void convert(char *val)
	{
		std::swap(*val, *(val + T - 1));
		convert<T - 2>(val + 1);
	}
	template<> inline void convert<0>(char *) {}
	template<> inline void convert<1>(char *) {}

	template<typename T> inline void EndianConvert(T *val)
	{
		convert<sizeof(T)>((char *)(val));
	}

	inline void EndianConvertRT(char *p, size_t size)
	{
		std::reverse(p, p + size);
	}

#if BB_IS_BIG_ENDIAN
	template<typename T> inline void ToLittleEndian(T& val) { EndianConvert<T>(&val); }
	inline void ToLittleEndianRT(void *p, size_t size) { EndianConvertRT((char*)p, size); }
	template<typename T> inline void ToBigEndian(T&) { }
	inline void ToBigEndianRT(void *p, size_t size) { }
#else
	template<typename T> inline void ToLittleEndian(T&) { }
	inline void ToLittleEndianRT(void *p, size_t size) { }
	template<typename T> inline void ToBigEndian(T& val) { EndianConvert<T>(&val); }
	inline void ToBigEndianRT(void *p, size_t size) { EndianConvertRT((char*)p, size); }
#endif

	template<typename T> void ToLittleEndian(T*);   // will generate link error
	template<typename T> void ToBigEndian(T*);      // will generate link error

}

#ifdef BB_OLD_GNUC
#  define BB_MAKE_WRITE_OP(T) inline ByteBuffer& operator<<(T val) { appendT(&val, sizeof(T)); return *this; }
#  define BB_MAKE_READ_OP(T) inline ByteBuffer& operator>>(T &val) { readT(&val, sizeof(T)); return *this; }
#else
#  define BB_MAKE_WRITE_OP(T) inline ByteBuffer& operator<<(T val) { append<T>(val); return *this; }
#  define BB_MAKE_READ_OP(T) inline ByteBuffer& operator>>(T &val) { val = read<T>(); return *this; }
#endif

class ByteBuffer
{
public:
	typedef void (*delete_func)(void*);
	typedef void *(*allocator_func)(size_t);

	enum Mode // for creation with existing pointers
	{
		COPY,  //- Make a copy of the buffer (default action).
		REUSE,   //- Use the passed-in buffer as is.  Requires the pointer
		//  to remain valid over the life of this object.
		TAKE_OVER //- Take over the passed-in buffer; it will be deleted on object destruction.
	};

	typedef int64_t      int64;
	typedef int32_t      int32;
	typedef int16_t      int16;
	typedef int8_t       int8;
	typedef uint64_t     uint64;
	typedef uint32_t     uint32;
	typedef uint16_t     uint16;
	typedef uint8_t      uint8;

	class Exception
	{
	public:
		Exception(const ByteBuffer *bb, const char *act, size_t sp = 0)
		{
			action = act;
			rpos = bb->rpos();
			wpos = bb->wpos();
			sizeparam = sp;
			cursize = bb->size();
		}
		size_t rpos, wpos, sizeparam, cursize;
		const char *action;
	};

#ifdef BYTEBUFFER_NO_EXCEPTIONS
#define BYTEBUFFER_EXCEPT(bb, desc, sz) { Exception __e(bb, desc, sz); \
	fprintf(stderr, "Exception in ByteBuffer: '%s', rpos: %u, wpos: %u, cursize: %u, sizeparam: %u", \
	__e.action, (unsigned)__e.rpos, (unsigned)__e.wpos, (unsigned)__e.cursize, (unsigned)__e.sizeparam);  abort(); }
#else
#define BYTEBUFFER_EXCEPT(bb, desc, sz) throw Exception(bb, desc, sz)
#endif

protected:

	uint8 *_buf; // the ptr to the buffer that holds all the bytes
	size_t _rpos, // read position, [0 ... _size]
		_wpos, // write position, [0 ... _size]
		_res,  // reserved buffer size, [0 ... _size ... _res]
		_size; // used buffer size
	delete_func _delfunc;
	allocator_func _allocfunc;
	bool _mybuf; // if true, destructor deletes buffer
	bool _growable; // default true, if false, buffer will not re-allocate more space

public:


	ByteBuffer()
                : _buf(NULL), _rpos(0), _wpos(0), _res(0), _size(0), _delfunc(NULL),
                _allocfunc(NULL), _mybuf(false), _growable(true)
	{
	}
	ByteBuffer(size_t res)
                : _buf(NULL), _rpos(0), _wpos(0), _res(0), _size(0), _delfunc(NULL),
                  _allocfunc(NULL), _mybuf(false), _growable(true)
	{
		_allocate(res);
	}
	ByteBuffer(ByteBuffer &buf, Mode mode = COPY, size_t extra = 0)
                : _buf(NULL), _rpos(0), _wpos(0), _res(0), _size(0), _delfunc(NULL),
                  _allocfunc(NULL), _mybuf(false), _growable(true)
        {
		init(buf, mode, extra);
	}
	// del param only used with TAKE_OVER, extra only used with COPY
	ByteBuffer(void *buf, size_t size, Mode mode = COPY, delete_func del = NULL, size_t extra = 0)
                : _buf(NULL), _rpos(0), _wpos(0), _res(0), _size(0), _delfunc(NULL),
                  _allocfunc(NULL), _mybuf(false), _growable(true)  // for mode == REUSE
	{
		init(buf, size, mode, del, extra);
	}

	void init(void *buf, size_t size, Mode mode = COPY, delete_func del = NULL, size_t extra = 0)
	{
		_mybuf = false;
		switch(mode)
		{
			case COPY:
				_allocate(size + extra);
				append(buf, size);
				break;

			case TAKE_OVER:
				_mybuf = true; // fallthrough
			case REUSE:
				_buf = (uint8*)buf;
				_res = size;
				_size = size;
		}
	}

	void init(ByteBuffer& bb, Mode mode = COPY, size_t extra = 0)
	{
		_allocfunc = bb._allocfunc;

		switch(mode)
		{
			case COPY:
				reserve(bb.size() + extra);
				append(bb);
				break;

			case TAKE_OVER:
			case REUSE:
				_mybuf = bb._mybuf;
				_delfunc = bb._delfunc;
				_buf = bb._buf;
				_res = bb._res;
				_size = bb._size;
				_growable = bb._growable;
				break;
		}

		if(mode == TAKE_OVER)
		{
			bb._buf = NULL;
			bb._size = 0;
			bb._res = 0;
		}
	}

	virtual ~ByteBuffer()
	{
		clear();
	}

	void clear(void)
	{
		_delete();
		reset();
	}

	inline void reset(void)
	{
		_rpos = _wpos = _size = 0;
	}

	void resize(size_t newsize)
	{
		reserve(newsize);
		_rpos = 0;
		_wpos = newsize;
		_size = newsize;
	}

	void reserve(size_t newsize)
	{
		if(_res < newsize)
			_allocate(newsize);
	}

	// ---------------------- Write methods -----------------------

	BB_MAKE_WRITE_OP(char);
	BB_MAKE_WRITE_OP(uint8);
	BB_MAKE_WRITE_OP(uint16);
	BB_MAKE_WRITE_OP(uint32);
	BB_MAKE_WRITE_OP(uint64);
	BB_MAKE_WRITE_OP(float);
	BB_MAKE_WRITE_OP(double);

	ByteBuffer &operator<<(const char *str)
	{
		append((uint8 *)str, str ? strlen(str) : 0);
		appendByte(0);
		return *this;
	}

	ByteBuffer &operator<<(const std::string &value)
	{
		append((uint8 *)value.c_str(), value.length());
		appendByte(0);
		return *this;
	}

	// -------------------- Read methods --------------------

	BB_MAKE_READ_OP(char);
	BB_MAKE_READ_OP(uint8);
	BB_MAKE_READ_OP(uint16);
	BB_MAKE_READ_OP(uint32);
	BB_MAKE_READ_OP(uint64);
	BB_MAKE_READ_OP(float);
	BB_MAKE_READ_OP(double);

	inline uint8 operator[](size_t pos) const
	{
		if(pos >= size())
			BYTEBUFFER_EXCEPT(this, "operator[]", 1);
		return _buf[pos];
	}

	ByteBuffer &operator>>(std::string& value)
	{
		value.clear();
		char c;
		while(readable() && (c = readByte()))
			value += c;
		return *this;
	}

	// --------------------------------------------------

	size_t rpos() const { return _rpos; }
	size_t rpos(size_t rpos)
	{
		_rpos = rpos < size() ? rpos : size();
		return _rpos;
	}

	size_t wpos() const { return _wpos; }
	size_t wpos(size_t wpos)
	{
		_wpos = wpos < size() ? wpos : size();
		return _wpos;
	}

	template <typename T> T read()
	{
		const size_t pos = _rpos;
		_rpos += sizeof(T);
		return read<T>(pos);
	}

	template <typename T> T read(size_t pos) const
	{
		if(pos + sizeof(T) > size())
			BYTEBUFFER_EXCEPT(this, "read", sizeof(T));
		T val = *((T const*)(_buf + pos));
		ByteBufferTools::ToLittleEndian<T>(val);
		return val;
	}

	inline uint8 readByte()
	{
		if (_rpos < size())
			return _buf[_rpos++];
		BYTEBUFFER_EXCEPT(this, "readByte", 1);
		return 0;
	}

	void readT(void *dest, size_t len)
	{
		read(dest, len);
		ByteBufferTools::ToLittleEndianRT(dest, len);
	}

	void read(void *dest, size_t len)
	{
		if (_rpos + len <= size())
			memcpy(dest, &_buf[_rpos], len);
		else
			BYTEBUFFER_EXCEPT(this, "read-into", len);
		_rpos += len;
	}

	void skipRead(size_t len)
	{
		_rpos += len;
	}

	inline const uint8 *contents() const { return _buf; }
	inline       uint8 *contents()       { return _buf; }

	inline const void *ptr() const { return _buf; }
	inline       void *ptr()       { return _buf; }

	inline size_t size() const { return _size; }

	inline size_t bytes() const { return size(); }
	inline size_t bits() const { return bytes() * 8; }

	inline size_t capacity() const { return _res; }

	inline size_t readable(void) const { return size() - rpos(); }
	inline size_t writable(void) const { return size() - wpos(); } // free space left before realloc will occur

	template <typename T> inline void append(T value)
	{
		ByteBufferTools::ToLittleEndian<T>(value);
		_enlargeIfReq(_wpos + sizeof(T));
		*((T*)(_buf + _wpos)) = value;
		_wpos += sizeof(T);
		if(_size < _wpos)
			_size = _wpos;
	}

	inline void appendByte(uint8 value)
	{
		_enlargeIfReq(_wpos + 1);
		_buf[_wpos++] = value;
		if(_size < _wpos)
			_size = _wpos;
	}

	// GCC 2.95 fails with an internal error in the template function above
	void appendT(const void *src, size_t bytes)
	{
		append(src, bytes);
		ByteBufferTools::ToLittleEndianRT(_buf + (_wpos - bytes), bytes);
	}

	void append(const void *src, size_t bytes)
	{
		if (!bytes) return;
		_enlargeIfReq(_wpos + bytes);
		memcpy(_buf + _wpos, src, bytes);
		_wpos += bytes;
		if(_size < _wpos)
			_size = _wpos;
	}
	void append(const ByteBuffer& buffer)
	{
		if(buffer.size())
			append(buffer.contents(), buffer.size());
	}

	void put(size_t pos, const void *src, size_t bytes)
	{
		memcpy(_buf + pos, src, bytes);
	}

	template <typename T> void put(size_t pos, const T& value)
	{
		if(pos >= size())
			BYTEBUFFER_EXCEPT(this, "put", sizeof(T));

		ByteBufferTools::ToLittleEndian<T>(value);
		*((T*)(_buf + pos)) = value;
	}

	inline bool growable(void) { return _growable; }
	inline void growable(bool b) { _growable = b; }

	// dangerous functions

	void _setPtr(void *p)
	{
		_buf = (uint8*)p;
	}

	void _setAllocFunc(allocator_func f)
	{
		_allocfunc = f;
	}

	void _setDelFunc(delete_func f)
	{
		_delfunc = f;
	}

	void _setSize(size_t s)
	{
		_size = s;
	}

	void _setReserved(size_t s)
	{
		_res = s;
	}

protected:

	void _delete(void)
	{
		if(_mybuf)
		{
			if(_delfunc)
				_delfunc(_buf);
			else
				delete [] _buf;
			_buf = NULL;
			_res = 0;
		}
	}

	// allocate larger buffer and copy contents. if we own the current buffer, delete old, otherwise, leave it as it is.
	void _allocate(size_t s)
	{
		if(!_growable && _buf) // only throw if we already have a buf
			BYTEBUFFER_EXCEPT(this, "_alloc+locked", s);

		// dangerous: It's up to the user to be sure that _allocfunc and _delfunc are matching
		uint8 *newbuf = (uint8*)(_allocfunc ? _allocfunc(s) : new char[s]);
		if(_buf)
		{
			memcpy(newbuf, _buf, _size);
			_delete();
		}
		_buf = newbuf;
		_res = s;
		_mybuf = true;

		if (!_allocfunc)
			_delfunc = NULL;
	}

	void _enlargeIfReq(size_t minSize)
	{
		if(_res < minSize)
		{
			size_t a = _res * 2;
			if(a < minSize) // fallback if doubling the space was not enough
				a += minSize;
			_allocate(a);
		}
	}


};


#undef BB_MAKE_WRITE_OP
#undef BB_MAKE_READ_OP
#undef BB_IS_BIG_ENDIAN



#endif
