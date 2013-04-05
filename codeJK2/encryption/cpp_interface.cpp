#include "encryption.h"

#ifdef _ENCRYPTION_

#pragma warning( disable : 4786) 
#pragma warning( disable : 4100) 
#pragma warning( disable : 4511) 

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <map>
#include <string>
#pragma warning (pop)

#include "sockets.h"
#include "cpp_interface.h"


using namespace std;


extern "C" char	*Cvar_VariableString( const char *var_name );


const char	*GOOD_REQUEST = "GRANTED";
const char	*BAD_REQUEST = "DENIED";


static cWinsock Winsock;


class cAuto
{
public:
	cAuto(void);
	~cAuto(void);
};

cAuto::cAuto(void)
{
	Winsock.Init();
};


cAuto::~cAuto(void)
{
	Winsock.Shutdown();
};


static cAuto Auto;

const int	key_size	= 4096;
const int	user_size	= 256;
const int				buffer_size	= 131072;
const unsigned short	SERVER_PORT = 80;


unsigned char					LastKey[key_size];
bool							LastKeyValid = false;

class cEncryptedFile
{
private:
	unsigned char					MainBuffer[buffer_size];
	int								MainBufferPosition;
	int								MainBufferSize;
	unsigned char					Key[key_size];
	bool							KeyValid;
	string							FileName;
	long							FileSize;
	long							CurrentPosition;

public:
	cEncryptedFile(char *InitFileName, long InitSize );
	
	static cEncryptedFile	*Create(char *FileName, unsigned char *InitKey = NULL);

	bool					GetKey(char *KeyUser, char *KeyPassword);
	void					SetKey(unsigned char *NewKey);

	int						Read(void *Buffer, int Position, int Amount, FILE *FH = NULL);
	void					SetPosition(long Offset, int origin);
	long					GetCurrentPosition(void) { return CurrentPosition; }
};

class cGameConnection : public cConnection
{
private:
	cEncryptedFile	*File;

public:
	cGameConnection(cSocket *Init_Socket, cEncryptedFile	*InitFile);

	virtual bool		ReadCallback(void);
	virtual bool		WriteCallback(void);
};

cGameConnection::cGameConnection(cSocket *Init_Socket,  cEncryptedFile	*InitFile)
:cConnection(Init_Socket, NULL, false),
 File(InitFile)
{
}

bool cGameConnection::ReadCallback(void)
{
	char			*token;
	bool			GoodRequest  = false;
	bool			ValidKey = false;
	unsigned char	*Key = NULL;

	token = strtok((char *)GetBuffer().Get(), " \n\r");
	while(token)
	{
		if (!strcmp(token, "Request:"))
		{
			token = strtok(NULL, "\n\r");
			if (token)
			{
				printf("Request: '%s'\n", token);

				if (!strcmp(token, GOOD_REQUEST))
				{
					GoodRequest = true;
				}
			}
		}
		else if (!strcmp(token, "Key:"))
		{
			token = strtok(NULL, "\n\r");
			if (token)
			{
				if (token[0] == '\"' && strlen(token) == key_size+2 && token[key_size+1] == '\"')
				{
					ValidKey = true;
					Key = (unsigned char *)token+1;
				}
			}
		}
		else
		{
			break;
		}
		token = strtok(NULL, " \n\r");
	}

	if (GoodRequest && ValidKey)
	{
		File->SetKey(Key);
	}

	return true;
}

bool cGameConnection::WriteCallback(void) 
{ 
	Reading = true;
	Buffer.FreeBeforeNextAdd();

	return false; 
}




cEncryptedFile::cEncryptedFile(char *InitFileName, long InitSize)
:FileName(InitFileName), FileSize(InitSize)
{
	MainBufferPosition = -1;
	KeyValid = false;
	CurrentPosition = 0;
}

cEncryptedFile	*cEncryptedFile::Create(char *FileName, unsigned char *InitKey)
{
	FILE				*FH;
	cEncryptedFile		*new_file;
	char				userdata[user_size*2+2];
	char				KeyUser[user_size], KeyPassword[user_size];
	long				Size;

	FH = fopen(FileName, "rb");
	if (!FH)
	{
		return NULL;
	}

	fseek(FH, -(long)sizeof(userdata), SEEK_END);
	Size = ftell(FH);
	
	if (!InitKey)
	{
		fread(userdata, 1, sizeof(userdata), FH);
		fclose(FH);

		strcpy(KeyUser, userdata);
		strcpy(KeyPassword, &userdata[strlen(KeyUser)+1]);

		new_file = new cEncryptedFile(FileName, Size);
		if (!new_file->GetKey(KeyUser, KeyPassword))
		{
			delete new_file;
			return NULL;
		}
	}
	else
	{
		fclose(FH);

		new_file = new cEncryptedFile(FileName, Size);
		new_file->SetKey(InitKey);
	}
	
	return new_file;
}

bool cEncryptedFile::GetKey(char *KeyUser, char *KeyPassword)
{
	cSocket				*Socket;
	cWinsock			Winsock;
	cGameConnection		*Connection;
	unsigned long		size;
	char				temp[256];
	HKEY				entry;
	DWORD				dwSize, dwType;
	int					value;

	Winsock.Init();

	Socket = new cSocket;
	Connection = new cGameConnection(Socket, this);

	if (Socket->Connect(204,97,248,145,SERVER_PORT))
	{
		Connection->Print("User: %s\r\n", KeyUser);
		Connection->Print("Password: %s\r\n", KeyPassword);

		size = sizeof(temp);
		temp[0] = 0;
		GetComputerName(temp, &size);
		Connection->Print("Info: Computer Name '%s'\r\n", temp);

		size = sizeof(temp);
		temp[0] = 0;
		GetUserName(temp, &size);
		Connection->Print("Info: Network User Name '%s'\r\n", temp);

		RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\MS Setup (ACME)\\User Info", NULL, KEY_READ, &entry);
		dwSize = sizeof(temp);
		temp[0] = 0;
		value = RegQueryValueEx(entry, "DefCompany", NULL, &dwType, (unsigned char *)temp, &dwSize);
		Connection->Print("Info: Company '%s'\r\n", temp);

		temp[0] = 0;
		value = RegQueryValueEx(entry, "DefName", NULL, &dwType, (unsigned char *)temp, &dwSize);
		Connection->Print("Info: Name '%s'\r\n", temp);
		RegCloseKey(entry);

extern int Sys_GetProcessorId( void );
		Connection->Print("Info: procId: 0x%x\r\n", Sys_GetProcessorId() );

#include "../qcommon/stv_version.h"
		Connection->Print("Info: Version '" Q3_VERSION " " __DATE__ "'\r\n");

		Connection->Print("\r\n");
		while(!Connection->Handle())
		{
			Sleep(50);
		}
	}

	delete Connection; // delete's Socket

	return KeyValid;
}

void cEncryptedFile::SetKey(unsigned char *NewKey)
{
	if (!LastKeyValid)
	{
		memcpy(LastKey, NewKey, key_size);
		LastKeyValid = true;
	}

	memcpy(Key, NewKey, key_size);
	KeyValid = true;
}

int cEncryptedFile::Read(void *Buffer, int Position, int Amount, FILE *FH)
{
	int				total;
	unsigned char	*pos;
	int				i;
	int				offset;
	int				size;
	bool			do_close = false;

	if (!KeyValid)
	{
		return -1;
	}

	if (Position == -1)
	{
		Position = CurrentPosition;
	}
	else
	{
		CurrentPosition = Position;
	}

	pos = (unsigned char *)Buffer;
	total = 0;
	offset = Position % key_size;
	Position = Position - (Position % key_size);
	while(Amount > 0)
	{
		if (MainBufferPosition == Position)
		{
		}
		else
		{
			if (!FH)
			{
				FH = fopen(FileName.c_str(), "rb");
				if (!FH)
				{
					return -1;
				}
				
				do_close = true;
			}
			fseek(FH, Position, SEEK_SET);

			MainBufferPosition = Position;
			MainBufferSize = fread(MainBuffer, 1, buffer_size, FH);
			for(i=0;i<MainBufferSize;i++)
			{
//				MainBuffer[i] ^= Key[i % key_size];
				MainBuffer[i] ^= Key[((Position + i) >> 17) % key_size];
			}
		}

		size = MainBufferSize - offset;
		if (size < 0)
		{
			break;
		}
		if (size > Amount)
		{
			size = Amount;
		}

		memcpy(pos, &MainBuffer[offset], size);
		pos += size;
		total += size;
		Amount -= size;

		Position += MainBufferSize;
		offset = 0;
	}

	if (do_close)
	{
		fclose(FH);
	}

	CurrentPosition += total;

	return total;
}

void cEncryptedFile::SetPosition(long Offset, int origin)
{
	switch(origin)
	{
		case SEEK_SET:
			CurrentPosition = Offset;
			break;
		case SEEK_CUR:
			CurrentPosition += Offset;
			break;
		case SEEK_END:
			CurrentPosition = FileSize - Offset;
			break;
	}
}


void	*ENCRYPT_fopen(const char *Name, const char *Mode)
{
	cEncryptedFile	*File;

	File = 	cEncryptedFile::Create((char *)Name, (LastKeyValid ? LastKey : NULL) );

	return File;
}

void	ENCRYPT_fclose(void *File)
{
	delete (cEncryptedFile *)File;
}

int		ENCRYPT_fseek(void *File, long offset, int origin)
{
	((cEncryptedFile *)File)->SetPosition(offset, origin);

	return 0;
}

size_t	ENCRYPT_fread(void *buffer, size_t size, size_t count, void *File)
{
	return ((cEncryptedFile *)File)->Read(buffer, -1, size*count) / size;
}

long	ENCRYPT_ftell(void *File)
{
	return ((cEncryptedFile *)File)->GetCurrentPosition();
}
#endif
