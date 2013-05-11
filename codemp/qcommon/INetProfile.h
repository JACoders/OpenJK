#pragma once

#ifdef _DONETPROFILE_
class INetProfile
{
public:
	virtual void Reset(void)=0;
	virtual void AddField(char *fieldName,int sizeBytes)=0;
	virtual void IncTime(int msec)=0;
	virtual void ShowTotals(void)=0;
};

INetProfile &ClReadProf(void);
INetProfile &ClSendProf(void);
#endif // _DONETPROFILE_
