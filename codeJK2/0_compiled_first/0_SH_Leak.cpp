// leave this as first line for PCH reasons...
//
#pragma warning( disable : 4786) 
#pragma warning( disable : 4100) 
#pragma warning( disable : 4663) 
#include <windows.h>

#include "..\smartheap\smrtheap.h"
#include "../game/q_shared.h"
#include "..\qcommon\qcommon.h"

#include <stdio.h>
#include <map>

using namespace std;

#if MEM_DEBUG
#include "..\smartheap\heapagnt.h"

static const int maxStack=2048;
static int TotalMem;
static int TotalBlocks;
static int nStack;
static char StackNames[maxStack][256];
static int StackSize[maxStack];
static int StackCount[maxStack];
static int StackCache[48];
static int StackCacheAt=0;
static int CheckpointSize[3000];
static int CheckpointCount[3000];

//#define _FASTRPT_


#ifdef _FASTRPT_
class CMyStrComparator
{
public:
	bool operator()(const char *s1, const char *s2) const { return(strcmp(s1, s2) < 0); } 
};

hmap<const char *,int,CMyStrComparator> Lookup;
#endif

cvar_t	*mem_leakfile;
cvar_t	*mem_leakreport;

MEM_BOOL MEM_CALLBACK MyMemReporter2(MEM_ERROR_INFO *info)
{
	static char buffer[10000];
	if (!info->objectCreationInfo)
		return 1;
	info=info->objectCreationInfo;
	int idx=info->checkpoint;
	if (idx<0||idx>=1000)
	{
		idx=0;
	}
	CheckpointCount[idx]++;
	CheckpointSize[idx]+=info->argSize;
	dbgMemFormatCall(info,buffer,9999);
	if (strstr(buffer,"ntdll"))
		return 1;
	if (strstr(buffer,"CLBCATQ"))
		return 1;
	int i;
	TotalBlocks++;
	if (TotalBlocks%1000==0)
	{
		char mess[1000];
		sprintf(mess,"%d blocks processed\n",TotalBlocks);
		OutputDebugString(mess);
	}
	for (i=strlen(buffer);i>0;i--)
	{
		if (buffer[i]=='\n')
			break;
	}
	if (!i)
		return 1;
	buffer[i]=0;
	char *buf=buffer;
	while (*buf)
	{
		if (*buf=='\n')
		{
			buf++;
			break;
		}
		buf++;
	}
	char *start=0;
	char *altName=0;
	while (*buf)
	{
		while (*buf==' ')
			buf++;
		start=buf;
		while (*buf!=0&&*buf!='\n')
			buf++;
		if (*start)
		{
			if (*buf)
			{
				*buf=0;
				buf++;
			}
			if (strlen(start)>255)
				start[255]=0;
			if (strstr(start,"std::"))
			{
				altName="std::??";
//				start=0;
				continue;
			}
			if (strstr(start,"Malloc"))
			{
				altName="Malloc??";
				start=0;
				continue;
			}
			if (strstr(start,"G_Alloc"))
			{
				altName="G_Alloc";
				start=0;
				continue;
			}
			if (strstr(start,"Hunk_Alloc"))
			{
				altName="Hunk_Alloc";
				start=0;
				continue;
			}
			if (strstr(start,"FS_LoadFile"))
			{
				altName="FS_LoadFile";
				start=0;
				continue;
			}
			if (strstr(start,"CopyString"))
			{
				altName="CopyString";
				start=0;
				continue;
			}
			break;
		}
	}
	if (!start||!*start)
	{
		start=altName;
		if (!start||!*start)
		{
			start="UNKNOWN";
		}
	}
#ifdef _FASTRPT_
	hmap<const char *,int,CMyStrComparator>::iterator f=Lookup.find(start);
	if(f==Lookup.end())
	{
		strcpy(StackNames[nStack++],start);
		Lookup[(const char *)&StackNames[nStack-1]]=nStack-1;
		StackSize[nStack-1]=info->argSize;
		StackCount[nStack-1]=1;
	}
	else
	{
		StackSize[(*f).second]+=info->argSize;
		StackCount[(*f).second]++;
	}
#else
	for (i=0;i<48;i++)
	{
		if (StackCache[i]<0||StackCache[i]>=nStack)
			continue;
		if (!strcmpi(start,StackNames[StackCache[i]]))
			break;
	}
	if (i<48)
	{
		StackSize[StackCache[i]]+=info->argSize;
		StackCount[StackCache[i]]++;
	}
	else
	{
		for (i=0;i<nStack;i++)
		{
			if (!strcmpi(start,StackNames[i]))
				break;
		}
		if (i<nStack)
		{
			StackSize[i]+=info->argSize;
			StackCount[i]++;
			StackCache[StackCacheAt]=i;
			StackCacheAt++;
			if (StackCacheAt>=48)
				StackCacheAt=0;
		}
		else if (i<maxStack)
		{
			strcpy(StackNames[i],start);
			StackSize[i]=info->argSize;
			StackCount[i]=1;
			nStack++;
		}
		else if (nStack<maxStack)
		{
			nStack++;
			strcpy(StackNames[maxStack-1],"*****OTHER*****");
			StackSize[maxStack-1]=info->argSize;
			StackCount[maxStack-1]=1;
		}
		else
		{
			StackSize[maxStack-1]+=info->argSize;
			StackCount[maxStack-1]++;
		}
	}
#endif
	TotalMem+=info->argSize;
	return 1;
}

MEM_BOOL MEM_CALLBACK MyMemReporter3(MEM_ERROR_INFO *info)
{
	static char buffer[10000];
	if (!info->objectCreationInfo)
		return 1;
	info=info->objectCreationInfo;
	int idx=info->checkpoint;
	if (idx<0||idx>=3000)
	{
		idx=0;
	}
	CheckpointCount[idx]++;
	CheckpointSize[idx]+=info->argSize;
	dbgMemFormatCall(info,buffer,9999);
	int i;
	TotalBlocks++;
//	if (TotalBlocks%1000==0)
//	{
//		char mess[1000];
//		sprintf(mess,"%d blocks processed\n",TotalBlocks);
//		OutputDebugString(mess);
//	}
	for (i=strlen(buffer);i>0;i--)
	{
		if (buffer[i]=='\n')
			break;
	}
	if (!i)
		return 1;
	buffer[i]=0;
	char *buf=buffer;
	while (*buf)
	{
		if (*buf=='\n')
		{
			buf++;
			break;
		}
		buf++;
	}
	char *start=0;
	char *altName=0;
	while (*buf)
	{
		while (*buf==' ')
			buf++;
		start=buf;
		while (*buf!=0&&*buf!='\n')
			buf++;
		if (*start)
		{
			if (*buf)
			{
				*buf=0;
				buf++;
			}
			if (strlen(start)>255)
				start[255]=0;
			if (strstr(start,"SV_AreaEntities"))
			{
				altName="SV_AreaEntities??";
				start=0;
				continue;
			}
			if (strstr(start,"SV_Trace"))
			{
				altName="SV_Trace??";
				start=0;
				continue;
			}
			if (strstr(start,"SV_PointContents"))
			{
				altName="SV_PointContents??";
				start=0;
				continue;
			}
			if (strstr(start,"CG_Trace"))
			{
				altName="??";
				start=0;
				continue;
			}
			if (strstr(start,"CG_PointContents"))
			{
				altName="??";
				start=0;
				continue;
			}
/*
			if (strstr(start,""))
			{
				altName="??";
				start=0;
				continue;
			}
			if (strstr(start,""))
			{
				altName="??";
				start=0;
				continue;
			}
*/
			break;
		}
	}
	if (!start||!*start)
	{
		start=altName;
		if (!start||!*start)
		{
			start="UNKNOWN";
		}
	}
#ifdef _FASTRPT_
	hmap<const char *,int,CMyStrComparator>::iterator f=Lookup.find(start);
	if(f==Lookup.end())
	{
		strcpy(StackNames[nStack++],start);
		Lookup[(const char *)&StackNames[nStack-1]]=nStack-1;
		StackSize[nStack-1]=info->argSize;
		StackCount[nStack-1]=1;
	}
	else
	{
		StackSize[(*f).second]+=info->argSize;
		StackCount[(*f).second]++;
	}
#else
	for (i=0;i<48;i++)
	{
		if (StackCache[i]<0||StackCache[i]>=nStack)
			continue;
		if (!strcmpi(start,StackNames[StackCache[i]]))
			break;
	}
	if (i<48)
	{
		StackSize[StackCache[i]]+=info->argSize;
		StackCount[StackCache[i]]++;
	}
	else
	{
		for (i=0;i<nStack;i++)
		{
			if (!strcmpi(start,StackNames[i]))
				break;
		}
		if (i<nStack)
		{
			StackSize[i]+=info->argSize;
			StackCount[i]++;
			StackCache[StackCacheAt]=i;
			StackCacheAt++;
			if (StackCacheAt>=48)
				StackCacheAt=0;
		}
		else if (i<maxStack)
		{
			strcpy(StackNames[i],start);
			StackSize[i]=info->argSize;
			StackCount[i]=1;
			nStack++;
		}
		else if (nStack<maxStack)
		{
			nStack++;
			strcpy(StackNames[maxStack-1],"*****OTHER*****");
			StackSize[maxStack-1]=info->argSize;
			StackCount[maxStack-1]=1;
		}
		else
		{
			StackSize[maxStack-1]+=info->argSize;
			StackCount[maxStack-1]++;
		}
	}
#endif
	TotalMem+=info->argSize;
	return 1;
}

void SH_Checking_f(void);
#endif

class Leakage
{
	MEM_POOL MyPool;

public:
	Leakage()
	{
		MyPool = MemInitDefaultPool();
//		MemPoolSetSmallBlockSize(MyPool, 16); 
		MemPoolSetSmallBlockAllocator(MyPool,MEM_SMALL_BLOCK_SH3);
#if MEM_DEBUG
		dbgMemSetGuardSize(2);
		EnableChecking(100000);
#endif
	}

	void LeakReport(void)
	{
#if MEM_DEBUG

		// This just makes sure we have map nodes available without allocation
		// during the heap walk (which could be bad).
		int i;
#ifdef _FASTRPT_
		hlist<int> makeSureWeHaveNodes;
		for(i=0;i<5000;i++)
		{
			makeSureWeHaveNodes.push_back(0);
		}
		makeSureWeHaveNodes.clear();
		Lookup.clear();
#endif
		char mess[1000];
		int blocks=dbgMemTotalCount();
		int mem=dbgMemTotalSize()/1024;
		sprintf(mess,"Final Memory Summary %d blocks %d K\n",blocks,mem);
		OutputDebugString(mess);

		for (i=0;i<3000;i++)
		{
			CheckpointSize[i]=0;
			CheckpointCount[i]=0;
		}

		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter2);
		dbgMemReportLeakage(NULL,1,1000);
		MemSetErrorHandler(MemDefaultErrorHandler);
		multimap<int,pair<int,char *> > sortit;
		multimap<int,pair<int,char *> >::iterator j;
		if (TotalBlocks)
		{
			// Sort by size.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("**********Memory Leak Report**********\n");
			OutputDebugString("*************** By Size **************\n");
			OutputDebugString("**************************************\n");
			sprintf(mess,"Actual leakage %d blocks %d K\n",TotalBlocks,TotalMem/1024);
			OutputDebugString(mess);
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackSize[i],pair<int,char *>(StackCount[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%5d KB %6d cnt  %s\n",-(*j).first/1024,(*j).second.first,(*j).second.second);
	//			if (!(-(*j).first/1024))
	//				break;
				Sleep(5);
				OutputDebugString(mess);
			}

			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("**********Memory Leak Report**********\n");
			OutputDebugString("************** By Count **************\n");
			OutputDebugString("**************************************\n");
			sprintf(mess,"Actual leakage %d blocks %d K\n",TotalBlocks,TotalMem/1024);
			OutputDebugString(mess);
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%5d KB %6d cnt  %s\n",(*j).second.first/1024,-(*j).first,(*j).second.second);
	//			if (!(-(*j).first/1024))
	//				break;
				Sleep(5);
				OutputDebugString(mess);
			}
		}
		else
		{
			OutputDebugString("No Memory Leaks\n");
		}

		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter3);
		dbgMemReportLeakage(NULL,2001,2001);
		MemSetErrorHandler(MemDefaultErrorHandler);
		if (TotalBlocks)
		{
			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("SV_PointContents     ");
			sprintf(mess,"%d Calls.\n",TotalBlocks);
			OutputDebugString(mess);
			OutputDebugString("**************************************\n");
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%7d cnt  %s\n",-(*j).first,(*j).second.second);
				Sleep(5);
				OutputDebugString(mess);
			}
		}
		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter3);
		dbgMemReportLeakage(NULL,2002,2002);
		MemSetErrorHandler(MemDefaultErrorHandler);
		if (TotalBlocks)
		{
			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("SV_Trace     ");
			sprintf(mess,"%d Calls.\n",TotalBlocks);
			OutputDebugString(mess);
			OutputDebugString("**************************************\n");
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%7d cnt  %s\n",-(*j).first,(*j).second.second);
				Sleep(5);
				OutputDebugString(mess);
			}
		}
		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter3);
		dbgMemReportLeakage(NULL,2003,2003);
		MemSetErrorHandler(MemDefaultErrorHandler);
		if (TotalBlocks)
		{
			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("SV_AreaEntities     ");
			sprintf(mess,"%d Calls.\n",TotalBlocks);
			OutputDebugString(mess);
			OutputDebugString("**************************************\n");
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%7d cnt  %s\n",-(*j).first,(*j).second.second);
				Sleep(5);
				OutputDebugString(mess);
			}
		}
		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter3);
		dbgMemReportLeakage(NULL,2004,2004);
		MemSetErrorHandler(MemDefaultErrorHandler);
		if (TotalBlocks)
		{
			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("CG_Trace     ");
			sprintf(mess,"%d Calls.\n",TotalBlocks);
			OutputDebugString(mess);
			OutputDebugString("**************************************\n");
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%7d cnt  %s\n",-(*j).first,(*j).second.second);
				Sleep(5);
				OutputDebugString(mess);
			}
		}
		TotalMem=0;
		TotalBlocks=0;
		nStack=0;
		MemSetErrorHandler(MyMemReporter3);
		dbgMemReportLeakage(NULL,2005,2005);
		MemSetErrorHandler(MemDefaultErrorHandler);
		if (TotalBlocks)
		{
			// Sort by count.
			Sleep(100);
			OutputDebugString("**************************************\n");
			OutputDebugString("CG_PointContents     ");
			sprintf(mess,"%d Calls.\n",TotalBlocks);
			OutputDebugString(mess);
			OutputDebugString("**************************************\n");
			sortit.clear();
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackCount[i],pair<int,char *>(StackSize[i],StackNames[i])));
			Sleep(5);
			for (j=sortit.begin();j!=sortit.end();j++)
			{
				sprintf(mess,"%7d cnt  %s\n",-(*j).first,(*j).second.second);
				Sleep(5);
				OutputDebugString(mess);
			}
		}
#if 0 //sw doesn't have the tag stuff		
		// Sort by size.
		Sleep(5);
		OutputDebugString("***************************************\n");
		OutputDebugString("By Tag, sort: size ********************\n");
		OutputDebugString("size(K)   count  name  \n");
		OutputDebugString("-----------------------\n");
		Sleep(5);
		multimap<int,int> sorted;
		for (i=0;i<1000;i++)
		{
			if (CheckpointCount[i])
			{
				sorted.insert(pair<int,int>(-CheckpointSize[i],i));
			}
		}
		multimap<int,int>::iterator k;
		for (k=sorted.begin();k!=sorted.end();k++)
		{
			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],(*k).second>=2?tagDefs[(*k).second-2]:"unknown");
			Sleep(5);
			OutputDebugString(mess);
		}
		
		// Sort by count.
		Sleep(5);
		OutputDebugString("By Tag, sort: count *******************\n");
		OutputDebugString("size(K)   count  name  \n");
		OutputDebugString("-----------------------\n");
		Sleep(5);
		sorted.clear();
		for (i=0;i<1000;i++)
		{
			if (CheckpointCount[i])
			{
				sorted.insert(pair<int,int>(-CheckpointCount[i],i));
			}
		}
		for (k=sorted.begin();k!=sorted.end();k++)
		{
			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],(*k).second>=2?tagDefs[(*k).second-2]:"unknown");
			Sleep(5);
			OutputDebugString(mess);
		}
#endif
#endif
	}

	~Leakage()
	{
#if MEM_DEBUG
		if (mem_leakfile && mem_leakfile->integer)
		{
			dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE,"leakage.out");
			dbgMemReportLeakage(NULL,1,1);
			dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_PROMPT,NULL);
		}
		if (mem_leakreport && mem_leakreport->integer)
		{
			LeakReport();
		}
#endif
	}
#if MEM_DEBUG

	void EnableChecking(int x)
	{
		if (x)
		{
			dbgMemSetSafetyLevel(MEM_SAFETY_DEBUG);
			dbgMemPoolSetCheckFrequency(MyPool, x);
			dbgMemSetCheckFrequency(x);
			dbgMemDeferFreeing(TRUE);
			dbgMemSetDeferQueueLen(50000);
		}
		else
		{
			dbgMemSetSafetyLevel(MEM_SAFETY_SOME);
			dbgMemDeferFreeing(FALSE);
		}

	}
#endif

};

static Leakage TheLeakage;

#if MEM_DEBUG

void MEM_Checking_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("mem_checking <frequency>\n");
		return;
	}

	if (atol(Cmd_Argv(1)) > 0 && atol(Cmd_Argv(1)) < 100)
	{
		Com_Printf ("mem_checking frequency is too low ( < 100 )\n");
		return;
	}

	TheLeakage.EnableChecking(atol(Cmd_Argv(1)));
}

void MEM_Report_f(void)
{
	if (0)
	{
		dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE,"leakage.out");
		dbgMemReportLeakage(NULL,1,1);
		dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_PROMPT,NULL);
	}
	TheLeakage.LeakReport();
}

/*
void myexit(void)
{
	TheLeakage.LeakReport();
}
*/
void SH_Register(void)
{
	Cmd_AddCommand ("mem_checking", MEM_Checking_f);
	Cmd_AddCommand ("mem_report", MEM_Report_f);

	mem_leakfile = Cvar_Get( "mem_leakfile", "0", 0 );
	mem_leakreport = Cvar_Get( "mem_leakreport", "1", 0 );
//	atexit(myexit);
}

#endif
