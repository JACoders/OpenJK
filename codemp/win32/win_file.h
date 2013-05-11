#pragma once

/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

#ifndef _WIN_FILE_
#define _WIN_FILE_

typedef int wfhandle_t;

extern void WF_Init(void);
extern void WF_Shutdown(void);
extern wfhandle_t WF_Open(const char* name, bool read, bool aligned);
extern void WF_Close(wfhandle_t handle);
extern int WF_Read(void* buffer, int len, wfhandle_t handle);
extern int WF_Write(const void* buffer, int len, wfhandle_t handle);
extern int WF_Seek(int offset, int origin, wfhandle_t handle);
extern int WF_Tell(wfhandle_t handle);
extern int WF_Resize(int size, wfhandle_t handle);

int Sys_GetFileCode(const char *name);
void Sys_InitFileCodes(void);
void Sys_ShutdownFileCodes(void);
