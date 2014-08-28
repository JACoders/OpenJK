#include "g_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

#define LOCAL_DB_PATH "japro/data.db"
#define GLOBAL_DB_PATH sv_globalDBPath.string

#define CALL_SQLITE(f) {                                        \
        int i;                                                  \
        i = sqlite3_ ## f;                                      \
        if (i != SQLITE_OK) {                                   \
            fprintf (stderr, "%s failed with status %d: %s\n",  \
                     #f, i, sqlite3_errmsg (db));               \
        }                                                       \
    }                                                           \

#define CALL_SQLITE_EXPECT(f,x) {                               \
        int i;                                                  \
        i = sqlite3_ ## f;                                      \
        if (i != SQLITE_ ## x) {                                \
            fprintf (stderr, "%s failed with status %d: %s\n",  \
                     #f, i, sqlite3_errmsg (db));               \
        }                                                       \
    }   

typedef struct RaceRecord_s {
	char				username[16];
	char				coursename[40];
	unsigned int		duration_ms;
	unsigned short		topspeed;
	unsigned short		average;
	unsigned short		style; //only needs to be 3 bits	
	char				end_time[64];
	unsigned int		end_timeInt;
} RaceRecord_t;

RaceRecord_t	HighScores[32][7][10];//32 courses, 7 styles, 10 spots on highscore list

typedef struct UserStats_s {
	char				username[16];
	unsigned short		kills;
	unsigned short		deaths;
	unsigned short		suicides;
	unsigned short		captures;
	unsigned short		returns;
} UserStats_t;

UserStats_t	UserStats[256];//256 max logged in users per map :\

typedef struct UserAccount_s {
	char			username[16];
	char			password[16];
	int				lastIP;
} UserAccount_t;

/*
typedef struct PlayerID_s {
	char			name[MAX_NETNAME];
	unsigned int	ip[NET_ADDRSTRMAXLEN];
	int				guid[33];
} PlayerID_t;
*/

unsigned int ip_to_int (const char * ip) {
    unsigned v = 0;
    int i;
    const char * start;

    start = ip;
    for (i = 0; i < 4; i++) {
        char c;
        int n = 0;
        while (1) {
            c = * start;
            start++;
            if (c >= '0' && c <= '9') {
                n *= 10;
                n += c - '0';
            }
            else if ((i < 3 && c == '.') || i == 3)
                break;
            else
                return 0;
        }
        if (n >= 256)
            return 0;
        v *= 256;
        v += n;
    }
    return v;
}

/*
static void CleanStrin(char &string) {
	
	int i = 0;
	//while (string[i]) {
		//string[i] = tolower(string[i]);
		i++;
	//}
	//Q_CleanStr(string);
}
*/

int CheckUserExists(char *username) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int row = 0;

	//load fixme replace this with simple select count

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT id FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        int s;
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//Trap print?
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	if (row == 0) {
		return 0;
	}
	else if (row == 1) { //Only 1 matching accound
		return 1;
	}
	else {
		trap->Print("ERROR: Multiple accounts with same accountname!\n");
		return 0;
	}
}
void G_AddPlayerLog(char *name, char *strIP, char *guid) {
	fileHandle_t f;
	char string[128], string2[128], buf[80*1024];
	char *p = NULL;
	int	fLen = 0;
	char*	pch;
	qboolean unique = qtrue;

	if (!Q_stricmp(name, "Padawan"))
		return;

	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;

	Com_sprintf(string, sizeof(string), "%s;%s;%s", name, strIP, guid); //Store ip as int or char??.. lets do int

	fLen = trap->FS_Open(PLAYER_LOG, &f, FS_READ);
	if (!f) {
		Com_Printf ("ERROR: Couldn't load player logfile %s\n", PLAYER_LOG);
		return;
	}
	
	if (fLen >= 80*1024) {
		trap->FS_Close(f);
		Com_Printf ("ERROR: Couldn't load player logfile %s, file is too large\n", PLAYER_LOG);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);

	pch = strtok (buf,"\n");

	while (pch != NULL) {
		if (!Q_stricmp(string, pch)) {
			unique = qfalse;
			break;
		}
    	pch = strtok (NULL, "\n");
	}

	if (unique) {
		Com_sprintf(string2, sizeof(string2), "%s\n", string);
		trap->FS_Write(string2, strlen(string2), level.playerLog );
	}

	//If line does not already exist, write it.

	//ftell, fseek

	//Check to see if IP already exists
		//If so, check if username is same
			//If not, update username (or add to it?)
	
		//If not, check if GUID exists
			//If yes, update IP of guid, check to see if name is same
				//If no, update username (or add to it)

			//If not (no IP or guid), add entry.

	//Somehow make this sorted..
}

void G_AddDuel(char *winner, char *loser, int duration, int type, int winner_hp, int winner_shield) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	time_t	rawtime;
	char	string[1024] = {0};

	time( &rawtime );
	localtime( &rawtime );

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", winner, loser, duration, type, winner_hp, winner_shield, rawtime);

	if (level.duelLog)
		trap->FS_Write(string, strlen(string), level.duelLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	//Might want to make this log to file, and have that sent to db on map change.  But whatever.. duel finishes are not as frequent as race course finishes usually.

	if (CheckUserExists(winner) && CheckUserExists(loser)) {
		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "INSERT INTO LocalDuel(winner, loser, duration, type, winner_hp, winner_shield, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, winner, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, loser, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, duration));
		CALL_SQLITE (bind_int (stmt, 4, type));
		CALL_SQLITE (bind_int (stmt, 5, winner_hp));
		CALL_SQLITE (bind_int (stmt, 6, winner_shield));
		CALL_SQLITE (bind_int (stmt, 7, rawtime));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
	}
}

void G_AddToDBFromFile(void) //loda fixme
{
	fileHandle_t f;	
	int		fLen = 0, args = 1; //MAX_FILESIZE = 4096
	char	buf[80 * 1024] = {0}, empty[8] = {0};//eh
	char*	pch;
	sqlite3 * db;
	char * sql;
	sqlite3_stmt * stmt;
	RaceRecord_t	TempRaceRecord;

	fLen = trap->FS_Open(TEMP_RACE_LOG, &f, FS_READ);

	if (!f) {
		Com_Printf ("ERROR: Couldn't load defrag data from %s\n", TEMP_RACE_LOG);
		return;
	}
	if (fLen >= 80*1024) {
		trap->FS_Close(f);
		Com_Printf ("ERROR: Couldn't load defrag data from %s, file is too large\n", TEMP_RACE_LOG);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);
	Com_Printf ("Loaded previous maps racetimes from %s\n", TEMP_RACE_LOG);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";	 //loda fixme, make multiple?

	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	//Todo: make TempRaceRecord an array of structs instead, maybe like 32 long idk, and build a query to insert 32 at a time or something.. instead of 1 by 1
	pch = strtok (buf,";\n");
	while (pch != NULL)
	{
		if ((args % 7) == 1)
			Q_strncpyz(TempRaceRecord.username, pch, sizeof(TempRaceRecord.username));
		else if ((args % 7) == 2)
			Q_strncpyz(TempRaceRecord.coursename, pch, sizeof(TempRaceRecord.coursename));
		else if ((args % 7) == 3)
			TempRaceRecord.duration_ms = atoi(pch);
		else if ((args % 7) == 4)
			TempRaceRecord.topspeed = atoi(pch);
		else if ((args % 7) == 5)
			TempRaceRecord.average = atoi(pch);
		else if ((args % 7) == 6)
			TempRaceRecord.style = atoi(pch);
		else if ((args % 7) == 0) {
			TempRaceRecord.end_timeInt = atoi(pch);
			CALL_SQLITE (bind_text (stmt, 1, TempRaceRecord.username, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_text (stmt, 2, TempRaceRecord.coursename, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 3, TempRaceRecord.duration_ms));
			CALL_SQLITE (bind_int (stmt, 4, TempRaceRecord.topspeed));
			CALL_SQLITE (bind_int (stmt, 5, TempRaceRecord.average));
			CALL_SQLITE (bind_int (stmt, 6, TempRaceRecord.style));
			CALL_SQLITE (bind_int (stmt, 7, TempRaceRecord.end_timeInt));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
    		pch = strtok (NULL, ";\n");
		args++;
	}
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));	

	trap->FS_Open(TEMP_RACE_LOG, &f, FS_WRITE);
	trap->FS_Write( empty, strlen( empty ), level.tempRaceLog );
	trap->FS_Close(f);
}

gentity_t *G_SoundTempEntity( vec3_t origin, int event, int channel );
void PlayActualGlobalSound(char * sound) {
	gentity_t	*te;
	vec3_t temp = {0, 0, 0};

	te = G_SoundTempEntity( temp, EV_GENERAL_SOUND, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex(sound);
	//te->s.saberEntityNum = channel;
	te->s.eFlags = EF_SOUNDTRACKER;
	te->r.svFlags |= SVF_BROADCAST;
}

void G_AddRaceTime(char *username, char *message, int duration_ms, int style, int topspeed, int average) {//should be short.. but have to change elsewhere? is it worth it?
	time_t	rawtime;
	char		string[1024] = {0}, info[1024] = {0}, courseName[40];
	int i, course = 0, newRank = -1, rowToDelete = 9;
	qboolean duplicate = qfalse;

	time( &rawtime );
	localtime( &rawtime );

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(courseName, Info_ValueForKey( info, "mapname" ), sizeof(courseName));

	if (message) {// [0]?
		Q_strlwr(message);
		Q_CleanStr(message);
		Q_strcat(courseName, sizeof(courseName), va(" (%s)", message));
	}

	if (!CheckUserExists(username))
		return;

	Q_strlwr(courseName);
	Q_CleanStr(courseName);

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", username, courseName, duration_ms, topspeed, average, style, rawtime);

	if (level.raceLog)
		trap->FS_Write(string, strlen(string), level.raceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.
	//if (level.tempRaceLog)
		//trap->FS_Write(string, strlen(string), level.tempRaceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	//Now for live highscore stuff:

	for (i = 0; i < level.numCourses; i++) {
		if (!Q_stricmp(message, level.courseName[i])) {
			course = i;
			break;
		}
	}

	for (i = 0; i < 10; i++) {
		if (duration_ms < HighScores[course][style][i].duration_ms) { //We were faster
			if (newRank == -1) {
				newRank = i;
				//trap->Print("Newrank set %i!\n", newRank);
			}
		}
		//trap->Print("us: %s, them: %s\n", username, HighScores2[course][style][i].username);
		if (!Q_stricmp(username, HighScores[course][style][i].username)) { //Its us
			if (newRank >= 0) {
				rowToDelete = i;
				//trap->Print("duplicate set!\n");
				duplicate = qtrue;
			}
			else {
				//trap->Print("This user already has a faster time!!\n");
				return;
			}
		}
		if (!HighScores[course][style][i].username[0]) { //Empty
			//trap->Print("Empty!\n");
			if (newRank == -1) {
				//trap->Print("OKAY1!\n");
				if (!duplicate) {
					//trap->Print("OKAY2!\n");
					newRank = i;
				}
			}
			if (!duplicate)
				rowToDelete = -1;
		}
	}

	//trap->Print("NewRank: %i, RowToDelete: %i\n", newRank, rowToDelete);
		
	if (newRank >= 0) {
		if (rowToDelete >= 0) {
			for (i = rowToDelete; i < 10; i++) {
				if (i < 9)
					HighScores[course][style][i] = HighScores[course][style][i + 1];
				else 
					Q_strncpyz(HighScores[course][style][i].username, "", sizeof(HighScores[course][style][i].username));
			}
		}
		
		for (i = 8; i >= newRank; i--) {
			HighScores[course][style][i + 1] = HighScores[course][style][i];
		}

		Q_strncpyz(HighScores[course][style][newRank].username, username, sizeof(HighScores[i][style][newRank].username));
		Q_strncpyz(HighScores[course][style][newRank].coursename, username, sizeof(HighScores[i][style][newRank].coursename));
		HighScores[course][style][newRank].duration_ms = duration_ms;
		HighScores[course][style][newRank].topspeed = topspeed;
		HighScores[course][style][newRank].average = average;
		HighScores[course][style][newRank].style = style;
		Q_strncpyz(HighScores[course][style][newRank].end_time, "Just now", sizeof(HighScores[i][style][newRank].end_time));

		if (level.tempRaceLog) //Lets try only writing to temp file if we know its a highscore
			trap->FS_Write(string, strlen(string), level.tempRaceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

		if (newRank == 0) //Play the sound
			PlayActualGlobalSound("sound/chars/rosh_boss/misc/victory3");
		else 
			PlayActualGlobalSound("sound/chars/rosh/misc/taunt1");
	}
		//One by one
			//If my time is faster, and newRank is -1, store i as NewRank.  We will eventually insert our time into spot i

			//else If spot is empty, and newRank is -1, set newRank to i, and rowToDelete to -1, we will insert our time here.

			//Else if newrank is set, and this row is also us, mark this row for deletion?.

		
		//If newrank
			//remove rowToDelete, and fill gap by moving slower times up

			//Shift every row after newRank down one
			//Insert our new time into newRank spot
}

//So the best way is to probably add every run as soon as its taken and not filter them.
//to cut down on database size, there should be a cleanup on every mapload or.. every week..or...?
//which removes any time not in the top 100 of its category AND more than 1 week old?

void Cmd_ACLogin_f( gentity_t *ent ) { //loda fixme show lastip ? or use lastip somehow
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0, s, count = 0;
	unsigned int ip, lastip;
	char username[16], enteredPassword[16], password[16], strIP[NET_ADDRSTRMAXLEN] = {0};
	char *p = NULL;

	if (trap->Argc() != 3) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /login <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, enteredPassword, sizeof(password));

	Q_strncpyz(strIP, ent->client->sess.IP, sizeof(strIP));
	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;
	ip = ip_to_int(strIP);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	if (ip) {
		sql = "SELECT COUNT(*) FROM LocalAccount WHERE lastip = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int64 (stmt, 1, ip));

		s = sqlite3_step(stmt);

		if (s == SQLITE_ROW)
			count = sqlite3_column_int(stmt, 0);
		else if (s != SQLITE_DONE) {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}

		CALL_SQLITE (finalize(stmt));
	}

	sql = "SELECT password, lastip FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        int s;
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			Q_strncpyz(password, (char*)sqlite3_column_text(stmt, 0), sizeof(password));
			lastip = sqlite3_column_int(stmt, 1);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//Trap print?
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));

	if (row == 0) { // No accounts found
		trap->SendServerCommand(ent-g_entities, "print \"Account not found! To make a new account, use the /register command.\n\"");
		CALL_SQLITE (close(db));
		return;
	}
	else if (row > 1) { // More than 1 account found
		trap->Print("ERROR: Multiple accounts with same accountname!\n");
		CALL_SQLITE (close(db));
		return;
	}

	if ((count > 0) && lastip && (lastip != ip)) { //IF lastip already tied to account, and lastIP (of attempted login username) does not match current IP, deny.?
		trap->SendServerCommand(ent-g_entities, "print \"Your IP address already belongs to an account. You are only allowed one account.\n\"");
		CALL_SQLITE (close(db));
		return;
	}
		
	if (enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		time_t	rawtime;

		time( &rawtime );
		localtime( &rawtime );

		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
		trap->SendServerCommand(ent-g_entities, "print \"Login sucessful.\n\"");

		sql = "UPDATE LocalAccount SET lastip = ?, lastlogin = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int64 (stmt, 1, ip));
		CALL_SQLITE (bind_int (stmt, 2, rawtime));
		CALL_SQLITE (bind_text (stmt, 3, username, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (finalize(stmt));
	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect password!\n\"");
	}	
	CALL_SQLITE (close(db));
}

void Cmd_ChangePassword_f( gentity_t *ent ) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0;
	char username[16], enteredPassword[16], newPassword[16], password[16];

	if (trap->Argc() != 4) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /changepassword <username> <password> <newpassword>\n\"");
		return;
	}

	if (!Q_stricmp(ent->client->pers.userName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are not logged in!\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, enteredPassword, sizeof(password));
	trap->Argv(3, newPassword, sizeof(password));

	if (Q_stricmp(ent->client->pers.userName, username)) {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect username!\n\"");
		return;
	}

	Q_strlwr(newPassword);
	Q_CleanStr(newPassword);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT password FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, ent->client->pers.userName, -1, SQLITE_STATIC));
	
    while (1) {
        int s;
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			Q_strncpyz(password, (char*)sqlite3_column_text(stmt, 0), sizeof(password));
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//Trap print?
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));

	if (enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		sql = "UPDATE LocalAccount SET password = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, newPassword, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, ent->client->pers.userName, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (finalize(stmt));

		trap->SendServerCommand(ent-g_entities, "print \"Password Changed.\n\"");
	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect password!\n\"");
	}	
	CALL_SQLITE (close(db));
}

void Svcmd_ChangePass_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], newPassword[16];

	if (trap->Argc() != 3) {
		trap->Print( "Usage: /changepassword <username> <newpassword>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, newPassword, sizeof(newPassword));

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strlwr(newPassword);
	Q_CleanStr(newPassword);

	if (!CheckUserExists(username)) {
		trap->Print( "User does not exist!\n");
		return;
	}

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "UPDATE LocalAccount SET password = ? WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, newPassword, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));
	trap->Print( "Password changed.\n");
	CALL_SQLITE (close(db));
}

void Svcmd_ClearIP_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16];

	if (trap->Argc() != 2) {
		trap->Print( "Usage: /clearIP <username>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));

	Q_strlwr(username);
	Q_CleanStr(username);

	if (!CheckUserExists(username)) {
		trap->Print( "User does not exist!\n");
		return;
	}

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "UPDATE LocalAccount SET lastip = 0 WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));
	trap->Print( "IP Cleared.\n");
	CALL_SQLITE (close(db));
}

void Svcmd_Register_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], password[16];
	time_t	rawtime;

	if (trap->Argc() != 3) {
		trap->Print( "Usage: /register <username> <password>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, password, sizeof(password));

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strlwr(password);
	Q_CleanStr(password);

	if (CheckUserExists(username)) {
		trap->Print( "User already exists!\n");
		return;
	}

	time( &rawtime );
	localtime( &rawtime );

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "INSERT INTO LocalAccount (username, password, kills, deaths, suicides, captures, returns, playtime, lastlogin, lastip) VALUES (?, ?, 0, 0, 0, 0, 0, 0, ?, 0)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, password, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, rawtime));
    CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	trap->Print( "Account created.\n");
	CALL_SQLITE (close(db));
}

void Svcmd_DeleteAccount_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], confirm[16];

	if (trap->Argc() != 3) {
		trap->Print( "Usage: /deleteAccount <username> <confirm>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, confirm, sizeof(confirm));

	if (Q_stricmp(confirm, "confirm")) {
		trap->Print( "Usage: /deleteAccount <username> <confirm>\n");
		return;
	}

	Q_strlwr(username);
	Q_CleanStr(username);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	
	if (CheckUserExists(username)) {
		sql = "DELETE FROM LocalAccount WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (finalize(stmt));
		trap->Print( "Account deleted.\n");
	}
	else 
		trap->Print( "User does not exist, deleting highscores for username anyway.\n");

	sql = "DELETE FROM LocalRun WHERE username = ?";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
    CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));
}

void Svcmd_AccountInfo_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], timeStr[64] = {0}, buf[MAX_STRING_CHARS-64] = {0};
	int row = 0, lastlogin;
	unsigned int lastip;
	time_t	timeGMT;

	if (trap->Argc() != 2) {
		trap->Print( "Usage: /accountInfo <username>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));

	Q_strlwr(username);
	Q_CleanStr(username);

	if (!CheckUserExists(username)) {
		trap->Print( "User does not exist!\n");
		return;
	}

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT lastlogin, lastip FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        int s;
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			lastlogin = sqlite3_column_int(stmt, 0);
			lastip = sqlite3_column_int(stmt, 1);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//Trap print?
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	if (row == 0) { //no account found, or more than 1 account with same name, problem
		trap->Print( "Account not found!\n");
		return;
	}
	else if (row > 1) {
		trap->Print( "ERROR: Multiple accounts found!\n");
		return;
	}

	timeGMT = (time_t)lastlogin;
	strftime( timeStr, sizeof( timeStr ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &timeGMT ) );

	Q_strncpyz(buf, va("Stats for %s:\n", username), sizeof(buf));
		Q_strcat(buf, sizeof(buf), va("   ^5Last login: ^2%s\n", timeStr));
	Q_strcat(buf, sizeof(buf), va("   ^5Last IP^3: ^2%u\n", lastip));

	trap->Print( "%s", buf);
}

void Cmd_ACRegister_f( gentity_t *ent ) { //Temporary, until global shit is done
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], password[16], strIP[NET_ADDRSTRMAXLEN] = {0};
	char *p = NULL;
	time_t	rawtime;
	int s, ip;

	if (trap->Argc() != 3) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /register <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) { //if (ent->client->pers.accountName) { //check cuz its a string if [0] = \0 or w/e
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, password, sizeof(password));

	if (CheckUserExists(username)) {
		trap->SendServerCommand(ent-g_entities, "print \"This account name has already been taken!\n\"");
		return;
	}

	time( &rawtime );
	localtime( &rawtime );

	Q_strncpyz(strIP, ent->client->sess.IP, sizeof(strIP));
	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;
	ip = ip_to_int(strIP);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	if (ip) {
		sql = "SELECT COUNT(*) FROM LocalAccount WHERE lastip = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, ip));

		s = sqlite3_step(stmt);

		if (s == SQLITE_ROW) {
			int count;
			count = sqlite3_column_int(stmt, 0);
			if (count > 0) {
				trap->SendServerCommand(ent-g_entities, "print \"Your IP address already belongs to an account. You are only allowed one account.\n\"");
				CALL_SQLITE (finalize(stmt));
				CALL_SQLITE (close(db));
				return;
			}
		}
		else if (s != SQLITE_DONE) {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}
		CALL_SQLITE (finalize(stmt));
	}

    sql = "INSERT INTO LocalAccount (username, password, kills, deaths, suicides, captures, returns, playtime, lastlogin, lastip) VALUES (?, ?, 0, 0, 0, 0, 0, 0, ?, ?)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, password, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, rawtime));
	CALL_SQLITE (bind_int64 (stmt, 4, ip));
    CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	trap->SendServerCommand(ent-g_entities, "print \"Account created.\n\"");
	Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
}

void Cmd_ACLogout_f( gentity_t *ent ) { //If logged in, print logout msg, remove login status.
	if (Q_stricmp(ent->client->pers.userName, "")) {
		Q_strncpyz(ent->client->pers.userName, "", sizeof(ent->client->pers.userName));
		trap->SendServerCommand(ent-g_entities, "print \"Logged out.\n\"");
	}
	else
		trap->SendServerCommand(ent-g_entities, "print \"You are not logged in!\n\"");
}

void Cmd_Stats_f( gentity_t *ent ) { //Should i bother to cache player stats in memory? id then have to live update them.. but its doable.. worth it though?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16];
	int row = 0, kills = 0, deaths = 0, suicides = 0, captures = 0, returns = 0, lastlogin = 0, playtime = 0, realdeaths, s, highscores = 0, i;
	float kdr, realkdr;
	char buf[MAX_STRING_CHARS-64] = {0};
	char timeStr[64] = {0};
	time_t timeGMT;

	if (trap->Argc() != 2) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /stats <username>\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	Q_strlwr(username);
	Q_CleanStr(username);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT kills, deaths, suicides, captures, returns, lastlogin, playtime FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			kills = sqlite3_column_int(stmt, 0);
			deaths = sqlite3_column_int(stmt, 1);
			suicides = sqlite3_column_int(stmt, 2);
			captures = sqlite3_column_int(stmt, 3);
			returns = sqlite3_column_int(stmt, 4);
			lastlogin = sqlite3_column_int(stmt, 5);
			playtime = sqlite3_column_int(stmt, 6);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//Trap print?
			break;
        }
    }
	CALL_SQLITE (finalize(stmt));

	sql = "SELECT COUNT(*) FROM LocalRun WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));

	s = sqlite3_step(stmt);
	if (s == SQLITE_ROW)
		highscores = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
		return;
	}
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	for (i = 0; i < 256; i++) { //size of UserStats?.. Live update stats feature
		if (!UserStats[i].username || !UserStats[i].username[0])
			break;
		if (!Q_stricmp(UserStats[i].username, username)) { //User found, update their stat totals with recent stuff from memory
			kills += UserStats[i].kills;
			deaths += UserStats[i].deaths;
			suicides += UserStats[i].suicides;
			captures += UserStats[i].captures;
			returns += UserStats[i].returns;
			break;
		}
	}

	if (row == 0) { //no account found, or more than 1 account with same name, problem
		trap->SendServerCommand(ent-g_entities, "print \"Account not found!\n\"");
		return;
	}
	else if (row > 1) {
		trap->SendServerCommand(ent-g_entities, "print \"ERROR: Multiple accounts found!\n\"");
		return;
	}

	realdeaths = deaths - suicides;
	kdr = (float)kills/(float)deaths;
	realkdr = (float)kills/(float)realdeaths;

	if (deaths == 0)
		kdr = kills;
	if (realdeaths == 0)
		realkdr = kills;
	if (kills == 0) {
		kdr = 0;
		realkdr = 0;
	}

	timeGMT = (time_t)lastlogin;
	strftime( timeStr, sizeof( timeStr ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &timeGMT ) );

	Q_strncpyz(buf, va("Stats for %s:\n", username), sizeof(buf));
	Q_strcat(buf, sizeof(buf), va("   ^5Kills / Deaths / Suicides: ^2%i / %i / %i\n", kills, deaths, suicides));
	Q_strcat(buf, sizeof(buf), va("   ^5Captures / Returns^3: ^2%i / %i\n", captures, returns));
	Q_strcat(buf, sizeof(buf), va("   ^5KDR / Real KDR^3: ^2%.2f / %.2f\n", kdr, realkdr));
	Q_strcat(buf, sizeof(buf), va("   ^5Race Highscores: ^2%i\n", highscores));
	Q_strcat(buf, sizeof(buf), va("   ^5Last login: ^2%s\n", timeStr));
	//Q_strcat(buf, sizeof(buf), va("  ^5Playtime / Lastlogin^3: ^2%i / %i\n", playtime, lastlogin);

	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", buf));
}

//Search array list to find players row
//If found, update it
//If not found, add a new row at next empty spot
//A new function will read the array on mapchange, and do the querys updates
void G_AddSimpleStat(char *username, int type) {
	int row;

	if (sv_cheats.integer) //Dont record stats if cheats were enabled
		return;
	for (row = 0; row < 256; row++) { //size of UserStats ?
		if (!UserStats[row].username || !UserStats[row].username[0])
			break;
		if (!Q_stricmp(UserStats[row].username, username)) { //User found, update his stats in memory, is this check right?
			if (type == 1) //Kills
				UserStats[row].kills++;
			else if (type == 2) //Deaths
				UserStats[row].deaths++;
			else if (type == 3) //Suicides
				UserStats[row].suicides++;
			else if (type == 4) //Captures
				UserStats[row].captures++;
			else if (type == 5) //Returns
				UserStats[row].returns++;
			return;
		}
	}
	Q_strncpyz(UserStats[row].username, username, sizeof(UserStats[row].username )); //If we are here it means name not found, so add it
	UserStats[row].kills = UserStats[row].deaths = UserStats[row].suicides = UserStats[row].captures = UserStats[row].returns = 0; //I guess set all their shit to 0
	//Add the one type ..
	if (type == 1) //Kills
		UserStats[row].kills++;
	else if (type == 2) //Deaths
		UserStats[row].deaths++;
	else if (type == 3) //Suicides
		UserStats[row].suicides++;
	else if (type == 4) //Captures
		UserStats[row].captures++;
	else if (type == 5) //Returns
		UserStats[row].returns++;
}


void G_AddSimpleStatsToDB() { //For each item in array.. do an update query?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int row = 0;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "UPDATE LocalAccount SET "
		"kills = kills + ?, deaths = deaths + ?, suicides = suicides + ?, captures = captures + ?, returns = returns + ? "
		"WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	for (row = 0; row < 256; row++) { //size of UserStats ?
		if (!UserStats[row].username || !UserStats[row].username[0])
			break;

		CALL_SQLITE (bind_int (stmt, 1, UserStats[row].kills));
		CALL_SQLITE (bind_int (stmt, 2, UserStats[row].deaths));
		CALL_SQLITE (bind_int (stmt, 3, UserStats[row].suicides));
		CALL_SQLITE (bind_int (stmt, 4, UserStats[row].captures));
		CALL_SQLITE (bind_int (stmt, 5, UserStats[row].returns));
		CALL_SQLITE (bind_text (stmt, 6, UserStats[row].username, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (reset (stmt));
		CALL_SQLITE (clear_bindings (stmt));
	}

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}

void CleanupLocalRun() { //loda fixme, take prepare out of loop even more?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char mapName[40], courseName[40], info[1024] = {0};
	int i, mstyle;

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(mapName, Info_ValueForKey( info, "mapname" ), sizeof(mapName));

	Q_strlwr(mapName);
	Q_CleanStr(mapName);

	trap->Print("Cleaning up racetimes for %s\n", mapName);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "DROP TABLE IF EXISTS TempLocalRun";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE TempLocalRun(id INTEGER PRIMARY KEY, old_id INTEGER, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED SMALLINT, end_time UNSIGNED INT)";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	for (i = 0; i < level.numCourses; i++) { //32 max
		Q_strncpyz(courseName, mapName, sizeof(courseName));
		if (level.courseName[i] && level.courseName[i][0])
			Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));

		sql = "INSERT INTO TempLocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) " //Place 2
			"SELECT LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "
				"FROM (SELECT id, MIN(duration_ms) "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP BY username) " 
				"AS X INNER JOIN LocalRun AS LR ON LR.id = X.id ORDER BY duration_ms LIMIT 50";//memes	
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		
		for (mstyle = 0; mstyle < 7; mstyle++) { //7 movement styles. 0-6
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 2, mstyle));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
		CALL_SQLITE (finalize(stmt));

		sql = "DELETE FROM LocalRun WHERE coursename = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (finalize(stmt));
	}

	sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) "
		"SELECT username, coursename, duration_ms, topspeed, average, style, end_time FROM TempLocalRun";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));
}

void BuildMapHighscores() { //loda fixme, take prepare,query out of loop
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, mstyle;
	char mapName[40], courseName[40], info[1024] = {0}, dateStr[64] = {0};
	time_t timeGMT;

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(mapName, Info_ValueForKey( info, "mapname" ), sizeof(mapName));
	Q_strlwr(mapName);
	Q_CleanStr(mapName);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	for (i = 0; i < level.numCourses; i++) { //32 max
		Q_strncpyz(courseName, mapName, sizeof(courseName));
		if (level.courseName[i][0])
			Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));
		for (mstyle = 0; mstyle < 7; mstyle++) { //7 movement styles. 0-6
			int rank = 0;

			CALL_SQLITE (open (LOCAL_DB_PATH, & db));

			sql = "SELECT LR.id, LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "  //Place 1
				"FROM (SELECT id, MIN(duration_ms) "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP by username) " 
				"AS X INNER JOIN LocalRun AS LR ON LR.id = X.id ORDER BY duration_ms LIMIT 10";

			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 2, mstyle));

			while (1) {
				int s;
				s = sqlite3_step (stmt);
				if (s == SQLITE_ROW) {
					char *username; //loda fixme should this be char[]
					char *course;
					unsigned int duration_ms;
					unsigned short topspeed, average;
					unsigned short style;
					unsigned int end_time;
					int garbage;

					garbage = sqlite3_column_int(stmt, 0); //cn i just delete this, it seemed to throw off the order..??
					username = (char*)sqlite3_column_text(stmt, 1); 
					course = (char*)sqlite3_column_text(stmt, 2); //again, not needed
					duration_ms = sqlite3_column_int(stmt, 3);
					topspeed = sqlite3_column_int(stmt, 4);
					average = sqlite3_column_int(stmt, 5);
					style = sqlite3_column_int(stmt, 6);
					end_time = sqlite3_column_int(stmt, 7);

					Q_strncpyz(HighScores[i][style][rank].username, username, sizeof(HighScores[i][style][rank].username));
					Q_strncpyz(HighScores[i][style][rank].coursename, username, sizeof(HighScores[i][style][rank].coursename));
					HighScores[i][style][rank].duration_ms = duration_ms;
					HighScores[i][style][rank].topspeed = topspeed;
					HighScores[i][style][rank].average = average;
					HighScores[i][style][rank].style = style;

					timeGMT = (time_t)end_time; //Maybe this should be done in the caching itself...but would also have to be done every racetime addition
					strftime( dateStr, sizeof( dateStr ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &timeGMT ) );
					Q_strncpyz(HighScores[i][style][rank].end_time, dateStr, sizeof(HighScores[i][style][rank].end_time));
					rank++;
				}
				else if (s == SQLITE_DONE)
					break;
				else {
					fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
					break;
				}
			}
			CALL_SQLITE (finalize(stmt));
		}
	}
	CALL_SQLITE (close(db));

	if (level.numCourses)
		trap->Print("Highscores built for %s\n", mapName);
}

void IntegerToRaceName(int style, char *styleString) {
	switch(style) {
		case 0: Q_strncpyz(styleString, "siege", sizeof(styleString)); break;
		case 1: Q_strncpyz(styleString, "jka", sizeof(styleString)); break;
		case 2:	Q_strncpyz(styleString, "qw", sizeof(styleString));	break;
		case 3:	Q_strncpyz(styleString, "cpm", sizeof(styleString)); break;
		case 4:	Q_strncpyz(styleString, "vq3", sizeof(styleString)); break;
		case 5:	Q_strncpyz(styleString, "pjk", sizeof(styleString)); break;
		case 6:	Q_strncpyz(styleString, "wsw", sizeof(styleString)); break;
		default: Q_strncpyz(styleString, "ERROR", sizeof(styleString)); break;
	}
}

int RaceNameToInteger(char *style) {
	Q_strlwr(style);
	Q_CleanStr(style);

	if (!Q_stricmp(style, "siege") || !Q_stricmp(style, "0"))
		return 0;
	if (!Q_stricmp(style, "jka") || !Q_stricmp(style, "jk3") || !Q_stricmp(style, "1"))
		return 1;
	if (!Q_stricmp(style, "hl2") || !Q_stricmp(style, "hl1") || !Q_stricmp(style, "hl") || !Q_stricmp(style, "qw") || !Q_stricmp(style, "2"))
		return 2;
	if (!Q_stricmp(style, "cpm") || !Q_stricmp(style, "cpma") || !Q_stricmp(style, "3"))
		return 3;
	if (!Q_stricmp(style, "q3") || !Q_stricmp(style, "vq3") || !Q_stricmp(style, "4"))
		return 4;
	if (!Q_stricmp(style, "pjk") || !Q_stricmp(style, "5"))
		return 5;
	if (!Q_stricmp(style, "wsw") || !Q_stricmp(style, "warsow") || !Q_stricmp(style, "6"))
		return 6;
	return -1;
}

void Cmd_PersonalBest_f(gentity_t *ent) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s, style, duration_ms, time;
	char username[16], courseName[40], courseNameFull[40], styleString[16], durationStr[32];
	time_t timeGMT;

	if (trap->Argc() != 4 && trap->Argc() != 5) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /personalBest <username> <full coursename> <style>.  Example: /personalBest user mapname (coursename) style\n\"");
		return;
	}


	trap->Argv(1, username, sizeof(username));

	if (trap->Argc() == 5) {
		trap->Argv(2, courseNameFull, sizeof(courseNameFull));
		trap->Argv(3, courseName, sizeof(courseName));
		Q_strcat(courseNameFull, sizeof(courseNameFull), va(" %s", courseName));
		trap->Argv(4, styleString, sizeof(styleString));
	}
	else {
		trap->Argv(2, courseNameFull, sizeof(courseNameFull));
		trap->Argv(3, styleString, sizeof(styleString));
	}

	Q_strlwr(styleString);
	Q_CleanStr(styleString);

	style = RaceNameToInteger(styleString);

	if (style < 0) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /personalBest <username> <full coursename> <style>.\n\"");
		return;
	}

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strlwr(courseNameFull);
	Q_CleanStr(courseNameFull);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT MIN(duration_ms) FROM LocalRun WHERE username = ? AND coursename = ? AND style = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, courseNameFull, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, style));

	s = sqlite3_step(stmt);

	if (s == SQLITE_ROW) {
		duration_ms = sqlite3_column_int(stmt, 0);
		//time = sqlite3_column_int(stmt, 0);
	}
	else if (s != SQLITE_DONE) {
		fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
		return;
	}

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	//timeGMT = (time_t)time;
	//strftime( timeStr, sizeof( timeStr ), "[%Y-%m-%d] [%H:%M:%S] ", gmtime( &timeGMT ) );

	if (duration_ms >= 60000) {
		int minutes, seconds, milliseconds;
		minutes = (int)((duration_ms / (1000*60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000; 
		Com_sprintf(durationStr, sizeof(durationStr), "%i:%02i.%03i", minutes, seconds, milliseconds);//more precision?
	}
	else
		Q_strncpyz(durationStr, va("%.3f", ((float)duration_ms * 0.001)), sizeof(durationStr));

	if (duration_ms)
		trap->SendServerCommand( ent-g_entities, va("print \"^5 This players fastest time is ^3%s^5.\n\"", durationStr));
	else
		trap->SendServerCommand(ent-g_entities, "print \"^5 No results found.\n\"");
}

void Cmd_DFTop10_f(gentity_t *ent) {
	int i, style, course = -1;
	char courseName[40], courseNameFull[40], styleString[16], timeStr[32];
	char info[1024] = {0};
	char msg[1024-128] = {0};

	if (level.numCourses == 0) {
		trap->SendServerCommand(ent-g_entities, "print \"This map does not have any courses.\n\"");
		return;
	}

	if (trap->Argc() > 3 || (level.numCourses == 1 && trap->Argc() > 2)) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /dftop10 <course (if needed)> <style (optional)>.  This displays the specified top10 for the current map.\n\"");
		return;
	}

	if (level.numCourses > 1 && trap->Argc() < 2) {
		trap->SendServerCommand(ent-g_entities, "print \"This map has multiple courses, you must specify one of the following with /dftop10 <coursename> <style (optional)>\n\"");
		for (i = 0; i < level.numCourses; i++) { //32 max
			if (level.courseName[i] && level.courseName[i][0])
				trap->SendServerCommand(ent-g_entities, va("print \"  ^5%i ^7- ^3%s\n\"", i, level.courseName[i]));
		}
		return;
	}

	if (level.numCourses == 1 && trap->Argc() == 1) { //dftop10
		style = 1;
		Q_strncpyz(courseName, "", sizeof(courseName));
	}
	else if (level.numCourses == 1 && trap->Argc() == 2) { //dftop10 cpm
		char input[32];
		trap->Argv(1, input, sizeof(input));

		Q_strncpyz(courseName, "", sizeof(courseName));
		//trap->Print("Input: %s, atoi: %i\n", input, atoi(input));
		style = RaceNameToInteger(input);

		if (style < 0) {
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /dftop10 <course (if needed)> <style (optional)>.  This displays the specified top10 for the current map.\n\"");
			return;
		}
	}
	else if (level.numCourses > 1 && trap->Argc() == 2) { //dftop10 dash1
		char input[32];
		trap->Argv(1, input, sizeof(input));
		Q_strncpyz(courseName, input, sizeof(courseName));
		style = 1;
	}
	else if (level.numCourses > 1 && trap->Argc() == 3) { //dftop10 dash1 cpm
		char input1[40], input2[16];
		trap->Argv(1, input1, sizeof(input1));
		trap->Argv(2, input2, sizeof(input2));
		Q_strncpyz(courseName, input1, sizeof(courseName));
		style = RaceNameToInteger(input2);
		if (style < 0) {
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /dftop10 <course (if needed)> <style (optional)>.  This displays the specified top10 for the current map.\n\"");
			return;
		}
	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /dftop10 <course (if needed)> <style (optional)>.  This displays the specified top10 for the current map.\n\"");
		return;
	}

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(courseNameFull, Info_ValueForKey( info, "mapname" ), sizeof(courseNameFull));
	if (courseName[0]) //&& courseName[0]?
		Q_strcat(courseNameFull, sizeof(courseNameFull), va(" (%s)", courseName));

	Q_strlwr(courseName);
	Q_CleanStr(courseName);
	Q_strlwr(courseNameFull);
	Q_CleanStr(courseNameFull);

	for (i = 0; i < level.numCourses; i++) {
		if (!Q_stricmp(courseName, level.courseName[i])) {
			course = i;
			break;
		}
	}

	if (level.numCourses == 1)
		course = 0;

	if (course == -1) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /dftop10 <course (if needed)> <style (optional)>.  This displays the specified top10 for the current map.\n\"");
		return;
	}

	IntegerToRaceName(style, styleString);
	trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s style:\n    ^5Username           Time         Topspeed    Average      Date\n\"", courseNameFull, styleString));

	for (i = 0; i < 10; i++) {
		char *tmpMsg = NULL;
		if (HighScores[course][style][i].username && HighScores[course][style][i].username[0])
		{
			if (HighScores[course][style][i].duration_ms >= 60000) {
				int minutes, seconds, milliseconds;
				minutes = (int)((HighScores[course][style][i].duration_ms / (1000*60)) % 60);
				seconds = (int)(HighScores[course][style][i].duration_ms / 1000) % 60;
				milliseconds = HighScores[course][style][i].duration_ms % 1000; 
				Com_sprintf(timeStr, sizeof(timeStr), "%i:%02i.%03i", minutes, seconds, milliseconds);//more precision?
			}
			else
				Q_strncpyz(timeStr, va("%.3f", ((float)HighScores[course][style][i].duration_ms * 0.001)), sizeof(timeStr));

			tmpMsg = va("^5%2i^3: ^3%-18s ^3%-12s ^3%-11i ^3%-12i %s\n", i + 1, HighScores[course][style][i].username, timeStr, HighScores[course][style][i].topspeed, HighScores[course][style][i].average, HighScores[course][style][i].end_time);
			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}
	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

void Cmd_DFRefresh_f(gentity_t *ent) {
	if (ent->r.svFlags & SVF_FULLADMIN) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << A_BUILDHIGHSCORES))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (dfRefresh).\n\"" );
			return;
		}
	}
	else if (ent->r.svFlags & SVF_JUNIORADMIN) {//Logged in as junior admin
		if (!(g_juniorAdminLevel.integer & (1 << A_BUILDHIGHSCORES))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (dfRefresh).\n\"" );
			return;
		}
	}
	else {//Not logged in
		trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (dfRefresh).\n\"" );
		return;
	}
	G_AddToDBFromFile(); //From file to db
	BuildMapHighscores(); //From db, built to memory
}

void Cmd_ACWhois_f( gentity_t *ent ) { //Should this only show logged in people..?
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;
	qboolean	admin = qfalse;
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	if (ent->r.svFlags & SVF_FULLADMIN) {//Logged in as full admin
		if (g_fullAdminLevel.integer & (1 << A_WHOIS))
			admin = qtrue;
	}
	else if (ent->r.svFlags & SVF_JUNIORADMIN) {//Logged in as junior admin
		if (g_juniorAdminLevel.integer & (1 << A_WHOIS))
			admin = qtrue;
	}

	if (admin) {
		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "SELECT username FROM LocalAccount WHERE lastip = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	}

	trap->SendServerCommand(ent-g_entities, "print \"^5   Username          Nickname\n\"");

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clients
		char *tmpMsg = NULL;
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (cl->pers.netname[0]) { // && cl->pers.userName[0] ?
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strUser[16] = {0};

			Q_strncpyz(strNum, va("^5%2i^3:", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));
			Q_strncpyz( strUser, cl->pers.userName, sizeof(strUser));	

			if (admin && !strUser[0]) { //No username means not logged in, so check if they have an account tied to their ip
				char strIP[NET_ADDRSTRMAXLEN] = {0};
				char *p = NULL;
				unsigned int ip;

				Q_strncpyz(strIP, cl->sess.IP, sizeof(strIP));
				p = strchr(strIP, ':');
				if (p) //loda - fix ip sometimes not printing
					*p = 0;
				ip = ip_to_int(strIP);

				CALL_SQLITE (bind_int64 (stmt, 1, ip));

				s = sqlite3_step(stmt);

				if (s == SQLITE_ROW) {
					if (ip)
						Q_strncpyz(strUser, (char*)sqlite3_column_text(stmt, 0), sizeof(strUser));
				}
				else if (s != SQLITE_DONE) {
					fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
					CALL_SQLITE (reset (stmt));
					CALL_SQLITE (clear_bindings (stmt));
					break;
				}

				CALL_SQLITE (reset (stmt));
				CALL_SQLITE (clear_bindings (stmt));

				tmpMsg = va("%-2s ^3%-18s^7%s\n", strNum, strUser, strName);
			}
			else
				tmpMsg = va("%-2s ^7%-18s^7%s\n", strNum, strUser, strName);
			
			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}

	if (admin) {
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}

	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

void InitGameAccountStuff( void ) { //Called every mapload
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "CREATE TABLE IF NOT EXISTS LocalAccount(id INTEGER PRIMARY KEY, username VARCHAR(16), password VARCHAR(16), kills UNSIGNED SMALLINT, deaths UNSIGNED SMALLINT, "
		"suicides UNSIGNED SMALLINT, captures UNSIGNED SMALLINT, returns UNSIGNED SMALLINT, lastlogin UNSIGNED INT, playtime UNSIGNED INTEGER, lastip UNSIGNED INTEGER)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS LocalRun(id INTEGER PRIMARY KEY, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED SMALLINT, end_time UNSIGNED INT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS LocalDuel(id INTEGER PRIMARY KEY, winner VARCHAR(16), loser VARCHAR(16), duration UNSIGNED SMALLINT, "
		"type UNSIGNED TINYINT, winner_hp UNSIGNED TINYINT, winner_shield UNSIGNED TINYINT, end_time UNSIGNED INT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	CleanupLocalRun(); //Deletes useless shit from LocalRun database table
	G_AddToDBFromFile(); //Add last maps highscores
	BuildMapHighscores();//Build highscores into memory from database

	CALL_SQLITE (close(db));
}