#ifndef __linux__
#error You shouldnt be including this file on non-Linux platforms
#endif

#ifndef __GLW_LINUX_H__
#define __GLW_LINUX_H__

typedef struct
{
	void *OpenGLLib; // instance of OpenGL library

	FILE *log_fp;
} glwstate_t;

extern glwstate_t glw_state;

#endif
