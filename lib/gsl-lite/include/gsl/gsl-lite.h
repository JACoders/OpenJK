//
// gsl-lite is based on GSL: Guidelines Support Library,
// https://github.com/microsoft/gsl
//
// Copyright (c) 2015 Martin Moene
// Copyright (c) 2015 Microsoft Corporation. All rights reserved. 
// 
// This code is licensed under the MIT License (MIT). 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
// THE SOFTWARE. 

#pragma once

#ifndef GSL_GSL_LITE_H_INCLUDED
#define GSL_GSL_LITE_H_INCLUDED

#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#define  gsl_lite_VERSION "0.0.0"

// Configuration:

#ifndef  gsl_FEATURE_HAVE_IMPLICIT_MACRO
# define gsl_FEATURE_HAVE_IMPLICIT_MACRO  1
#endif

#ifndef  gsl_FEATURE_HAVE_OWNER_MACRO
# define gsl_FEATURE_HAVE_OWNER_MACRO  1
#endif

#ifndef  gsl_CONFIG_THROWS_FOR_TESTING
# define gsl_CONFIG_THROWS_FOR_TESTING  0
#endif

#ifndef  gsl_CONFIG_CONFIRMS_COMPILATION_ERRORS
# define gsl_CONFIG_CONFIRMS_COMPILATION_ERRORS  0
#endif

// Compiler detection:

#if defined(_MSC_VER)
# define gsl_COMPILER_MSVC_VERSION   (_MSC_VER / 100 - 5 - (_MSC_VER < 1900))
#else
# define gsl_COMPILER_MSVC_VERSION   0
# define gsl_COMPILER_NON_MSVC       1
#endif

#if ( __cplusplus >= 201103L )
# define gsl_CPP11_OR_GREATER  1
#endif

#if ( __cplusplus >= 201402L )
# define gsl_CPP14_OR_GREATER  1
#endif

// half-open range [lo..hi):
#define gsl_BETWEEN( v, lo, hi ) ( lo <= v && v < hi )

// Presence of C++ language features:

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_ALIAS_TEMPLATE  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 10
# define gsl_HAVE_AUTO  1
#endif

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_CONSTEXPR_11  1
#endif

#if gsl_CPP14_OR_GREATER
# define gsl_HAVE_CONSTEXPR_14  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 14
# define gsl_HAVE_ENUM_CLASS  1
#endif

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_IS_DEFAULT_CTOR  1
#endif

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_EXPLICIT_CONVERSION  1
#endif

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_INITIALIZER_LIST  1
#endif

#if gsl_CPP11_OR_GREATER
# define gsl_HAVE_NOEXCEPT  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 10
# define gsl_HAVE_NULLPTR  1
#endif

// Presence of C++ library features:

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 11
# define gsl_HAVE_ARRAY  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 9
# define gsl_HAVE_CONTAINER_DATA_METHOD  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 14
# define gsl_HAVE_SIZED_TYPES  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 10
# define gsl_HAVE_SHARED_PTR  1
# define gsl_HAVE_UNIQUE_PTR  1
#endif

#if gsl_CPP11_OR_GREATER || gsl_COMPILER_MSVC_VERSION >= 11
# define gsl_HAVE_TYPE_TRAITS  1
#endif

// For the rest, consider VC12, VC14 as C++11 for GSL Lite:

#if gsl_COMPILER_MSVC_VERSION >= 12
# undef  gsl_CPP11_OR_GREATER
# define gsl_CPP11_OR_GREATER  1
#endif

// C++ feature usage:

#if gsl_HAVE_CONSTEXPR_11
# define gsl_constexpr constexpr
#else
# define gsl_constexpr /*nothing*/
#endif

#if gsl_HAVE_CONSTEXPR_14
# define gsl_constexpr14 constexpr
#else
# define gsl_constexpr14 /*nothing*/
#endif

#if gsl_HAVE_EXPLICIT_CONVERSION
# define gsl_explicit explicit
#else
# define gsl_explicit /*nothing*/
#endif

#if gsl_FEATURE_HAVE_IMPLICIT_MACRO
# define implicit
#endif

#if !gsl_HAVE_NOEXCEPT || gsl_CONFIG_THROWS_FOR_TESTING
# define gsl_noexcept /*nothing*/
#else
# define gsl_noexcept noexcept
#endif

#define gsl_DIMENSION_OF( a ) ( sizeof(a) / sizeof(0[a]) )

#if gsl_HAVE_ARRAY
# include <array>
#endif

#if gsl_HAVE_TYPE_TRAITS
# include <type_traits>
#endif

namespace gsl {

//
// GSL.owner: ownership pointers 
//
#if gsl_HAVE_SHARED_PTR
  using std::unique_ptr;
  using std::shared_ptr;
#endif

#if gsl_HAVE_ALIAS_TEMPLATE
  template< class T > using owner = T;
#else
  template< class T > struct owner { typedef T type; };
#endif

#define gsl_HAVE_OWNER_TEMPLATE  gsl_HAVE_ALIAS_TEMPLATE

#if gsl_FEATURE_HAVE_OWNER_MACRO
# if gsl_HAVE_OWNER_TEMPLATE 
#  define Owner(t)  ::gsl::owner<t>
# else
#  define Owner(t)  ::gsl::owner<t>::type
# endif
#endif 

//
// GSL.assert: assertions
//
#define Expects(x)  ::gsl::fail_fast_assert((x))
#define Ensures(x)  ::gsl::fail_fast_assert((x))

#if gsl_CONFIG_THROWS_FOR_TESTING

struct fail_fast : public std::runtime_error 
{
    fail_fast() 
    : std::runtime_error( "GSL assertion" ) {}
    
    explicit fail_fast( char const * const message ) 
    : std::runtime_error( message ) {}
};

inline void fail_fast_assert( bool cond ) 
{ 
    if ( !cond ) 
        throw fail_fast(); 
}

inline void fail_fast_assert( bool cond, char const * const message ) 
{ 
    if ( !cond ) 
        throw fail_fast( message ); 
}

#else // gsl_CONFIG_THROWS_FOR_TESTING

inline void fail_fast_assert( bool cond ) 
{ 
    if ( !cond ) 
        std::terminate(); 
}

inline void fail_fast_assert( bool cond, char const * const ) 
{ 
    if ( !cond ) 
        std::terminate(); 
}

#endif // gsl_CONFIG_THROWS_FOR_TESTING

//
// GSL.util: utilities
//

#if gsl_CPP11_OR_GREATER

template< class Fn >
class Final_act
{
public:
    explicit Final_act( Fn action ) gsl_noexcept
    : action_( std::move( action ) ) {}
    
    ~Final_act() gsl_noexcept { action_(); }

private:
    Fn action_;
};

template< class Fn >
Final_act<Fn> finally( Fn const & action ) gsl_noexcept
{ 
    return Final_act<Fn>( action ); 
}

template< class Fn >
Final_act<Fn> finally( Fn && action ) gsl_noexcept
{ 
    return Final_act<Fn>( std::forward<Fn>( action ) ); 
}

#else // gsl_CPP11_OR_GREATER

class Final_act
{
public:
    typedef void (*Action)();

    Final_act( Action action ) 
    : action_( action ) {}

    ~Final_act() 
    {
        action_();
    }

private:    
    Action action_;
};

template< class Fn >
Final_act finally( Fn const & f )
{
    return Final_act(( f ));
}

#endif // gsl_CPP11_OR_GREATER

template< class T, class U >
T narrow_cast( U u ) gsl_noexcept
{ 
    return static_cast<T>( u ); 
}

struct narrowing_error : public std::exception {};

template< class T, class U >
T narrow( U u ) 
{ 
    T t = narrow_cast<T>( u ); 
    
    if ( static_cast<U>( t ) != u ) 
    {
        throw narrowing_error(); 
    }
    return t; 
}

//
// GSL.views: views
//

//
// at() - Bounds-checked way of accessing static arrays, std::array, std::vector.
//

template< class T, size_t N >
T & at( T(&arr)[N], size_t index ) 
{ 
    Expects( index < N ); 
    return arr[index]; 
}

#if gsl_HAVE_ARRAY

template< class T, size_t N >
T & at( std::array<T, N> & arr, size_t index ) 
{ 
    Expects( index < N ); 
    return arr[index]; 
}
#endif

template< class Cont >
typename Cont::value_type & at( Cont & cont, size_t index ) 
{ 
    Expects( index < cont.size() ); 
    return cont[index]; 
}

//
// not_null<> - Wrap any indirection and enforce non-null.
//
template<class T>
class not_null
{
public:
    not_null(             T t         ) : ptr_ ( t ){ Expects( ptr_ != NULL ); }
    not_null & operator=( T const & t ) { ptr_ = t ;  Expects( ptr_ != NULL ); return *this; }

    not_null(             not_null const & other ) : ptr_ ( other.ptr_  ) {}
    not_null & operator=( not_null const & other ) { ptr_ = other.ptr_; }

#if gsl_HAVE_DEFAULT_FUNCTION_TEMPLATE_ARG

    template< typename U, typename Dummy = 
        typename std::enable_if<std::is_convertible<U, T>::value, void>::type > 
    not_null( not_null<U> const & other ) : ptr_( other.get() ) {}

    template< typename U, typename Dummy = 
        typename std::enable_if<std::is_convertible<U, T>::value, void>::type > 
    not_null & operator=( not_null<U> const & other ) 
    { 
        ptr_ = other.get(); 
        return *this; 
    }

#else

    template< typename U > 
    not_null( not_null<U> const & other ) : ptr_( other.get() ) {}

    template< typename U > 
    not_null & operator=( not_null<U> const & other ) 
    { 
        ptr_ = other.get(); 
        return *this; 
    }
#endif

private:
    // Prevent compilation when initialized with a nullptr or literal 0:
#if gsl_HAVE_NULLPTR
    not_null(             std::nullptr_t );
    not_null & operator=( std::nullptr_t );
#endif
    not_null(             int );
    not_null & operator=( int );

public:
    T get() const 
    {
#if gsl_HAVE_NULLPTR && gsl_COMPILER_MSVC_VERSION > 0
        // The assume() should help the optimizer:
        __assume( ptr_ != nullptr );
#endif
        return ptr_;
    } 
    
         operator T() const { return get(); }
    T    operator->() const { return get(); }

    bool operator==(T const & rhs) const { return    ptr_ == rhs; }
    bool operator!=(T const & rhs) const { return !(*this == rhs); }

private:
    T ptr_;

    not_null & operator++();
    not_null & operator--();
    not_null   operator++( int );
    not_null   operator--( int );
    not_null & operator+ ( size_t );
    not_null & operator+=( size_t );
    not_null & operator- ( size_t );
    not_null & operator-=( size_t );
};

//
// Byte-specific type.
//
#if gsl_HAVE_ENUM_CLASS
# include <cstdint>
  enum class byte : std::uint8_t {};
#elif gsl_HAVE_SIZED_TYPES
# include <cstdint>
  typedef std::uint8_t byte;
#else
  typedef unsigned char byte;
#endif

//
// array_view<> - A 1D view of contiguous T's, replace (*,len).
//
template< class T >
class array_view
{
    template< class U > friend class array_view;
   
public:
    typedef size_t size_type;

    typedef T value_type;
    typedef T & reference;
    typedef T * pointer;
    typedef T const * const_pointer;

    typedef pointer       iterator;
    typedef const_pointer const_iterator;
    
#if gsl_BETWEEN( gsl_COMPILER_MSVC_VERSION, 6, 7 )
    typedef std::reverse_iterator< iterator, T >             reverse_iterator;
    typedef std::reverse_iterator< const_iterator, const T > const_reverse_iterator;
#else
    typedef std::reverse_iterator< iterator >                reverse_iterator;
    typedef std::reverse_iterator< const_iterator >          const_reverse_iterator;
#endif

    typedef typename std::iterator_traits< iterator >::difference_type difference_type;    

    gsl_constexpr14 array_view()
        : begin_( NULL )
        , end_  ( NULL )
    {
        Expects( size() == 0 );
    }

#if gsl_HAVE_NULLPTR
    gsl_constexpr14 array_view( std::nullptr_t, size_type size )
        : begin_( nullptr )
        , end_  ( nullptr )
    {
        Expects( size == 0 );
    }
#endif    

    gsl_constexpr14 array_view( pointer begin, pointer end )
        : begin_( begin )
        , end_  ( end )
    {
        Expects( begin <= end );
    }

    gsl_constexpr14 array_view( pointer & data, size_type size )
        : begin_( data )
        , end_  ( data + size )
    {
        Expects( size == 0 || ( size > 0 && data != NULL ) );
    }

    template< class U, size_t N >
    gsl_constexpr14 array_view( U (&arr)[N] )
        : begin_( arr )
        , end_  ( arr + N )
    {}

    template< class U, size_t N >
    gsl_constexpr14 array_view( U (&arr)[N], size_type size )
        : begin_( arr )
        , end_  ( arr + size )
    {
        Expects( size <= N );
    }

#if gsl_HAVE_ARRAY
    template< class U, size_t N >
    gsl_constexpr14 array_view( std::array< U, N > & arr ) 
        : begin_( arr.data() )
        , end_  ( arr.data() + N )
    {}
#endif

    template< class Cont >
    gsl_constexpr14 array_view( Cont & cont ) 
#if gsl_HAVE_CONTAINER_DATA_METHOD
        : begin_( cont.data() )
        , end_  ( cont.data() + cont.size() )
#else
        : begin_( &cont[0] )
        , end_  ( &cont[0] + cont.size() )
#endif
    {}

#if gsl_HAVE_IS_DEFAULT_CTOR
    gsl_constexpr14 array_view( array_view const & ) = default;
#else
    gsl_constexpr14 array_view( array_view const & other )
        : begin_( other.begin() )
        , end_  ( other.end() )
    {}
#endif

    template< typename U >
    gsl_constexpr14 array_view( array_view<U> const & other )
        : begin_( other.begin() )
        , end_  ( other.end() )
    {}

    array_view & operator=( array_view other )
    {
        other.swap( *this ); 
        return *this;
    }

#if 0
    // Converting from other array_view ?    
    template< typename U > operator=();
#endif

    gsl_constexpr14 iterator begin() const
    {
        return iterator( begin_ );
    }

    gsl_constexpr14 iterator end() const
    {
        return iterator( end_ );
    }

    gsl_constexpr14 const_iterator cbegin() const
    {
        return const_iterator( begin() );
    }

    gsl_constexpr14 const_iterator cend() const
    {
        return const_iterator( end() );
    }

    gsl_constexpr14 reverse_iterator rbegin() const
    {
        return reverse_iterator( end() );
    }

    gsl_constexpr14 reverse_iterator rend() const
    {
        return reverse_iterator( begin() );
    }

    gsl_constexpr14 const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator( cend() );
    }

    gsl_constexpr14 const_reverse_iterator crend() const
    {
        return const_reverse_iterator( cbegin() );
    }

    gsl_constexpr14 operator bool () const gsl_noexcept
    {
        return begin_ != NULL;
    }

    gsl_constexpr14 reference operator[]( size_type index )
    {
        return at( index );
    }

    gsl_constexpr14 bool operator==( array_view const & other ) const gsl_noexcept
    {
        return  size() == other.size() 
            && (begin_ == other.begin_ || std::equal( this->begin(), this->end(), other.begin() ) );	    
    }

    gsl_constexpr14 bool operator!=( array_view const & other ) const gsl_noexcept 
    { 
        return !( *this == other ); 
    }

    gsl_constexpr14 bool operator< ( array_view const & other ) const gsl_noexcept
    { 
        return std::lexicographical_compare( this->begin(), this->end(), other.begin(), other.end() ); 
    }

    gsl_constexpr14 bool operator<=( array_view const & other ) const gsl_noexcept
    { 
        return !( other < *this ); 
    }

    gsl_constexpr14 bool operator> ( array_view const & other ) const gsl_noexcept
    { 
        return ( other < *this ); 
    }

    gsl_constexpr14 bool operator>=( array_view const & other ) const gsl_noexcept
    {
        return !( *this < other );
    }

    gsl_constexpr14 reference at( size_type index )
    {
        Expects( index >= 0 && index < size());
        return begin_[ index ];
    }

    gsl_constexpr14 pointer data() const gsl_noexcept
    {
        return begin_;
    }

    gsl_constexpr14 bool empty() const gsl_noexcept
    {
        return size() == 0;
    }

    gsl_constexpr14 size_type size() const gsl_noexcept
    {
        return std::distance( begin_, end_ );
    }

    gsl_constexpr14 size_type length() const gsl_noexcept
    {
        return size();
    }

    gsl_constexpr14 size_type used_length() const gsl_noexcept
    {
        return length();
    }

    gsl_constexpr14 size_type bytes() const gsl_noexcept
    {
        return sizeof( value_type ) * size();
    }

    gsl_constexpr14 size_type used_bytes() const gsl_noexcept
    {
        return bytes();
    }

    void swap( array_view & other ) gsl_noexcept
    {
        using std::swap;
        swap( begin_, other.begin_ );
        swap( end_  , other.end_   );
    }

    array_view< const byte > as_bytes() const gsl_noexcept
    {
        return array_view< const byte >( reinterpret_cast<const byte *>( data() ), bytes() );
    }

    array_view< byte > as_writeable_bytes() const gsl_noexcept
    {
        return array_view< byte >( reinterpret_cast<byte *>( data() ), bytes() );
    }

    template< typename U >
    array_view< U > as_array_view() const gsl_noexcept
    {
        Expects( ( this->bytes() % sizeof(U) ) == 0 );
        return array_view< U >( reinterpret_cast<U *>( this->data() ), this->bytes() / sizeof( U ) );
    }

private:
    // helpers for member as_array_view()
    template< typename U >
    gsl_constexpr14 array_view( U * & data, size_type size )
        : begin_( data )
        , end_  ( data + size )
    {
        Expects( size == 0 || ( size > 0 && data != NULL ) );
    }

    template< typename U >
    gsl_constexpr14 array_view( U * const & data, size_type size )
        : begin_( data )
        , end_  ( data + size )
    {
        Expects( size == 0 || ( size > 0 && data != NULL ) );
    }

private:
    pointer begin_;
    pointer end_;
};

// array_view creator functions (see ctors)

template< typename T >
gsl_constexpr14 array_view<T> as_array_view( T * begin, T * end )
{
    return array_view<T>( begin, end );
}

template< typename T >
gsl_constexpr14 array_view<T> as_array_view( T * begin, size_t size )
{
    return array_view<T>( begin, size );
}

template< typename T, size_t N >
gsl_constexpr14 array_view<T> as_array_view( T (&arr)[N] )
{ 
    return array_view<T>( arr, N );
}

#if gsl_HAVE_ARRAY
template< typename T, size_t N >
gsl_constexpr14 array_view<T> as_array_view( std::array<T,N> & arr )
{
    return array_view<T>( arr );
}
#endif

#if gsl_HAVE_AUTO
template< class Cont >
gsl_constexpr14 auto as_array_view( Cont & cont ) ->  array_view< typename Cont::value_type > 
{ 
    return array_view< typename Cont::value_type >( cont );
}
#else
template< class T >
array_view<T> as_array_view( std::vector<T> & cont )
{ 
    return array_view<T>( cont );
}
#endif

//
// String types:
//

typedef char * zstring;
typedef wchar_t * zwstring;
typedef const char * czstring;
typedef const wchar_t * cwzstring;

typedef array_view< char > string_view;
typedef array_view< wchar_t > wstring_view;
typedef array_view< const char > cstring_view;
typedef array_view< const wchar_t > cwstring_view;

// to_string() allow (explicit) conversions from string_view to string

inline std::string to_string( string_view const & view )
{
    return std::string( view.data(), view.length() );
}

inline std::string to_string( cstring_view const & view )
{
    return std::string( view.data(), view.length() );
}

inline std::wstring to_string( wstring_view const & view )
{
    return std::wstring( view.data(), view.length() );
}

inline std::wstring to_string( cwstring_view const & view )
{
    return std::wstring( view.data(), view.length() );
}

//
// ensure_sentinel() 
//
// Provides a way to obtain an array_view from a contiguous sequence
// that ends with a (non-inclusive) sentinel value.
//
// Will fail-fast if sentinel cannot be found before max elements are examined.
//
namespace detail {

template<class T, class SizeType, const T Sentinel>
static array_view<T> ensure_sentinel( T * seq, SizeType max = std::numeric_limits<SizeType>::max() )
{
    typedef T * pointer;
    typedef typename std::iterator_traits<pointer>::difference_type difference_type;
    
    pointer cur = seq;

    while ( std::distance( seq, cur ) < static_cast<difference_type>( max ) && *cur != Sentinel ) 
        ++cur;
    
    Expects( *cur == Sentinel );
    
    return array_view<T>( seq, cur - seq );
}
} // namespace detail

//
// ensure_z - creates a string_view for a czstring or cwzstring.
// Will fail fast if a null-terminator cannot be found before
// the limit of size_type.
//

template< class T >
inline array_view<T> ensure_z( T * const & sz, size_t max = std::numeric_limits<size_t>::max() )
{
    return detail::ensure_sentinel<T, size_t, 0>( sz, max );
//    return detail::ensure<T, size_t, 0>::sentinel( sz, max );
}

template< class T, size_t N >
array_view<T> ensure_z( T (&sz)[N] ) 
{ 
    return ensure_z( &sz[0], N ); 
}

# if gsl_HAVE_TYPE_TRAITS

template< class Cont >
array_view< typename std::remove_pointer<typename Cont::pointer>::type > 
ensure_z( Cont& cont )
{
    return ensure_z( cont.data(), cont.length() );
}
# endif

} // namespace gsl

#endif // GSL_GSL_LITE_H_INCLUDED

// end of file
