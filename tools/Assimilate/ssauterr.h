// Filename:-	ssauterr.h

#ifndef SSAUTERR_H
#define SSAUTERR_H

//*****************************************************************************
// ssauterr.h
//
//
// Copyright (c) 1995 by Microsoft Corporation, All Rights Reserved
//*****************************************************************************

#define MAKEHR(iStat) MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, (USHORT) (iStat))

#define ESS_CORRUPT					MAKEHR(-10600)	// File %s may be corrupt
#define ESS_DT_BADDATESTR			MAKEHR(-10159)	// Invalid date string: "%s"
#define ESS_DT_INVALID				MAKEHR(-10161)	// Invalid time or date string
#define ESS_NOMORE_HANDLES			MAKEHR(-10164)	// Too many file handles open.
#define ESS_FILE_ACCESSDENIED		MAKEHR(-10165)	// Access to file "%s" denied
#define ESS_FILE_BADDRIVE			MAKEHR(-10166)	// Invalid drive: %s
#define ESS_FILE_BADHANDLE			MAKEHR(-10167)	// Invalid handle.
#define ESS_FILE_BADNAME			MAKEHR(-10168)	// Invalid filename: "%s"
#define ESS_FILE_BADPARAM			MAKEHR(-10170)	// Invalid access code (bad parameter)
#define ESS_FILE_BADPATH			MAKEHR(-10171)	// Invalid DOS path: %s
#define ESS_FILE_CURRENTDIR			MAKEHR(-10172)	// Folder %s is in use
#define ESS_FILE_DISKFULL			MAKEHR(-10173)	// Disk full
#define ESS_FILE_EXISTS				MAKEHR(-10175)	// File "%s" already exists
#define ESS_FILE_LOCKED				MAKEHR(-10176)	// File "%s" is locked
#define ESS_FILE_NOTFOUND			MAKEHR(-10178)	// File "%s" not found
#define ESS_FILE_READ				MAKEHR(-10180)	// Error reading from file
#define ESS_FILE_SHARE				MAKEHR(-10181)	// File %s is already open
#define ESS_FILE_TOOMANY			MAKEHR(-10182)	// Too many file handles open
#define ESS_FILE_VOLNOTSAME			MAKEHR(-10183)	// Cannot rename to another volume
#define ESS_FILE_WRITE				MAKEHR(-10184)	// Error writing to file
#define ESS_INI_BADBOOL				MAKEHR(-10200)	// Initialization variable "%s" must be set to "Yes" or "No"
#define ESS_INI_BADLINE				MAKEHR(-10201)	// Invalid syntax on line %d of file %s
#define ESS_INI_BADNUMBER			MAKEHR(-10202)	// Initialization variable ""%s"" set to invalid number
#define ESS_INI_BADPATH				MAKEHR(-10203)	// Initialization variable ""%s"" set to invalid path
#define ESS_INI_BADVALUE			MAKEHR(-10205)	// Initialization variable ""%s"" set to invalid value
#define ESS_INI_NOSUCHVAR			MAKEHR(-10206)	// Cannot find initialization variable "%s"
#define ESS_INI_NUMRANGE			MAKEHR(-10207)	// Initialization variable "%s" must be between %d and %d
#define ESS_INI_TOO_MANY_ENV		MAKEHR(-10208)	// Too many SS.INI environment strings
#define ESS_LOCK_TIMEOUT			MAKEHR(-10266)	// Timeout locking file: %s
#define ESS_MEM_NOMEMORY			MAKEHR(-10270)	// Out of memory
#define ESS_NO_TWEAK_CHKDOUT		MAKEHR(-10625)	// You cannot modify the properties of a file that is checked out.
#define ESS_NOMERGE_BIN_NODELTA		MAKEHR(-10279)	// You cannot perform a merge on a binary file, or a file that stores latest version only.
#define ESS_NOMULTI_BINARY			MAKEHR(-10280)	// Cannot check out %s. It is binary and is already checked out.
#define ESS_NOMULTI_NODELTA			MAKEHR(-10281)	// %s stores only the latest version and is already checked out.
#define ESS_OS_NOT_EXE				MAKEHR(-10285)	// Error executing: %s
#define ESS_SS_ADDPRJASSOCFILE		MAKEHR(-10626)	// %s is a SourceSafe configuration file and cannot be added.
#define ESS_SS_ADMIN_LOCKOUT		MAKEHR(-10456)	// The SourceSafe database has been locked by the Administrator.
#define ESS_SS_BADRENAME			MAKEHR(-10402)	// Unable to rename %s to %s.
#define ESS_SS_CANT_FIND_SSINI		MAKEHR(-10403)	// Cannot find SS.INI file for user %s
#define ESS_SS_CHECKED_OUT			MAKEHR(-10405)	// File %s is currently checked out by %s
#define ESS_SS_CHECKED_OUT_YOU		MAKEHR(-10406)	// You currently have file %s checked out
#define ESS_SS_CHECKOUT_OLD			MAKEHR(-10408)	// Cannot check out an old version of a file
#define ESS_SS_CHKOUT_USER			MAKEHR(-10413)	// File %s is currently checked out by %s
#define ESS_SS_CONFLICTS			MAKEHR(-10415)	// An automatic merge has occurred and there are conflicts.\nEdit %s to resolve them.
#define ESS_SS_DEL_ROOT				MAKEHR(-10418)	// Cannot delete the root project
#define ESS_SS_DEL_SHARED			MAKEHR(-10419)	// A deleted link to %s already exists
#define ESS_SS_FILE_NOTFOUND		MAKEHR(-10421)	// File ""%s"" not found
#define ESS_SS_HISTOPEN				MAKEHR(-10404)	// A history operation is already in progress
#define ESS_SS_INSUFRIGHTS			MAKEHR(-10423)	// You do not have access rights to %s
#define ESS_SS_LATERCHKEDOUT		MAKEHR(-10426)	// A more recent version is checked out
#define ESS_SS_LOCALRW				MAKEHR(-10427)	// A writable copy of %s already exists
#define ESS_SS_MOVE_CHANGENAME		MAKEHR(-10428)	// Move does not change the name of a project
#define ESS_SS_MOVE_NOPARENT		MAKEHR(-10429)	// Project %s does not exist
#define ESS_SS_MOVE_ROOT			MAKEHR(-10430)	// Cannot move the root project
#define ESS_SS_MUST_USE_VERS		MAKEHR(-10431)	// Cannot roll back to the most recent version of %s
#define ESS_SS_NOCOMMANCESTOR		MAKEHR(-10432)	// Files have no common ancestor
#define ESS_SS_NOCONFLICTS2			MAKEHR(-10434)	// %s has been merged with no conflicts.
#define ESS_SS_NODOLLAR				MAKEHR(-10435)	// File %s is invalid. Files may not begin with $.
#define ESS_SS_NOT_CHKEDOUT			MAKEHR(-10436)	// File %s is not checked out
#define ESS_SS_NOT_SHARED			MAKEHR(-10437)	// File %s is not shared by any other projects
#define ESS_SS_NOTSEPARATED			MAKEHR(-10438)	// Files are not branched
#define ESS_SS_OPEN_LOGGIN			MAKEHR(-10457)	// Unable to open user login file %s.
#define ESS_SS_PATHTOOLONG			MAKEHR(-10439)	// Path %s too long
#define ESS_SS_RENAME_MOVE			MAKEHR(-10442)	// Rename does not move an item to another project
#define ESS_SS_RENAME_ROOT			MAKEHR(-10443)	// Cannot Rename the root project
#define ESS_SS_ROLLBACK_NOTOLD		MAKEHR(-10447)	// Cannot Rollback to the most recent version of %s
#define ESS_SS_SHARE_ANCESTOR		MAKEHR(-10449)	// A project cannot be shared under a descendant.
#define ESS_SS_SHARED				MAKEHR(-10450)	// File %s is already shared by this project
#define ESS_SSPEC_SYNTAX			MAKEHR(-10515)	// Invalid SourceSafe syntax: "%s"
#define ESS_UM_BAD_CHAR				MAKEHR(-10550)	// Bad username syntax: "%s"
#define ESS_UM_BAD_PASSWORD			MAKEHR(-10551)	// Invalid password
#define ESS_UM_BADVERSION			MAKEHR(-10552)	// Incompatible database version
#define ESS_UM_DEL_ADMIN			MAKEHR(-10553)	// Cannot delete the Admin user
#define ESS_UM_PERM_DENIED			MAKEHR(-10554)	// Permission denied
#define ESS_UM_RENAME_ADMIN			MAKEHR(-10555)	// Can not rename the Admin user
#define ESS_UM_TOO_LONG				MAKEHR(-10556)	// Username too long
#define ESS_UM_USER_EXISTS			MAKEHR(-10557)	// User "%s" already exists
#define ESS_UM_USER_NOT_FOUND		MAKEHR(-10558)	// User "%s" not found
#define ESS_URL_BADPATH				MAKEHR(-10192)	// The URL for project %s was not set properly.
#define ESS_VS_CHECKED_OUT			MAKEHR(-10601)	// File %s checked out
#define ESS_VS_CHILD_NOT_FOUND		MAKEHR(-10602)	// Subproject or file not found
#define ESS_VS_COLLISION			MAKEHR(-10603)	// Collision accessing database, please try again.
#define ESS_VS_EXCLUSIVE_CHECKED_OUT MAKEHR(-10614)	// File %s is exclusively checked out.
#define ESS_VS_ITEMEXISTS			MAKEHR(-10604)	// An item with the name %s already exists
#define ESS_VS_LONGNAME				MAKEHR(-10605)	// %s is an invalid %s name
#define ESS_VS_MOVE_CYCLE			MAKEHR(-10606)	// You can not move a project under itself
#define ESS_VS_NO_DELTA				MAKEHR(-10607)	// File %s does not retain old versions of itself
#define ESS_VS_NOT_CHECKED_OUT		MAKEHR(-10608)	// File %s cannot be checked into this project
#define ESS_VS_NOT_FOUND			MAKEHR(-10609)	// File or project not found
#define ESS_VS_PARENT_NOT_FOUND		MAKEHR(-10610)	// Parent not found
#define ESS_VS_VERS_NOT_FOUND		MAKEHR(-10615)	// Version not found
#define ESS_VS_WANT_FILE			MAKEHR(-10616)	// This command only works on files.
#define ESS_VS_WANT_PRJ				MAKEHR(-10617)	// This command only works on projects.
#define ESS_URL_BUFOVERFLOW			MAKEHR(-10194)	// A link in %s was ignored because it was longer than SourceSafe can understand
#define ESS_URL_CANTCHECKHTML		MAKEHR(-10193)	// An error occurred while  trying to check hyperlinks for %s
#define ESS_SS_ADDINFAILED			MAKEHR(-10440)	// Error loading SourceSafe add-in: %s
#define ESS_CANCEL					MAKEHR(-32766)
#define ESS_LOADSTRING_FAILED		MAKEHR(-10999)	// Error loading resource string

// SourceSafe questions answered affirmatively.
//
// A deleted copy of this %s file already exists in this project.\nDo you want to recover the existing file?
// Folder %s not found, create?
// Have any conflicts in %s been properly resolved?
// File %s is currently checked out by %s.\nProceed anyway?
// File %s was checked out to folder %s.\nProceed in %s?
// File %s is checked out to project %s, and you are in %s.\nProceed anyway?
// File %s is currently checked out by %s.  Delete anyway?
// You currently have file %s checked out.  Delete anyway?
// An item named %s was already deleted from this project.\nPurge the old item and delete this one now?
// This version of %s already has a label: overwrite?
// The label %s is already used.  Remove the old label?
// %s has been merged with no conflicts.\nCheck in now?
// Redo the automatic merge?
// Delete local file: %s?
// %s is already checked out, continue?
// File %s has been destroyed, and cannot be rebuilt.\nContinue anyway?
// Project $%s has been destroyed, and cannot be rebuilt.\nContinue anyway?
// $%s was moved out of this project, and cannot be rebuilt.\nContinue anyway?
// %s has changed. Undo check out and lose changes?
//
// SourceSafe questions answered in the negative.
//
// A deleted file of the same name already exists in this SourceSafe project.\nDo you want to recover the deleted file instead of adding your local %s?
// %s is writable, replace?
// %s is checked out, replace?


#endif	// #ifndef SSAUTERR_H

////////////////////////////// eof ////////////////////////////////
