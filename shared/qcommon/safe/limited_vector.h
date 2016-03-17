#pragma once

#include <array>
#include <utility>
#include <cassert>
#include <cstddef>
#include <algorithm>

#include "qcommon/q_platform.h"

namespace Q
{
	/**
	Like an std::vector with a fixed upper limit
	*/
	template< typename T, std::size_t maximum >
	class LimitedVector
	{
	private:
		using Memory = std::array< char, sizeof( T ) * maximum >;
	public:
		using value_type = T;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using iterator = T*;
		using const_iterator = const T*;
	public:
		LimitedVector() NOEXCEPT = default;
		~LimitedVector() NOEXCEPT
		{
			clear();
		}
		LimitedVector( const LimitedVector& rhs ) NOEXCEPT_IF( IS_NOEXCEPT(
			T{ static_cast< const T& >( std::declval< T >() ) }
			) )
		{
			*this = rhs;
		}
		LimitedVector& operator=( const LimitedVector& rhs ) NOEXCEPT_IF( IS_NOEXCEPT(
			T{ static_cast< const T& >( std::declval< T >() ) }
			) )
		{
			clear();
			std::copy( rhs.begin(), rhs.end(), std::back_inserter( *this ) );
			return *this;
		}
		LimitedVector( LimitedVector&& rhs ) NOEXCEPT_IF( IS_NOEXCEPT(
			T{ std::move( std::declval< T >() ) }
			) )
		{
			*this = std::move( rhs );
		}
		LimitedVector& operator=( LimitedVector&& rhs ) NOEXCEPT_IF( IS_NOEXCEPT(
			T{ std::move( std::declval< T >() ) }
			) )
		{
			clear();
			std::move( rhs.begin(), rhs.end(), std::back_inserter( *this ) );
			rhs.clear();
			return *this;
		}

		void swap( LimitedVector& rhs )
		{
			LimitedVector temp = std::move( rhs );
			rhs = std::move( *this );
			*this = std::move( temp );
		}

		size_type size() const NOEXCEPT
		{
			return _size;
		}

		bool empty() const NOEXCEPT
		{
			return size() == 0;
		}

		size_type max_size() const NOEXCEPT
		{
			return maximum;
		}

		template< typename... Args >
		bool emplace_back( Args&&... args ) NOEXCEPT_IF( IS_NOEXCEPT(
			T{ std::forward< Args >( std::declval< Args >() )... }
			) )
		{
			if( size() == max_size() )
			{
				return false;
			}
			T* memory = &data()[ size() ];
			// construct in-place (placement new)
			new( memory ) T( std::forward< Args >( args )... );
			++_size;
			return true;
		}

		bool push_back( const T& value ) NOEXCEPT_IF( IS_NOEXCEPT(
			emplace_back( static_cast< const T& >( std::declval< T >() ) )
			) )
		{
			return emplace_back( value );
		}

		bool push_back( T&& value ) NOEXCEPT_IF( IS_NOEXCEPT(
			emplace_back( std::move( std::declval< T >() ) )
			) )
		{
			return emplace_back( std::move( value ) );
		}

		void pop_back() NOEXCEPT
		{
			if( size() == 0 )
			{
				return;
			}
			( &( *this )[ size() - 1 ] )->~T();
			--_size;
		}

		void clear() NOEXCEPT
		{
			while( !empty() )
			{
				pop_back();
			}
		}

		const_pointer data() const NOEXCEPT
		{
			return reinterpret_cast< const T* >( _memory.data() );
		}

		pointer data() NOEXCEPT
		{
			return reinterpret_cast< T* >( _memory.data() );
		}

		const T& operator[]( size_type index ) const NOEXCEPT
		{
			assert( index < _size );
			return data()[ index ];
		}

		T& operator[]( size_type index ) NOEXCEPT
		{
			assert( index < _size );
			return data()[ index ];
		}

		iterator begin() NOEXCEPT
		{
			return{ data() };
		}

		const_iterator begin() const NOEXCEPT
		{
			return{ data() };
		}

		const_iterator cbegin() const NOEXCEPT
		{
			return begin();
		}

		iterator end() NOEXCEPT
		{
			return{ data() + size() };
		}

		const_iterator end() const NOEXCEPT
		{
			return{ data() + size() };
		}

		const_iterator cend() const NOEXCEPT
		{
			return end();
		}

		bool operator==( const LimitedVector& rhs ) const NOEXCEPT_IF( IS_NOEXCEPT(
			std::declval< T >() == std::declval< T >()
			) )
		{
			return size() == rhs.size() && std::equal( begin(), end(), rhs.begin() );
		}

		bool operator!=( const LimitedVector& rhs ) const NOEXCEPT_IF( IS_NOEXCEPT(
			std::declval< T >() == std::declval< T >()
			) )
		{
			return !( *this == rhs );
		}

	private:
		Memory _memory;
		size_type _size = 0;
	};
}
