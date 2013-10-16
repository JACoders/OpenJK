//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#ifdef _DONETPROFILE_

#pragma warning( disable : 4786) 
#pragma warning( disable : 4100) 
#pragma warning( disable : 4663) 

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "hstring.h"
#include "INetProfile.h"

using namespace std;

class CNetProfile : public INetProfile
{
	float						mElapsedTime;
	map <hstring,unsigned int>	mFieldCounts;
	float						mFrameCount;

public:
	void Reset(void)
	{
		mFieldCounts.clear();
		mFrameCount=0;
	}

	void AddField(char *fieldName,int sizeBytes)
	{
		assert(sizeBytes>=0);
		if(sizeBytes==0)
		{
			return;
		}
		map<hstring,unsigned int>::iterator f=mFieldCounts.find(fieldName);
		if(f==mFieldCounts.end())
		{
			mFieldCounts[fieldName]=(unsigned int)sizeBytes;
		}
		else
		{
			mFieldCounts[fieldName]+=(unsigned int)sizeBytes;
		}
	}
	
	void IncTime(int msec)
	{
		mElapsedTime+=msec;
	}

	void ShowTotals(void)
	{
		float									totalBytes=0;
		multimap<unsigned int,hstring>			sort;
		map<hstring,unsigned int>::iterator		f;		
		for(f=mFieldCounts.begin();f!=mFieldCounts.end();++f)
		{
			sort.insert(pair<unsigned int,hstring> ((*f).second,(*f).first));
			totalBytes+=(*f).second;
		}

		multimap<unsigned int,hstring>::iterator	j;
		char										msg[1024];
		float										percent;
		sprintf(msg,
			    "******** Totals: bytes %d : bytes per sec %d ********\n",
			    (unsigned int)totalBytes,
			    (unsigned int)((totalBytes/mElapsedTime)*1000));
		Sleep(10);
		Com_OPrintf("%s", msg);
		for(j=sort.begin();j!=sort.end();++j)
		{
			percent=(((float)(*j).first)/totalBytes)*100.0f;
			assert(strlen((*j).second.c_str())<1024);
			sprintf(msg,"%36s : %3.4f percent : %d bytes \n",(*j).second.c_str(),percent,(*j).first);
			Sleep(10);
			Com_OPrintf("%s", msg);
		}
	}
};

INetProfile &ClReadProf(void)
{
	static CNetProfile theClReadProf;
	return(theClReadProf);
}

INetProfile &ClSendProf(void)
{
	static CNetProfile theClSendProf;
	return(theClSendProf);
}

#endif // _DONETPROFILE_