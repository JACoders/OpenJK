#if !defined(VECT3_INC)
#define VECT3_INC

#include <math.h>
#include <assert.h>

class Vect3
{
	float v[3];
public:
	Vect3(const float val) {v[0]=val;v[1]=val;v[2]=val;}
	Vect3() {}//never put anything in here! too slow}
	Vect3(const float x,const float y,const float z) {v[0]=x;v[1]=y;v[2]=z;}
	Vect3(const Vect3& t) {v[0]=t.v[0];v[1]=t.v[1];v[2]=t.v[2];}
	Vect3(const float *t) {v[0]=t[0];v[1]=t[1];v[2]=t[2];}
	float& operator[](int i) {return v[i];}
	float& x() {return v[0];}
	float& y() {return v[1];}
	float& z() {return v[2];}
	const float& operator[](int i) const {return v[i];}
	const float& x() const {return v[0];}
	const float& y() const {return v[1];}
	const float& z() const {return v[2];}
	void Set(const float x,const float y,const float z) {v[0]=x;v[1]=y;v[2]=z;}

	float Len() const {return (float)sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);}
	float Len2() const {return v[0]*v[0]+v[1]*v[1]+v[2]*v[2];}
	void Norm() {(*this)/=this->Len();} 
	bool ZeroNorm() {float d=this->Len();if (d>1E-10) {(*this)/=d;return true;} (*this)=0.0f; return false;} 
	void SafeNorm() {assert(this->Len()>1E-10);(*this)/=this->Len();} 

	const Vect3 &operator= (const float d) {v[0]=d;v[1]=d;v[2]=d; return *this; }
	const Vect3 &operator= (const Vect3& t) {v[0]=t.v[0];v[1]=t.v[1];v[2]=t.v[2]; return *this; }

	const Vect3 &operator+= (const float d) {v[0]+=d;v[1]+=d;v[2]+=d; return *this; }
	const Vect3 &operator+= (const Vect3& t) {v[0]+=t.v[0];v[1]+=t.v[1];v[2]+=t.v[2]; return *this; }

	const Vect3 &operator-= (const float d) {v[0]-=d;v[1]-=d;v[2]-=d; return *this; }
	const Vect3 &operator-= (const Vect3& t) {v[0]-=t.v[0];v[1]-=t.v[1];v[2]-=t.v[2]; return *this; }

	const Vect3 &operator*= (const float d) {v[0]*=d;v[1]*=d;v[2]*=d; return *this; }
	const Vect3 &operator*= (const Vect3& t) {v[0]*=t.v[0];v[1]*=t.v[1];v[2]*=t.v[2]; return *this; }

	const Vect3 &operator/= (const float d);
	const Vect3 &operator/= (const Vect3& t) {v[0]/=t.v[0];v[1]/=t.v[1];v[2]/=t.v[2]; return *this; }

	float operator^ (const Vect3& t) const {return v[0]*t.v[0]+v[1]*t.v[1]+v[2]*t.v[2];}

	float Dist(const Vect3&) const;
	float Dist2(const Vect3&) const;
	void Cross(const Vect3&);
	void NegCross(const Vect3&);
	void Perp();
	void Min(const Vect3&);
	void Max(const Vect3&);
	float MaxElement() const;
	int MaxElementIndex() const;

	void Interp(const Vect3 &v1,const Vect3 &v2,float t) {*this=v1;*this-=v2;*this*=t;*this+=v2;}
//	bool operator== (const Vect3& t) const {return v[0]==t.v[0]&&v[1]==t.v[1]&&v[2]==t.v[2];}
	bool operator== (const Vect3& t) const {return fabs(v[0]-t.v[0])<.001f&&fabs(v[1]-t.v[1])<.001f&&fabs(v[2]-t.v[2])<.001f;}
	bool operator< (const Vect3& t) const {assert(0);return false;}
	bool operator!= (const Vect3& t) const {return !(v[0]==t.v[0]&&v[1]==t.v[1]&&v[2]==t.v[2]);}
	bool operator> (const Vect3& t) const {assert(0);return false;}

	inline Vect3 operator +(const Vect3 &rhs) const { return Vect3(v[0]+rhs.v[0], v[1]+rhs.v[1], v[2]+rhs.v[2]); }
	inline Vect3 operator -(const Vect3 &rhs) const { return Vect3(v[0]-rhs.v[0], v[1]-rhs.v[1], v[2]-rhs.v[2]); }
	inline Vect3 operator *(const Vect3 &rhs) const { return Vect3(v[0]*rhs.v[0], v[1]*rhs.v[1], v[2]*rhs.v[2]); }
	inline Vect3 operator *(const float scalar) const { return Vect3(v[0]*scalar, v[1]*scalar, v[2]*scalar); }
	inline friend Vect3 operator *(const float scalar, const Vect3 &rhs);
	inline Vect3 operator /(const Vect3 &rhs) const { return Vect3(v[0]/rhs.v[0], v[1]/rhs.v[1], v[2]/rhs.v[2]); }
	inline Vect3 operator /(const float scalar) const { return Vect3(v[0]/scalar, v[1]/scalar, v[2]/scalar); }
};

inline Vect3 operator *(const float scalar, const Vect3 &rhs)
{
	return Vect3(scalar*rhs.v[0], scalar*rhs.v[1], scalar*rhs.v[2]);
}


extern const Vect3 Vect3X;
extern const Vect3 Vect3Y;
extern const Vect3 Vect3Z;
extern const Vect3 Vect3negX;
extern const Vect3 Vect3negY;
extern const Vect3 Vect3negZ;
extern const Vect3 Vect3Zero;




#endif
