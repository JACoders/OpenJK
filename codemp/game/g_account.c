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
	unsigned int		end_time;
} RaceRecord_t;

RaceRecord_t	HighScores[10*7*32];//top10, 7 styles, 32 max courses per map

typedef struct UserAccount_s {
	char			username[16];
	char			password[16];
	int				lastIP;
} UserAccount_t;

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

void G_AddDuelToDB(char *winner, char *loser, int duration, int type, int winner_hp, int winner_shield) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	time_t	rawtime;

	if (CheckUserExists(winner) && CheckUserExists(loser)) {
		time( &rawtime );
		localtime( &rawtime );

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "INSERT INTO LocalDuel(player1, player2, end_time, duration, type, winner_hp, winner_shield) VALUES (?, ?, ?, ?, ?, ?, ?)";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, winner, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, loser, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, rawtime));
		CALL_SQLITE (bind_int (stmt, 4, duration));
		CALL_SQLITE (bind_int (stmt, 5, type));
		CALL_SQLITE (bind_int (stmt, 6, winner_hp));
		CALL_SQLITE (bind_int (stmt, 7, winner_shield));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
	}
}

void G_AdToDBFromFile(void) //loda fixme
{
	fileHandle_t f;	
	int		fLen = 0, MAX_FILESIZE = 4096, args = 1;
	char	info[1024] = {0}, buf[4096] = {0}, empty[8] = {0};//eh
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
	//if (fLen >= MAX_FILESIZE) {
		//trap->FS_Close(f);
		//Com_Printf ("ERROR: Couldn't load defrag data from %s, file is too large\n", TEMP_RACE_LOG);
		//return;
	//}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);
	Com_Printf ("Loaded defrag data from %s\n", TEMP_RACE_LOG);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";	
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	//Todo: make TempRaceRecord an array of structs instead, maybe like 32 long idk, and build a query to insert 32 at a time or something.. instead of 1 by 1
	pch = strtok (buf,":\n");
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
			TempRaceRecord.end_time = atoi(pch);
			CALL_SQLITE (bind_text (stmt, 1, TempRaceRecord.username, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_text (stmt, 2, TempRaceRecord.coursename, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 3, TempRaceRecord.duration_ms));
			CALL_SQLITE (bind_int (stmt, 4, TempRaceRecord.topspeed));
			CALL_SQLITE (bind_int (stmt, 5, TempRaceRecord.average));
			CALL_SQLITE (bind_int (stmt, 6, TempRaceRecord.style));
			CALL_SQLITE (bind_int (stmt, 7, TempRaceRecord.end_time));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
    		pch = strtok (NULL, ":\n");
		args++;
	}
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));	

	trap->FS_Open(TEMP_RACE_LOG, &f, FS_WRITE);
	trap->FS_Write( empty, strlen( empty ), level.tempRaceLog );
	trap->FS_Close(f);
}

void G_AddRunToTempFile(char *username, char *courseName, int duration_ms, int style, int topspeed, int average) {//should be short.. but have to change elsewhere? is it worth it?
	time_t	rawtime;

	time( &rawtime );
	localtime( &rawtime );

	if (!CheckUserExists(username))
		return;

	G_TempRaceLogPrintf("%s:%s:%i:%i:%i:%i:%i\n", username, courseName, duration_ms, topspeed, average, style, rawtime);

	//Now for live highscore stuff.
	//Find worst time in category of highscores
	//Check if our time is better
		//if yes, delete worst time and resort?
		//if no, exit

	//what about if the time is better than the worst, but if the user already has a better time in the highscore?
		//do nothing then and exit

	//kinda low priority since we can just rebuild the entire highscore with little performance hit.

#if 0
		sql = "SELECT MAX(duration_ms), id, COUNT(*) FROM Highscores WHERE coursename = ? AND username = ? AND style = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style));
		

		s = sqlite3_step(stmt);
		if (s == SQLITE_ROW) {
			worsttime = sqlite3_column_int (stmt, 0);
			id = sqlite3_column_int (stmt, 1);
			count = sqlite3_column_int (stmt, 2);
			trap->Print("worst time, id, count, coursename, style, username: %i, %i, %i, %s, %i, %s\n", worsttime, id, count, courseName, style, username);
		}
		else if (s != SQLITE_DONE) {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}

		trap->Print("worsttime, time: %i, %i\n", worsttime, itime);

		CALL_SQLITE (finalize(stmt));

		
		if (duration < worsttime) { //gay
			sql = "DELETE FROM Highscores WHERE id = ?";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, id));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (finalize(stmt));

			sql = "INSERT INTO Highscores (username, coursename, style, topspeed, average, duration_ms, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_text (stmt, 2, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 3, style));
			CALL_SQLITE (bind_int (stmt, 4, topspeed));
			CALL_SQLITE (bind_int (stmt, 5, average));
			CALL_SQLITE (bind_int (stmt, 6, itime));
			CALL_SQLITE (bind_int (stmt, 7, datetime));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (finalize(stmt));
		}
		
#endif
}

//So the best way is to probably add every run as soon as its taken and not filter them.
//to cut down on database size, there should be a cleanup on every mapload or.. every week..or...?
//which removes any time not in the top 100 of its category AND more than 1 week old?

void Cmd_ACLogin_f( gentity_t *ent ) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0, i = 0;
	//char *username;
	//char *password;
	char username[16], enteredPassword[16], password[16];

	if (trap->Argc() != 3) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /login <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap->Print("das it mane0\n");

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, enteredPassword, sizeof(password));

	if (!CheckUserExists(username)) {
		trap->SendServerCommand(ent-g_entities, "print \"Account not found! To make a new account, use the /register command.\n\"");
		return;
	}

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT password FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
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

	if (enteredPassword && password && enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		char *p = NULL;
		char strIP[NET_ADDRSTRMAXLEN] = {0};

		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
		trap->SendServerCommand(ent-g_entities, "print \"Login sucessful.\n\"");

		Q_strncpyz(strIP, ent->client->sess.IP, sizeof(strIP));
		p = strchr(strIP, ':');
		if (p) //loda - fix ip sometimes not printing
			*p = 0;

		sql = "UPDATE LocalAccount SET lastip = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int64 (stmt, 1, ip_to_int(strIP)));
		CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
		CALL_SQLITE_EXPECT (step (stmt), DONE);
		CALL_SQLITE (finalize(stmt));
	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect password!\n\"");
	}	
	CALL_SQLITE (close(db));
}

void Cmd_ACRegister_f( gentity_t *ent ) { //Temporary, until global shit is done
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i = 0, row = 0;
	char username[16], password[16], strIP[NET_ADDRSTRMAXLEN] = {0};
	char *p = NULL;
	time_t	rawtime;

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

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "INSERT INTO LocalAccount (username, password, kills, deaths, captures, returns, playtime, lastlogin, lastip) VALUES (?, ?, 0, 0, 0, 0, 0, ?, ?)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, password, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, rawtime));
	CALL_SQLITE (bind_int64 (stmt, 4, ip_to_int(strIP)));
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

void BuildMapHighscores() {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, row = 0, mstyle;
	char mapName[40], courseName[40], info[1024] = {0};

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(mapName, Info_ValueForKey( info, "mapname" ), sizeof(mapName));

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	for (i = 0; i < level.numCourses; i++) { //32 max
		for (mstyle = 0; mstyle < 7; mstyle++) { //7 movement styles. 0-6
			Q_strncpyz(courseName, mapName, sizeof(courseName));
			Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));

			CALL_SQLITE (open (LOCAL_DB_PATH, & db));
			//sql = "SELECT LocalAccount.username, LocalRun.coursename, MIN(LocalRun.duration_ms), LocalRun.topspeed, LocalRun.average, LocalRun.style, LocalRun.end_time FROM LocalRun, LocalAccount "
			//	"WHERE LocalAccount.id = LocalRun.user_id AND LocalRun.coursename = ? AND LocalRun.style = ? GROUP BY user_id, coursename, style ORDER BY MIN(duration_ms) ASC LIMIT 10";

			sql = "SELECT LocalRun.username, LocalRun.coursename, MIN(LocalRun.duration_ms), LocalRun.topspeed, LocalRun.average, LocalRun.style, LocalRun.end_time FROM LocalRun "
				"WHERE LocalRun.coursename = ? AND LocalRun.style = ? GROUP BY username, coursename, style ORDER BY MIN(duration_ms) ASC LIMIT 10";
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

					username = (char*)sqlite3_column_text(stmt, 0);
					course = (char*)sqlite3_column_text(stmt, 1);
					duration_ms = sqlite3_column_int(stmt, 2);
					topspeed = sqlite3_column_int(stmt, 3);
					average = sqlite3_column_int(stmt, 4);
					style = sqlite3_column_int(stmt, 5);
					end_time = sqlite3_column_int(stmt, 6);

					Q_strncpyz(level.Highscores[row].username, username, sizeof(level.Highscores[0].username));
					Q_strncpyz(level.Highscores[row].coursename, course, sizeof(level.Highscores[0].coursename));
					level.Highscores[row].duration_ms = duration_ms;
					level.Highscores[row].topspeed = topspeed;
					level.Highscores[row].average = average;
					level.Highscores[row].style = style;
					level.Highscores[row].end_time = end_time;

					row++;
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
	char cleanedStyle[16];
	int i = 0;

	Q_strncpyz(cleanedStyle, style, sizeof(cleanedStyle));

	/*while (style[i]) {
		cleanedStyle[i] = tolower(style[i]);
		i++;
	}*/
	Q_CleanStr(cleanedStyle);//also strip other chars..?

	if (!Q_stricmp(cleanedStyle, "siege") || !Q_stricmp(cleanedStyle, "0"))
		return 0;
	if (!Q_stricmp(cleanedStyle, "jka") || !Q_stricmp(cleanedStyle, "jk3") || !Q_stricmp(style, "1"))
		return 1;
	if (!Q_stricmp(cleanedStyle, "hl2") || !Q_stricmp(cleanedStyle, "hl1") || !Q_stricmp(cleanedStyle, "hl") || !Q_stricmp(cleanedStyle, "qw") || !Q_stricmp(cleanedStyle, "2"))
		return 2;
	if (!Q_stricmp(cleanedStyle, "cpm") || !Q_stricmp(cleanedStyle, "cpma") || !Q_stricmp(cleanedStyle, "3"))
		return 3;
	if (!Q_stricmp(cleanedStyle, "q3") || !Q_stricmp(cleanedStyle, "vq3") || !Q_stricmp(cleanedStyle, "4"))
		return 4;
	if (!Q_stricmp(cleanedStyle, "pjk") || !Q_stricmp(cleanedStyle, "5"))
		return 5;
	if (!Q_stricmp(cleanedStyle, "wsw") || !Q_stricmp(cleanedStyle, "warsow") || !Q_stricmp(cleanedStyle, "6"))
		return 6;
	return -1;
}

void Cmd_DFTop10_f(gentity_t *ent) {
	int i, style, rank = 1;
	char courseName[40], courseNameFull[40], styleString[16], timeStr[32];
	char info[1024] = {0};

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

	if (level.numCourses == 1 && trap->Argc() < 2) { //dftop10
		style = 1;
	}
	else if (level.numCourses == 1 && trap->Argc() == 2) { //dftop10 cpm
		char input[32];
		trap->Argv(1, input, sizeof(input));
		if (atoi(input) >= 0 && atoi(input) < 7) {
			style = atoi(input);
		}
		else {
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
	if (courseName && courseName[0]) //&& courseName[0]?
		Q_strcat(courseNameFull, sizeof(courseNameFull), va(" (%s)", courseName));
	IntegerToRaceName(style, styleString);
	trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s style:\n\"", courseName, styleString));
	trap->SendServerCommand(ent-g_entities, "print \"   ^5Username           Time         Topspeed    Average\n\"");

	for (i = 0; i < (10 * level.numCourses * 7); i++) {
		if (level.Highscores[i].username) {
			if ((!Q_stricmp(level.Highscores[i].coursename, courseNameFull)) && (level.Highscores[i].style == style)) {
				if (level.Highscores[i].duration_ms >= 60000) {
					int minutes, seconds, milliseconds;
					minutes = (int)((level.Highscores[i].duration_ms / (1000*60)) % 60);
					seconds = (int)(level.Highscores[i].duration_ms / 1000) % 60;
					milliseconds = level.Highscores[i].duration_ms % 1000; 
					Com_sprintf(timeStr, sizeof(timeStr), "%i:%02i.%i", minutes, seconds, milliseconds);
				}
				else
					Q_strncpyz(timeStr, va("%.3f", ((float)level.Highscores[i].duration_ms * 0.001)), sizeof(timeStr));
				trap->SendServerCommand(ent-g_entities, va("print \"^5%i^3: ^3%-18s ^3%-12s ^3%-11i ^3%i\n\"", rank, level.Highscores[i].username, timeStr, level.Highscores[i].topspeed, level.Highscores[i].average));
				rank++;
			}
		}
		else break;
		if (rank > 10) //just in case
			break;
	}
}

void Cmd_DFBuildTop10_f(gentity_t *ent) {
	if (ent->r.svFlags & SVF_FULLADMIN) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << A_BUILDHIGHSCORES))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (dfBuildTop10).\n\"" );
			return;
		}
	}
	else if (ent->r.svFlags & SVF_JUNIORADMIN) {//Logged in as junior admin
		if (!(g_juniorAdminLevel.integer & (1 << A_BUILDHIGHSCORES))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (dfBuildTop10).\n\"" );
			return;
		}
	}
	else {//Not logged in
		trap->SendServerCommand( ent-g_entities, "print \"You must be logged in to use this command (dfBuildTop10).\n\"" );
		return;
	}
	G_AdToDBFromFile(); //From file to db
	BuildMapHighscores(); //From db, built to memory
}

void Cmd_ACWhois_f( gentity_t *ent ) {
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;

	trap->SendServerCommand(ent-g_entities, "print \"^5   Nickname                               Username\n\"");

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clients
		char *tmpMsg = NULL;
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (cl->pers.netname[0]) {
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strUser[16] = {0};

			Q_strncpyz(strNum, va("^5%i^3:^7", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));
			Q_strncpyz( strUser, cl->pers.userName, sizeof(strUser));	
			tmpMsg = va("%-2s %-39s^7%s\n", strNum, strName, strUser);

			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}
	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

void InitGameAccountStuff( void ) { //Called every mapload
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;

	// initialize all highscores for this map
	memset(&HighScores, 0, sizeof(HighScores));
	level.Highscores = HighScores;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "CREATE TABLE IF NOT EXISTS LocalAccount(id UNSIGNED SMALLINT PRIMARY KEY, username VARCHAR(16), password VARCHAR(16), kills UNSIGNED SMALLINT, deaths UNSIGNED SMALLINT, "
		"captures UNSIGNED SMALLINT, returns UNSIGNED SMALLINT, lastlogin UNSIGNED INT, playtime UNSIGNED INTEGER, lastip UNSIGNED INTEGER)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS LocalRun(id UNSIGNED SMALLINT PRIMARY KEY, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED SMALLINT, end_time UNSIGNED INT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS LocalDuel(id UNSIGNED SMALLINT PRIMARY KEY, player1 VARCHAR(16), player2 VARCHAR(16), end_time UNSIGNED INT, duration UNSIGNED SMALLINT, "
		"type UNSIGNED SMALLINT, winner_hp UNSIGNED SMALLINT, winner_shield UNSIGNED SMALLINT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));


	G_AdToDBFromFile(); //Add last maps highscores

	/*
	sql = "DROP TABLE IF EXISTS Highscores"; //Remake highscores for map every mapload
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS Highscores(id UNSIGNED SMALLINT PRIMARY KEY, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED SMALLINT, end_time UNSIGNED INT)";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	*/

	//Style should be TINYINT, same with shields, HP, unsigned

	BuildMapHighscores();//Build highscores into memory from database




	//delete last maps highscores file?

	CALL_SQLITE (close(db));
	//Check if stuff needs to be sent to global db? idk
}

#if 0
void Cmd_ACLogin_f( gentity_t *ent ) {
	//Client inputs account name, server querys user database on website, checks last known IP for that username, if match, client is logged in.
	//This will require client to revisit the master website if their IP changes (and be logged in there).
	//But it stops client from having to send password over JKA, and lets global accounts work nice

	//Connect to master acounts db, check lastKnownIP of username, compare to ent->client->ip, if match, log player in.
	//If not, print message and quit (no account found, invalid pass, etc).

	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0;
	char accountName[16] = {0};
	int i = 0;
	qboolean newAccount;

	if (trap->Argc() != 2) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /login <username>\n\"");
		return;
	}

	//if (ent->client->pers.accountName) { //check cuz its a string if [0] = \0 or w/e
	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	while (ent->client->pers.userName[i]) {
		accountName[i] = tolower(ent->client->pers.userName[i]);
		i++;
	}
	Q_CleanStr(accountName);//also strip other chars..?

	//set accountName equal to account name on global database

	//Query GLOBAL database with this: select lastKnownIP and user_name from users where user_name == trap->arg2, 
	//If that ip matches, ent->client->pers.accountName = user_name
	//else account_id = "", print that 

	//To query the global database, i guess we could use libcurl to open a webpage, and login, make sure we are a whitelisted IP, and have the webpage do the sql query

	//To log someone in, open the page mysite.com/gameserverlogin.php and send username with POST? (since we are probably going to add passwords later)
	//If the page says "account not found" print the error to user and return.
	//If the page returns an IP address, compare it to ent->clients ip address, and if it matches log them in.

	//mysite.com/gameserverlogin.php
		//makes sure querying server address is whitelisted
		//check against sql injection
		//checks to see if ACCOUNTNAME is a valid account, if not, return a webpage with "account not found"
		//for v2, check if PASSWORD matches
		//if the account exists, return the last known IP for that account


	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = va("SELECT COUNT(*) FROM LocalAccount where username=%s"), accountName;
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, &stmt, NULL));

	while (1) {
		int s;
		s = sqlite3_step (stmt);
		if (s == SQLITE_ROW) {
			int bytes;
			const unsigned char *text;
			bytes = sqlite3_column_bytes(stmt, 0);
			text = sqlite3_column_text (stmt, 0);
			trap->Print("%d: %s\n", row, text);
			if (atoi((const char *)text) == 0) //woof
				newAccount = qtrue;
			row++;
		}
		else if (s == SQLITE_DONE)
			break;
		else {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			break;
		}
    }

	if (row != 1) { //Count only returns one row, double check
		trap->Print("ERROR: SQL Select Failed.\n");
		return;
	}
	
	if (newAccount) {//Found no results, make the account

		trap->SendServerCommand(ent-g_entities, "print \"Account not found! To make a new account, use the /register command.\n\"");

		
		//sql = va("INSERT INTO LocalAccount (accountname) VALUES (%s)", accountName);
		//CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));//what does this do, send the insert query?
		//CALL_SQLITE (bind_text (stmt, 1, "testtesttest", 6, SQLITE_STATIC));//what does this do?
		//CALL_SQLITE_EXPECT (step (stmt), DONE); //what does this do?
		
	}


	Q_strncpyz(ent->client->pers.userName, accountName, sizeof(ent->client->pers.userName));
}
#endif

#if 0
void Cmd_DFTop10_f( gentity_t *ent ) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, style, row = 0, rank = 1;
	char courseName[40], courseNameFull[40], styleString[16], timeStr[32];
	char info[1024] = {0};

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

	if (level.numCourses == 1 && trap->Argc() < 2) { //dftop10
		style = 1;
	}
	else if (level.numCourses == 1 && trap->Argc() == 2) { //dftop10 cpm
		char input[32];
		trap->Argv(1, input, sizeof(input));
		if (atoi(input) >= 0 && atoi(input) < 7) {
			style = atoi(input);
		}
		else {
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
	if (courseName && courseName[0]) //&& courseName[0]?
		Q_strcat(courseNameFull, sizeof(courseNameFull), va(" (%s)", courseName));

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT username, topspeed, average, duration_ms, end_time FROM Highscores WHERE coursename = ? AND style = ? LIMIT 11";//Limit 11 just in case... should only be max 10 anyway tho
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, courseNameFull, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 2, style));

	IntegerToRaceName(style, styleString);
	trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s style:\n\"", courseName, styleString));
	trap->SendServerCommand(ent-g_entities, "print \"   ^5username           time         topspeed    average\n\"");
	
	while (1) {
		int s;
		s = sqlite3_step (stmt);
		if (s == SQLITE_ROW) {
			const unsigned char *username;
			int topspeed, average, time;
			username = sqlite3_column_text (stmt, 0);
			topspeed = sqlite3_column_int(stmt, 1);
			average = sqlite3_column_int(stmt, 2);
			time = sqlite3_column_int(stmt, 3);
			//datetime = ?
			if (username) {
				if (time >= 60000) {
					int minutes, seconds, milliseconds;
					minutes = (int)(time / 1000) % 60;
					seconds = (int)((time / (1000*60)) % 60);
					milliseconds = time % 1000; //milliseconds = fmodf(time, milliseconds);
					Com_sprintf(timeStr, sizeof(timeStr), "%i:%02i.%i", minutes, seconds, milliseconds);
				}
				else
					Q_strncpyz(timeStr, va("%.3f", ((float)time * 0.001)), sizeof(timeStr));
				trap->SendServerCommand(ent-g_entities, va("print \"%i) ^3%-18s ^3%-12s ^3%-11i ^3%i\n\"", rank, username, timeStr, topspeed, average));
				rank++;
			}
			//row++;
		}
		else if (s == SQLITE_DONE)
			break;
		else {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			break;
		}
	}

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	//This is a server specific top 10, not global.

	//if more than 1 course per map, require usage "/dftop10 <coursename>", how to find this out?  or do it based on how many courses there are for this map in the db?
	//else display map top10

	//dftop10 <mapname> <coursename> <style> is full syntax... how deal with partial entries?

	//At mapload, get 10thfastest time for that map
	//Every time someone completes course, check if it was faster than 10thfastest
	//If so, rebuild top10 list?  And redefine 10thfastest?
}
#endif

#if 0
static void DFBuildTop10() { //Build a highscores of only the fast runs, so we can use that with better performance
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, style, row = 0;
	char mapName[40], courseName[40], info[1024] = {0};

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(mapName, Info_ValueForKey( info, "mapname" ), sizeof(mapName));

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	for (i = 0; i < level.numCourses; i++) { //32 max
		for (style = 0; style < 7; style++) { //7 movement styles. 0-6
			Q_strncpyz(courseName, mapName, sizeof(courseName));
			Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));

			sql = "INSERT INTO Highscores (username, coursename, duration_ms, topspeed, average, style, end_time) "
				"SELECT LocalAccount.username, LocalRun.coursename, MIN(LocalRun.duration_ms), LocalRun.topspeed, LocalRun.average, LocalRun.style, LocalRun.end_time FROM LocalRun, LocalAccount "
				"WHERE LocalAccount.id = LocalRun.user_id AND LocalRun.coursename = ? AND LocalRun.style = ? GROUP BY user_id, coursename, style ORDER BY MIN(duration_ms) ASC LIMIT 10";

			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 2, style));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (finalize(stmt));
		}
	}
	CALL_SQLITE (close(db));
}
#endif

#if 0
void G_AddRunToDB(char *username, char *courseName, float duration, int style, int topspeed, int average) {//should be short.. but have to change elsewhere? is it worth it?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int row = 0, worsttime, id, count, itime = (int)(duration*1000);

	//Get 10th place run (aka slowest) for current course/style from highscores database
	//If we are faster, delete 10th place run.  Insert this one.


	int datetime = 0, user_id;//yeah use rawtime or something?

	user_id = CheckUserExists(username);

	
	trap->Print("itime, ftime: %i, %f\n", itime, duration);

	if (user_id >= 0) { //he exists
		int s;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "INSERT INTO LocalRun(user_id, courseName, duration_ms, style, topspeed, average, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, user_id));
		CALL_SQLITE (bind_text (stmt, 2, courseName, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, itime));
		CALL_SQLITE (bind_int (stmt, 4, style));
		CALL_SQLITE (bind_int (stmt, 5, topspeed));
		CALL_SQLITE (bind_int (stmt, 6, average));
		CALL_SQLITE (bind_int (stmt, 7, datetime));
		CALL_SQLITE_EXPECT (step (stmt), DONE);

		//Now check if it would work as a highscore

		//Highscore logic:
		//If we dont have a faster time, and its fast enough, add it.

		sql = "SELECT MAX(duration_ms), id, COUNT(*) FROM Highscores WHERE coursename = ? AND username = ? AND style = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style));

		s = sqlite3_step(stmt);
		if (s == SQLITE_ROW) {
			worsttime = sqlite3_column_int (stmt, 0);
			id = sqlite3_column_int (stmt, 1);
			count = sqlite3_column_int (stmt, 2);
			trap->Print("worst time, id, count, coursename, style, username: %i, %i, %i, %s, %i, %s\n", worsttime, id, count, courseName, style, username);
		}
		else if (s != SQLITE_DONE) {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}

		trap->Print("worsttime, time: %i, %i\n", worsttime, itime);

		CALL_SQLITE (finalize(stmt));

		
		if (duration < worsttime) { //gay
			sql = "DELETE FROM Highscores WHERE id = ?";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, id));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (finalize(stmt));

			sql = "INSERT INTO Highscores (username, coursename, style, topspeed, average, duration_ms, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_text (stmt, 2, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 3, style));
			CALL_SQLITE (bind_int (stmt, 4, topspeed));
			CALL_SQLITE (bind_int (stmt, 5, average));
			CALL_SQLITE (bind_int (stmt, 6, itime));
			CALL_SQLITE (bind_int (stmt, 7, datetime));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (finalize(stmt));
		}
		

	}
	CALL_SQLITE (close(db));
}
#endif


#if 0
//No register function, thats done on master website.
//ent->client->accountID ? if 0 , not logged in ?, use this in interaction with db?

//When user logs in, if no PlayerServerAccount is created for them yet, create it.  If it already exists, just update it.

//In the v2 update, maybe also add ingame passwords, so people sharing a net connection (at university etc) wont be able to login to eachothers accounts ingame.

/*
"LocalRun" Entity
+------------+-------------+------+-----+---------+-------+
| Field		 | Type        | Null | Key | Default | Extra |
+------------+-------------+------+-----+---------+-------+
| id         | int         | YES  |     | NULL    |       | 
| user_id	 | int		   | YES  |     | NULL    |       |
| coursename | varchar(40) | YES  |     | NULL    |       | //Mapname + coursename, or just mapname if only 1 course per map
| duration_ms| int		   | YES  |     | NULL    |       | //24 bits would allow 4+ hours of time to be recorded, should be plenty? or maybe go to 26bits for people with autism.
| style      | int         | YES  |     | NULL    |       | //max value atm is 6, future proof to 16?
| topspeed   | int         | YES  |     | NULL    |       | 
| average    | int         | YES  |     | NULL    |       | 
| end_time	 | datetime    | YES  |     | NULL    |       | 
+------------+-------------+------+-----+---------+-------+

"LocalAccount" Entity, account of player specific to this server (though he logs in using IP matching tied to his website account)
+------------+-------------+------+-----+---------+-------+
| Field		 | Type        | Null | Key | Default | Extra |
+------------+-------------+------+-----+---------+-------+
| username   | varchar(16) | YES  |     | NULL    |       |
| id		 | int		   | YES  |     | NULL    |       |
//The following (rest of fields in gameaccount, and the Duel entity) will be in v2 update, after defrag functionality is complete
//Simple stuff that does not need to be described or have its own entity
| playtime	 | int		   | YES  |     | NULL    |       |
| kills      | int         | YES  |     | NULL    |       |
| deaths     | int         | YES  |     | NULL    |       |
| captures   | int         | YES  |     | NULL    |       |
| returns    | int         | YES  |     | NULL    |       | //I guess all these could be 16bits or something really
+------------+-------------+------+-----+---------+-------+

"LocalDuel" Entity, can we just user winner_id, loser_id, and ignore ties? or do we have to do id1, id2, and make 'outcome' to store wether id1>id2, id2>id1, or tie?
+---------------+---------=----+------+-----+---------+-------+
| Field		    | Type         | Null | Key | Default | Extra |
+---------------+--------------+------+-----+---------+-------+
| player1_id	| int		   | YES  |     | NULL    |       |
| player2_id	| int		   | YES  |     | NULL    |       |
| outcome		| int		   | YES  |     | NULL    |       | //2 bits, 0 = draw, 1 = player1 wins, 2 = player2 wins.  cuz ties in duels are a thing so whatever
| end_time		| datetime     | YES  |     | NULL    |       |
| duration		| int          | YES  |     | NULL    |       | //in ms or sec?
| type			| int          | YES  |     | NULL    |       | //What weapon was used, (melee, saber, pistol, etc.. "fullforce duel",...
| winner_hp		| int          | YES  |     | NULL    |       | //7bits
| winner_shield | int          | YES  |     | NULL    |       | //7bits
+----------------+-------------+------+-----+---------+-------+
*/
#endif