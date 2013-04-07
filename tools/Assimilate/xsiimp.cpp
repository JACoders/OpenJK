/**********************************************************************
 *<
	FILE: aiimp.cpp

	DESCRIPTION:  .AI file import module

	CREATED BY: Tom Hudson

	HISTORY: created 28 June 1996

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#include "Stdafx.h"
#include "Includes.h"


/*
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <direct.h>
*/
#include "matrix4.h"
#include "xsiimp.h"


#define M_PI ( 3.14159265359f)


void
split_fn(char *path,char *file,char *pf)
	{
	int ix,jx,bs_loc,fn_loc;
	if(strlen(pf)==0) {
		if(path) *path=0;
		if(file) *file=0;
		return;
		}
	bs_loc=strlen(pf);
	for(ix=bs_loc-1; ix>=0; --ix) {
		if(pf[ix]=='\\')  {
			bs_loc=ix;
			fn_loc=ix+1;
			goto do_split;
			}
		if(pf[ix]==':') {
			bs_loc=ix+1;
			fn_loc=ix+1;
			goto do_split;
			}
		}
	bs_loc= -1;
	fn_loc=0;

	do_split:
	if(file)
		strcpy(file,&pf[fn_loc]);
	if(path) {
		if(bs_loc>0)  {
			for(jx=0; jx<bs_loc; ++jx)
				path[jx]=pf[jx];
			path[jx]=0;
			}
		else  path[0]=0;
		}
	}



int mystrncmp(char *nt,char *nont,int len)
{
	int i;
	for (i=0;i<len;i++)
	{
		if (nt[i]!=nont[i])
			return 1;
	}
	if (nt[len]!=0)
		return 1;
	return 0;
}

void mystrncpy(char *nt,char *nont,int len)
{
	memcpy(nt,nont,len);
	nt[len]=0;
}

float myatof(char *nont,int len)
{
	static char tmp[100];
	memcpy(tmp,nont,len);
	tmp[len]=0;
	return (float)atof(tmp);
}

int myatoi(char *nont,int len)
{
	static char tmp[100];
	memcpy(tmp,nont,len);
	tmp[len]=0;
	return atoi(tmp);
}


void PrintIdent(int ident)
{
	static char mess[10000];
	int i;
	for (i=0;i<ident;i++)
		mess[i]=' ';
	mess[i]=0;
	OutputDebugString(mess);
}

struct TxtNode
{
	char *text;
	int length;
	TxtNode *child;
	TxtNode *sibling;
	TxtNode()
	{
		text=0;
		length=0;
		child=0;
		sibling=0;
	}
	~TxtNode()
	{
		if (child)
			delete child;
		child=0;
		if (sibling)
			delete sibling;
		sibling=0;
	}
	void Print(int ident)
	{
		if (text&&length)
		{
			PrintIdent(ident);
			static char mess[10000];
			static char str[10000];
			mystrncpy(str,text,length);
			sprintf(mess,"|%s|\n",str);
			OutputDebugString(mess);
		}
		if (child)
		{
			PrintIdent(ident);
			OutputDebugString("{\n");
			TxtNode *sib=child;
			while (sib)
			{
				sib->Print(ident+1);
				sib=sib->sibling;
			}

			PrintIdent(ident);
			OutputDebugString("}\n");
		}
	}
};		   

#define PDEB (0)

char *ReadToken(char *at,char *end,int &len,char **out)
{
	char *ret=0;
	len=0;
	while (at<end&&(isspace(*at)||*at==';'||*at==','))
		at++;
	if (at==end)
	{
		*out=end;
		return 0;
	}
	if ((*at=='\"'||*at=='<')&&at+1<end)
	{
		char endchar='\"';
		if (*at=='<')
			endchar='>';
		at++;
		ret=at;
		while (at<end&&*at!=endchar)
		{
			at++;
			len++;
		}
		if (at<end)
			at++;
	}
	else if ((*at=='{'||*at=='}')&&at+1<end)
	{
		ret=at;
		len=1;
		at++;
	}
	else
	{
		ret=at;
		while (at<end&&!(isspace(*at)||*at==';'||*at==','||*at=='}'||*at=='>'||*at=='{'||*at=='<'||*at=='\"'))
		{
			len++;
			at++;
		}
	}
	*out=at;
#if 0
	static char mess[10000];
	static char str[10000];
	mystrncpy(str,ret,len);
	sprintf(mess,"Token |%s|\n",str);
	OutputDebugString(mess);
#endif
	return ret;
}


TxtNode *ReadANode(char *at,char *end,char **atout)
{
	TxtNode *ret=0;
	int len;
	char *retat;
	char *start;
//OutputDebugString("***");
	start=ReadToken(at,end,len,&retat);
	at=retat;
	*atout=at;
	if (!start)
	{
//OutputDebugString("Eof\n");
		return 0;
	}
	if (*start=='}')
	{
//OutputDebugString("End}\n");
		return 0;
	}
	if (*start=='{')
	{
//OutputDebugString("Start{\n");
		ret=new TxtNode;
		TxtNode **last=&ret->child;
		while (1)
		{
			TxtNode *t=ReadANode(at,end,&retat);
			at=retat;
			if (!t)
				break;
			*last=t;
			last=&t->sibling;
		}
	}
	else
	{
		ret=new TxtNode;
		ret->text=start;
		ret->length=len;	 
	}
	*atout=at;
	return ret;
}

struct SceneNodes
{
	char name[100];
	SceneNodes *parent;
	Matrix4 mat;
	Matrix4 BaseMat;
	Matrix4 *animat;
	int nanimat;
};

static SceneNodes sNodes[5000];
static nsNodes=0;


int FindNodes(SceneNodes *parent,TxtNode *t)
{
	static char tmp[1000];
	if (t->child)
	{
		TxtNode *sib=t->child;
		while (sib)
		{
			if (sib->text&&!mystrncmp("Frame",sib->text,sib->length))
			{
				SceneNodes tmpNode;
				Matrix4 tm,tmBase;
				tm.Identity();
				sib=sib->sibling;
				if (!sib)
					return 1;
				assert(sib->length<100);
				if (!strncmp("frm-",sib->text,4))
					mystrncpy(tmpNode.name,sib->text+4,sib->length-4);
				else
					mystrncpy(tmpNode.name,sib->text,sib->length);
//OutputDebugString(tmpNode.name);
//OutputDebugString("\n");

				sib=sib->sibling;
				if (!sib)
					return 2;
				{
					TxtNode *FrameLook=sib;
					TxtNode *sub=sib->child;
					if (!sub)
						return 3;
					if (!sub->text)
						return 4;
					if (mystrncmp("FrameTransformMatrix",sub->text,sub->length))
						return 5;
					sub=sub->sibling;
					{
						TxtNode *xf=sub->child;
						int r,c;
#if 0
	OutputDebugString("matrix\n");
#endif
						for (r=0;r<4;r++)
						{
							int rr=r;
							float sign=1.0f;
							if (r==1)
								rr=2;
							if (r==2)
							{
								sign=-1.0f;
								rr=1;
							}
							for (c=0;c<4;c++)
							{
								if (!xf||!xf->text)
									return 6;
								if (c==0)
									tm[rr][0]=sign*(float)myatof(xf->text,xf->length);
								else if (c==1)
									tm[rr][2]=sign*(float)myatof(xf->text,xf->length);
								else if (c==2)
									tm[rr][1]=-sign*(float)myatof(xf->text,xf->length);
								xf=xf->sibling;
							}
						}
#if 0
						for (r=0;r<4;r++)
						{
							sprintf(tmp,"%6f %6f %6f\n",tm[r][0],tm[r][1],tm[r][2]);
							OutputDebugString(tmp);
						}
#endif
						tm.CalcFlags();
					}
					tmBase=tm;
					sub=sub->sibling;
					while (sub)
					{
						if (sub->text&&!mystrncmp("SI_FrameBasePoseMatrix",sub->text,sub->length))
						{
							sub=sub->sibling;
							{
								TxtNode *xf=sub->child;
								int r,c;
#if 0
	OutputDebugString("pose matrix\n");
#endif
								for (r=0;r<4;r++)
								{
									int rr=r;
									float sign=1.0f;
									if (r==1)
										rr=2;
									if (r==2)
									{
										sign=-1.0f;
										rr=1;
									}
									for (c=0;c<4;c++)
									{
										if (!xf||!xf->text)
											return 6;
										if (c==0)
											tmBase[rr][0]=sign*(float)myatof(xf->text,xf->length);
										else if (c==1)
											tmBase[rr][2]=sign*(float)myatof(xf->text,xf->length);
										else if (c==2)
											tmBase[rr][1]=-sign*(float)myatof(xf->text,xf->length);
										xf=xf->sibling;
									}
								}
								tmBase.CalcFlags();
#if 0
						for (r=0;r<4;r++)
						{
							sprintf(tmp,"%6f %6f %6f\n",tmBase[r][0],tmBase[r][1],tmBase[r][2]);
							OutputDebugString(tmp);
						}
#endif
							}
						}
						sub=sub->sibling;
					}
					tmpNode.parent=parent;
					tmpNode.mat=tm;
					Matrix4 par,tmat;
					if (parent)
						tmat.Concat(tmBase,parent->BaseMat);
					else
						tmat=tmBase;
					tmpNode.BaseMat=tmat;
					SceneNodes *newparent=sNodes+nsNodes;
					tmpNode.animat=0;
					tmpNode.nanimat=0;
					sNodes[nsNodes++]=tmpNode;
					int err=FindNodes(newparent,FrameLook);
					if (err)
						return err;
				}
			}
			sib=sib->sibling;
		}
	}
	return 0;
}

static int maxframe=-10000;
static int minframe=10000;
static int startframe=10000;
static int endframe=-10000;
static int CacheNodes[10000];
static int nCacheNodes=0;
static char CacheName[100];

static int maxmatframe;
static Matrix4 mats[1000];
static Vect3 scales[1000];

int ProcNKeys(TxtNode *t,int& foundkeys)
{
	if (t->child)
	{
		TxtNode *sib=t->child;
		if (!sib->text)
			return 207;
		int tp=myatoi(sib->text,sib->length);
		sib=sib->sibling;
		if (!sib||!sib->text)
			return 208;
		int num=myatoi(sib->text,sib->length);
		sib=sib->sibling;
		int i;
		int lastframe=startframe-1;
		TxtNode *s=sib;
		for (i=0;i<num;i++)
		{
			if (!s||!s->text)
				return 110;
			int frame=myatoi(s->text,s->length);
			if (frame>33000)
				frame=frame-65536;
			if (frame<startframe)
			{
				assert(!i);
				lastframe=frame-1;
				startframe=frame;
			}
			if (frame>endframe)
				endframe=frame;
			if (frame-startframe>maxmatframe)
				maxmatframe=frame-startframe;
			s=s->sibling;
			if (!s||!s->text)
				return 212;
			int junk=myatoi(s->text,s->length);
			if (junk!=3)
				return 213;
			s=s->sibling;
			if (!s||!s->text)
				return 216;
			float x=(float)myatof(s->text,s->length);
			s=s->sibling;
			if (!s||!s->text)
				return 216;
			float y=(float)myatof(s->text,s->length);
			s=s->sibling;
			if (!s||!s->text)
				return 216;
			float z=(float)myatof(s->text,s->length);
			s=s->sibling;
			int k;
			if (tp==1) //scale
			{
				Vect3 v=Vect3(x,z,y);
				for (k=lastframe+1;k<=frame;k++)
					scales[k-startframe]=v;
				foundkeys|=4;
			}
			else if (tp==2) //translation
			{
				Vect3 v(x,-z,y);
				for (k=lastframe+1;k<=frame;k++)
				{
					mats[k-startframe].SetRow(3,v);
					mats[k-startframe].CalcFlags();
				}
				foundkeys|=1;
			}
			else if (tp==3) //rotation
			{
				Matrix4 tmp;
				tmp.Rotate(-x*M_PI/180.0f,-y*M_PI/180.0f,-z*M_PI/180.0f);
				Vect3 t,tt;
				tmp.GetRow(2,t);
				t*=-1.0f;
				tmp.GetRow(1,tt);
				tmp.SetRow(2,tt);
				tmp.SetRow(1,t);
				for (k=0;k<3;k++)
				{
					float tt=-tmp[k][2];
					tmp[k][2]=tmp[k][1];
					tmp[k][1]=tt;
				}
				for (k=lastframe+1;k<=frame;k++)
				{
					int l;
					for (l=0;l<3;l++)
					{
						tmp.GetRow(l,t);
						mats[k-startframe].SetRow(l,t);
						mats[k-startframe].CalcFlags();
					}
				}
				foundkeys|=2;
			}
			lastframe=frame;
		}
	}
	return 0;
}

int FindPKeys(TxtNode *t)
{
	if (t->child)
	{
		TxtNode *sib=t->child;
		if (!sib)
			return 200;
		char framename[100];
		{
			if (!sib->child)
				return 201;
			if (!sib->child->text)
				return 202;
			if (!strncmp("frm-",sib->child->text,4))
				mystrncpy(framename,sib->child->text+4,sib->child->length-4);
			else
			{
				if (!strncmp("SCENE",sib->child->text,4))
					return 0;
				mystrncpy(framename,sib->child->text,sib->child->length);
			}
			sib=sib->sibling;
		}
		int foundkeys=0;
		maxmatframe=0;
		while (sib)
		{
			if (sib->text&&!mystrncmp("SI_AnimationKey",sib->text,sib->length))
			{
				if (!sib->sibling)
					return 103;
				sib=sib->sibling;
				int err=ProcNKeys(sib,foundkeys);
				if (err)
					return err;
			}
			sib=sib->sibling;
		}
//		if (foundkeys)
		{
			if (maxmatframe>maxframe)
				maxframe=maxmatframe;
			int i;
			for (i=0;i<nsNodes;i++)
			{
				if (!strcmp(sNodes[i].name,framename))
				{
					int j;
					if (sNodes[i].nanimat<maxmatframe+1)
					{
						delete[] sNodes[i].animat;
						sNodes[i].nanimat=maxmatframe+1;
						sNodes[i].animat=new Matrix4[maxmatframe+1];
						for (j=0;j<=maxmatframe;j++)
							sNodes[i].animat[j].Identity();
					}
					for (j=0;j<=maxmatframe;j++)
					{
						Matrix4 xf;
						xf=sNodes[i].mat;
						Vect3 tt,t2;
						if (foundkeys&1)
						{
							mats[j].GetRow(3,tt);
							xf.SetRow(3,tt);
						}
						if (foundkeys&2)
						{
							float l;
							xf.GetRow(0,t2);
							l=t2.Len();
							mats[j].GetRow(0,tt);
							tt*=l;
							xf.SetRow(0,tt);

							xf.GetRow(1,t2);
							l=t2.Len();
							mats[j].GetRow(1,tt);
							tt*=l;
							xf.SetRow(1,tt);

							xf.GetRow(2,t2);
							l=t2.Len();
							mats[j].GetRow(2,tt);
							tt*=l;
							xf.SetRow(2,tt);
						}
						if (foundkeys&4)
						{
							xf.GetRow(0,tt);
							tt.Norm();
							tt*=scales[j].x();
							xf.SetRow(0,tt);

							xf.GetRow(1,tt);
							tt.Norm();
							tt*=scales[j].x();
							xf.SetRow(1,tt);

							xf.GetRow(2,tt);
							tt.Norm();
							tt*=scales[j].x();
							xf.SetRow(2,tt);

						}
						xf.CalcFlags();
						Matrix4 par,tmat;
						if (sNodes[i].parent)
						{
							if (j<sNodes[i].parent->nanimat)
								par=sNodes[i].parent->animat[j];
							else
								par=sNodes[i].parent->BaseMat;
							tmat.Concat(xf,par);
						}
						else
							tmat=xf;
						sNodes[i].animat[j]=tmat;

#if 0
						par.Inverse(sNodes[i].BaseMat);
						tmat.Concat(par,sNodes[i].animat[j]);
	char tmp[1000];
	int r;
	sprintf(tmp,"final frame %d %s\n",j,sNodes[i].name);
	OutputDebugString(tmp);
	for (r=0;r<4;r++)
	{
		sprintf(tmp,"%6f %6f %6f\n",tmat[r][0],tmat[r][1],tmat[r][2]);
		OutputDebugString(tmp);
	}
	sprintf(tmp,"baseinv frame %d %s\n",j,sNodes[i].name);
	OutputDebugString(tmp);
	for (r=0;r<4;r++)
	{
		sprintf(tmp,"%6f %6f %6f\n",par[r][0],par[r][1],par[r][2]);
		OutputDebugString(tmp);
	}
	sprintf(tmp,"bone frame %d %s\n",j,sNodes[i].name);
	OutputDebugString(tmp);
	for (r=0;r<4;r++)
	{
		sprintf(tmp,"%6f %6f %6f\n",sNodes[i].animat[j][r][0],sNodes[i].animat[j][r][1],sNodes[i].animat[j][r][2]);
		OutputDebugString(tmp);
	}
#endif
					}
				}
			}
		}
	}
	return 0;
}

int FindSets(TxtNode *t)
{
	if (t->child)
	{
		TxtNode *sib=t->child;
		while (sib)
		{
			if (sib->text&&!mystrncmp("Animation",sib->text,sib->length))
			{
				if (!sib->sibling)
					return 101;
				sib=sib->sibling;
				if (!sib->sibling)
					return 102;
				sib=sib->sibling;
				int err=FindPKeys(sib);
				if (err)
					return err;
			}
			sib=sib->sibling;
		}
	}
	return 0;
}

int FindAnims(TxtNode *t)
{
	int i;
	for (i=0;i<nsNodes;i++)
	{
		sNodes[i].animat=0;
		sNodes[i].nanimat=0;
	}
	if (t->child)
	{
		TxtNode *sib=t->child;
		while (sib)
		{
			if (sib->text&&!mystrncmp("AnimationSet",sib->text,sib->length))
			{
				if (!sib->sibling)
					return 100;
				sib=sib->sibling;
				int err=FindSets(sib);
				if (err)
					return err;
			}
			sib=sib->sibling;
		}
	}
	return 0;
}

/*
** returns the number of frames loaded
*/
int XSI_LoadFile(const char *filename)
{
	char *filebin=0;
	TxtNode *root;

	FILE *fp=fopen(filename,"ra");
	if (!fp){
		InfoBox(va("File not found: \"%s\"",filename));
		return 0;
	}
	fseek(fp,0,SEEK_END);
	int len=ftell(fp);
	filebin=new char[len];
	fseek(fp,0,SEEK_SET);
	if (fread(filebin,1,len,fp)!=(size_t)len)
	{	
		fclose(fp);
		InfoBox(va("Bad XSI file\n",filename));
		delete(filebin);
		return 0;
	}
	fclose(fp);
	
	char *at=filebin;
	char *end=filebin+len;
	root=new TxtNode;
	TxtNode **last=&root->child;
	char *retat;
	while (1)
	{
		TxtNode *t=ReadANode(at,end,&retat);
		if (!t)
		{
			at=end;
			break;
		}
		at=retat;
		*last=t;
		last=&t->sibling;
	}
	if (!root->child)
	{
		printf("Bad XSI file\n");
		delete(filebin);
		delete root;
		return 0;
	}
#if PDEB
	root->Print(0);
#endif

	nsNodes=0;
	maxframe=-10000;
	minframe=10000;
	startframe=10000;
	endframe=-10000;
	CacheName[0]=0;
	int fn=FindNodes(0,root);
	if (!fn)
	{
		fn=FindAnims(root);
	}

	delete(filebin);
	delete root;

	if (fn==2000)
	{
		printf("XSI import failed, Missing Keys\n",fn);
		return 0;
	}
	if (fn)
	{
		printf("XSI import failed, code = %d",fn);
		return 0;
	}
	return maxframe+1;
}

int XSI_GetNumFrames()
{
	if (maxframe+1>0)
		return maxframe+1;
	return 0;
}

int XSI_FindBone(char *bonename)
{
	int j;
	for (j=0;j<nsNodes;j++)
	{
		if (!strcmpi(bonename,sNodes[j].name))
			return j;
	}
	return -1;
}

void XSI_GetBoneMatrix(int bone,int frame,float mat[3][4])
{
	assert(bone>=0&&bone<nsNodes);
	assert(frame>=0&&frame<maxframe+1);
	int i,j;
	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++)
		{
			mat[j][i]=sNodes[bone].animat[frame][i][j];
		}
	}
}

void XSI_GetBoneMatrix4(int bone,int frame,Matrix4 &m)
{
	assert(bone>=0&&bone<nsNodes);
	assert(frame>=0&&frame<maxframe+1);
	m=sNodes[bone].animat[frame];
}

int XSI_GetNumBones()
{
	return nsNodes;
}

void XSI_GetBoneName(int bone,char *bonename)
{
	assert(bone>=0&&bone<nsNodes);
	strcpy(bonename,sNodes[bone].name);
}

void XSI_Cleanup()
{
	int j;
	for (j=0;j<nsNodes;j++)
	{
		delete[] sNodes[j].animat;
	}

	nsNodes=0;
	maxframe=-10000;
	minframe=10000;
	startframe=10000;
	endframe=-10000;
	CacheName[0]=0;
}

