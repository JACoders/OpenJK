#ifndef __BUFFER_H
#define __BUFFER_H

extern const int	BufferIncrease;

class cBuffer
{
private:
	char			*Buffer;
	int				Size, ActualSize, Pos;
	bool			FreeNextAdd;
	int				Increase;

public:
			cBuffer(int InitIncrease = BufferIncrease);
			~cBuffer(void) { Free(); }

	const char			*Get(void) { return Buffer; } 
	const char			*GetWithPos(void) { return Buffer + Pos; } 
	const int			GetSize(void) { return Size; }
	const int			GetRemaining(void) { return Size - Pos; }

	void				FreeBeforeNextAdd(void) { FreeNextAdd = true; }
	void				ResetPos(void) { Pos = 0; }
	void				ResetSize(void) { Size = 0; }

	char				*Add(char *Data, int Amount);
	void				Read(void);
	char				*Read(int Amount);
	bool				ValidPos(void) { return (Pos < Size); }

	size_t				strcspn(const char *notin);

	void				Free(void);
};

#endif // __BUFFER_H