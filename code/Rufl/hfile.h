////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD USEFUL FUNCTION LIBRARY
//  (c) 2002 Activision
//
//
// Handle File
// -----------
//
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(RUFL_HFILE_INC)
#define RUFL_HFILE_INC


////////////////////////////////////////////////////////////////////////////////////////
// HFile Bindings
//
// These are the standard C hfile bindings, copy these function wrappers to your .cpp
// before including hfile, and modify them if needed to support a different file
// system.
////////////////////////////////////////////////////////////////////////////////////////
//bool	HFILEopen_read(int& handle,	const char* filepath)		{handle=(int)fopen(filepath, "rb");	return (handle!=0);}
//bool	HFILEopen_write(int& handle, const char* filepath)		{handle=(int)fopen(filepath, "wb");	return (handle!=0);}
//bool	HFILEread(int& handle,		void*		data, int size)	{return (fread(data, size, 1, (FILE*)(handle))>0);}
//bool	HFILEwrite(int& handle,		const void* data, int size)	{return (fwrite(data, size, 1, (FILE*)(handle))>0);}
//bool	HFILEclose(int& handle)									{return (fclose((FILE*)handle)==0);}



////////////////////////////////////////////////////////////////////////////////////////
// The Handle String Class
////////////////////////////////////////////////////////////////////////////////////////
class hfile
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
	hfile(const char *file);
	~hfile();

	bool		load(void* data, int datasize);
	bool		save(void* data, int datasize);

	bool		is_open(void)			const;
	bool		is_open_for_read(void)	const;
	bool		is_open_for_write(void)	const;

	bool		open_read(float version=1.0f, int checksum=0)		{return open(version, checksum, true);}
	bool		open_write(float version=1.0f, int checksum=0)		{return open(version, checksum, false);}

	bool		close();

private:
	bool		open(float version, int checksum, bool read);


	int			mHandle;
};


#endif // hfile_H