#ifdef _DONETPROFILE_

#define _INETPROFILE_H_
#ifdef _INETPROFILE_H_

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

#endif // _INETPROFILE_H_

#endif // _DONETPROFILE_