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

void G_AddDuel(char *winner, char *loser, int duration, int type, int winner_hp, int winner_shield) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	time_t	rawtime;
	char	string[1024] = {0};

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", winner, loser, duration, type, winner_hp, winner_shield, rawtime);
	if (level.duelLog)
		trap->FS_Write(string, strlen(string), level.duelLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	//Might want to make this log to file, and have that sent to db on map change.  But whatever.. duel finishes are not as frequent as race course finishes usually.

	if (CheckUserExists(winner) && CheckUserExists(loser)) {
		time( &rawtime );
		localtime( &rawtime );

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "INSERT INTO LocalDuel(player1, player2, end_time, duration, type, winner_hp, winner_shield) VALUES (?, ?, ?, ?, ?, ?, ?)";
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
	Com_Printf ("Loaded previous maps racetimes from %s\n", TEMP_RACE_LOG);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";	
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
    		pch = strtok (NULL, ";\n");
		args++;
	}
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));	

	trap->FS_Open(TEMP_RACE_LOG, &f, FS_WRITE);
	trap->FS_Write( empty, strlen( empty ), level.tempRaceLog );
	trap->FS_Close(f);
}

void G_AddRaceTime(char *username, char *coursename, int duration_ms, int style, int topspeed, int average) {//should be short.. but have to change elsewhere? is it worth it?
	time_t	rawtime;
	char		string[1024] = {0};

	time( &rawtime );
	localtime( &rawtime );

	if (!CheckUserExists(username))
		return;

	Q_strlwr(coursename);
	Q_CleanStr(coursename);

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", username, coursename, duration_ms, topspeed, average, style, rawtime);

	if (level.raceLog)
		trap->FS_Write(string, strlen(string), level.raceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.
	if (level.tempRaceLog)
		trap->FS_Write(string, strlen(string), level.tempRaceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	//Now for live highscore stuff:

	//Search memory: coursename = ?, style = ?, username = ? , get fastest time
		//If our time is faster than that, proceed to check top10?
			//Uhh...
		//If not, exit.

	//kinda low priority since we can just rebuild the entire highscore with little performance hit.
}



//So the best way is to probably add every run as soon as its taken and not filter them.
//to cut down on database size, there should be a cleanup on every mapload or.. every week..or...?
//which removes any time not in the top 100 of its category AND more than 1 week old?

void Cmd_ACLogin_f( gentity_t *ent ) { //loda fixme show lastip ? or use lastip somehow
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0, i = 0;
	char username[16], enteredPassword[16], password[16];

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
		time_t	rawtime;

		time( &rawtime );
		localtime( &rawtime );


		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
		trap->SendServerCommand(ent-g_entities, "print \"Login sucessful.\n\"");

		Q_strncpyz(strIP, ent->client->sess.IP, sizeof(strIP));
		p = strchr(strIP, ':');
		if (p) //loda - fix ip sometimes not printing
			*p = 0;

		sql = "UPDATE LocalAccount SET lastip = ?, lastlogin = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int64 (stmt, 1, ip_to_int(strIP)));
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
    int row = 0, i = 0;
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

	if (enteredPassword && password && enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		char *p = NULL;
		char strIP[NET_ADDRSTRMAXLEN] = {0};

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

void Cmd_Stats_f( gentity_t *ent ) { //Should i bother to cache player stats in memory? id then have to live update them.. but its doable.. worth it though?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16];
	int row = 0, kills, deaths, suicides, captures, returns, lastlogin, playtime, realdeaths;
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
        int s;
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
	CALL_SQLITE (close(db));

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
	Q_strcat(buf, sizeof(buf), va("   ^5Last login: ^2%s\n", timeStr));
	//Q_strcat(buf, sizeof(buf), va("  ^5Playtime / Lastlogin^3: ^2%i / %i\n", playtime, lastlogin);

	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", buf));
}

void G_AddSimpleStatsToDB() {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	gclient_t	*cl;
	int i;

	//cancel if cheats enabled loda

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "UPDATE LocalAccount SET "
		"kills = kills + ?, "
		"deaths = deaths + ?, "
		"suicides = suicides + ?, "
		"captures = captures + ?, "
		"returns = returns + ? "
		"WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	for (i = 0; i < MAX_CLIENTS; i++) {//Build a list of clients
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (cl->pers.netname[0] && cl->pers.userName && cl->pers.userName[0]) {

			trap->Print("Kills for %s, %i", cl->pers.userName, cl->pers.stats.kills);

			CALL_SQLITE (bind_int (stmt, 1, cl->pers.stats.kills));
			CALL_SQLITE (bind_int (stmt, 2, cl->ps.persistant[PERS_KILLED]));
			CALL_SQLITE (bind_int (stmt, 3, cl->ps.fd.suicides));
			CALL_SQLITE (bind_int (stmt, 4, cl->pers.teamState.captures));
			CALL_SQLITE (bind_int (stmt, 5, cl->pers.teamState.flagrecovery));
			CALL_SQLITE (bind_text (stmt, 6, cl->pers.userName, -1, SQLITE_STATIC));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
	}
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}

void CleanupLocalRun() {
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
		Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));

		//sql = "INSERT INTO TempLocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) "
			//	"SELECT LocalRun.username, LocalRun.coursename, MIN(LocalRun.duration_ms), LocalRun.topspeed, LocalRun.average, LocalRun.style, LocalRun.end_time FROM LocalRun "
			//	"WHERE LocalRun.coursename = ? AND LocalRun.style = ? GROUP BY username, coursename, style, topspeed, average, end_time ORDER BY MIN(duration_ms) ASC LIMIT 10"; //loda fixme, maybe keep more in table? 

		/*
		sql = "INSERT INTO TempLocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) "
			"SELECT LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "
				"from (SELECT username, MIN(duration_ms) AS mintime "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP BY username, coursename, style) " 
				"AS x INNER JOIN LocalRun AS LR ON LR.username = x.username AND LR.duration_ms = x.mintime";
				*/


		//LODA FIXME, use id instead of username to join
		//is there any way to fix this first query so it does this?
		//The problem is that we dont want to insert the ID field, but we have to select it to use it in the join later..?
		//Any way to get around this.. short of making a useless column in this table (as the second query does)?
		/*
		sql = "INSERT INTO TempLocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) " 
			"SELECT LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "
				"from (SELECT username, MIN(duration_ms) AS mintime "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP BY username, coursename, style) " 
				"AS x INNER JOIN LocalRun AS LR ON LR.username = x.username AND LR.duration_ms = x.mintime";
		*/
		
		sql = "INSERT INTO TempLocalRun (old_id, username, coursename, duration_ms, topspeed, average, style, end_time) " 
			"SELECT LR.id, LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "
				"from (SELECT id, MIN(duration_ms) AS mintime "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP BY username, coursename, style) " 
				"AS x INNER JOIN LocalRun AS LR ON LR.id = x.id AND LR.duration_ms = x.mintime";
				

		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		
		for (mstyle = 0; mstyle < 7; mstyle++) { //7 movement styles. 0-6
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 2, mstyle));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
		CALL_SQLITE (finalize(stmt));

		sql = "DELETE FROM LocalRun WHERE coursename = ?"; //This isnt getting executed??
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

	//drop templocalrun here or?

	CALL_SQLITE (close(db));

}

void BuildMapHighscores() { //loda fixme, take prepare,query out of loop
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, row = 0, mstyle;
	char mapName[40], courseName[40], info[1024] = {0};

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

			CALL_SQLITE (open (LOCAL_DB_PATH, & db));

			//sql = "SELECT LocalRun.username, LocalRun.coursename, MIN(LocalRun.duration_ms), LocalRun.topspeed, LocalRun.average, LocalRun.style, LocalRun.end_time FROM LocalRun "
			//	"WHERE LocalRun.coursename = ? AND LocalRun.style = ? GROUP BY username, coursename, style, topspeed, average, end_time ORDER BY MIN(duration_ms) ASC LIMIT 10";

			/*
			sql = "SELECT LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "  //LODA FIXME, user LR.id instead of LR.username in the join? 
				"FROM (SELECT username, MIN(duration_ms) AS mintime "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP by username, coursename, style) " 
				"AS x INNER JOIN LocalRun AS LR ON LR.username = x.username AND LR.duration_ms = x.mintime";
			*/

			sql = "SELECT LR.id, LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "
				"FROM (SELECT id, MIN(duration_ms) AS mintime "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP by username, coursename, style) " 
				"AS x INNER JOIN LocalRun AS LR ON LR.id = x.id AND LR.duration_ms = x.mintime";

			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int (stmt, 2, mstyle));
			//CALL_SQLITE_EXPECT (step (stmt), DONE); //wtf why not lol

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

					username = (char*)sqlite3_column_text(stmt, 1); //Increment each of these by 1 if we use id to join
					course = (char*)sqlite3_column_text(stmt, 2);
					duration_ms = sqlite3_column_int(stmt, 3);
					topspeed = sqlite3_column_int(stmt, 4);
					average = sqlite3_column_int(stmt, 5);
					style = sqlite3_column_int(stmt, 6);
					end_time = sqlite3_column_int(stmt, 7);

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
	int i = 0;

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

	if (level.numCourses == 1 && trap->Argc() == 1) { //dftop10
		style = 1;
		Q_strncpyz(courseName, "", sizeof(courseName));
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

	Q_strlwr(courseName);
	Q_CleanStr(courseName);
	Q_strlwr(courseNameFull);
	Q_CleanStr(courseNameFull);

	IntegerToRaceName(style, styleString);
	trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s style:\n\"", courseNameFull, styleString));
	trap->SendServerCommand(ent-g_entities, "print \"   ^5Username           Time         Topspeed    Average\n\"");

	for (i = 0; i < (10 * level.numCourses * 7); i++) {
		if (level.Highscores[i].username && level.Highscores[i].username[0]) {
			if ((!Q_stricmp(level.Highscores[i].coursename, courseNameFull)) && (level.Highscores[i].style == style)) {
				if (level.Highscores[i].duration_ms >= 60000) {
					int minutes, seconds, milliseconds;
					minutes = (int)((level.Highscores[i].duration_ms / (1000*60)) % 60);
					seconds = (int)(level.Highscores[i].duration_ms / 1000) % 60;
					milliseconds = level.Highscores[i].duration_ms % 1000; 
					Com_sprintf(timeStr, sizeof(timeStr), "%i:%02i.%i", minutes, seconds, milliseconds);//more precision?
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
	G_AddToDBFromFile(); //From file to db
	BuildMapHighscores(); //From db, built to memory
}

void Cmd_ACWhois_f( gentity_t *ent ) { //Should this only show logged in people..?
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;

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

			Q_strncpyz(strNum, va("^5%i^3:", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));
			Q_strncpyz( strUser, cl->pers.userName, sizeof(strUser));	
			tmpMsg = va("%-2s ^7%-18s^7%s\n", strNum, strUser, strName);

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

	memset(&HighScores, 0, sizeof(HighScores)); // initialize all highscores for this map
	level.Highscores = HighScores;

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

	sql = "CREATE TABLE IF NOT EXISTS LocalDuel(id INTEGER PRIMARY KEY, player1 VARCHAR(16), player2 VARCHAR(16), duration UNSIGNED SMALLINT, "
		"type UNSIGNED TINYINT, winner_hp UNSIGNED TINYINT, winner_shield UNSIGNED TINYINT, end_time UNSIGNED INT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));

	CleanupLocalRun(); //Deletes useless shit from LocalRun database table
	G_AddToDBFromFile(); //Add last maps highscores
	BuildMapHighscores();//Build highscores into memory from database

	CALL_SQLITE (close(db));
}

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