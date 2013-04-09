CC			= gcc
CXX			= g++
CXXFLAGS		+= -fpermissive -m32
CFLAGS			+= -m32

SRCS			= BlockStream.cpp \
			  ibize.cpp \
			  Interpreter.cpp \
			  md4.cpp \
			  Tokenizer.cpp

OBJS			= $(SRCS:.cpp=.o)

all: $(OBJS)
	$(CXX) -m32 -o IBIze $(OBJS)

clean:
	$(RM) $(OBJS)
