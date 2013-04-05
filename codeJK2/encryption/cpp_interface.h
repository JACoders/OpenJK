#ifndef __CPP_INTERFACE_H
#define __CPP_INTERFACE_H

#ifdef __cplusplus
extern "C"
{
#endif

void	*ENCRYPT_fopen(const char *Name, const char *Mode);
void	ENCRYPT_fclose(void *File);
int		ENCRYPT_fseek(void *File, long offset, int origin);
size_t	ENCRYPT_fread(void *buffer, size_t size, size_t count, void *File);
long	ENCRYPT_ftell(void *File);

#ifdef __cplusplus
}
#endif


#endif // __CPP_INTERFACE_H