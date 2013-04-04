//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#pragma warning( disable : 4786) 
#include "client.h"

#include <windows.h>
#include "..\smartheap\smrtheap.h"
#if !defined(__Q_SHARED_H)
	#include "../game/q_shared.h"
#endif
#if !defined(_QCOMMON_H_)
	#include "../qcommon/qcommon.h"
#endif
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
static int CheckpointSize[1000];
static int CheckpointCount[1000];

#define _FASTRPT_

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
//return 1;
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
//				start=0;
				continue;
			}
			if (strstr(start,"Malloc"))
			{
				start=0;
				continue;
			}
			if (strstr(start,"FS_LoadFile"))
			{
				start=0;
				continue;
			}
			if (strstr(start,"CopyString"))
			{
				start=0;
				continue;
			}
			break;
		}
	}
	if (!start||!*start)
	{
		start="UNKNOWN";
	}

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
#endif
		EnableChecking(100000);
	}

	void LeakReport(void)
	{
#if MEM_DEBUG

		int i;
		char mess[1000];
		int blocks=dbgMemTotalCount();
		int mem=dbgMemTotalSize()/1024;
		sprintf(mess,"Final Memory Summary %d blocks %d K\n",blocks,mem);
		OutputDebugString(mess);
		for (i=0;i<1000;i++)
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
			multimap<int,pair<int,char *> > sortit;
			for (i=0;i<nStack;i++)
				sortit.insert(pair<int,pair<int,char *> >(-StackSize[i],pair<int,char *>(StackCount[i],StackNames[i])));
			multimap<int,pair<int,char *> >::iterator j;
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
//			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],(*k).second>=2?tagDefs[(*k).second-2]:"unknown");
			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],"unknown");
			Sleep(5);
			OutputDebugString(mess);
		}
		
		// Sort by count.
		Sleep(5);
		OutputDebugString("***************************************\n");
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
//			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],(*k).second>=2?tagDefs[(*k).second-2]:"unknown");
			sprintf(mess,"%8d %8d %s\n",CheckpointSize[(*k).second]/1024,CheckpointCount[(*k).second],"unknown");
			Sleep(5);
			OutputDebugString(mess);
		}
#endif
	}

	~Leakage()
	{
#if MEM_DEBUG
#if 0
		if (mem_leakfile && mem_leakfile->integer)
		{
			dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_FILE,"leakage.out");
			dbgMemReportLeakage(NULL,1,1);
			dbgMemSetDefaultErrorOutput(DBGMEM_OUTPUT_PROMPT,NULL);
		}
#endif
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
			if (x>50000)
			{
				dbgMemSetDeferQueueLen(x+5000);
			}
			else
			{
				dbgMemSetDeferQueueLen(50000);
			}
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

	mem_leakfile = Cvar_Get( "mem_leakfile", "1", 0 );
	mem_leakreport = Cvar_Get( "mem_leakreport", "1", 0 );
//	atexit(myexit);
}

#endif
