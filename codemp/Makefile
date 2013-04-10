CC			= gcc
CXX			= g++
CXXFLAGS		+= -fpermissive -m32 -w -D_JK2 -D__linux__ -D_M_IX86 -I. -DUSE_LOCAL_HEADERS -ISDL12/include -DDEDICATED
CFLAGS			+= -fpermissive -m32 -w -D_JK2 -D__linux__ -D_M_IX86 -I. -DUSE_LOCAL_HEADERS -ISDL12/include -DDEDICATED

SRCS			= 

MP3OBJS			= $(MP3SRCS:.c=.o)

all: $(OBJS) $(MP3OBJS) botlib
	$(CXX) -m32 -o jamp $(OBJS) -lopenal

clean:
	$(RM) $(OBJS) $(MP3OBJS) $(BOTLIBOBJS)
