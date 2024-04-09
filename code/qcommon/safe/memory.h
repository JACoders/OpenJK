#pragma once

#ifdef _JK2EXE
# include "qcommon/qcommon.h"
#else
# include "game/g_shared.h"
#endif

#include <type_traits>
#include <memory>

namespace Zone
{
	namespace detail
	{
		inline void* Malloc( int iSize, memtag_t eTag, qboolean bZeroit = qfalse )
		{
#ifdef _JK2EXE
			return Z_Malloc( iSize, eTag, bZeroit );
#else
			return gi.Malloc( iSize, eTag, bZeroit );
#endif
		}
		inline int Free( void* pvAddress )
		{
#ifdef _JK2EXE
			return Z_Free( pvAddress );
#else
			return gi.Free( pvAddress );
#endif
		}
	}
	/**
	Zone Deleter - for use with smart pointers.
	*/
	struct Deleter
	{
		void operator()( void* memory ) const NOEXCEPT
		{
			detail::Free( memory );
		}
	};

	/**
	Zone Allocator - for use with the STL
	*/
	template< typename T, memtag_t tag >
	struct Allocator
	{
		Allocator() = default;
		template< typename U >
		Allocator( const Allocator< U, tag >& )
		{
		};
		using value_type = T;
		using is_always_equal = std::true_type;
		T* allocate( std::size_t n ) const
		{
			void* mem = detail::Malloc( n * sizeof( T ), tag );
			return static_cast< T* >( mem );
		}
		void deallocate( T* mem, std::size_t n ) const NOEXCEPT
		{
			Deleter{}( mem );
		}
		template< typename T2, memtag_t tag2 >
		bool operator==( const Allocator< T2, tag2 >& ) const NOEXCEPT
		{
			return true; // free works regardless of size and tag
		}
		template< typename T2, memtag_t tag2 >
		bool operator!=( const Allocator< T2, tag2 >& ) const NOEXCEPT
		{
			return false;
		}
		// I think the standard guarantees automatic generation of this, but MSVC can't handle it.
		template< typename U >
		struct rebind
		{
			using other = Allocator< U, tag >;
		};
	};

	template< typename T >
	using UniquePtr = std::unique_ptr< T, Deleter >;

	/**
	make_unique using Zone Allocations (with appropriate deleter)
	*/
	template< typename T, memtag_t tag, typename... Args >
	inline UniquePtr< T > make_unique( Args&&... args )
	{
		UniquePtr< void > memory( Allocator< T, tag >{}.allocate( 1 ) );
		// placement new. may throw, in which case the unique_ptr will take care of freeing the memory.
		T* obj = new( memory )T( std::forward< Args >( args )... );
		memory.release();
		return UniquePtr< T >( obj );
	}
}
