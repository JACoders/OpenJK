
/*****************************************************************************
 * name:		l_log.h
 *
 * desc:		log file
 *
 * $Archive: /source/code/botlib/l_log.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

//open a log file
void Log_Open(char *filename);
//close the current log file
void Log_Close(void);
//close log file if present
void Log_Shutdown(void);
//write to the current opened log file
void QDECL Log_Write(char *fmt, ...);
//write to the current opened log file with a time stamp
void QDECL Log_WriteTimeStamped(char *fmt, ...);
//returns a pointer to the log file
FILE *Log_FilePointer(void);
//flush log file
void Log_Flush(void);

