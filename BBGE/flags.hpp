//  flags.hpp
//
//  Copyright (c) 2004 Eugene Gladyshev
//
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.
//

#ifndef __ttl_flg_flags__hpp
#define __ttl_flg_flags__hpp

#include <algorithm>

namespace ttl
{
namespace flg
{
	namespace impl
	{
		template< 
				int Bits, 
				int type = Bits <= sizeof(char)*8?1:(Bits <= sizeof(short)*8?2:(Bits <= sizeof(int)*8?3:4))
			> struct bestfit;

		template< int Bits >
		struct bestfit<Bits, 1>
		{
			typedef unsigned char type;
		};

		template< int Bits >
		struct bestfit<Bits, 2>
		{
			typedef unsigned short type;
		};

		template< int Bits >
		struct bestfit<Bits, 3>
		{
			typedef unsigned int type;
		};

		template< int Bits >
		struct bestfit<Bits, 4>
		{
			typedef unsigned int type;
		};
	};
	
	template< typename T, int Bits = sizeof(int)*8, typename Holder = typename impl::bestfit<Bits>::type >
	struct flags
	{
		typedef flags this_t;
		typedef T value_type;

		Holder f_;

		flags() : f_(0) {}
		flags( T f1 ) : f_(f1) {}
		flags( T f1, T f2 ) : f_(f1|f2) {}
		flags( T f1, T f2, T f3 ) : f_(f1|f2|f3) {}
		flags( T f1, T f2, T f3, T f4 ) : f_(f1|f2|f3|f4) {}
		flags( T f1, T f2, T f3, T f4, T f5 ) : f_(f1|f2|f3|f4|f5) {}
		flags( T f1, T f2, T f3, T f4, T f5, T f6 ) : f_(f1|f2|f3|f4|f5|f6) {}
		flags( T f1, T f2, T f3, T f4, T f5, T f6, T f7 ) : f_(f1|f2|f3|f4|f5|f6|f7) {}
		flags( T f1, T f2, T f3, T f4, T f5, T f6, T f7, T f8 ) : f_(f1|f2|f3|f4|f5|f6|f7|f8) {}
		flags( T f1, T f2, T f3, T f4, T f5, T f6, T f7, T f8, T f9 ) : f_(f1|f2|f3|f4|f5|f6|f7|f8|f9) {}
		flags( T f1, T f2, T f3, T f4, T f5, T f6, T f7, T f8, T f9, T f10 ) : f_(f1|f2|f3|f4|f5|f6|f7|f8|f9|f10) {}

		this_t& operator |=( const this_t& f ) { f_ |= f.f_; return *this; }
		this_t& operator &=( const this_t& f ) { f_ &= f.f_; return *this; }

		bool operator ==( const this_t& l ) const { return f_ == l.f_; }
		bool operator !=( const this_t& l ) const { return f_ != l.f_; }

		bool operator !() { return f_ == 0; }
		
		Holder get_holder() const { return f_; }
		
		bool test( const this_t l ) const { return (f_ & l.f_)?true:false; }
		bool test() const { return f_!=0; }
	};

	template< typename T, int Bits, typename Holder >
	flags<T, Bits, Holder> operator ~(const flags<T, Bits, Holder>& l) 
	{ 
		flags<T, Bits, Holder> tmp;
		tmp.f_ = ~l.f_;
		return tmp; 
	}
	
	template< typename T, int Bits, typename Holder >
	flags<T, Bits, Holder> operator |(const flags<T, Bits, Holder>& l, const T& r) 
	{ 
		flags<T, Bits, Holder> tmp( r );
		return l|tmp; 
	}

	template< typename T, int Bits, typename Holder >
	flags<T, Bits, Holder> operator |(const flags<T, Bits, Holder>& l, const flags<T, Bits, Holder>& r) 
	{ 
		flags<T, Bits, Holder> tmp( l );
		tmp|=r;
		return tmp; 
	}

	template< typename T, int Bits, typename Holder >
	flags<T, Bits, Holder> operator &(const flags<T, Bits, Holder>& l, const T& r) 
	{ 
		flags<T, Bits, Holder> tmp( r );
		return l&tmp; 
	}

	template< typename T, int Bits, typename Holder >
	flags<T, Bits, Holder> operator &(const flags<T, Bits, Holder>& l, const flags<T, Bits, Holder>& r) 
	{ 
		flags<T, Bits, Holder> tmp( l );
		tmp&=r;
		return tmp; 
	}

	struct map_first2second {};
	struct map_second2first {};

	template < typename Pair, typename MapDirection = map_first2second >
	struct flag_mapper;
	
	template< typename Pair >
	struct flag_mapper<Pair, map_first2second>
	{
		typedef typename Pair::first_type type;
		typedef typename Pair::second_type result_type;
		
		const type& d_;
		result_type& r_;

		flag_mapper( const type& d, result_type& r ) : d_(d), r_(r) {}

		void operator()( const Pair& p ) const
		{
			if( (p.first&d_) == true ) r_ |= p.second;
		}
	};

	template< typename Pair >
	struct flag_mapper<Pair, map_second2first>
	{
		typedef typename Pair::second_type type;
		typedef typename Pair::first_type result_type;

		const type& d_;
		result_type r_;

		flag_mapper( const type& d ) : d_(d), r_() {}

		void operator()( const Pair& p ) const
		{
			if( (p.second&d_) == true ) r_ |= p.first;
		}
	};

	//It is a an iterator of elements like std::pair	
	template< typename It >
	typename It::value_type::second_type map( 
		const typename It::value_type::first_type f, 
		It first, It end )
	{
		typename It::value_type::second_type r;
		flag_mapper<typename It::value_type> m(f, r);
		std::for_each( first, end, m );
		return r;
	}
	
};	//flg
};

#endif //__flags__hpp

