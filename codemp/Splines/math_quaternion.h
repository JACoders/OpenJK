#ifndef __MATH_QUATERNION_H__
#define __MATH_QUATERNION_H__

#include <assert.h>
#include <math.h>

class idVec3_t;
class angles_t;
class mat3_t;

class quat_t {
public:
	float			x;
	float			y;
	float			z;
	float			w;

					quat_t();
					quat_t( float x, float y, float z, float w );

	friend void		toQuat( idVec3_t &src, quat_t &dst );
	friend void		toQuat( angles_t &src, quat_t &dst );
	friend void		toQuat( mat3_t &src, quat_t &dst );

	float			*vec4( void );
			
	float			operator[]( int index ) const;
	float			&operator[]( int index );

	void 			set( float x, float y, float z, float w );

	void			operator=( quat_t a );

	friend quat_t	operator+( quat_t a, quat_t b );
	quat_t			&operator+=( quat_t a );

	friend quat_t	operator-( quat_t a, quat_t b );
	quat_t			&operator-=( quat_t a );

	friend quat_t	operator*( quat_t a, float b );
	friend quat_t	operator*( float a, quat_t b );
	quat_t			&operator*=( float a );

	friend int		operator==(	quat_t a, quat_t b );
	friend int		operator!=(	quat_t a, quat_t b );

	float			Length( void );
	quat_t			&Normalize( void );

	quat_t			operator-();
};

inline quat_t::quat_t() {
}

inline quat_t::quat_t( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

inline float *quat_t::vec4( void ) {
	return &x;
}

inline float quat_t::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

inline float& quat_t::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index < 4 ) );
	return ( &x )[ index ];
}

inline void quat_t::set( float x, float y, float z, float w ) {
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}

inline void quat_t::operator=( quat_t a ) {
	x = a.x;
	y = a.y;
	z = a.z;
	w = a.w;
}

inline quat_t operator+( quat_t a, quat_t b ) {
	return quat_t( a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w );
}

inline quat_t& quat_t::operator+=( quat_t a ) {
	x += a.x;
	y += a.y;
	z += a.z;
	w += a.w;

	return *this;
}

inline quat_t operator-( quat_t a, quat_t b ) {
	return quat_t( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w );
}

inline quat_t& quat_t::operator-=( quat_t a ) {
	x -= a.x;
	y -= a.y;
	z -= a.z;
	w -= a.w;

	return *this;
}

inline quat_t operator*( quat_t a, float b ) {
	return quat_t( a.x * b, a.y * b, a.z * b, a.w * b );
}

inline quat_t operator*( float a, quat_t b ) {
	return b * a;
}

inline quat_t& quat_t::operator*=( float a ) {
	x *= a;
	y *= a;
	z *= a;
	w *= a;

	return *this;
}

inline int operator==( quat_t a, quat_t b ) {
	return ( ( a.x == b.x ) && ( a.y == b.y ) && ( a.z == b.z ) && ( a.w == b.w ) );
}

inline int operator!=( quat_t a, quat_t b ) {
	return ( ( a.x != b.x ) || ( a.y != b.y ) || ( a.z != b.z ) && ( a.w != b.w ) );
}

inline float quat_t::Length( void ) {
	float length;
	
	length = x * x + y * y + z * z + w * w;
	return ( float )sqrt( length );
}

inline quat_t& quat_t::Normalize( void ) {
	float length;
	float ilength;

	length = this->Length();
	if ( length ) {
		ilength = 1 / length;
		x *= ilength;
		y *= ilength;
		z *= ilength;
		w *= ilength;
	}
		
	return *this;
}

inline quat_t quat_t::operator-() {
	return quat_t( -x, -y, -z, -w );
}

#endif /* !__MATH_QUATERNION_H__ */
