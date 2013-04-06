#if !defined(MATRIX4_INC)
#define MATRIX4_INC

#include "vect3.h"
#include <math.h>
#include <memory.h>

#pragma warning( disable : 4244) 
extern const float Pi;
extern const float Half_Pi;


#define MATFLAG_IDENTITY (1)
class Matrix4
{
	float m[4][4];
	int flags;
public:
	Matrix4() {flags=0;}
	Matrix4(const Matrix4 &o);
	float* operator[](int i) {return m[i];flags=0;}
	const float* operator[](int i) const {return m[i];}
	void SetElem(int r,int c,float v) {m[r][c]=v;flags=0;}
	const float &Elem(int r,int c) const {return m[r][c];}
	void SetFromMem(const float *mem) {memcpy(m,mem,sizeof(float)*16);CalcFlags();}
	void GetFromMem(float *mem) const {memcpy(mem,m,sizeof(float)*16);}
	void Identity();
	void Zero();
	int CalcFlags();
	int GetFlags() const {return flags;}
	bool IntegrityCheck() const;
	void Translate(const float tx,const float ty,const float tz);
	void Translate(const Vect3 &t);
	void Rotate(int axis,const float theta);
	void Rotate(const float rx,const float ry,const float rz);
	void Rotate(const Vect3 v);
	void Scale(const float sx,const float sy,const float sz);
	void Scale(const float sx);
	void Concat(const Matrix4 &m1,const Matrix4 &m2);
	void Interp(const Matrix4 &m1,float s1,const Matrix4 &m2,float s2);
	void Interp(const Matrix4 &m1,const Matrix4 &m2,float s1);
	void XFormPoint(Vect3 &dest,const Vect3 &src) const;
	float HXFormPoint(Vect3 &dest,const Vect3 &src) const;
	void HXFormPointND(float *dest,const float *src) const;
	void XFormVect(Vect3 &dest,const Vect3 &src) const;
	void XFormVectTranspose(Vect3 &dest,const Vect3 &src) const;
	void SetRow(int i,const Vect3 &t);
	void SetColumn(int i,const Vect3 &t);
	void SetRow(int i);
	void SetColumn(int i);
	void SetFromDouble(double *d);
	void MultiplyColumn(int i,float f);
	float GetColumnLen(int i);
	void Inverse(const Matrix4 &old);
	void OrthoNormalInverse(const Matrix4 &old);
	void FindFromPoints(const Vect3 base1[4],const Vect3 base2[4]);
	void MakeEquiscalar();
	float Det();
	float MaxAbsElement();
	void GetRow(int r,Vect3 &v) const {v.x()=m[r][0];v.y()=m[r][1];v.z()=m[r][2];}
	void GetColumn(int r,Vect3 &v) const {v.x()=m[0][r];v.y()=m[1][r];v.z()=m[2][r];}
	void Transpose();
	bool operator== (const Matrix4& t) const;
	void From3x4(const float mat[3][4]);
	void To3x4(float mat[3][4]) const;
};


int GetTextureSystem(
  const Vect3 &p0,const Vect3 &uv0, //vertex 0
  const Vect3 &p1,const Vect3 &uv1, //vertex 1
  const Vect3 &p2,const Vect3 &uv2, //vertex 2
  const Vect3 &n, // normal vector for triangle
  Vect3 &M, // returned gradient wrt u
  Vect3 &N, // returned gradient wrt v
  Vect3 &P); // returned location in world space where u=v=0


#endif