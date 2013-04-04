#ifndef NS_BINKVIDEO
#define NS_BINKVIDEO

#include "bink.h"
#define NS_BV_DEFAULT_CIN_BPS (2)
#define MAX_WIDTH			512
#define MAX_HEIGHT			512

#define XBOX_MEM_STAGE_1	32640
#define XBOX_MEM_STAGE_2	786528
#define XBOX_MEM_STAGE_3	557152
#define XBOX_MEM_STAGE_4	106560
#define XBOX_MEM_STAGE_5	138304
#define XBOX_MEM_STAGE_6	25696
#define XBOX_MEM_STAGE_7	100
#define XBOX_MEM_STAGE_8	100

#define XBOX_BUFFER_SIZE	NS_BV_DEFAULT_CIN_BPS * MAX_WIDTH * MAX_HEIGHT

typedef enum {
	NS_BV_PLAYING,		// Movie is playing
	NS_BV_STOPPED,		// Movie is stopped
	NS_BV_PAUSED		// Movie is paused
};

class BinkVideo
{
private:
	HBINK	bink;
	void*	buffer;
	int		texture;
	int		status;
	bool	looping;
	float	x1;
	float	y1;
	float   x2;
	float	y2;
	float	w;
	float	h;

#ifdef _XBOX
	bool	initialized;
#endif

	void	Draw(void);
	S32		DecompressFrame();
	

public:
	
	BinkVideo();
	~BinkVideo();
	bool	Start(const char *filename, float xOrigin, float yOrigin, float width, float height);
	bool	Run(void);
	void	Stop(void);
	void	Pause(void);
	void	SetExtents(float xOrigin, float yOrigin, float width, float height);
	int		GetStatus(void) { return status; }
	void	SetLooping(bool loop) { looping = loop; }
	void*	GetBinkData(void);
	int		GetBinkWidth(void) { return this->bink->Width; }
	int		GetBinkHeight(void) { return this->bink->Height; }
	void	SetMasterVolume(s32 volume);
#ifdef _XBOX
	void	AllocateXboxMem(void);
	void	FreeXboxMem(void);
#endif
	static void*	Allocate(U32 size);
	static void		Free(void* ptr);
	bool	Ready(void) { return this->bink != NULL; }
};

#endif
