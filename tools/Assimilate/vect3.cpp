#include "vect3.h"


const Vect3 Vect3X(1.f,0.f,0.f);
const Vect3 Vect3Y(0.f,1.f,0.f);
const Vect3 Vect3Z(0.f,0.f,1.f);
const Vect3 Vect3negX(-1.f,0.f,0.f);
const Vect3 Vect3negY(0.f,-1.f,0.f);
const Vect3 Vect3negZ(0.f,0.f,-1.f);
const Vect3 Vect3Zero(0.f,0.f,0.f);

const Vect3 &Vect3::operator/= (const float d)
{
	float inv=1.f/d;
	return (*this)*=inv;
}
	 
void Vect3::Cross(const Vect3& p)
{
	Vect3 t=*this;
	v[0]=t.v[1]*p.v[2]-t.v[2]*p.v[1];
	v[1]=t.v[2]*p.v[0]-t.v[0]*p.v[2];
	v[2]=t.v[0]*p.v[1]-t.v[1]*p.v[0];
}

void Vect3::NegCross(const Vect3& p)
{
	Vect3 t=*this;
	v[0]=p.v[1]*t.v[2]-p.v[2]*t.v[1];
	v[1]=p.v[2]*t.v[0]-p.v[0]*t.v[2];
	v[2]=p.v[0]*t.v[1]-p.v[1]*t.v[0];
}

float Vect3::Dist(const Vect3& p) const
{
	Vect3 t=*this;
	t-=p;
	return t.Len();
}

float Vect3::Dist2(const Vect3& p) const
{
	Vect3 t=*this;
	t-=p;
	return t^t;
}

void Vect3::Perp()
{
	float rlen,tlen;
	Vect3 r,t;
	r=*this;
	r.Cross(Vect3X);
	rlen=r.Len();
	t=*this;
	t.Cross(Vect3Y);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	t=*this;
	t.Cross(Vect3Z);
	tlen=t.Len();
	if (tlen>rlen)
	{
		r=t;
		rlen=tlen;
	}
	*this=r;
}

void Vect3::Min(const Vect3& p)
{
	if (p.v[0]<v[0])
		v[0]=p.v[0];	
	if (p.v[1]<v[1])
		v[1]=p.v[1];	
	if (p.v[2]<v[2])
		v[2]=p.v[2];	
}

void Vect3::Max(const Vect3& p)
{
	if (p.v[0]>v[0])
		v[0]=p.v[0];	
	if (p.v[1]>v[1])
		v[1]=p.v[1];	
	if (p.v[2]>v[2])
		v[2]=p.v[2];	
}

float Vect3::MaxElement() const
{
	return v[MaxElementIndex()];	
}

int Vect3::MaxElementIndex() const
{
	if (fabs(v[0])>fabs(v[1])&&fabs(v[0])>fabs(v[2]))
		return 0;
	if (fabs(v[1])>fabs(v[2]))
		return 1;
	return 2;	
}


