#ifndef NS_BINKVIDEO
#define NS_BINKVIDEO

#include "bink.h"
#define NS_BV_DEFAULT_CIN_BPS (4)
#define MAX_WIDTH			512
#define MAX_HEIGHT			512

#define XBOX_BUFFER_SIZE	NS_BV_DEFAULT_CIN_BPS * MAX_WIDTH * MAX_HEIGHT
#define XBOX_BINK_SND_MEM	16448

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
	bool	alpha;
	float	x1;
	float	y1;
	float   x2;
	float	y2;
	float	w;
	float	h;

	bool	initialized;

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
	void	AllocateXboxMem(void);
	void	FreeXboxMem(void);
	static void*	Allocate(U32 size);
	static void		Free(void* ptr);

	bool	Ready(void) { return (bool)this->bink; }
};

#endif
