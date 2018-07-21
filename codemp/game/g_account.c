#include "g_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"

#define _USE_CURL 0

#if _USE_CURL
#include "curl/curl.h"
#include "curl/easy.h"
#endif

#define LOCAL_DB_PATH "japro/data.db"
//#define GLOBAL_DB_PATH sv_globalDBPath.string
//#define MAX_TMP_RACELOG_SIZE 80 * 1024

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

#define JAPRO_ACCOUNTFLAG_IPLOCK		(1<<0)
#define JAPRO_ACCOUNTFLAG_TRUSTED		(1<<1)

#if 0
typedef struct RaceRecord_s {
	char				username[16];
	char				coursename[40];
	unsigned int		duration_ms;
	unsigned short		topspeed;
	unsigned short		average;
	unsigned short		style; //only needs to be 3 bits	
	char				end_time[64]; //Why a char?
	unsigned int		end_timeInt;
} RaceRecord_t;

RaceRecord_t	HighScores[32][MV_NUMSTYLES][10];//32 courses, 9 styles, 10 spots on highscore list

typedef struct PersonalBests_s {
	char				username[16];
	char				coursename[40];
	unsigned int		duration_ms;
	unsigned short		style; //only needs to be 3 bits	
} PersonalBests_t;

PersonalBests_t	PersonalBests[9][50];//9 styles, 50 cached spots
#endif

#if 0
typedef struct UserStats_s {
	char				username[16];
	unsigned short		kills;
	unsigned short		deaths;
	unsigned short		suicides;
	unsigned short		captures;
	unsigned short		returns;
} UserStats_t;

UserStats_t	UserStats[256];//256 max logged in users per map :/
#endif

/*
typedef struct UserAccount_s {
	char			username[16];
	char			password[16];
	int				lastIP;
} UserAccount_t;
*/

/*
typedef struct PlayerID_s {
	char			name[MAX_NETNAME];
	unsigned int	ip[NET_ADDRSTRMAXLEN];
	int				guid[33];
} PlayerID_t;
*/

void getDateTime(int time, char * timeStr, size_t timeStrSize) {
	time_t	timeGMT;
	//time -= 60*60*5; //EST timezone -5? 
	timeGMT = (time_t)time;
	strftime( timeStr, timeStrSize, "%m/%d/%y %I:%M %p", localtime( &timeGMT ) );
}

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

void G_ErrorPrint( const char *fmt, int s ) {
	trap->SendServerCommand( -1, va("print \"%s %i\n\"", fmt, s) );
	G_SecurityLogPrintf(fmt);
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

/*
void DebugWriteToDB(char *entrypoint) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;
	char username[16], password[16];

	Q_strncpyz(username, "test", sizeof(username));
	Q_strncpyz(password, "test", sizeof(password));

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "UPDATE LocalAccount SET password = ? WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));

	s = sqlite3_step(stmt);

	if (s != SQLITE_DONE)
		G_SecurityLogPrintf( "ERROR: Could not write to database with error %i, entrypoint %s\n", s, entrypoint );

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}
*/

int CheckUserExists(char *username) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int row = 0, s;

	//load fixme replace this with simple select count

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT id FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
			G_ErrorPrint("ERROR: SQL Select Failed (CheckUserExists)", s);
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	//DebugWriteToDB("CheckUserExists");

	if (row == 0) {
		return 0;
	}
	if (row == 1) { //Only 1 matching accound
		return 1;
	}

	trap->Print("ERROR: Multiple accounts with same accountname!\n");
	return 0;

}
void G_AddPlayerLog(char *name, char *strIP, char *guid) {
	fileHandle_t f;
	char string[128], string2[128], buf[80*1024], cleanName[MAX_NETNAME];
	char *p = NULL;
	int	fLen = 0;
	char*	pch;
	qboolean unique = qtrue;

	p = strchr(strIP, ':');
	if (p) //loda - fix ip sometimes not printing
		*p = 0;

	if (!Q_stricmp(strIP, "") && !Q_stricmp(guid, "NOGUID")) //No IP or GUID info to be gained.. so forget it
		return;

	Q_strncpyz(cleanName, name, sizeof(cleanName));

	Q_strlwr(cleanName);
	Q_CleanStr(cleanName);

	if (!Q_stricmp(name, "padawan")) //loda fixme, also ignore Padawan[0] etc..
		return;
	if (!Q_stricmp(name, "padawan[0]")) //loda fixme, also ignore Padawan[0] etc..
		return;
	if (!Q_stricmp(name, "padawan[1]")) //loda fixme, also ignore Padawan[0] etc..
		return;

	Com_sprintf(string, sizeof(string), "%s;%s;%s", cleanName, strIP, guid); //Store ip as char

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

#if _ELORANKING	

int GetDuelCount(char *username, int type, int end_time, sqlite3 * db) {
    char * sql;
    sqlite3_stmt * stmt;
	int s;
	int count = 0;

	//Get Current Count, Get last duel id, get new duel id
	sql = "SELECT COUNT(*) FROM LocalDuel WHERE type = ? AND (winner = ? OR loser = ?) AND end_time < ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_int (stmt, 1, type));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 3, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 4, end_time));

	s = sqlite3_step(stmt);

	if (s == SQLITE_ROW) {
		count = sqlite3_column_int(stmt, 0);
	}
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (GetDuelCount)", s);
	}

	CALL_SQLITE (finalize(stmt));

	return count;
}

float GetDuelElo( char *username, int type, int end_time, sqlite3 * db) {
	float elo = -999.0f;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	sql = "SELECT winner_elo AS elo, end_time FROM LocalDuel where type = ? AND winner = ? AND end_time < ? "
		"UNION ALL SELECT loser_elo AS elo, end_time FROM LocalDuel where type = ? AND loser = ? AND end_time < ? "
		"ORDER BY end_time DESC LIMIT 1";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_int (stmt, 1, type));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, end_time));
	CALL_SQLITE (bind_int (stmt, 4, type));
	CALL_SQLITE (bind_text (stmt, 5, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 6, end_time));
	
	s = sqlite3_step(stmt);

	if (s == SQLITE_ROW) {
		elo = sqlite3_column_double(stmt, 0);
	}
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (GetDuelElo)", s);
	}

	if (elo == -999.0f) {//This needs to just be done to check if its null
		elo = 1000; //Elo not found, give them initial value
	}

	CALL_SQLITE (finalize(stmt));

	//Com_Printf("Getting duel elo %.2f %s %i %i\n", elo, username, type, end_time);

	return elo;
}

void UpdatePlayerRating(char *username, int type, qboolean winner, float newElo, float odds, int id, sqlite3 * db) {
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	if (id) {//We are doing a rebuild of everything so we cant trust end_time, also we can optimize it by using the known id to update
		if (winner)
			sql = "UPDATE LocalDuel SET winner_elo = ?, odds = ? WHERE id = ?";
		else
			sql = "UPDATE LocalDuel SET loser_elo = ?, odds = ? WHERE id = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_double (stmt, 1, newElo));
		CALL_SQLITE (bind_double (stmt, 2, odds));
		CALL_SQLITE (bind_int (stmt, 3, id));
	}
	else {
		if (winner)
			sql = "UPDATE LocalDuel SET winner_elo = ?, odds = ? WHERE type = ? AND winner = ? AND end_time = (SELECT MAX(end_time) FROM LocalDuel WHERE type = ? and winner = ?)";
		else
			sql = "UPDATE LocalDuel SET loser_elo = ?, odds = ? WHERE type = ? AND loser = ? AND end_time = (SELECT MAX(end_time) FROM LocalDuel WHERE type = ? and loser = ?)";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_double (stmt, 1, newElo));
		CALL_SQLITE (bind_double (stmt, 2, odds));
		CALL_SQLITE (bind_int (stmt, 3, type));
		CALL_SQLITE (bind_text (stmt, 4, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 5, type));
		CALL_SQLITE (bind_text (stmt, 6, username, -1, SQLITE_STATIC));
	}

	s = sqlite3_step(stmt);

	if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Update Failed (UpdatePlayerRating)", s);
	}

	CALL_SQLITE (finalize(stmt));
}

int GetEloKValue(int numDuels) { //Also take rank into account
	int k1 = g_eloKValue1.integer;
	int k2 = g_eloKValue2.integer;
	int k3 = g_eloKValue3.integer;

	int p = g_eloProvisionalCutoff.integer;
	int n = g_eloNewUserCutoff.integer;

	if (k1 < 1)
		k1 = 1;
	if (k2 < 1)
		k2 = 1;
	if (k3 < 1)
		k3 = 1;
	if (p < 0)
		p = 0;
	if (n < 0)
		n = 0;

	if (numDuels <= n) //BRAND NEW PLAYER
		return k1;
	if (numDuels <= p) //PROVISIONAL
		return k2;
	return k3;
}

void G_AddDuelToDB(char *winner, char *loser, int type, int duration, int winner_hp, int winner_shield, int end_time) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "INSERT INTO LocalDuel(winner, loser, duration, type, winner_hp, winner_shield, end_time, winner_elo, loser_elo, odds) VALUES (?, ?, ?, ?, ?, ?, ?, -999, -999, 0)";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, winner, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, loser, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, duration));
	CALL_SQLITE (bind_int (stmt, 4, type));
	CALL_SQLITE (bind_int (stmt, 5, winner_hp));
	CALL_SQLITE (bind_int (stmt, 6, winner_shield));
	CALL_SQLITE (bind_int (stmt, 7, end_time));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Insert Failed (G_AddDuelToDB)", s);
	}

	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));
}

void G_AddDuelElo(char *winner, char *loser, int type, int duration, int winner_hp, int winner_shield, int id, int end_time, sqlite3 *db) { //id and end_time are passed through if its a /rebuildElo 
	int winnerDuelCount, loserDuelCount, winnerType, loserType, winnerK, loserK;
	float expectedScoreWinner, expectedScoreLoser, WA, LA, loserElo, winnerElo, newWinnerElo, newLoserElo;
	const int NEWUSER = 0, PROVISIONAL = 1, NORMAL = 2;

	int newUserCutoff = g_eloNewUserCutoff.integer;
	int provisionalCutoff = g_eloProvisionalCutoff.integer;
	float provisionalChangeBig = g_eloProvisionalChangeBig.value;
	float provisionalChangeSmall = g_eloProvisionalChangeSmall.value;

	if (newUserCutoff < 0)
		newUserCutoff = 0;
	if (provisionalCutoff < 0);
		provisionalCutoff = 0;
	if (provisionalChangeBig < 0.1f);
		provisionalChangeBig = 0.1f;
	if (provisionalChangeSmall < 0.1f);
		provisionalChangeSmall = 0.1f;

	winnerDuelCount = GetDuelCount(winner, type, end_time, db);
	loserDuelCount = GetDuelCount(loser, type, end_time, db);

	if (winnerDuelCount < 0) //Error i guess
		return;
	if (loserDuelCount < 0)
		return;

	if (winnerDuelCount <= newUserCutoff)
		winnerType = NEWUSER;
	else if (winnerDuelCount <= provisionalCutoff)
		winnerType = PROVISIONAL;
	else
		winnerType = NORMAL;

	if (loserDuelCount <= newUserCutoff)
		loserType = NEWUSER;
	else if (loserDuelCount <= provisionalCutoff)
		loserType = PROVISIONAL;
	else
		loserType = NORMAL;

	if (winnerType == NEWUSER)
		winnerElo = 1000; //always have newusers kept at 1k elo until they get enough duels?
	else 
		winnerElo = GetDuelElo(winner, type, end_time, db);

	if (winnerElo == -999.0f) //Error i guess
		return; 

	if (loserType == NEWUSER)
		loserElo = 1000; //loda fixme
	else
		loserElo = GetDuelElo(loser, type, end_time, db);

	if (loserElo == -999.0f) //Error i guess
		return; 
		
	winnerK = GetEloKValue(winnerDuelCount);
	loserK = GetEloKValue(loserDuelCount);

	if (winnerType == PROVISIONAL) {
		if (loserType == PROVISIONAL || loserType == NEWUSER) //Loser is also provisional
			winnerK *= provisionalChangeSmall;
		else 
			winnerK *= provisionalChangeBig;
	}

	if (loserType == PROVISIONAL) { //PROVISIONAL, calculate loser rank
		if (winnerType == PROVISIONAL || winnerType == NEWUSER) //Winner is also provisional
			loserK *= provisionalChangeSmall;
		else 
			loserK *= provisionalChangeBig;
	}

	WA = pow(10, winnerElo / 400.0f);
	LA = pow(10, loserElo / 400.0f);

	expectedScoreWinner = WA / (WA + LA);
	expectedScoreLoser = 1 - expectedScoreWinner;
	//Round to.. 5th digit? or..
	//expectedScoreLoser = LA / (LA + WA); //This is just 1 - expected score winner..?

	//if (winnerType == PROVISIONAL || winnerType == NORMAL) //Nvm about this.. rank their first duels i guess.
		newWinnerElo = winnerElo + winnerK * (1 - expectedScoreWinner);

	//if (loserType == PROVISIONAL || loserType == NORMAL)
		newLoserElo = loserElo + loserK * (0 - expectedScoreLoser);

	if (!id) { //We are not doing a rebuild, so add the duel here after we get the needed info
		 G_AddDuelToDB(winner, loser, type, duration, winner_hp, winner_shield, end_time);
	}

	if (newWinnerElo != winnerElo) //Update winner elo
		UpdatePlayerRating(winner, type, qtrue, newWinnerElo, expectedScoreWinner, id, db);

	if (newLoserElo != loserElo) //Update loser elo
		UpdatePlayerRating(loser, type, qfalse, newLoserElo, expectedScoreLoser, id, db);

	//Com_Printf("Adding duel: odds = %.2f, %s [%.2f -> %.2f (c=%i) (t=%i)] > %s [%.2f -> %.2f (c=%i) (t=%i)] {%i}\n", 
		//expectedScoreWinner, winner, winnerElo, newWinnerElo, winnerDuelCount, winnerType, loser, loserElo, newLoserElo, loserDuelCount, loserType, type);
}

void SV_RebuildElo_f() {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;
	int time1 = trap->Milliseconds();

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "UPDATE LocalDuel SET winner_elo = -999, loser_elo = -999, odds = 0";//Save rank into row - use null
    //sql = "DELETE FROM DuelRanks";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Update Failed (SV_RebuildElo_f 1)", s);
	}
	CALL_SQLITE (finalize(stmt));

	sql = "SELECT winner, loser, type, id, end_time from LocalDuel ORDER BY end_time ASC";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			G_AddDuelElo((char*)sqlite3_column_text(stmt, 0), (char*)sqlite3_column_text(stmt, 1), sqlite3_column_int(stmt, 2), 0, 0, 0, sqlite3_column_int(stmt, 3), sqlite3_column_int(stmt, 4), db);
        }
        else if (s == SQLITE_DONE) {
            break;
		}
        else {
			G_ErrorPrint("ERROR: SQL Select Failed (SV_RebuildElo_f 2)", s);
			break;
        }
    }
	
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	Com_Printf("Duel ranks cleared in %i ms.\n", trap->Milliseconds() - time1);
}

int DuelTypeToInteger(char *style) {
	Q_strlwr(style);
	Q_CleanStr(style);

	if (!Q_stricmp(style, "saber") || !Q_stricmp(style, "nf"))
		return 0;
	if (!Q_stricmp(style, "force") || !Q_stricmp(style, "fullforce") || !Q_stricmp(style, "ff"))
		return 1;
	if (!Q_stricmp(style, "melee") || !Q_stricmp(style, "fists") || !Q_stricmp(style, "fisticuffs"))
		return 4;
	if (!Q_stricmp(style, "pistol"))
		return 6;
	if (!Q_stricmp(style, "blaster") || !Q_stricmp(style, "e11"))
		return 7;
	if (!Q_stricmp(style, "sniper") || !Q_stricmp(style, "disruptor"))
		return 8;
	if (!Q_stricmp(style, "bowcaster") || !Q_stricmp(style, "crossbow"))
		return 9;
	if (!Q_stricmp(style, "repeater"))
		return 10;
	if (!Q_stricmp(style, "demp2"))
		return 11;
	if (!Q_stricmp(style, "flechette") || !Q_stricmp(style, "shotgun"))
		return 12;
	if (!Q_stricmp(style, "rocket") || !Q_stricmp(style, "rocket launcher") || !Q_stricmp(style, "rl"))
		return 13;
	if (!Q_stricmp(style, "thermal") || !Q_stricmp(style, "grenade"))
		return 14;
	if (!Q_stricmp(style, "tripmine"))
		return 15;
	if (!Q_stricmp(style, "detpack"))
		return 16;
	if (!Q_stricmp(style, "concussion") || !Q_stricmp(style, "conc"))
		return 17;
	if (!Q_stricmp(style, "bryar pistol") || !Q_stricmp(style, "bryar"))
		return 18;
	if (!Q_stricmp(style, "stun baton") || !Q_stricmp(style, "stun"))
		return 19;
	if (!Q_stricmp(style, "all"))
		return 20;
	return -1;
}

void IntegerToDuelType(int type, char *typeString, size_t typeStringSize) {
	switch(type) {
		case 0: Q_strncpyz(typeString, "saber", typeStringSize); break;
		case 1: Q_strncpyz(typeString, "force", typeStringSize); break;
		case 4:	Q_strncpyz(typeString, "melee", typeStringSize);	break;
		case 6:	Q_strncpyz(typeString, "pistol", typeStringSize); break;
		case 7:	Q_strncpyz(typeString, "blaster", typeStringSize); break;
		case 8:	Q_strncpyz(typeString, "sniper", typeStringSize); break;
		case 9:	Q_strncpyz(typeString, "bowcaster", typeStringSize); break;
		case 10: Q_strncpyz(typeString, "repeater", typeStringSize); break;
		case 11: Q_strncpyz(typeString, "demp2", typeStringSize); break;
		case 12: Q_strncpyz(typeString, "flechette", typeStringSize); break;
		case 13: Q_strncpyz(typeString, "rocket", typeStringSize); break;
		case 14: Q_strncpyz(typeString, "thermal", typeStringSize); break;
		case 15: Q_strncpyz(typeString, "trip mine", typeStringSize); break;
		case 16: Q_strncpyz(typeString, "det pack", typeStringSize); break;
		case 17: Q_strncpyz(typeString, "concussion", typeStringSize); break;
		case 18: Q_strncpyz(typeString, "bryar pistol", typeStringSize); break;
		case 19: Q_strncpyz(typeString, "stun baton", typeStringSize); break;
		case 20: Q_strncpyz(typeString, "all weapons", typeStringSize); break;
		default: Q_strncpyz(typeString, "ERROR", typeStringSize); break;
	}
}

void Cmd_DuelTop10_f(gentity_t *ent) {
	char username[32], typeString[32], inputString[32];
	const int args = trap->Argc();
	int type = -1, page = -1, start = 0, input, i, minimumCount = g_eloMinimumDuels.integer;

	if (args <= 1 || args > 3) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /top10 <dueltype> <page>\n\"");
		return;
	}

	for (i = 1; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (type == -1) {
			input = DuelTypeToInteger(inputString);
			if (input != -1) {
				type = input;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /top10 <dueltype> <page>\n\"");
		return;
	}

	if (type == -1) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /top10 <dueltype> <page>\n\"");
		return;
	}

	IntegerToDuelType(type, typeString, sizeof(typeString));
	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	if (minimumCount < 0)
		minimumCount = 0;

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int rank, count, TS, s, row = 1;
		char msg[1024-128] = {0};

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));

		//We dont need to select from loser since we know a users highscore will always be from a winning duel.  And we can ignore users who have never won a duel(?)
		//How to get count?
		//sql = "SELECT winner, winner_elo, 100, 100 FROM (SELECT winner, winner_elo, odds, end_time FROM LocalDuel WHERE type = ? ORDER BY end_time ASC) GROUP BY winner ORDER BY winner_elo DESC LIMIT 10";
		sql = "SELECT D1.username, elo, 100-ROUND(100*(win_ts + loss_ts)/(win_count+loss_count), 0) AS TS, win_count+loss_count AS count "
				"FROM ((SELECT username, type, elo FROM ((SELECT winner AS username, type, ROUND(winner_elo,0) AS elo, end_time FROM LocalDuel WHERE type = ? "
				"UNION ALL SELECT loser AS username, type, ROUND(loser_elo,0) AS elo, end_time FROM LocalDuel WHERE type = ? ORDER BY end_time ASC)) GROUP BY username ORDER BY elo DESC) AS D1 "
				"INNER JOIN (SELECT winner AS username2, COUNT(*) AS win_count, SUM(odds) AS win_ts FROM LocalDuel WHERE type = ? GROUP BY username2) AS D2 "
				"ON D1.username = D2.username2) "
				"INNER JOIN (SELECT loser AS username3, COUNT(*) AS loss_count, SUM(1-odds) AS loss_ts FROM LocalDuel WHERE type = ? GROUP BY username3) AS D3 "
				"ON D1.username = D3.username3 ORDER BY elo desc LIMIT ?, 10";

		//loda fixme
		/*
		sql = "SELECT username, rank, count, TSSUM \
			FROM DuelRanks WHERE type = ? AND count > ? \
			GROUP BY username ORDER BY rank DESC LIMIT 10";
		*/

		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, type));
		CALL_SQLITE (bind_int (stmt, 2, type));
		CALL_SQLITE (bind_int (stmt, 3, type));
		CALL_SQLITE (bind_int (stmt, 4, type));
		CALL_SQLITE (bind_int (stmt, 5, start));

		//CALL_SQLITE (bind_int (stmt, 2, type));
		//CALL_SQLITE (bind_int (stmt, 3, minimumCount));

		trap->SendServerCommand(ent-g_entities, va("print \"Topscore results for %s duels:\n    ^5Username           Skill        TS        Count\n\"", typeString));
	
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;

				Q_strncpyz(username, (char*)sqlite3_column_text(stmt, 0), sizeof(username));
				rank = sqlite3_column_int(stmt, 1);
				count = sqlite3_column_int(stmt, 2);
				TS = sqlite3_column_int(stmt, 3);

				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-12i ^3%-9i %i\n", start+row, username, rank, count, TS);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE) {
				trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
				break;
			}
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DuelTop10_f)", s);
				break;
			}
		}

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}
}
#endif

void G_AddDuel(char *winner, char *loser, int start_time, int type, int winner_hp, int winner_shield) {
	sqlite3 * db;
	time_t	rawtime;
	char	string[256] = {0};
	const int duration = start_time ? (level.time - start_time) : 0;

	time( &rawtime );
	localtime( &rawtime );

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", winner, loser, duration, type, winner_hp, winner_shield, rawtime);

	if (level.duelLog)
		trap->FS_Write(string, strlen(string), level.duelLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	//Might want to make this log to file, and have that sent to db on map change.  But whatever.. duel finishes are not as frequent as race course finishes usually.

#if _ELORANKING	
	if (g_eloRanking.integer) {
		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		G_AddDuelElo(winner, loser, type, duration, winner_hp, winner_shield, 0, rawtime, db);
		CALL_SQLITE (close(db));
	}
#endif

}

#if 0
void G_TestAddDuel() {
	char winner[32], loser[32], type[32];
	int time1;

	if (trap->Argc() != 4) {
		Com_Printf("Usage: /addDuel winner loser type\n");
		return;
	}

	trap->Argv(1, winner, sizeof(winner));
	trap->Argv(2, loser, sizeof(loser));
	trap->Argv(3, type, sizeof(type));

	time1 = trap->Milliseconds();

	G_AddDuel(winner, loser, level.time-1000, atoi(type), 420, 420);

	Com_Printf("Adding duel elo, took %i ms\n", trap->Milliseconds() - time1);
}
#endif

#if !_NEWRACERANKING
void G_AddToDBFromFile(void) { //loda fixme, we can filter out the slower times from file before we add them??.. keep a record idk..?
	fileHandle_t f;	
	int		fLen = 0, args = 1, s = 0, row = 0, i, j; //MAX_FILESIZE = 4096
	char	buf[MAX_TMP_RACELOG_SIZE] = {0}, empty[8] = {0};//eh
	char*	pch;
	sqlite3 * db;
	char* sql;
	sqlite3_stmt * stmt;
	RaceRecord_t	TempRaceRecord[1024] = {0}; //Max races per map to count i gues..? should this be zeroed?
	qboolean good = qfalse, foundFaster = qfalse;

	fLen = trap->FS_Open(TEMP_RACE_LOG, &f, FS_READ);

	if (!f) {
		trap->Print("ERROR: Couldn't load defrag data from %s\n", TEMP_RACE_LOG);
		return;
	}
	if (fLen >= MAX_TMP_RACELOG_SIZE) {
		trap->FS_Close(f);
		trap->Print("ERROR: Couldn't load defrag data from %s, file is too large\n", TEMP_RACE_LOG);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);
	
	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, end_time) VALUES (?, ?, ?, ?, ?, ?, ?)";	 //loda fixme, make multiple?

	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	//Todo: make TempRaceRecord an array of structs instead, maybe like 32 long idk, and build a query to insert 32 at a time or something.. instead of 1 by 1
	pch = strtok (buf,";\n");
	while (pch != NULL)
	{
		if ((args % 7) == 1)
			Q_strncpyz(TempRaceRecord[row].username, pch, sizeof(TempRaceRecord[row].username));
		else if ((args % 7) == 2)
			Q_strncpyz(TempRaceRecord[row].coursename, pch, sizeof(TempRaceRecord[row].coursename));
		else if ((args % 7) == 3)
			TempRaceRecord[row].duration_ms = atoi(pch);
		else if ((args % 7) == 4)
			TempRaceRecord[row].topspeed = atoi(pch);
		else if ((args % 7) == 5)
			TempRaceRecord[row].average = atoi(pch);
		else if ((args % 7) == 6)
			TempRaceRecord[row].style = atoi(pch);
		else if ((args % 7) == 0) {
			TempRaceRecord[row].end_timeInt = atoi(pch);

			if (row >= 1024) {
				trap->Print("ERROR: Too many entries in %s! Could not add to database.\n", TEMP_RACE_LOG);
				return;
			}
			row++;
		}
    	pch = strtok (NULL, ";\n");
		args++;
	}

	for (i=0;i<1024;i++) {
		//This is the time we are looking at [i]

		//See if we can find any faster times

		for (j=0;j<1024;j++) {	
			foundFaster = qfalse;

			if (!Q_stricmp(TempRaceRecord[j].username, TempRaceRecord[i].username) &&
				!Q_stricmp(TempRaceRecord[j].coursename, TempRaceRecord[i].coursename) &&
				TempRaceRecord[j].style == TempRaceRecord[i].style)
			{
				if (TempRaceRecord[j].duration_ms < TempRaceRecord[i].duration_ms) { //we found a faster time
					foundFaster = qtrue;
					//trap->Print("Found a faster time!\n");
					break;
				}
			}
			if (!TempRaceRecord[j].username[0])
				break;
		}
		if (!TempRaceRecord[i].username[0])//see what happens if we move this up here..
			break;

		if (TempRaceRecord[i].end_timeInt <= 0)
			break;

		if (!foundFaster) {
			const int place = i;//The fuck is this.. shut the compiler up

			//Debug this..
			//G_SecurityLogPrintf( "ADDING RACE TIME TO DB WITH PLACE %i: %s, %s, %u, %u, %u, %u, %u \n", 
				//place, TempRaceRecord[place].username, TempRaceRecord[place].coursename, TempRaceRecord[place].duration_ms, TempRaceRecord[place].topspeed, TempRaceRecord[place].average, TempRaceRecord[place].style, TempRaceRecord[place].end_timeInt);

			CALL_SQLITE (bind_text (stmt, 1, TempRaceRecord[place].username, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_text (stmt, 2, TempRaceRecord[place].coursename, -1, SQLITE_STATIC));
			CALL_SQLITE (bind_int64 (stmt, 3, TempRaceRecord[place].duration_ms));
			CALL_SQLITE (bind_int64 (stmt, 4, TempRaceRecord[place].topspeed));
			CALL_SQLITE (bind_int64 (stmt, 5, TempRaceRecord[place].average));
			CALL_SQLITE (bind_int64 (stmt, 6, TempRaceRecord[place].style));
			CALL_SQLITE (bind_int64 (stmt, 7, TempRaceRecord[place].end_timeInt));

			//CALL_SQLITE_EXPECT (step (stmt), DONE);
			s = sqlite3_step(stmt);

			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));

			//AddRunToWebServer(TempRaceRecord[place]);
		}
	}

	//s = sqlite3_step(stmt); //this duplicates last one..? LODA FIXME, this inserts empty row

	if (s == SQLITE_DONE) {
		good = qtrue;
	}

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));	

	if (good) { //dont delete tmp file if mysql database is not responding 
		trap->FS_Open(TEMP_RACE_LOG, &f, FS_WRITE);
		trap->FS_Write( empty, strlen( empty ), level.tempRaceLog );
		trap->FS_Close(f);
		trap->Print("Loaded previous map racetimes from %s.\n", TEMP_RACE_LOG);
	}
	else 
		trap->Print("Unable to insert previous map racetimes into database.\n");

	//DebugWriteToDB("G_AddToDBFromFile");
}
#endif

gentity_t *G_SoundTempEntity( vec3_t origin, int event, int channel );
#if 0
void PlayActualGlobalSound2(char * sound) { //loda fixme, just go through each client and play it on them..?
	gentity_t	*te;
	vec3_t temp = {0, 0, 0};

	te = G_SoundTempEntity( temp, EV_GENERAL_SOUND, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex(sound);
	//te->s.saberEntityNum = channel;
	te->s.eFlags = EF_SOUNDTRACKER;
	te->
	r.svFlags |= SVF_BROADCAST;
}
#endif
void PlayActualGlobalSound(char * sound) {
	gentity_t *player;
	int i;

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clients
		if (!g_entities[i].inuse)
			continue;
		player = &g_entities[i];
		G_Sound(player, CHAN_AUTO, G_SoundIndex(sound));
	}
}

#if _RACECACHE
void WriteToTmpRaceLog(char *string, size_t stringSize) {
	int fLen = 0;
	fileHandle_t f;

	if (level.tempRaceLog) //Lets try only writing to temp file if we know its a highscore
		trap->FS_Write(string, stringSize, level.tempRaceLog ); //Always write to text file, this file is remade every mapchange and its contents are put to database.

	fLen = trap->FS_Open(TEMP_RACE_LOG, &f, FS_READ);

	if (!f) {
		trap->Print("ERROR: Couldn't read defrag data from %s\n", TEMP_RACE_LOG);
		return;
	}	

	if (fLen >= (MAX_TMP_RACELOG_SIZE - ((int)stringSize * 2))) { //Just to be safe..
		trap->FS_Close(f);
		trap->SendServerCommand(-1, va("print \"WARNING: %s file is too large, reloading map to clear it.\n\"", TEMP_RACE_LOG));
		trap->SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		return;
	}
}
#endif

void IntegerToRaceName(int style, char *styleString, size_t styleStringSize) {
	switch(style) {
		case 0: Q_strncpyz(styleString, "siege", styleStringSize); break;
		case 1: Q_strncpyz(styleString, "jka", styleStringSize); break;
		case 2:	Q_strncpyz(styleString, "qw", styleStringSize);	break;
		case 3:	Q_strncpyz(styleString, "cpm", styleStringSize); break;
		case 4:	Q_strncpyz(styleString, "q3", styleStringSize); break;
		case 5:	Q_strncpyz(styleString, "pjk", styleStringSize); break;
		case 6:	Q_strncpyz(styleString, "wsw", styleStringSize); break;
		case 7:	Q_strncpyz(styleString, "rjq3", styleStringSize); break;
		case 8:	Q_strncpyz(styleString, "rjcpm", styleStringSize); break;
		case 9:	Q_strncpyz(styleString, "swoop", styleStringSize); break;
		case 10: Q_strncpyz(styleString, "jetpack", styleStringSize); break;
		case 11: Q_strncpyz(styleString, "speed", styleStringSize); break;
		case 12: Q_strncpyz(styleString, "sp", styleStringSize); break;
		case 13: Q_strncpyz(styleString, "slick", styleStringSize); break;
		case 14: Q_strncpyz(styleString, "botcpm", styleStringSize); break;
		default: Q_strncpyz(styleString, "ERROR", styleStringSize); break;
	}
}

void CleanupLocalRun() { //This should never actually change anything since we insert/update properly, but just to be safe..
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	//sql = "DELETE FROM LocalRun WHERE id NOT IN (SELECT id FROM (SELECT id, MIN(duration_ms) FROM LocalRun GROUP BY username, coursename, style))";
	sql = "DELETE FROM LocalRun WHERE id NOT IN (SELECT id FROM (SELECT id, coursename, username, style, season FROM LocalRun ORDER BY duration_ms DESC) AS T GROUP BY T.username, T.coursename, T.style, T.season)";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	s = sqlite3_step(stmt);
	if (s == SQLITE_DONE)
		trap->Print("Cleaned up racetimes\n");
	else 
		G_ErrorPrint("ERROR: SQL Delete Failed (CleanupLocalRun)", s);

	CALL_SQLITE (finalize(stmt));
	//loda fixme, maybe remake table or something.. ?
	CALL_SQLITE (close(db));

	//DebugWriteToDB("CleanupLocalRun");
}

void G_GetRaceScore(int id, char *username, char *coursename, int style, int season, int time, sqlite3 * db) { //Need to use transactions here or something
	char * sql;
	sqlite3_stmt * stmt;
	int s, season_count = 0, season_rank = 0, global_count = 0, global_rank = 0, i = 1;
	
	sql = "SELECT COUNT(*) FROM LocalRun WHERE coursename = ? AND style = ? AND season = ? "
		"UNION ALL SELECT COUNT(DISTINCT username) FROM LocalRun WHERE coursename = ? AND style = ?";//Select count for that course,style
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 2, style));
	CALL_SQLITE (bind_int (stmt, 3, season));
	CALL_SQLITE (bind_text (stmt, 4, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 5, style));

	s = sqlite3_step(stmt);
	if (s == SQLITE_ROW)
		season_count = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (G_GetRaceScore 1)", s);
	}

	s = sqlite3_step(stmt);
	if (s == SQLITE_ROW)
		global_count = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (G_GetRaceScore 2)", s);
	}
	CALL_SQLITE (finalize(stmt));

	sql = "SELECT id FROM LocalRun WHERE coursename = ? AND style = ? AND season = ? ORDER BY duration_ms ASC, end_time ASC"; //assume just one per person to speed this up..
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 2, style));
	CALL_SQLITE (bind_int (stmt, 3, season));
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			if (id == sqlite3_column_int(stmt, 0)) {
				season_rank = i;
				break;
			}
			i++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            G_ErrorPrint("ERROR: SQL Select Failed (G_GetRaceScore 3)", s);
			break;
        }
    }
	CALL_SQLITE (finalize(stmt));

	i = 1; // AH HA ha

	//We dont want to count slower season entries in their global scoring, just their fastest season entry.  So either set rank to 0 for "unranked" seasons.  Or ignore it later.
	sql = "SELECT id, MIN(duration_ms) AS duration FROM LocalRun WHERE coursename = ? AND style = ? GROUP BY username ORDER BY duration ASC, end_time ASC"; 
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 2, style));
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			if (id == sqlite3_column_int(stmt, 0)) {
				global_rank = i;
				break;
			}
			i++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            G_ErrorPrint("ERROR: SQL Select Failed (G_GetRaceScore 4)", s);
			break;
        }
    }
	CALL_SQLITE (finalize(stmt));
	
	sql = "UPDATE LocalRun SET rank = ?, entries = ?, season_rank = ?, season_entries = ?, last_update = ? WHERE id = ?";//Save rank into row
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_int (stmt, 1, global_rank));
	CALL_SQLITE (bind_int (stmt, 2, global_count));
	CALL_SQLITE (bind_int (stmt, 3, season_rank));
	CALL_SQLITE (bind_int (stmt, 4, season_count));
	CALL_SQLITE (bind_int (stmt, 5, time));
	CALL_SQLITE (bind_int (stmt, 6, id));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Update Failed (G_GetRaceScore 5)", s);
	CALL_SQLITE (finalize(stmt));

}

void SV_RebuildRaceRanks_f() {
	char * sql;
    sqlite3_stmt * stmt;
	sqlite3 * db;
	int s;//, row = 0;
	time_t	rawtime;

	time( &rawtime );
	localtime( &rawtime );

	CleanupLocalRun();//Make sure no duplicate entries

	CALL_SQLITE (open (LOCAL_DB_PATH, & db)); 

	/*
	sql = "DELETE FROM RaceRanks"; //Clear RaceRanks table
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		trap->Print( "Error: Could not write to database: %i.\n", s);
	CALL_SQLITE (finalize(stmt));
	*/

	sql = "SELECT id, username, coursename, style, season FROM LocalRun ORDER BY end_time ASC";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			G_GetRaceScore(sqlite3_column_int(stmt, 0), (char*)sqlite3_column_text(stmt, 1), (char*)sqlite3_column_text(stmt, 2), sqlite3_column_int(stmt, 3), sqlite3_column_int(stmt, 4), rawtime, db);
        }
        else if (s == SQLITE_DONE)
            break;
        else {
			G_ErrorPrint("ERROR: SQL Select Failed (SV_RebuildRaceRanks_f)", s);
			break;
        }
    }
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));




	//how the fuck..
	//Make sure no duplicate entries

	//Clear RaceRanks table

	//For each entry
		//Select count for that course,style
		//Select position of that entry in ordered list of course/style (as rank)
		//save rank into row
		//Get score = count/rank
		//Add score to that username/style entry in raceScores
		//If rank =1, add a gold, etc.


}

static int G_GetSeason() {

	//We want 4 month seasons?

	return 2;
}

static void G_UpdateOurLocalRun(sqlite3 * db, int seasonOldRank_self, int seasonNewRank_self, int globalOldRank_self, int globalNewRank_self, int style_self, char *username_self, char *coursename_self, 
	int duration_ms_self, int topspeed_self, int average_self, int end_time_self, int seasonCount, int globalCount) {
	char * sql;
	sqlite3_stmt * stmt;
	int s;
	const int season = G_GetSeason();

	//Get count
	//Insert it +1 (including ourself)

	//If it is our best time of all seasons (if globalNewRank_self), we need to make our other season entries set to rank=0 !!!!
	//This also needs to update lastupdatetime for the website!
	if (globalNewRank_self && globalOldRank_self) { //And globalOldRankSelf ? - We dont want to update other rows if we dont have any other rows
		sql = "UPDATE LocalRun SET rank = 0, last_update = ? WHERE username = ? AND coursename = ? AND style = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, end_time_self));
		CALL_SQLITE (bind_text (stmt, 2, username_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 3, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 4, style_self));
		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOurLocalRun 1)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}

	if (seasonOldRank_self == -1) { //First attempt of the season
		sql = "INSERT INTO LocalRun (username, coursename, duration_ms, topspeed, average, style, season, end_time, rank, entries, season_rank, season_entries, last_update) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, username_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, duration_ms_self));
		CALL_SQLITE (bind_int (stmt, 4, topspeed_self));
		CALL_SQLITE (bind_int (stmt, 5, average_self));
		CALL_SQLITE (bind_int (stmt, 6, style_self));
		CALL_SQLITE (bind_int (stmt, 7, season));
		CALL_SQLITE (bind_int (stmt, 8, end_time_self));
		CALL_SQLITE (bind_int (stmt, 9, globalNewRank_self));
		CALL_SQLITE (bind_int (stmt, 10, globalCount));
		CALL_SQLITE (bind_int (stmt, 11, seasonNewRank_self));
		CALL_SQLITE (bind_int (stmt, 12, seasonCount));
		CALL_SQLITE (bind_int (stmt, 13, end_time_self));
		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			char string[1024] = {0};

			Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i;%i\n", username_self, coursename_self, duration_ms_self, topspeed_self, average_self, style_self, season, end_time_self);
			trap->FS_Write( string, strlen( string ), level.failRaceLog );

			G_ErrorPrint("ERROR: SQL Insert Failed (G_UpdateOurLocalRun 2)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}
	else {
		sql = "UPDATE LocalRun SET duration_ms = ?, topspeed = ?, average = ?, end_time = ?, rank = ?, season_rank = ?, last_update = ? WHERE username = ? AND coursename = ? AND style = ? AND season = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, duration_ms_self));
		CALL_SQLITE (bind_int (stmt, 2, topspeed_self));
		CALL_SQLITE (bind_int (stmt, 3, average_self));
		CALL_SQLITE (bind_int (stmt, 4, end_time_self));
		CALL_SQLITE (bind_int (stmt, 5, globalNewRank_self));
		CALL_SQLITE (bind_int (stmt, 6, seasonNewRank_self));
		CALL_SQLITE (bind_int (stmt, 7, end_time_self));
		CALL_SQLITE (bind_text (stmt, 8, username_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 9, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 10, style_self));
		CALL_SQLITE (bind_int (stmt, 11, season));
		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			char string[1024] = {0};

			Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i;%i\n", username_self, coursename_self, duration_ms_self, topspeed_self, average_self, style_self, season, end_time_self);
			trap->FS_Write( string, strlen( string ), level.failRaceLog );

			G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOurLocalRun 3)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}

}

static void G_UpdateOtherLocalRun(sqlite3 * db, int seasonNewRank_self, int seasonOldRank_self, int globalNewRank_self, int globalOldRank_self, int style_self, char *coursename_self, int time) {
	char * sql;
	sqlite3_stmt * stmt;
	int s;
	const int season = G_GetSeason();

	//Problem? someone completeing a first attempt of the season which is also their first global attempt.  This affects the entries of people from other seasons?
	
	if (seasonOldRank_self == -1) //Our first attempt of the season, so do this to THEM
		sql = "UPDATE LocalRun SET season_rank = season_rank + 1, last_update = ? WHERE coursename = ? and style = ? AND season = ? AND season_rank >= ?";
	else
		sql = "UPDATE LocalRun SET season_rank = season_rank + 1, last_update = ? WHERE coursename = ? and style = ? AND season = ? AND season_rank >= ? AND season_rank < ?";

	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_int (stmt, 1, time));
	CALL_SQLITE (bind_text (stmt, 2, coursename_self, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, style_self));
	CALL_SQLITE (bind_int (stmt, 4, season));
	CALL_SQLITE (bind_int (stmt, 5, seasonNewRank_self));

	if (seasonOldRank_self != -1)
		CALL_SQLITE (bind_int (stmt, 6, seasonOldRank_self));

	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOtherLocalRun)", s);
	}
	CALL_SQLITE (finalize(stmt));

	//Should not care about what season we update if its a global pb ?
	if (globalNewRank_self) { //Dont update other peoples global ranks if our run was not a global personal best...
		if (globalOldRank_self == -1) //Our first attempt overall
			sql = "UPDATE LocalRun SET rank = rank + 1, last_update = ? WHERE coursename = ? and style = ? AND rank >= ?";
		else
			sql = "UPDATE LocalRun SET rank = rank + 1, last_update = ? WHERE coursename = ? and style = ? AND rank >= ? AND rank < ?";

		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, time));
		CALL_SQLITE (bind_text (stmt, 2, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style_self));
		CALL_SQLITE (bind_int (stmt, 4, globalNewRank_self));

		if (globalOldRank_self != -1)
			CALL_SQLITE (bind_int (stmt, 5, globalOldRank_self));

		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOtherLocalRun 2)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}

	//loda this can be combined with above query probably
	if (seasonOldRank_self == -1) { //First attempt  
		sql = "UPDATE LocalRun SET season_entries = season_entries + 1, last_update = ? WHERE coursename = ? AND style = ? AND season = ?";
	//+1 count for all, +1 rank only if affected
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, time));
		CALL_SQLITE (bind_text (stmt, 2, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style_self));
		CALL_SQLITE (bind_int (stmt, 4, season));

		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOtherLocalRun 3)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}


	if (globalOldRank_self == -1) { //First attempt  
		sql = "UPDATE LocalRun SET entries = entries + 1, last_update = ? WHERE coursename = ? AND style = ?";
	//+1 count for all, +1 rank only if affected
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int (stmt, 1, time));
		CALL_SQLITE (bind_text (stmt, 2, coursename_self, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style_self));

		s = sqlite3_step(stmt);
		if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Update Failed (G_UpdateOtherLocalRun 4)", s);
		}
		CALL_SQLITE (finalize(stmt));
	}

}

void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMs) { 
	if (duration_ms > (60*60*1000)) { //thanks, eternal
		int hours, minutes, seconds, milliseconds; 
		hours = (int)((duration_ms / (1000*60*60)) % 24); //wait wut
		minutes = (int)((duration_ms / (1000*60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000; 
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i", hours, minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i.%03i", hours, minutes, seconds, milliseconds);
	}
	else if (duration_ms > (60*1000)) {
		int minutes, seconds, milliseconds;
		minutes = (int)((duration_ms / (1000*60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000; 
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i", minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i.%03i", minutes, seconds, milliseconds);
	}
	else {
		if (noMs)
			Q_strncpyz(timeStr, va("%.0f", ((float)duration_ms * 0.001)), strSize);
		else
			Q_strncpyz(timeStr, va("%.3f", ((float)duration_ms * 0.001)), strSize);
	}
}

void PrintRaceTime(char *username, char *playername, char *message, char *style, int topspeed, int average, char *timeStr, int clientNum, int season_newRank, qboolean spb, int global_newRank, qboolean loggedin, qboolean valid, int season_oldRank, int global_oldRank, float addedScore) {
	int nameColor, color;
	char awardString[28] = {0}, messageStr[64] = {0}, nameStr[32] = {0};

	//TODO print rank increase

	//Com_Printf("SOldrank %i SNewrank %i GOldrank %i GNewrank %i Addscore %.1f\n", season_oldRank, season_newRank, global_oldRank, global_newRank, addedScore);

	if (global_newRank == 1) //WR, Play the sound
		PlayActualGlobalSound("sound/chars/rosh_boss/misc/victory3");
	else if (global_newRank > 0) //PB
		PlayActualGlobalSound("sound/chars/rosh/misc/taunt1");

	nameColor = 7 - (clientNum % 8);//sad hack
	if (nameColor < 2)
		nameColor = 2;
	else if (nameColor > 7 || nameColor == 5)
		nameColor = 7;

	if (valid && loggedin)
		color = 5;
	else if (valid)
		color = 2;
	else
		color = 1;

	if (username)
		Com_sprintf(nameStr, sizeof(nameStr), "%s", username);
	else
		Com_sprintf(nameStr, sizeof(nameStr), "%s", playername);

	if (message)
		Com_sprintf(messageStr, sizeof(messageStr), "^3%-16s^%i completed", message, color);
	else
		Com_sprintf(messageStr, sizeof(messageStr), "^%iCompleted", color);

	if (valid) {
		if (global_newRank == 1) { //was 1 when it shouldnt have been.. ?
			Q_strncpyz(awardString, "^5(WR)", sizeof(awardString));
		}
		else if (season_newRank == 1 && global_newRank > 0) {
			Q_strncpyz(awardString, "^5(SR+PB)", sizeof(awardString));
		}
		else if (season_newRank == 1) {
			Q_strncpyz(awardString, "^5(SR)", sizeof(awardString));
		}
		else if (global_newRank > 0) {
			Q_strncpyz(awardString, "^5(PB)", sizeof(awardString));
		}
		else if (season_newRank > 0) {
			Q_strncpyz(awardString, "^5(SPB)", sizeof(awardString));
		}

		if (global_newRank > 0) { //Print global rank increased, global score added
			if (global_newRank != global_oldRank) {//Can be from -1 to #.  What do we do in this case..
				if (global_oldRank > 0)
					Q_strcat(awardString, sizeof(awardString), va(" (%i->%i +%.1f)", global_oldRank, global_newRank, addedScore));
				else
					Q_strcat(awardString, sizeof(awardString), va(" (%i +%.1f)", global_newRank, addedScore));
			}
		}
		else if (season_newRank > 0) {//Print season rank increased, global score added
			if (season_newRank != season_oldRank) {
				if (season_oldRank > 0)
					Q_strcat(awardString, sizeof(awardString), va(" (%i->%i +%.1f)", season_oldRank, season_newRank, addedScore));
				else
					Q_strcat(awardString, sizeof(awardString), va(" (%i +%.1f)", season_newRank, addedScore));
			}
		}

	}

	trap->SendServerCommand( -1, va("print \"%s in ^3%-12s^%i max:^3%-10i^%i avg:^3%-10i^%i style:^3%-10s^%i by ^%i%s %s^7\n\"",
				messageStr, timeStr, color, topspeed, color, average, color, style, color, nameColor, nameStr, awardString));
}

void G_UpdatePlaytime(sqlite3 *db, char *username, int seconds ) {
	char * sql;
	sqlite3_stmt * stmt;
	int s;
	qboolean newDB = qfalse;

	if (!db) {
		CALL_SQLITE (open (LOCAL_DB_PATH, & db)); 
		newDB = qtrue;
	}
	
	sql = "UPDATE LocalAccount SET racetime = racetime + ? WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_int (stmt, 1, seconds));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));

	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Update Failed (G_UpdatePlaytime)", s);
	}

	CALL_SQLITE (finalize(stmt));
	if (newDB) {
		CALL_SQLITE (close(db));
	}
}

void StripWhitespace(char *s);
void G_AddRaceTime(char *username, char *message, int duration_ms, int style, int topspeed, int average, int clientNum) {//should be short.. but have to change elsewhere? is it worth it?
	time_t	rawtime;
	char	string[1024] = {0}, info[1024] = {0}, coursename[40], timeStr[32] = {0}, styleString[32] = {0};
	qboolean seasonPB = qfalse, globalPB = qfalse, WR = qfalse;
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;
	int season_oldBest, season_oldRank, season_newRank = -1, global_oldBest, global_oldRank, global_newRank = -1; //Changed newrank to be -1 ??
	float addedScore;
	gclient_t	*cl;
	const int season = G_GetSeason();

	cl = &level.clients[clientNum];

	time( &rawtime );
	localtime( &rawtime );

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(coursename, Info_ValueForKey( info, "mapname" ), sizeof(coursename));

	if (message) {// [0]?
		Q_strlwr(message);
		Q_CleanStr(message);
		Q_strcat(coursename, sizeof(coursename), va(" (%s)", message));
	}

	if (average > topspeed) {
		average = topspeed; //need to sample speeds every clientframe.. but how to calculate average if client frames are not evenly spaced.. can use pml.msec ?
	}

	Q_strlwr(coursename);
	Q_CleanStr(coursename);

	IntegerToRaceName(style, styleString, sizeof(styleString));

	Com_sprintf(string, sizeof(string), "%s;%s;%i;%i;%i;%i;%i\n", username, coursename, duration_ms, topspeed, average, style, rawtime);

	if (level.raceLog)
		trap->FS_Write(string, strlen(string), level.raceLog ); //Always write to text file races.log

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "SELECT MIN(duration_ms), season_rank FROM LocalRun WHERE username = ? AND coursename = ? AND style = ? AND season = ? "
		"UNION ALL SELECT MIN(duration_ms), rank FROM LocalRun WHERE username = ? AND coursename = ? AND style = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, style));
	CALL_SQLITE (bind_int (stmt, 4, season));
	CALL_SQLITE (bind_text (stmt, 5, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 6, coursename, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 7, style));

	s = sqlite3_step(stmt);

	if (s == SQLITE_ROW) {
		season_oldBest = sqlite3_column_int(stmt, 0);
		season_oldRank = sqlite3_column_int(stmt, 1);

		//trap->Print("Oldbest, Duration_ms: %i, %i\n", oldBest, duration_ms);

		if (season_oldBest) {// We found a time in the database
			if (duration_ms < season_oldBest) { //our time we just recorded is faster, so log it			
				seasonPB = qtrue;
			}
		}
		else { //No time found in database, so record the time we just recorded 
			seasonPB = qtrue;
			season_oldRank = -1;
		}
	}
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 1)", s);
	}

	s = sqlite3_step(stmt);

	if (s == SQLITE_ROW) {
		global_oldBest = sqlite3_column_int(stmt, 0);
		global_oldRank = sqlite3_column_int(stmt, 1);

		//trap->Print("Oldbest, Duration_ms: %i, %i\n", oldBest, duration_ms);

		if (global_oldBest) {// We found a time in the database
			if (duration_ms < global_oldBest) { //our time we just recorded is faster, so log it			
				globalPB = qtrue;
			}
		}
		else { //No time found in database, so record the time we just recorded 
			globalPB = qtrue;
			global_oldRank = -1;
		}
	}
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 2)", s);
	}
	CALL_SQLITE (finalize(stmt));


	if (seasonPB) {
		int season_oldCount, season_newCount, global_oldCount, global_newCount;
		int i = 1; //1st place is rank 1

		sql = "SELECT COUNT(*) FROM LocalRun WHERE coursename = ? AND style = ? AND season = ? "
			"UNION ALL SELECT COUNT(DISTINCT username) FROM LocalRun WHERE coursename = ? AND style = ?"; //entries ?
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, style));
		CALL_SQLITE (bind_int (stmt, 3, season));
		CALL_SQLITE (bind_text (stmt, 4, coursename, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 5, style));
		s = sqlite3_step(stmt);
		if (s == SQLITE_ROW) {
			season_oldCount = sqlite3_column_int(stmt, 0);
		}
		else if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 3)", s);
		}

		s = sqlite3_step(stmt);
		if (s == SQLITE_ROW) {
			global_oldCount = sqlite3_column_int(stmt, 0);
		}
		else if (s != SQLITE_DONE) {
			G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 4)", s);
		}

		CALL_SQLITE (finalize(stmt));

		//Get season rank
		sql = "SELECT duration_ms FROM LocalRun WHERE coursename = ? AND style = ? AND season = ? ORDER BY duration_ms ASC, end_time ASC"; //assume just one per person to speed this up..
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, style));
		CALL_SQLITE (bind_int (stmt, 3, season));
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				season_newRank = 0; //Make sure this doesnt reset a set newrank, but it wont since we break after setting
				if (duration_ms < sqlite3_column_int(stmt, 0)) { //We are faster than this time... If we dont find anything newrank stays -1
					season_newRank = i;
					break;
				}
				i++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 5)", s);
				break;
			}
		}
		CALL_SQLITE (finalize(stmt));

		i = 1; //oh no no

		//Get global rank, could union this with previous query maybe
		sql = "SELECT MIN(duration_ms) FROM LocalRun WHERE coursename = ? AND style = ? GROUP BY username ORDER BY duration_ms ASC, end_time ASC";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, coursename, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, style));
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				global_newRank = 0; //Make sure this doesnt reset a set newrank, but it wont since we break after setting --  what?
				if (duration_ms < sqlite3_column_int(stmt, 0)) { //We are faster than this time... If we dont find anything newrank stays -1
					global_newRank = i;
					break;
				}
				i++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (G_AddRaceTime 6)", s);
				break;
			}
		}
		CALL_SQLITE (finalize(stmt));

		if (season_newRank == 0) { //We wern't faster than any times, so set our rank to count (+ 1) ? -- loda checkme
			season_newRank = season_oldCount + 1;
		}
		if (season_newRank == -1) {//We didnt find any times, so we are first -- ?
			season_newRank = 1;
		}

		if (season_oldRank == -1) {
			season_newCount = season_oldCount + 1;
		}
		else {
			season_newCount = season_oldCount;
		}
		//--------------
		if (global_newRank == 0) { //We wern't faster than any times, so set our rank to count (+ 1) ? -- loda checkme
			global_newRank = global_oldCount + 1;
		}
		if (global_newRank == -1) {//We didnt find any times, so we are first -- ?
			global_newRank = 1;
		}

		if (global_oldRank == -1) {
			global_newCount = global_oldCount + 1;
		}
		else {
			global_newCount = global_oldCount;
		}

		//If this isnt our best time of all seasons, set rank to 0 so we wont get it in global queries.
		if (!globalPB) {
			global_newRank = 0;
		}

		if ((season_newRank != season_oldRank || global_newRank != global_oldRank)) { //Do this before messing with out race list rank - does this affect count?
			G_UpdateOtherLocalRun(db, season_newRank, season_oldRank, global_newRank, global_oldRank, style, coursename, rawtime); //Update other spots in race list
		}
		G_UpdateOurLocalRun(db, season_oldRank, season_newRank, global_oldRank, global_newRank, style, username, coursename, duration_ms, topspeed, average, rawtime, season_newCount, global_newCount);//Update our race list

		if (cl->pers.recordingDemo && globalPB) {
			char mapCourse[MAX_QPATH] = {0};

			Q_strncpyz(mapCourse, coursename, sizeof(mapCourse));
			StripWhitespace(mapCourse);
			Q_strstrip( mapCourse, "\n\r;:.?*<>|\\/\"", NULL );

			if (cl) {
				cl->pers.stopRecordingTime = level.time + 2000;
				cl->pers.keepDemo = qtrue;
				Com_sprintf(cl->pers.oldDemoName, sizeof(cl->pers.oldDemoName), "%s", cl->pers.userName);
				Com_sprintf(cl->pers.demoName, sizeof(cl->pers.demoName), "%s/%s-%s-%s", cl->pers.userName, cl->pers.userName, mapCourse, styleString); //TODO, change this to %s/%s-%s-%s so its puts in individual players folder
			}
		}

		//For print
		if (global_newRank > 0) {
			addedScore = ((global_newCount / (float)global_newRank) + (global_newCount - global_newRank)) * 0.5f; //Add new score
			if (global_oldRank > 0)
				addedScore -= ((global_oldCount / (float)global_oldRank) + (global_oldCount - global_oldRank)) * 0.5f; //Subtract old score, if there was one
		}
		else if (season_newRank > 0) {
			addedScore = ((season_newCount / (float)season_newRank) + (season_newCount - season_newRank)) * 0.5f;
			if (season_oldRank > 0)
				addedScore -= ((season_oldCount / (float)season_oldRank) + (season_oldCount - season_oldRank)) * 0.5f;
		}

	}
	//else.. set ranks to 0 for print, nothing to update

	cl->pers.stats.racetime += (duration_ms*0.001f) - cl->afkDuration*0.001f;
	cl->afkDuration = 0;
	if (cl->pers.stats.racetime > 120.0f) { //Avoid spamming the db
		G_UpdatePlaytime(db, username, (int)(cl->pers.stats.racetime+0.5f));
		cl->pers.stats.racetime = 0.0f;
	}
	
	CALL_SQLITE (close(db));

	TimeToString((int)(duration_ms), timeStr, sizeof(timeStr), qfalse);
	PrintRaceTime(username, cl->pers.netname, message, styleString, topspeed, average, timeStr, clientNum, season_newRank, seasonPB, global_newRank, qtrue, qtrue, season_oldRank, global_oldRank, addedScore);
	//DebugWriteToDB("G_AddRaceTime");
}

#if 0
void G_TestAddRace() {
	char username[40], coursename[40], input[16];
	int style, duration_ms, average, topspeed, end_time, oldrank, newrank;

	if (trap->Argc() != 7) {
		Com_Printf ("Usage: /addrace <username> <coursename> <style> <duration> <average> <topspeed>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, coursename, sizeof(coursename));

	trap->Argv(3, input, sizeof(input));
	style = atoi(input);
		
	trap->Argv(4, input, sizeof(input));
	duration_ms = atoi(input);

	trap->Argv(5, input, sizeof(input));
	average = atoi(input);

	trap->Argv(6, input, sizeof(input));
	topspeed = atoi(input);

	trap->Argv(7, input, sizeof(input));
	end_time = atoi(input);

	trap->Argv(8, input, sizeof(input));
	oldrank = atoi(input);

	trap->Argv(9, input, sizeof(input));
	newrank = atoi(input);

	G_AddRaceTime(username, coursename, duration_ms, style, topspeed, average, 0);
	//G_AddNewRaceToDB(username, coursename, style, duration_ms, average, topspeed, end_time, oldrank, newrank, 0);
}

#endif

//So the best way is to probably add every run as soon as its taken and not filter them.
//to cut down on database size, there should be a cleanup on every mapload or.. every week..or...?
//which removes any time not in the top 100 of its category AND more than 1 week old?

void ResetPlayerTimers(gentity_t *ent, qboolean print);//extern ?
void Cmd_ACLogin_f( gentity_t *ent ) { //loda fixme show lastip ? or use lastip somehow
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0, s, count = 0, i, key;
	unsigned int ip, lastip = 0;
	char username[16], enteredPassword[16], password[16], strIP[NET_ADDRSTRMAXLEN] = {0}, enteredKey[32];
	char *p = NULL;
	gclient_t	*cl;
	int flags = 0;

	if (!ent->client)
		return;

	if (trap->Argc() != 3 && trap->Argc() != 4) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /login <username> <password>\n\"");
		return;
	}

	if (Q_stricmp(ent->client->pers.userName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, enteredPassword, sizeof(password));

	trap->Argv(3, enteredKey, sizeof(enteredKey));
	key = atoi(enteredKey);
	if (key && sv_pluginKey.integer > 0) {
		int time = (ent->client->pers.cmd.serverTime + 500) / 1000 * 1000;
		int mod = sv_pluginKey.integer / 1000;
		int add = sv_pluginKey.integer % 1000;

		if (mod > 0) {
			//trap->Print("Client logged in with key: %i and time %i correct key is %i\n", key, time, (time % mod) + add);
			if (key == (time % mod) + add) {
				ent->client->pers.validPlugin = qtrue;
				//trap->Print("Valid login\n");
			}
			else
				ent->client->pers.validPlugin = qfalse;
		}
	}

	Q_strlwr(username);
	Q_CleanStr(username);

	Q_CleanStr(enteredPassword);

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
			G_ErrorPrint("ERROR: SQL Select Failed (Cmd_ACLogin_f 1)", s);
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}

		CALL_SQLITE (finalize(stmt));
	}

	sql = "SELECT password, lastip, flags FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			Q_strncpyz(password, (char*)sqlite3_column_text(stmt, 0), sizeof(password));
			lastip = sqlite3_column_int(stmt, 1);
			flags = (qboolean)sqlite3_column_int(stmt, 2);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            G_ErrorPrint("ERROR: SQL Select Failed (Cmd_ACLogin_f 2)", s);
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
		trap->Print("WARNING: Multiple accounts with same name!\n");
		CALL_SQLITE (close(db));
		return;
	}

	if (!(flags & JAPRO_ACCOUNTFLAG_TRUSTED) && (count > 0) && lastip && ip && (lastip != ip)) { //IF lastip already tied to account, and lastIP (of attempted login username) does not match current IP, deny.?
		trap->SendServerCommand(ent-g_entities, "print \"Your IP address already belongs to an account. You are only allowed one account.\n\"");
		CALL_SQLITE (close(db));
		return;
	}

	if ((flags & JAPRO_ACCOUNTFLAG_IPLOCK) && lastip != ip) {
		trap->SendServerCommand(ent-g_entities, "print \"This account is locked to a different IP address.\n\"");
		CALL_SQLITE (close(db));
		return;
	}

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clientsv - use numplayingclients fixme
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (!Q_stricmp(username, cl->pers.userName)) {
			trap->SendServerCommand(ent-g_entities, "print \"This account is already logged in!\n\"");
			CALL_SQLITE (close(db));
			return;
		}
	}
		
	if (enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		time_t	rawtime;

		time( &rawtime );
		localtime( &rawtime );

		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
		if (ent->client->sess.raceMode && ent->client->pers.stats.startTime) {
			ResetPlayerTimers(ent, qtrue);
			trap->SendServerCommand(ent-g_entities, "print \"Login sucessful. Time reset.\n\"");
		}
		else
			trap->SendServerCommand(ent-g_entities, "print \"Login sucessful.\n\"");

		if (!ip) //meh
			ip = lastip;

		sql = "UPDATE LocalAccount SET lastip = ?, lastlogin = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_int64 (stmt, 1, ip));
		CALL_SQLITE (bind_int (stmt, 2, rawtime));
		CALL_SQLITE (bind_text (stmt, 3, username, -1, SQLITE_STATIC));

		s = sqlite3_step(stmt);

		if (s != SQLITE_DONE)
			G_ErrorPrint("ERROR: SQL Update Failed (Cmd_ACLogin_f 3)", s);

		CALL_SQLITE (finalize(stmt));

	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect password!\n\"");
	}	
	CALL_SQLITE (close(db));

	//DebugWriteToDB("Cmd_ACLogin_f");
}

void Cmd_ChangePassword_f( gentity_t *ent ) {
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0, s;
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
	trap->Argv(2, enteredPassword, sizeof(enteredPassword));
	trap->Argv(3, newPassword, sizeof(newPassword));

	Q_strlwr(username);
	Q_CleanStr(username);

	if (Q_stricmp(ent->client->pers.userName, username)) {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect username!\n\"");
		return;
	}

	Q_CleanStr(enteredPassword);
	Q_CleanStr(newPassword);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT password FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, ent->client->pers.userName, -1, SQLITE_STATIC));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			Q_strncpyz(password, (char*)sqlite3_column_text(stmt, 0), sizeof(password));
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            G_ErrorPrint("ERROR: SQL Select Failed (Cmd_ChangePassword_f 1)", s);
			break;
        }
    }

	CALL_SQLITE (finalize(stmt));

	if (enteredPassword[0] && password[0] && !Q_stricmp(enteredPassword, password)) {
		int s;

		sql = "UPDATE LocalAccount SET password = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, newPassword, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, ent->client->pers.userName, -1, SQLITE_STATIC));
		s = sqlite3_step(stmt);
		if (s == SQLITE_DONE)
			trap->SendServerCommand(ent-g_entities, "print \"Password Changed.\n\""); //loda fixme check if this executed
		else
			G_ErrorPrint("ERROR: SQL Update Failed (Cmd_ChangePassword_f 2)", s);

		CALL_SQLITE (finalize(stmt));
	}
	else {
		trap->SendServerCommand(ent-g_entities, "print \"Incorrect password!\n\"");
	}	
	CALL_SQLITE (close(db));

	//DebugWriteToDB("Cmd_ChangePassword_f");
}

void Svcmd_ChangePass_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], newPassword[16];
	int s;

	if (trap->Argc() != 3) {
		trap->Print( "Usage: /changepassword <username> <newpassword>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, newPassword, sizeof(newPassword));

	Q_strlwr(username);
	Q_CleanStr(username);

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
	s = sqlite3_step(stmt);
	if (s == SQLITE_DONE)
			trap->Print( "Password changed.\n");
	else
		G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_ChangePass_f)", s);

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}

void Svcmd_ClearIP_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16];
	int s;

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
	s = sqlite3_step(stmt);

	if (s == SQLITE_DONE)
		trap->Print( "IP Cleared.\n");
	else
		G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_ClearIP_f)", s);

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}

void Svcmd_Register_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], password[16];
	time_t	rawtime;
	int s;

	if (trap->Argc() != 3) {
		trap->Print( "Usage: /register <username> <password>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, password, sizeof(password));

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strstrip(username, " \n\r;:.?*<>!#$&'()+@=`~{}[]^_|\\/\"", NULL);

	Q_CleanStr(password);

	if (!username[0]) {
		return;
	}
	if (!Q_stricmp(username, password)) {
		trap->Print("Username and password cannot be the same\n");
		return;
	}
	if (CheckUserExists(username)) {
		trap->Print( "User already exists!\n");
		return;
	}

	time( &rawtime );
	localtime( &rawtime );

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "INSERT INTO LocalAccount (username, password, kills, deaths, suicides, captures, returns, racetime, created, lastlogin, lastip) VALUES (?, ?, 0, 0, 0, 0, 0, 0, ?, ?, 0)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, password, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, rawtime));
	CALL_SQLITE (bind_int (stmt, 4, rawtime));
	s = sqlite3_step(stmt);

	if (s == SQLITE_DONE)
		trap->Print( "Account created.\n");
	else
		G_ErrorPrint("ERROR: SQL Insert Failed (Svcmd_Register_f)", s);

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));
}

void Svcmd_DeleteAccount_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], confirm[16];
	int s;

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
		s = sqlite3_step(stmt);
		if (s == SQLITE_DONE)
			trap->Print( "Account deleted.\n");
		else 
			G_ErrorPrint("ERROR: SQL Delete Failed (Svcmd_DeleteAccount_f 1)", s);
		CALL_SQLITE (finalize(stmt));
	}
	else 
		trap->Print( "User does not exist, deleting highscores for username anyway.\n");

	sql = "DELETE FROM LocalRun WHERE username = ?";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
    
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Delete Failed (Svcmd_DeleteAccount_f 2)", s);
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));
}

void Svcmd_RenameAccount_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], newUsername[16], confirm[16];
	int s;

	if (trap->Argc() != 4) {
		trap->Print( "Usage: /renameAccount <username> <new username> <confirm>\n");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	trap->Argv(2, newUsername, sizeof(newUsername));
	trap->Argv(3, confirm, sizeof(confirm));

	if (Q_stricmp(confirm, "confirm")) {
		trap->Print( "Usage: /renameAccount <username> <new username> <confirm>\n");
		return;
	}

	Q_strlwr(username);
	Q_CleanStr(username);

	Q_strlwr(newUsername);
	Q_CleanStr(newUsername);
	Q_strstrip(username, " \n\r;:.?*<>!#$&'()+@=`~{}[]^_|\\/\"", NULL);

	if (CheckUserExists(newUsername)) {
		trap->Print( "ERROR: Desired username already exists.\n");
	}

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	
	if (CheckUserExists(username)) {
		sql = "UPDATE LocalAccount SET username = ? WHERE username = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, newUsername, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
		s = sqlite3_step(stmt);
		if (s == SQLITE_DONE)
			trap->Print( "Account renamed.\n");
		else 
			G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_RenameAccount_f 1)", s);
		CALL_SQLITE (finalize(stmt));
	}
	else 
		trap->Print( "User does not exist, renaming in races and duels anyway.\n");

	sql = "UPDATE LocalRun SET username = ? WHERE username = ?";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, newUsername, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
    
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_RenameAccount_f 2)", s);

	CALL_SQLITE (finalize(stmt));

	sql = "UPDATE LocalDuel SET winner = ? WHERE winner = ?";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, newUsername, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
    
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_RenameAccount_f 3)", s);

	CALL_SQLITE (finalize(stmt));

	sql = "UPDATE LocalDuel SET loser = ? WHERE loser = ?";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, newUsername, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
    
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_RenameAccount_f 4)", s);

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
	int s;

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
	
    s = sqlite3_step(stmt);
    if (s == SQLITE_ROW) {
		lastlogin = sqlite3_column_int(stmt, 0);
		lastip = sqlite3_column_int(stmt, 1);
    }
    else if (s != SQLITE_DONE){
        G_ErrorPrint("ERROR: SQL Select Failed (Svcmd_AccountInfo_f)", s);
    }

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	getDateTime(lastlogin, timeStr, sizeof(timeStr));

	Q_strncpyz(buf, va("Stats for %s:\n", username), sizeof(buf));
	Q_strcat(buf, sizeof(buf), va("   ^5Last login: ^2%s\n", timeStr));
	Q_strcat(buf, sizeof(buf), va("   ^5Last IP^3: ^2%u\n", lastip));

	trap->Print( "%s", buf);
}

void Svcmd_AccountIPLock_f(void) {
	sqlite3 * db;
	char * sql;
    sqlite3_stmt * stmt;
	int s;
	char username[16];
	int flags = 0;

	if (trap->Argc() != 2) {
		trap->Print( "Usage: /iplock <username>\n");
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
	sql = "SELECT flags FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    s = sqlite3_step(stmt);
    if (s == SQLITE_ROW) {
		flags = (qboolean)sqlite3_column_int(stmt, 0);
    }
    else if (s != SQLITE_DONE){
        G_ErrorPrint("ERROR: SQL Select Failed (Svcmd_AccountIPLock_f 1)", s);
    }
	CALL_SQLITE (finalize(stmt));

	if (flags & JAPRO_ACCOUNTFLAG_IPLOCK) 
		sql = "UPDATE LocalAccount SET flags = 0 WHERE username = ?"; //loda redo this
	else 
		sql = "UPDATE LocalAccount SET flags = 1 WHERE username = ?";

	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	s = sqlite3_step(stmt);
	if (s == SQLITE_DONE) {
		if (flags & JAPRO_ACCOUNTFLAG_IPLOCK) 
			trap->Print( "IP unlocked.\n");
		else
			trap->Print( "IP locked.\n");
    }
    else {
        G_ErrorPrint("ERROR: SQL Update Failed (Svcmd_AccountIPLock_f 2)", s);
    }

	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

}

void Svcmd_DBInfo_f(void)
{
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s, numAccounts = 0, numRaces = 0, numDuels = 0;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT COUNT(*) FROM LocalAccount";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
		numAccounts = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (Svcmd_DBInfo_f 1)", s);
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
		return;
	}
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "SELECT COUNT(*) FROM LocalRun";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
		numRaces = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (Svcmd_DBInfo_f 2)", s);
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
		return;
	}
	CALL_SQLITE (finalize(stmt));

	sql = "SELECT COUNT(*) FROM LocalDuel";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    s = sqlite3_step(stmt);
    if (s == SQLITE_ROW)
		numDuels = sqlite3_column_int(stmt, 0);
	else if (s != SQLITE_DONE) {
		G_ErrorPrint("ERROR: SQL Select Failed (Svcmd_DBInfo_f 3)", s);
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
		return;
	}
	CALL_SQLITE (finalize(stmt));

	CALL_SQLITE (close(db));

	trap->Print( "There are %i accounts, %i race records, and %i duels in the database.\n", numAccounts, numRaces, numDuels);
}

void Cmd_ACRegister_f( gentity_t *ent ) { //Temporary, until global shit is done
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	char username[16], password[16], strIP[NET_ADDRSTRMAXLEN] = {0};
	char *p = NULL;
	time_t	rawtime;
	int s;
	unsigned int ip;

	if (!g_allowRegistration.integer) {
		trap->SendServerCommand(ent-g_entities, "print \"This server does not allow registration\n\"");
		return;
	}
		
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

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strstrip(username, " \n\r;:.?*<>!#$&'()+@=`~{}[]^_|\\/\"", NULL);

	Q_CleanStr(password);

	if (!username[0]) {
		return;
	}
	if (!Q_stricmp(username, password)) {
		trap->SendServerCommand(ent-g_entities, "print \"Username and password cannot be the same\n\"");
		return;
	}
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
		CALL_SQLITE (bind_int64 (stmt, 1, ip));

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
			G_ErrorPrint("ERROR: SQL Select Failed (Cmd_ACRegister_f 1)", s);
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}
		CALL_SQLITE (finalize(stmt));
	}

    sql = "INSERT INTO LocalAccount (username, password, kills, deaths, suicides, captures, returns, racetime, created, lastlogin, lastip) VALUES (?, ?, 0, 0, 0, 0, 0, 0, ?, ?, ?)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_text (stmt, 2, password, -1, SQLITE_STATIC));
	CALL_SQLITE (bind_int (stmt, 3, rawtime));
	CALL_SQLITE (bind_int (stmt, 4, rawtime));
	CALL_SQLITE (bind_int64 (stmt, 5, ip));
	s = sqlite3_step(stmt);

	if (s == SQLITE_DONE) {
		trap->SendServerCommand(ent-g_entities, "print \"Account created.\n\"");
		Q_strncpyz(ent->client->pers.userName, username, sizeof(ent->client->pers.userName));
	}
	else
		G_ErrorPrint("ERROR: SQL Insert Failed (Cmd_ACRegister_f 1)", s);


	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));

	//DebugWriteToDB("Cmd_ACRegister_f");
}

void Cmd_ACLogout_f( gentity_t *ent ) { //If logged in, print logout msg, remove login status.
	if (ent->client->pers.userName && ent->client->pers.userName[0]) {
		if (ent->client->sess.raceMode && ent->client->pers.stats.startTime) {
			ent->client->pers.stats.racetime += (trap->Milliseconds() - ent->client->pers.stats.startTime)*0.001f - ent->client->afkDuration*0.001f;
			ent->client->afkDuration = 0;
		}
		if (ent->client->pers.stats.racetime >= 1.0f) {
			G_UpdatePlaytime(0, ent->client->pers.userName, (int)(ent->client->pers.stats.racetime+0.5f));
			ent->client->pers.stats.racetime = 0.0f;
		}

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
	char username[16], pageStr[8];
	char timeStr[64] = {0}, dateStr[64] = {0};
	int s, page = 1, start, lastlogin = 0, row = 0;

	if (trap->Argc() != 2 && trap->Argc() != 3) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /stats <username> <page (optional)>\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));
	Q_strlwr(username);
	Q_CleanStr(username);

	if (trap->Argc() == 3) {
		trap->Argv(2, pageStr, sizeof(pageStr));
		page = atoi(pageStr);

		if (page < 1 || page > 100) {
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /stats <username> <page (optional)>\n\"");
			return;
		}
	}

	start = (page - 1) * 5;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	sql = "SELECT lastlogin FROM LocalAccount WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
	
    while (1) {
        s = sqlite3_step(stmt);
        if (s == SQLITE_ROW) {
			lastlogin = sqlite3_column_int(stmt, 0);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            G_ErrorPrint("ERROR: SQL Select Failed (Cmd_Stats_f 1)", s);
			break;
        }
    }
	CALL_SQLITE (finalize(stmt));

	if (row == 0) { //no account found, or more than 1 account with same name, problem
		trap->SendServerCommand(ent-g_entities, "print \"Account not found!\n\"");
		CALL_SQLITE (close(db));
		return;
	}
	else if (row > 1) {
		trap->SendServerCommand(ent-g_entities, "print \"ERROR: Multiple accounts found!\n\"");
		CALL_SQLITE (close(db));
		return;
	}

	{
		char msg[1024-128] = {0};

		getDateTime(lastlogin, timeStr, sizeof(timeStr));
		Q_strncpyz(msg, va("   ^5Last login: ^2%s\n", timeStr), sizeof(msg));
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
	}

	{
		char styleStr[16] = {0}, rankStr[8];
		char msg[1024-128] = {0};
		int s;
		row = 1;
		//Recent races
		sql = "SELECT coursename, style, rank, season_rank, duration_ms, end_time FROM LocalRun WHERE username = ? ORDER BY end_time DESC LIMIT ?, 5";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, start));
	
		trap->SendServerCommand(ent-g_entities, "print \"Recent Races:\n    ^5Course                      Style      Rank    Time         Date\n\""); //Color rank yellow for global, normal for season -fixme match race print scheme
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				IntegerToRaceName(sqlite3_column_int(stmt, 1), styleStr, sizeof(styleStr));
				TimeToString(sqlite3_column_int(stmt, 4), timeStr, sizeof(timeStr), qfalse);
				getDateTime(sqlite3_column_int(stmt, 5), dateStr, sizeof(dateStr)); 

				//If rank == 0, put "Season rank: season_rank".  Else put "Rank: rank"
				if (sqlite3_column_int(stmt, 2))
					Com_sprintf(rankStr, sizeof(rankStr), "^2%i^7", sqlite3_column_int(stmt, 2));
				else
					Com_sprintf(rankStr, sizeof(rankStr), "^3%i^7", sqlite3_column_int(stmt, 3));
				

				tmpMsg = va("^5%2i^3: ^3%-27s ^3%-10s ^3%-11s ^3%-12s %s\n", row+start, sqlite3_column_text(stmt, 0), styleStr, rankStr, timeStr, dateStr);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_Stats_f 2)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));

	}

	{
		char opponent[16], result[16], type[16];
		char msg[1024-128] = {0};
		int s;
		row = 1;
		//Recent duels
		sql = "SELECT winner, loser, type, end_time FROM LocalDuel WHERE winner = ? OR loser = ? ORDER BY end_time DESC LIMIT ?, 5";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, start));
	
		trap->SendServerCommand(ent-g_entities, "print \"Recent Duels:\n    ^5Opponent         Result   Type        Date\n\"");
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				IntegerToDuelType(sqlite3_column_int(stmt, 2), type, sizeof(type));
				getDateTime(sqlite3_column_int(stmt, 3), dateStr, sizeof(dateStr));
				
				if (!Q_stricmp((char*)sqlite3_column_text(stmt, 0), username)) { //They are winner
					Com_sprintf(opponent, sizeof(opponent), "^3%s^7", (char*)sqlite3_column_text(stmt, 1));
					Com_sprintf(result, sizeof(result), "^2Win^7");
				}
				else {
					Com_sprintf(opponent, sizeof(opponent), "^3%s^7", (char*)sqlite3_column_text(stmt, 0));
					Com_sprintf(result, sizeof(result), "^1Loss^7");
				}

				tmpMsg = va("^5%2i^3: ^3%-20s ^3%-12s ^3%-11s %s\n", row+start, opponent, result, type, dateStr);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_Stats_f 3)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
	}

	CALL_SQLITE (close(db));
	//DebugWriteToDB("Cmd_Stats_f");
}

//Search array list to find players row
//If found, update it
//If not found, add a new row at next empty spot
//A new function will read the array on mapchange, and do the querys updates
void G_AddSimpleStat(gentity_t *self, gentity_t *other, int type) {
//Useless feature
#if _STATLOG
	int row;
	char userName[16];

	if (sv_cheats.integer) //Dont record stats if cheats were enabled
		return;
	if (!self)
		return;
	if (!other)
		return;
	if (!self->client)
		return;
	if (!other->client)
		return;
	if (!self->client->pers.userName[0])
		return;
	if (!other->client->pers.userName[0])
		return;
	if (self->client->sess.raceMode)
		return;
	if (other->client->sess.raceMode) //EH?
		return;

	Q_strncpyz(userName, self->client->pers.userName, sizeof(userName));

	for (row = 0; row < 256; row++) { //size of UserStats ?
		if (!UserStats[row].username || !UserStats[row].username[0])
			break;
		if (!Q_stricmp(UserStats[row].username, userName)) { //User found, update his stats in memory, is this check right?
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
	Q_strncpyz(UserStats[row].username, userName, sizeof(UserStats[row].username )); //If we are here it means name not found, so add it
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

#endif
}

void G_AddSimpleStatsToFile() { //For each item in array.. do an update query?  Called on shutdown game.
	//Useless feature
#if _STATLOG
	//fileHandle_t	f;	
	char	buf[8 * 4096] = {0};
	int		row;

	Q_strncpyz(buf, "", sizeof(buf));

	for (row = 0; row < 256; row++) { //size of UserStats ?

		if (!UserStats[row].username || !UserStats[row].username[0])
			break;

		Q_strcat(buf, sizeof(buf), va("%s;%i;%i;%i;%i;%i\n", UserStats[row].username, UserStats[row].kills, UserStats[row].deaths, UserStats[row].suicides, UserStats[row].captures, UserStats[row].returns));
	}

	trap->FS_Write( buf, strlen( buf ), level.tempStatLog );
	trap->Print("Adding stats to file: %s\n", TEMP_STAT_LOG);

#endif
}

void G_AddSimpleStatsToDB() {
//Useless feature
#if _STATLOG
	fileHandle_t f;	
	int		fLen = 0, args = 1, s; //MAX_FILESIZE = 4096
	char	buf[8 * 1024] = {0}, empty[8] = {0};//eh
	char*	pch;
	sqlite3 * db;
	char * sql;
	sqlite3_stmt * stmt;
	UserStats_t	TempUserStats;
	qboolean good = qfalse;

	fLen = trap->FS_Open(TEMP_STAT_LOG, &f, FS_READ);

	if (!f) {
		trap->Print("ERROR: Couldn't load stat data from %s\n", TEMP_STAT_LOG);
		return;
	}
	if (fLen >= 8*1024) {
		trap->FS_Close(f);
		trap->Print("ERROR: Couldn't load stat data from %s, file is too large\n", TEMP_STAT_LOG);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);
	
	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = "UPDATE LocalAccount SET "
		"kills = kills + ?, deaths = deaths + ?, suicides = suicides + ?, captures = captures + ?, returns = returns + ? "
		"WHERE username = ?";
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

	//Todo: make TempRaceRecord an array of structs instead, maybe like 32 long idk, and build a query to insert 32 at a time or something.. instead of 1 by 1
	pch = strtok (buf,";\n");
	while (pch != NULL)
	{
		if ((args % 6) == 1)
			Q_strncpyz(TempUserStats.username, pch, sizeof(TempUserStats.username));
		else if ((args % 6) == 2)
			TempUserStats.kills = atoi(pch);
		else if ((args % 6) == 3)
			TempUserStats.deaths = atoi(pch);
		else if ((args % 6) == 4)
			TempUserStats.suicides = atoi(pch);
		else if ((args % 6) == 5)
			TempUserStats.captures = atoi(pch);
		else if ((args % 6) == 0) {
			TempUserStats.returns = atoi(pch);
			//trap->Print("Inserting stat into db: %s, %i, %i, %i, %i, %i\n", TempUserStats.username, TempUserStats.kills, TempUserStats.deaths, TempUserStats.suicides, TempUserStats.captures, TempUserStats.returns);
			CALL_SQLITE (bind_int (stmt, 1, TempUserStats.kills));
			CALL_SQLITE (bind_int (stmt, 2, TempUserStats.deaths));
			CALL_SQLITE (bind_int (stmt, 3, TempUserStats.suicides));
			CALL_SQLITE (bind_int (stmt, 4, TempUserStats.captures));
			CALL_SQLITE (bind_int (stmt, 5, TempUserStats.returns));
			CALL_SQLITE (bind_text (stmt, 6, TempUserStats.username, -1, SQLITE_STATIC));
			CALL_SQLITE_EXPECT (step (stmt), DONE);
			CALL_SQLITE (reset (stmt));
			CALL_SQLITE (clear_bindings (stmt));
		}
    	pch = strtok (NULL, ";\n");
		args++;
	}

	s = sqlite3_step(stmt); //this duplicates last one..?
	if (s == SQLITE_DONE)
		good = qtrue;
	CALL_SQLITE (finalize(stmt));
	CALL_SQLITE (close(db));	

	if (good) { //dont delete tmp file if mysql database is not responding 
		trap->FS_Open(TEMP_STAT_LOG, &f, FS_WRITE); 
		trap->FS_Write( empty, strlen( empty ), level.tempStatLog );
		trap->FS_Close(f);
		trap->Print("Loaded previous map stats from %s.\n", TEMP_STAT_LOG);
	}
	else 
		trap->Print("ERROR: Unable to insert previous map stats into database.\n");

	//DebugWriteToDB("G_AddSimpleStatToDB");
#endif
}

#if 0
void G_AddSimpleStatsToDB2() { //For each item in array.. do an update query?  Called on shutdown game.
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
#endif

#if 0
void BuildMapHighscores() { //loda fixme, take prepare,query out of loop
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int i, mstyle;
	char mapName[40], courseName[40], info[1024] = {0}, dateStr[64] = {0};

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(mapName, Info_ValueForKey( info, "mapname" ), sizeof(mapName));
	Q_strlwr(mapName);
	Q_CleanStr(mapName);

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	for (i = 0; i < level.numCourses; i++) { //32 max
		Q_strncpyz(courseName, mapName, sizeof(courseName));
		if (level.courseName[i][0])
			Q_strcat(courseName, sizeof(courseName), va(" (%s)", level.courseName[i]));
		for (mstyle = 0; mstyle < MV_NUMSTYLES; mstyle++) { //9 movement styles. 0-8
			int rank = 0;

			sql = "SELECT LR.id, LR.username, LR.coursename, LR.duration_ms, LR.topspeed, LR.average, LR.style, LR.end_time "  //Place 1
				"FROM (SELECT id, MIN(duration_ms) "
				   "FROM LocalRun "
				   "WHERE coursename = ? AND style = ? "
				   "GROUP by username) " 
				"AS X INNER JOIN LocalRun AS LR ON LR.id = X.id ORDER BY duration_ms, end_time LIMIT 10"; //end_time so in case of tie, first one shows up first?

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
					Q_strncpyz(HighScores[i][style][rank].coursename, course, sizeof(HighScores[i][style][rank].coursename));
					HighScores[i][style][rank].duration_ms = duration_ms;
					HighScores[i][style][rank].topspeed = topspeed;
					HighScores[i][style][rank].average = average;
					HighScores[i][style][rank].style = style;

					getDateTime(end_time, dateStr, sizeof(dateStr));
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

	//DebugWriteToDB("BuildMapHighscores");
}
#endif

int RaceNameToInteger(char *style) {
	Q_strlwr(style);
	Q_CleanStr(style);

	if (!Q_stricmp(style, "siege"))
		return 0;
	if (!Q_stricmp(style, "jka") || !Q_stricmp(style, "jk3"))
		return 1;
	if (!Q_stricmp(style, "hl2") || !Q_stricmp(style, "hl1") || !Q_stricmp(style, "hl") || !Q_stricmp(style, "qw"))
		return 2;
	if (!Q_stricmp(style, "cpm") || !Q_stricmp(style, "cpma"))
		return 3;
	if (!Q_stricmp(style, "q3") || !Q_stricmp(style, "q3"))
		return 4;
	if (!Q_stricmp(style, "pjk"))
		return 5;
	if (!Q_stricmp(style, "wsw") || !Q_stricmp(style, "warsow"))
		return 6;
	if (!Q_stricmp(style, "rjq3") || !Q_stricmp(style, "q3rj"))
		return 7;
	if (!Q_stricmp(style, "rjcpm") || !Q_stricmp(style, "cpmrj"))
		return 8;
	if (!Q_stricmp(style, "swoop"))
		return 9;
	if (!Q_stricmp(style, "jetpack"))
		return 10;
	if (!Q_stricmp(style, "speed") || !Q_stricmp(style, "ctf"))
		return 11;
#if _SPPHYSICS
	if (!Q_stricmp(style, "sp") || !Q_stricmp(style, "singleplayer"))
		return 12;
#endif
	if (!Q_stricmp(style, "slick"))
		return 13;
	if (!Q_stricmp(style, "botcpm"))
		return 14;
	return -1;
}

int SeasonToInteger(char *season) {
	Q_strlwr(season);
	Q_CleanStr(season);

	//Todo - Do this dynamically

	if (!Q_stricmp(season, "s1"))
		return 1;
	if (!Q_stricmp(season, "s2"))
		return 2;
	if (!Q_stricmp(season, "s3"))
		return 3;
	if (!Q_stricmp(season, "s4"))
		return 4;
	if (!Q_stricmp(season, "s5"))
		return 5;
	return -1;
}

void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}

#if 0
void Cmd_PersonalBest_f(gentity_t *ent) { //loda fixme bugged, always finds in cache even if not there
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s, style, duration_ms = 0, topspeed = 0, average = 0, i, course = -1;
	char username[16], courseName[40], courseNameFull[40], styleString[16], durationStr[32], tempCourseName[40];

	if (trap->Argc() != 4 && trap->Argc() != 5) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /best <username> <full coursename> <style>.  Example: /best user mapname (coursename) style\n\"");
		return;
	}

	trap->Argv(1, username, sizeof(username));

	Q_strlwr(username);
	Q_CleanStr(username);

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
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /best <username> <full coursename> <style>.\n\"");
		return;
	}

	Q_strlwr(username);
	Q_CleanStr(username);
	Q_strlwr(courseNameFull);
	Q_CleanStr(courseNameFull);

	Q_strncpyz(tempCourseName, courseName, sizeof(tempCourseName));
	remove_all_chars(tempCourseName, '(');
	remove_all_chars(tempCourseName, ')');

	for (i = 0; i < level.numCourses; i++) {
		//trap->Print("course, course : %s, %s\n", tempCourseName, level.courseName[i]);
		if (!Q_stricmp(tempCourseName, level.courseName[i])) {
			course = i;
			break;
		}
	}

	if (level.numCourses == 1)
		course = 0;

	if (course != -1) { //Found course on current map, check highscores cache
		for (i = 0; i < 10; i++) { //Search for highscore in highscore cache table
			//trap->Print("Cycling Highscores %s, %s matching with %s, %s\n", HighScores[course][style][i].username, HighScores[course][style][i].coursename , username, courseNameFull);
			if (!Q_stricmp(username, HighScores[course][style][i].username) && !Q_stricmp(courseNameFull, HighScores[course][style][i].coursename)) { //Its us, and right course
				//trap->Print("Found In Highscore Cache\n");
				duration_ms = HighScores[course][style][i].duration_ms;
				topspeed = HighScores[course][style][i].topspeed;
				average = HighScores[course][style][i].average;
				break;
			}
			if (!HighScores[course][style][i].username[0])
				break;
		}
	}

	if (!duration_ms) { //Search for highscore in personalbest cache table
		for (i = 0; i < 50; i++) {
			//trap->Print("Cycling PB %s, %s matching with %s, %s\n", PersonalBests[style][i].username, PersonalBests[style][i].coursename , username, courseNameFull);
			if (!Q_stricmp(username, PersonalBests[style][i].username) && !Q_stricmp(courseNameFull, PersonalBests[style][i].coursename)) { //Its us, and right course
				//trap->Print("Found In My Cache\n");
				duration_ms = PersonalBests[style][i].duration_ms;
				topspeed = 0;//HighScores[course][style][i].topspeed; //Topspeed and average are not yet stored in personalBest cache, so uh.... make them 0 for now...
				average = 0;//HighScores[course][style][i].average;
				break;
			}
			if (!PersonalBests[style][i].username[0])
				break;
		}
	}

	if (!duration_ms) { //Not found in cache, so check db
		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "SELECT MIN(duration_ms), topspeed, average FROM LocalRun WHERE username = ? AND coursename = ? AND style = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_text (stmt, 2, courseNameFull, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 3, style));

		s = sqlite3_step(stmt);

		if (s == SQLITE_ROW) {
			duration_ms = sqlite3_column_int(stmt, 0);
			topspeed = sqlite3_column_int(stmt, 1);
			average = sqlite3_column_int(stmt, 2);
		}
		else if (s != SQLITE_DONE) {
			fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			CALL_SQLITE (finalize(stmt));
			CALL_SQLITE (close(db));
			return;
		}

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}

	if (duration_ms >= 60000) { //FIXME, make this use the inttostring function if it tests bugfree
		int minutes, seconds, milliseconds;
		minutes = (int)((duration_ms / (1000*60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000; 
		Com_sprintf(durationStr, sizeof(durationStr), "%i:%02i.%03i", minutes, seconds, milliseconds);//more precision?
	}
	else
		Q_strncpyz(durationStr, va("%.3f", ((float)duration_ms * 0.001)), sizeof(durationStr));

	if (duration_ms) {
		if (!topspeed && !average)
			trap->SendServerCommand( ent-g_entities, va("print \"^5 This players fastest time is ^3%s^5.\n\"", durationStr)); //whatever, probably wont ever get around to fixing this since it fucks with the caching
		else
			trap->SendServerCommand( ent-g_entities, va("print \"^5 This players fastest time is ^3%s^5 with max ^3%i^5 and average ^3%i^5.\n\"", durationStr, topspeed, average));
	}
	else
		trap->SendServerCommand(ent-g_entities, "print \"^5 No results found.\n\"");

	//DebugWriteToDB("Cmd_PersonalBest_f");
}

void Cmd_NotCompleted_f(gentity_t *ent) {
	int i, style, course;
	char styleString[16] = {0}, username[16] = {0};
	char msg[128] = {0};
	qboolean printed, found;

	if (level.numCourses == 0) {
		trap->SendServerCommand(ent-g_entities, "print \"This map does not have any courses.\n\"");
		return;
	}

	if (trap->Argc() > 2) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /notCompleted <username (optional)>\n\"");
		return;
	}

	if (trap->Argc() == 1) { //notcompleted
		if (!ent->client->pers.userName || !ent->client->pers.userName[0]) {
			trap->SendServerCommand(ent-g_entities, "print \"You must be logged in to use this command.\n\"");
			return;
		}
		Q_strncpyz(username, ent->client->pers.userName, sizeof(username));
	}
	else if (trap->Argc() == 2) { //notcompleted user
		char input[16];
		trap->Argv(1, input, sizeof(input));
		Q_strncpyz(username, input, sizeof(username));
	}

	Q_strlwr(username);
	Q_CleanStr(username);

	if (trap->Argc() == 1)
		trap->SendServerCommand(ent-g_entities, "print \"Courses where you are not in the top 10:\n\"");
	else if (trap->Argc() == 2)
		trap->SendServerCommand(ent-g_entities, va("print \"Courses where %s is not in the top 10:\n\"", username));


	for (course=0; course<level.numCourses; course++) { //For each course
		Q_strncpyz(msg, "", sizeof(msg));
		printed = qfalse;
		for (style = 0; style < MV_NUMSTYLES; style++) { //For each style
			found = qfalse;
			for (i=0; i<10; i++) {
				if (HighScores[course][style][i].username && HighScores[course][style][i].username[0] && !Q_stricmp(HighScores[course][style][i].username, username)) {
					found = qtrue;
					break;
				}
				else if (!HighScores[course][style][i].username || (HighScores[course][style][i].username && !HighScores[course][style][i].username[0])) {
					found = qfalse;
					break;
				}
			}

			if (!found) {
				if (!printed) {
					Q_strcat(msg, sizeof(msg), va("\n^3%-12s", level.courseName[course]));
					printed = qtrue;
				}
				else {
					//Q_strcat(msg, sizeof(msg), ":");
				}
				//Q_strcat(msg, sizeof(msg), va("<%i, %i>", found, printed));
				//Q_strcat(msg, sizeof(msg), "-");
				IntegerToRaceName(style, styleString, sizeof(styleString));
				Q_strcat(msg, sizeof(msg), va(" ^5%-6s", styleString));
			}
			else
			{
				if (printed)
					Q_strcat(msg, sizeof(msg), "       ");
			}
		}
		if (printed) {
			Q_strcat(msg, sizeof(msg), "\n");
			trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
		}
	}
}
#endif

#if 1//NEWRACERANKING
void Cmd_DFTopRank_f(gentity_t *ent) { //Add season support?
	int style = -1, season = -1, page = -1, start = 0, i, input;
	char styleString[16] = {0}, inputString[32];
	const int args = trap->Argc();

	if (args > 4) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rRank <season (optional - example: s1)> <style (optional)> <page (optional)>.  This displays the rankings for the specified season and style.\n\"");
		return;
	}

	for (i = 1; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (season == -1) {
			input = SeasonToInteger(inputString);
			if (input != -1) {
				season = input;
				continue;
			}
		}
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rRank <season (optional - example: s1)> <style (optional)> <page (optional)>.  This displays the rankings for the specified season and style.\n\"");
		return;
	}

	if (style == -1) {
		Q_strncpyz(styleString, "all styles", sizeof(styleString));
	}
	else {
		IntegerToRaceName(style, styleString, sizeof(styleString));
		Q_strcat(styleString, sizeof(styleString), " style");
	}

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int s, oldscore, newscore, count, golds, silvers, bronzes, row = 1;
		float rank, percentile;
		char msg[1024-128] = {0}, username[40];

		CALL_SQLITE (open (LOCAL_DB_PATH, & db)); //Needs to select only top entry from each person not all seasons
		if (style == -1) {
			if (season == -1) {
				sql = "SELECT username, SUM(entries-rank) AS newscore, CAST(SUM(entries/CAST(rank AS FLOAT)) AS INT) AS oldscore, AVG(rank) as rank, AVG((entries - CAST(rank-1 AS float))/entries) AS percentile, SUM(CASE WHEN rank == 1 THEN 1 ELSE 0 END) AS golds, SUM(CASE WHEN rank == 2 THEN 1 ELSE 0 END) AS silvers, SUM(CASE WHEN rank == 3 THEN 1 ELSE 0 END) AS bronzes, COUNT(*) as count FROM LocalRun "
					"WHERE rank != 0 AND style != 14 "
					"GROUP BY username "
					"ORDER BY oldscore+newscore DESC, rank DESC LIMIT ?, 10";
				CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
				CALL_SQLITE (bind_int (stmt, 1, start));
			}
			else {
				sql = "SELECT username, SUM(season_entries-season_rank) AS newscore, CAST(SUM(season_entries/CAST(season_rank AS FLOAT)) AS INT) AS oldscore, AVG(season_rank) as season_rank, AVG((season_entries - CAST(season_rank-1 AS float))/season_entries) AS percentile, SUM(CASE WHEN season_rank == 1 THEN 1 ELSE 0 END) AS golds, SUM(CASE WHEN season_rank == 2 THEN 1 ELSE 0 END) AS silvers, SUM(CASE WHEN season_rank == 3 THEN 1 ELSE 0 END) AS bronzes, COUNT(*) as count FROM LocalRun "
					"WHERE season = ? AND style != 14 "
					"GROUP BY username "
					"ORDER BY oldscore+newscore DESC, rank DESC LIMIT ?, 10";
				CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
				CALL_SQLITE (bind_int (stmt, 1, season));
				CALL_SQLITE (bind_int (stmt, 2, start));
			}
		}
		else {
			if (season == -1) {
				sql = "SELECT username, SUM(entries-rank) AS newscore, CAST(SUM(entries/CAST(rank AS FLOAT)) AS INT) AS oldscore, AVG(rank) as rank, AVG((entries - CAST(rank-1 AS float))/entries) AS percentile, SUM(CASE WHEN rank == 1 THEN 1 ELSE 0 END) AS golds, SUM(CASE WHEN rank == 2 THEN 1 ELSE 0 END) AS silvers, SUM(CASE WHEN rank == 3 THEN 1 ELSE 0 END) AS bronzes, COUNT(*) as count FROM LocalRun "
					"WHERE rank != 0 AND style = ? "
					"GROUP BY username "
					"ORDER BY oldscore+newscore DESC, rank DESC LIMIT ?, 10";
				CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
				CALL_SQLITE (bind_int (stmt, 1, style));
				CALL_SQLITE (bind_int (stmt, 2, start));
			}
			else {
				sql = "SELECT username, SUM(season_entries-season_rank) AS newscore, CAST(SUM(season_entries/CAST(season_rank AS FLOAT)) AS INT) AS oldscore, AVG(season_rank) as season_rank, AVG((season_entries - CAST(season_rank-1 AS float))/season_entries) AS percentile, SUM(CASE WHEN season_rank == 1 THEN 1 ELSE 0 END) AS golds, SUM(CASE WHEN season_rank == 2 THEN 1 ELSE 0 END) AS silvers, SUM(CASE WHEN season_rank == 3 THEN 1 ELSE 0 END) AS bronzes, COUNT(*) as count FROM LocalRun "
					"WHERE season = ? AND style = ? "
					"GROUP BY username "
					"ORDER BY oldscore+newscore DESC, rank DESC LIMIT ?, 10";
				CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
				CALL_SQLITE (bind_int (stmt, 1, season));
				CALL_SQLITE (bind_int (stmt, 2, style));
				CALL_SQLITE (bind_int (stmt, 3, start));
			}
		}

		if (season == -1)
			trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s:\n    ^5Username           Score     SPR       Avg. Rank   Percentile   Golds   Silvers   Bronzes   Count \n\"", styleString));
		else
			trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s season %i:\n    ^5Username           Score     SPR       Avg. Rank   Percentile   Golds   Silvers   Bronzes   Count \n\"", styleString, season));

		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				Q_strncpyz(username, (char*)sqlite3_column_text(stmt, 0), sizeof(username));
				newscore = sqlite3_column_int(stmt, 1);
				oldscore = sqlite3_column_int(stmt, 2);
				rank = sqlite3_column_double(stmt, 3);
				percentile = sqlite3_column_double(stmt, 4);
				golds = sqlite3_column_int(stmt, 5);
				silvers = sqlite3_column_int(stmt, 6);
				bronzes = sqlite3_column_int(stmt, 7);
				count = sqlite3_column_int(stmt, 8);

				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-9i ^3%-9.2f ^3%-11.2f ^3%-12.2f ^3%-7i ^3%-9i ^3%-9i %i\n", row+start, username, (int)(1+((oldscore+newscore)*0.5f)), (count ? ((float)oldscore/(float)count) : oldscore), rank, percentile, golds, silvers, bronzes, count);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFTopRank_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));

	}
}
#endif

void Cmd_DFHardest_f(gentity_t *ent) {
	int style = -1, page = -1, start = 0, input, i;
	char inputString[16], inputStyleString[16];
	qboolean currentSeason = qfalse;
	const int args = trap->Argc();

	//Should this also only show results that you don't have a time on?

	if (args > 4) {//5 for 'me'
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rHardest <style (optional)> <current season (optional - example: s) <page (optional)>.  This displays hardest completed courses for the specified style.\n\"");
		return;
	}

	for (i = 1; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (!currentSeason) {
			if (!Q_stricmp(inputString, "s")) {
				currentSeason = qtrue;
				continue;
			}
		}
		/*
		if (!me) {
			if (!Q_stricmp(inputString, "me")) {
				me = qtrue;
				continue;
			}
		}
		*/
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rHardest <style (optional)> <include seasons (optional - example: s) <page (optional)>.  This displays hardest completed courses for the specified style.\n\"");
		return; //Arg doesnt match any expected values so error.
	}

	/*
	if (me) {
		if (!ent->client->pers.userName[0]) { //Not logged in
			trap->SendServerCommand(ent-g_entities, "print \"You must be logged in to use this command.\n\"");
			return;
		}
		Q_strncpyz(username, ent->client->pers.userName, sizeof(username));
		Q_strlwr(username);
		Q_CleanStr(username);
	}
	*/

	if (style == -1) {
		Q_strncpyz(inputStyleString, "all styles", sizeof(inputStyleString));
	}
	else {
		IntegerToRaceName(style, inputStyleString, sizeof(inputStyleString));
		Q_strcat(inputStyleString, sizeof(inputStyleString), " style");
	}

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int row = 1;
		char styleStr[16] = {0}, msg[128] = {0};
		int s;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));

		if (style == -1) {
			if (currentSeason)
				sql = "SELECT username, coursename, style, season_entries FROM LocalRun WHERE season = (SELECT MAX(season) FROM LocalRun) ORDER BY season_entries ASC, entries ASC LIMIT ?,10";
			else
				sql = "SELECT username, coursename, style, entries FROM LocalRun ORDER BY entries ASC LIMIT ?,10";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, start));
		}
		else {
			if (currentSeason)
				sql = "SELECT username, coursename, style, season_entries FROM LocalRun WHERE season = (SELECT MAX(season) FROM LocalRun) AND style = ? ORDER BY season_entries ASC, entries ASC LIMIT ?,10";
			else
				sql = "SELECT username, coursename, style, entries FROM LocalRun WHERE style = ? ORDER BY entries ASC LIMIT ?,10";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, style));
			CALL_SQLITE (bind_int (stmt, 2, start));
		}

		if (currentSeason)
				trap->SendServerCommand(ent-g_entities, va("print \"Results for %s (current season):\n    ^5Username           Coursename                     Style       Entries\n\"", inputStyleString));
		else
				trap->SendServerCommand(ent-g_entities, va("print \"Results for %s:\n    ^5Username           Coursename                     Style       Entries\n\"", inputStyleString));
		
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				IntegerToRaceName(sqlite3_column_int(stmt, 2), styleStr, sizeof(styleStr));

				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-30s ^3%-11s ^3%-8i\n", start+row, sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), styleStr, sqlite3_column_int(stmt, 3));
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFHardest_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}


}

void Cmd_DFRecent_f(gentity_t *ent) {
	int style = -1, page = -1, start = 0, input, i;
	char inputString[16], inputStyleString[16];
	qboolean showSeasons = qfalse;
	const int args = trap->Argc();

	if (args > 4) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rLatest <style (optional)> <include seasons (optional - example: s) <page (optional)>.  This displays recent records for the specified style.\n\"");
		return;
	}

	for (i = 1; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (!showSeasons) {
			if (!Q_stricmp(inputString, "s")) {
				showSeasons = qtrue;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rLatest <style (optional)> <include seasons (optional - example: s) <page (optional)>.  This displays recent records for the specified style.\n\"");
		return; //Arg doesnt match any expected values so error.
	}

	if (style == -1) {
		Q_strncpyz(inputStyleString, "all styles", sizeof(inputStyleString));
	}
	else {
		IntegerToRaceName(style, inputStyleString, sizeof(inputStyleString));
		Q_strcat(inputStyleString, sizeof(inputStyleString), " style");
	}

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int row = 1;
		char dateStr[64] = {0}, timeStr[32] = {0}, styleStr[16] = {0}, rankStr[16] = {0}, msg[128] = {0};
		int s;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));

		if (style == -1) {
			if (showSeasons)
				sql = "SELECT username, coursename, style, rank, duration_ms, end_time, season_rank FROM LocalRun ORDER BY end_time DESC LIMIT ?,10";
			else
				sql = "SELECT username, coursename, style, rank, duration_ms, end_time FROM LocalRun WHERE rank != 0 ORDER BY end_time DESC LIMIT ?,10";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, start));
		}
		else {
			if (showSeasons)
				sql = "SELECT username, coursename, style, rank, duration_ms, end_time, season_rank FROM LocalRun WHERE style = ? ORDER BY end_time DESC LIMIT ?,10";
			else
				sql = "SELECT username, coursename, style, rank, duration_ms, end_time FROM LocalRun WHERE rank != 0 AND style = ? ORDER BY end_time DESC LIMIT ?,10";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_int (stmt, 1, style));
			CALL_SQLITE (bind_int (stmt, 2, start));
		}

		if (showSeasons)
			trap->SendServerCommand(ent-g_entities, va("print \"Recent results for %s style (by season):\n    ^5Username           Coursename                     Style       Rank     Time         Date\n\"", inputStyleString));
		else
			trap->SendServerCommand(ent-g_entities, va("print \"Recent results for %s style:\n    ^5Username           Coursename                     Style       Rank     Time         Date\n\"", inputStyleString));
		
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				TimeToString(sqlite3_column_int(stmt, 4), timeStr, sizeof(timeStr), qfalse);
				getDateTime(sqlite3_column_int(stmt, 5), dateStr, sizeof(dateStr));
				IntegerToRaceName(sqlite3_column_int(stmt, 2), styleStr, sizeof(styleStr));

				if (showSeasons) {	//If rank == 0, put season rank in yellow, else put rank in green
					if (sqlite3_column_int(stmt, 3))
						Com_sprintf(rankStr, sizeof(rankStr), "^2%i^7", sqlite3_column_int(stmt, 3));
					else
						Com_sprintf(rankStr, sizeof(rankStr), "^3%i^7", sqlite3_column_int(stmt, 6));
				}
				else
					Com_sprintf(rankStr, sizeof(rankStr), "^2%i^7", sqlite3_column_int(stmt, 3));

				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-30s ^3%-11s ^3%-12s ^3%-12s %s\n", start+row, sqlite3_column_text(stmt, 0), sqlite3_column_text(stmt, 1), styleStr, rankStr, timeStr, dateStr);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFRecent_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}


}

void Cmd_DFTop10_f(gentity_t *ent) {
	int style = -1, page = -1, season = -1, start = 0, input, i;
	char inputString[40], inputStyleString[16];
	char partialCourseName[40] = {0}, fullCourseName[40] = {0};
	const int args = trap->Argc();
	qboolean enteredCourseName = qtrue;

	if (args > 5) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <season (optional - example: s1)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
		return;
	}

	//Get mapname. 
	//How to tell if mapname is specified -- If its a map with only 1 course, we have to check. Otherwise we can assume 1st arg is mapname.
		//It will always be the first arg.
		//If the first arg doesnt match the pattern of style/season/page, we can assume it is mapname.
		//This means we cant search for 

	if (args == 1)
		enteredCourseName = qfalse;
	else {
		trap->Argv(1, inputString, sizeof(inputString));
		//use strtol isntead of atoi maybe - partial coursename can start with number
		if ((RaceNameToInteger(inputString) != -1) || (SeasonToInteger(inputString) != -1) || (atoi(inputString))) {//If arg1 is style, or season, or page
			enteredCourseName = qfalse; //Use current mapname as coursename
		}
	}

	if (enteredCourseName) {
		trap->Argv(1, partialCourseName, sizeof(partialCourseName)); //Use arg1 as coursename
	}

	if (!enteredCourseName) {
		if (!level.numCourses) {
			trap->SendServerCommand(ent-g_entities, "print \"This map has no courses, you must specify one of the following with /rTop <coursename> <style (optional)> <season (optional - example: s1)> <page (optional)>.\n\"");
			return;
		}
		else if (level.numCourses > 1) {
			trap->SendServerCommand(ent-g_entities, "print \"This map has multiple courses, you must specify one of the following with /rTop <coursename> <style (optional)> <season (optional - example: s1)> <page (optional)>.\n\"");
			for (i = 0; i < level.numCourses; i++) { //32 max
				if (level.courseName[i] && level.courseName[i][0])
					trap->SendServerCommand(ent-g_entities, va("print \"  ^5%i ^7- ^3%s\n\"", i+1, level.courseName[i]));
			}
			return;
		}
	}

	//Go through args 2-x, if we have a specified mapname, or 1-x if we dont
	for (i = (enteredCourseName ? 2 : 1) ; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (season == -1) {
			input = SeasonToInteger(inputString);
			if (input != -1) {
				season = input;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <season (optional - example: s1)> <page (optional)>.  This displays highscores for the specified course.\n\"");
		return; //Arg doesnt match any expected values so error.
	}

	if (style == -1) //Default to JKA style
		style = 1;
	IntegerToRaceName(style, inputStyleString, sizeof(inputStyleString));
	Q_strcat(inputStyleString, sizeof(inputStyleString), " style");

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	Q_strlwr(partialCourseName);
	Q_CleanStr(partialCourseName);

	if (!enteredCourseName) {
		char info[1024] = {0};
		trap->GetServerinfo(info, sizeof(info));
		Q_strncpyz(fullCourseName, Info_ValueForKey( info, "mapname" ), sizeof(fullCourseName));
		Q_strlwr(fullCourseName);
		Q_CleanStr(fullCourseName);
	}

	//Com_Printf("Style %i, page %i, season %i, map %s, fullmap %s\n", style, page, season, partialCourseName, fullCourseName);

	{ //See if course is found in database and print it then..?
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int row = 1;
		int s;
		char dateStr[64] = {0}, dateStrColored[64] = {0}, timeStr[32], msg[1024-128] = {0};
		time_t	rawtime;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));

		if (enteredCourseName) { //Course e
			//Com_Printf("doing sql query %s %i\n", courseName, style);
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE coursename LIKE %?%";
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(coursename, ?) > 0 LIMIT 1";
			//sql = "SELECT coursename, MAX(entries) FROM LocalRun WHERE instr(coursename, ?) > 0 LIMIT 1";
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(coursename, ?) > 0 ORDER BY LENGTH(coursename) ASC, entries DESC LIMIT 1";
			sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(replace(coursename, ' ', ''), ?) > 0 ORDER BY entries DESC LIMIT 1";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, partialCourseName, -1, SQLITE_STATIC));
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				//Check if it actually has text, if not return.  then we can use cheaper (MAX) entries query above //loda fixme
				Q_strncpyz(fullCourseName, (char*)sqlite3_column_text(stmt, 0), sizeof(fullCourseName));
			}
			else if (s == SQLITE_DONE) {
				//Com_Printf("fail 4\n");
				trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <season (optional - example: s1)> <page (optional)>.  This displays highscores for the specified course.\n\"");
				CALL_SQLITE (finalize(stmt));
				CALL_SQLITE (close(db));
				return;
			}
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFTop10_f)", s);
				return;
			}
			CALL_SQLITE (finalize(stmt));

		}

		//Problem - crossmap query can return multiple records for same person since the cleanup cmd is only done on mapchange, 
		//fix by grouping by username here? and using min() so it shows right one? who knows if that will work
		//could be cheaper by using where rank != 0 instead of min(duration_ms) but w/e
		if (season == -1)
			sql = "SELECT username, MIN(duration_ms) AS duration, topspeed, average, end_time FROM LocalRun WHERE coursename = ? AND style = ? GROUP BY username ORDER BY duration ASC, end_time ASC LIMIT ?, 10";
		else 
			sql = "SELECT username, MIN(duration_ms) AS duration, topspeed, average, end_time FROM LocalRun WHERE coursename = ? AND style = ? AND season = ? GROUP BY username ORDER BY duration ASC, end_time ASC LIMIT ?, 10";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, fullCourseName, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, style));

		if (season == -1) {
			CALL_SQLITE (bind_int (stmt, 3, start));
		}
		else {
			CALL_SQLITE (bind_int (stmt, 3, season));
			CALL_SQLITE (bind_int (stmt, 4, start));
		}

		time( &rawtime );
		localtime( &rawtime );

		if (season == -1)
			trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s:\n    ^5Username           Time         Topspeed    Average      Date\n\"", fullCourseName, inputStyleString));
		else
			trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s season %i:\n    ^5Username           Time         Topspeed    Average      Date\n\"", fullCourseName, inputStyleString, season));
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				TimeToString(sqlite3_column_int(stmt, 1), timeStr, sizeof(timeStr), qfalse);
				getDateTime(sqlite3_column_int(stmt, 4), dateStr, sizeof(dateStr));
				if (rawtime - sqlite3_column_int(stmt, 4) < 60*60*24) { //Today
					Com_sprintf(dateStrColored, sizeof(dateStrColored), "^2%s^7", dateStr);
				}
				else {
					Q_strncpyz(dateStrColored, dateStr, sizeof(dateStrColored));
				}
				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-12s ^3%-11i ^3%-12i %s\n", row+start, sqlite3_column_text(stmt, 0), timeStr, sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3), dateStrColored);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFTop10_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}
}

#if 0
void Cmd_DFTop10_f(gentity_t *ent) {
	const int args = trap->Argc();
	char input1[40], input2[32], input3[32], courseName[40] = {0}, courseNameFull[40] = {0}, msg[1024-128] = {0}, timeStr[32], styleString[16] = {0};
	int i, style = -1, page = 1, start;
	qboolean partialCourseName = qtrue;

	//special case for if only 1 course on map.. aoh no
	//dftop10 (coursename always first) style, season, page can be any order


	if (args == 1) { //Dftop10  - current map JKA, only 1 course on map.  Or if there are multiple courses, display them all.
		if (level.numCourses == 0) { //No course on this map, so error.
			//Com_Printf("fail 1\n");
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
			return;
		}
		if (level.numCourses > 1) { //
			trap->SendServerCommand(ent-g_entities, "print \"This map has multiple courses, you must specify one of the following with /rTop <coursename> <style (optional)> <page (optional)>.\n\"");
			for (i = 0; i < level.numCourses; i++) { //32 max
				if (level.courseName[i] && level.courseName[i][0])
					trap->SendServerCommand(ent-g_entities, va("print \"  ^5%i ^7- ^3%s\n\"", i+1, level.courseName[i]));
			}
			return;
		}
		else {
			partialCourseName = qfalse;
		}
		style = 1;
	}
	else if (args == 2) {//CPM - current map cpm, only 1 course on map
		trap->Argv(1, input1, sizeof(input1));
		style = RaceNameToInteger(input1);
		//Check if 2nd arg is style or course.

		if (style < 0) { //Invalid style, so its a course intead.
			style = 1;
			Q_strncpyz(courseName, input1, sizeof(courseName));
		}
		else if (level.numCourses == 1) { //What if its a style, then we use course=0 ?
			partialCourseName = qfalse;
		}
	}
	else if (args == 3) { //dftop10 dash1 cpm - search for dash1 exact match(?) in memory, if not then fallback to SQL query.  cpm style.
		//Get 2nd arg as course
		//Get 3rd arg as style
		trap->Argv(1, input1, sizeof(input1));
		trap->Argv(2, input2, sizeof(input2));

		style = RaceNameToInteger(input2);
		if (style < 0) { //Invalid style
			//Com_Printf("fail 2\n");
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
			return;
		}

		Q_strncpyz(courseName, input1, sizeof(courseName));

	}
	else if (args == 4) { //dftop10 dash1 cpm - search for dash1 exact match(?) in memory, if not then fallback to SQL query.  cpm style.
		//Get 2nd arg as course
		//Get 3rd arg as style
		trap->Argv(1, input1, sizeof(input1));
		trap->Argv(2, input2, sizeof(input2));
		trap->Argv(3, input3, sizeof(input3));

		style = RaceNameToInteger(input2);
		if (style < 0) { //Invalid style
			//Com_Printf("fail 2\n");
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
			return;
		}
		page = atoi(input3);
		if (page < 1 || page > 100) {
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
			return;
		}

		Q_strncpyz(courseName, input1, sizeof(courseName));

	}
	else { //Error, print usage
		//Com_Printf("fail 3\n");
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
		return;
	}

	start = (page - 1) * 10;
	
	//At this point we should have a valid style and a potential coursename.
	Q_strlwr(courseName);
	Q_CleanStr(courseName);
	IntegerToRaceName(style, styleString, sizeof(styleString));

	if (!partialCourseName) {
		char info[1024] = {0};
		trap->GetServerinfo(info, sizeof(info));
		Q_strncpyz(courseNameFull, Info_ValueForKey( info, "mapname" ), sizeof(courseNameFull));
		Q_strlwr(courseNameFull);
		Q_CleanStr(courseNameFull);
	}
	else {
		if (!Q_stricmp(courseName, "")) {
			trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
			return;
		}
	}

	{ //See if course is found in database and print it then..?
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		int row = 1;
		int s;
		char dateStr[64] = {0}, dateStrColored[64] = {0};
		time_t	rawtime;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));

		if (partialCourseName) { //Course e
			//Com_Printf("doing sql query %s %i\n", courseName, style);
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE coursename LIKE %?%";
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(coursename, ?) > 0 LIMIT 1";
			//sql = "SELECT coursename, MAX(entries) FROM LocalRun WHERE instr(coursename, ?) > 0 LIMIT 1";
			//sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(coursename, ?) > 0 ORDER BY LENGTH(coursename) ASC, entries DESC LIMIT 1";
			sql = "SELECT DISTINCT(coursename) FROM LocalRun WHERE instr(coursename, ?) > 0 ORDER BY entries DESC LIMIT 1";
			CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
			CALL_SQLITE (bind_text (stmt, 1, courseName, -1, SQLITE_STATIC));
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				//Check if it actually has text, if not return.  then we can use cheaper (MAX) entries query above //loda fixme
				Q_strncpyz(courseNameFull, (char*)sqlite3_column_text(stmt, 0), sizeof(courseNameFull));
			}
			else {
				//Com_Printf("fail 4\n");
				trap->SendServerCommand(ent-g_entities, "print \"Usage: /rTop <course (if needed)> <style (optional)> <page (optional)>.  This displays the top10 for the specified course.\n\"");
				CALL_SQLITE (finalize(stmt));
				CALL_SQLITE (close(db));
				return;
			}
			CALL_SQLITE (finalize(stmt));

		}

		//Problem - crossmap query can return multiple records for same person since the cleanup cmd is only done on mapchange, 
		//fix by grouping by username here? and using min() so it shows right one? who knows if that will work
		//could be cheaper by using where rank != 0 instead of min(duration_ms) but w/e
		sql = "SELECT username, MIN(duration_ms) AS duration, topspeed, average, end_time FROM LocalRun WHERE coursename = ? AND style = ? GROUP BY username ORDER BY duration ASC, end_time ASC LIMIT ?, 10";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
		CALL_SQLITE (bind_text (stmt, 1, courseNameFull, -1, SQLITE_STATIC));
		CALL_SQLITE (bind_int (stmt, 2, style));
		CALL_SQLITE (bind_int (stmt, 3, start));

		time( &rawtime );
		localtime( &rawtime );

		trap->SendServerCommand(ent-g_entities, va("print \"Highscore results for %s using %s style:\n    ^5Username           Time         Topspeed    Average      Date\n\"", courseNameFull, styleString));
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				TimeToString(sqlite3_column_int(stmt, 1), timeStr, sizeof(timeStr), qfalse);
				getDateTime(sqlite3_column_int(stmt, 4), dateStr, sizeof(dateStr));
				if (rawtime - sqlite3_column_int(stmt, 4) < 60*60*24) { //Today
					Com_sprintf(dateStrColored, sizeof(dateStrColored), "^2%s^7", dateStr);
				}
				else {
					Q_strncpyz(dateStrColored, dateStr, sizeof(dateStrColored));
				}
				tmpMsg = va("^5%2i^3: ^3%-18s ^3%-12s ^3%-11i ^3%-12i %s\n", row+start, sqlite3_column_text(stmt, 0), timeStr, sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3), dateStrColored);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				fprintf (stderr, "ERROR: SQL Select Failed (Cmd_DFTop10_f).\n");//Trap print?
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}
}
#endif

void Cmd_DFTodo_f(gentity_t *ent) {
	const int args = trap->Argc();
	int style = -1, page = -1, start = 0, i, input;
	char styleString[16] = {0}, inputString[32], partialCourseName[40], username[16];
	qboolean enteredCoursename = qfalse;

	if (!ent->client->pers.userName || !ent->client->pers.userName[0]) {
		trap->SendServerCommand(ent-g_entities, "print \"You must be logged in to use this command.\n\"");
		return;
	}
	Q_strncpyz(username, ent->client->pers.userName, sizeof(username));

	if (args > 4) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rWorst <map (optional)> <style (optional)> <page (optional)>\n\"");
		return;
	}

	for (i = 1 ; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		if (!enteredCoursename) {
			input = SeasonToInteger(inputString);
			Q_strncpyz(partialCourseName, inputString, sizeof(partialCourseName));
			enteredCoursename = qtrue;
			continue;
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rWorst <map (optional)> <style (optional)> <page (optional)>\n\"");
		return; //Arg doesnt match any expected values so error.
	}

	if (style == -1) {
		Q_strncpyz(styleString, "all styles", sizeof(styleString));
	}
	else {
		IntegerToRaceName(style, styleString, sizeof(styleString));
		Q_strcat(styleString, sizeof(styleString), " style");
	}

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	Q_strlwr(partialCourseName);
	Q_CleanStr(partialCourseName);

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		char msg[1024-128] = {0}, timeStr[64], dateStr[64], styleStr[16], rankStr[16];
		int s, row = 1;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		if (style == -1) {
			if (enteredCoursename) {
				//sql = "SELECT coursename, style, rank, entries, duration_ms, end_time FROM LocalRun WHERE rank != 0 AND username = ? AND instr(coursename, ?) > 0 ORDER BY (entries - entries / rank) DESC LIMIT ?, 10";
				sql = "SELECT * FROM "
						"(SELECT coursename, style, rank, entries, duration_ms, end_time "
							"FROM LocalRun WHERE rank != 0 AND username = ? AND instr(coursename, ?) > 0 "
						"UNION ALL "
						"SELECT T1.coursename, T1.style, T1.entries AS rank, T1.entries, 0 AS duration_ms, 0 AS end_time "
							"FROM "
								"(SELECT coursename, style, entries FROM LocalRun WHERE instr(coursename, ?) > 0 GROUP BY coursename, style) T1 "
								"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE username = ? AND instr(coursename, ?) > 0 GROUP BY coursename, style) T2 "
								"ON T1.coursename = T2.coursename AND T1.style = T2.style "
							"WHERE T2.coursename IS NULL OR T2.style IS NULL) "	
					"ORDER BY (entries-((entries/cast(rank as float))+(entries-rank)/2.0)) DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 2, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 3, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 4, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 5, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 6, start));
			}
			else {
				//sql = "SELECT coursename, style, rank, entries, duration_ms, end_time FROM LocalRun WHERE rank != 0 AND username = ? ORDER BY (entries - entries / rank) DESC LIMIT ?, 10";
				sql = "SELECT * FROM "
						"(SELECT coursename, style, rank, entries, duration_ms, end_time "
							"FROM LocalRun WHERE rank != 0 AND username = ? "
						"UNION ALL "
						"SELECT T1.coursename, T1.style, T1.entries AS rank, T1.entries, 0 AS duration_ms, 0 AS end_time "
							"FROM "
								"(SELECT coursename, style, entries FROM LocalRun GROUP BY coursename, style) T1 "
								"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE username = ? GROUP BY coursename, style) T2 "
								"ON T1.coursename = T2.coursename AND T1.style = T2.style "
							"WHERE T2.coursename IS NULL OR T2.style IS NULL) "	
					"ORDER BY (entries-((entries/cast(rank as float))+(entries-rank)/2.0)) DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 2, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 3, start));
			}	
		}
		else {
			if (enteredCoursename) {
				//sql = "SELECT coursename, style, rank, entries, duration_ms, end_time FROM LocalRun WHERE rank != 0 AND username = ? AND style = ? AND instr(coursename, ?) > 0 ORDER BY (entries - entries / rank) DESC LIMIT ?, 10";
				sql = "SELECT * FROM "
						"(SELECT coursename, style, rank, entries, duration_ms, end_time "
							"FROM LocalRun WHERE rank != 0 AND username = ? AND style = ? AND instr(coursename, ?) > 0 "
						"UNION ALL "
						"SELECT T1.coursename, T1.style, T1.entries AS rank, T1.entries, 0 AS duration_ms, 0 AS end_time "
							"FROM "
								"(SELECT coursename, style, entries FROM LocalRun WHERE style = ? AND instr(coursename, ?) > 0 GROUP BY coursename, style) T1 "
								"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE username = ? AND style = ? AND instr(coursename, ?) > 0 GROUP BY coursename, style) T2 "
								"ON T1.coursename = T2.coursename AND T1.style = T2.style "
							"WHERE T2.coursename IS NULL OR T2.style IS NULL) "	
					"ORDER BY (entries-((entries/cast(rank as float))+(entries-rank)/2.0)) DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 2, style));
					CALL_SQLITE (bind_text (stmt, 3, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 4, style));
					CALL_SQLITE (bind_text (stmt, 5, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_text (stmt, 6, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 7, style));
					CALL_SQLITE (bind_text (stmt, 8, partialCourseName, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 9, start));
			}
			else {
				//sql = "SELECT coursename, style, rank, entries, duration_ms, end_time FROM LocalRun WHERE rank != 0 AND username = ? AND style = ? ORDER BY (entries - entries / rank) DESC LIMIT ?, 10";
				sql = "SELECT * FROM "
						"(SELECT coursename, style, rank, entries, duration_ms, end_time "
							"FROM LocalRun WHERE rank != 0 AND username = ? AND style = ?"
						"UNION ALL "
						"SELECT T1.coursename, T1.style, T1.entries AS rank, T1.entries, 0 AS duration_ms, 0 AS end_time "
							"FROM "
								"(SELECT coursename, style, entries FROM LocalRun WHERE style = ? GROUP BY coursename, style) T1 "
								"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE username = ? AND style = ? GROUP BY coursename, style) T2 "
								"ON T1.coursename = T2.coursename AND T1.style = T2.style "
							"WHERE T2.coursename IS NULL OR T2.style IS NULL) "	
					"ORDER BY (entries-((entries/cast(rank as float))+(entries-rank)/2.0)) DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 2, style));
					CALL_SQLITE (bind_int (stmt, 3, style));
					CALL_SQLITE (bind_text (stmt, 4, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 5, style));
					CALL_SQLITE (bind_int (stmt, 6, start));
			}
		}

		trap->SendServerCommand(ent-g_entities, va("print \"Most improvable scores for %s:\n    ^5Course                      Style      Rank    Entries      Time         Date\n\"", styleString)); //Color rank yellow for global, normal for season -fixme match race print scheme
		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				IntegerToRaceName(sqlite3_column_int(stmt, 1), styleStr, sizeof(styleStr));
				if (sqlite3_column_int(stmt, 5)) {
					Q_strncpyz(rankStr, va("%i", sqlite3_column_int(stmt, 2)), sizeof(rankStr));
					TimeToString(sqlite3_column_int(stmt, 4), timeStr, sizeof(timeStr), qfalse);
					getDateTime(sqlite3_column_int(stmt, 5), dateStr, sizeof(dateStr)); 
				}
				else {
					Q_strncpyz(rankStr, "N/A", sizeof(rankStr));
					Q_strncpyz(timeStr, "N/A", sizeof(timeStr));
					Q_strncpyz(dateStr, "N/A", sizeof(dateStr));
				}

				tmpMsg = va("^5%2i^3: ^3%-27s ^3%-10s ^3%-7s ^3%-12i ^3%-12s %s\n", row+start, sqlite3_column_text(stmt, 0), styleStr, rankStr, sqlite3_column_int(stmt, 3), timeStr, dateStr);
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFTodo_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
		CALL_SQLITE (finalize(stmt));

		CALL_SQLITE (close(db));
	}

}

void Cmd_DFPopular_f(gentity_t *ent) {
	const int args = trap->Argc();
	int style = -1, page = -1, season = -1, start = 0, i, input;
	char styleString[16] = {0}, inputString[32], username[16];
	qboolean enteredUsername = qfalse;

	if (args > 5) {
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rPopular <style (optional)> <season (optional - example: s1)> <me (optional)> <page (optional)>\n\"");
		return;
	}

	for (i = 1 ; i < args; i++) {
		trap->Argv(i, inputString, sizeof(inputString));
		if (style == -1) {
			input = RaceNameToInteger(inputString);
			if (input != -1) {
				style = input;
				continue;
			}
		}
		if (season == -1) {
			input = SeasonToInteger(inputString);
			if (input != -1) {
				season = input;
				continue;
			}
		}
		if (page == -1) {
			input = atoi(inputString);
			if (input > 0) {
				page = input;
				continue;
			}
		}
		if (!enteredUsername) {
			if (!Q_stricmp(inputString, "me")) {
				Q_strncpyz(username, ent->client->pers.userName, sizeof(username));
				enteredUsername = qtrue;
				continue;
			}
		}
		trap->SendServerCommand(ent-g_entities, "print \"Usage: /rPopular <style (optional)> <season (optional - example: s1)> <me (optional)> <page (optional)>\n\"");
		return; //Arg doesnt match any expected values so error.
	}

	if (enteredUsername && (!ent->client->pers.userName || !ent->client->pers.userName[0])) {
		trap->SendServerCommand(ent-g_entities, "print \"You must be logged in to use this command.\n\"");
		return;
	}

	if (style == -1) {
		Q_strncpyz(styleString, "all styles", sizeof(styleString));
	}
	else {
		IntegerToRaceName(style, styleString, sizeof(styleString));
		Q_strcat(styleString, sizeof(styleString), " style");
	}

	if (page < 1)
		page = 1;
	if (page > 1000)
		page = 1000;
	start = (page - 1) * 10;

	{
		sqlite3 * db;
		char * sql;
		sqlite3_stmt * stmt;
		char msg[1024-128] = {0}, styleStr[16], entriesStr[16];
		int s, row = 1;

		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		if (style == -1) {
			if (enteredUsername) {
				if (season == -1) { //User
					sql = "SELECT T1.coursename, T1.style, T1.entries, T1.username "
								"FROM (SELECT coursename, style, entries, username FROM LocalRun WHERE rank = 1 GROUP BY coursename, style) T1 "
									"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE username = ? GROUP BY coursename, style) T2 "
									"ON T1.coursename = T2.coursename AND T1.style = T2.style "
								"WHERE T2.coursename IS NULL OR T2.style IS NULL "
					"ORDER BY entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_text (stmt, 1, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 2, start));
				}
				else { //User, season
					sql = "SELECT T1.coursename, T1.style, T1.season_entries, T1.username "
								"FROM (SELECT coursename, style, season_entries, username FROM LocalRun WHERE season_rank = 1 AND season = ? GROUP BY coursename, style) T1 "
									"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE season = ? AND username = ? GROUP BY coursename, style) T2 "
									"ON T1.coursename = T2.coursename AND T1.style = T2.style "
								"WHERE T2.coursename IS NULL OR T2.style IS NULL "
					"ORDER BY season_entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, season));
					CALL_SQLITE (bind_int (stmt, 2, season));
					CALL_SQLITE (bind_text (stmt, 3, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 4, start));
				}
			}
			else {
				if (season == -1) {
					sql = "SELECT coursename, style, entries, username FROM LocalRun WHERE rank = 1 GROUP BY coursename, style ORDER BY entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, start));
				}
				else { //Season
					sql = "SELECT coursename, style, season_entries, username FROM LocalRun WHERE season_rank = 1 AND season = ? GROUP BY coursename, style ORDER BY season_entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, season));
					CALL_SQLITE (bind_int (stmt, 2, start));
				}
			}	
		}
		else {
			if (enteredUsername) {
				if (season == -1) { //Style, user
					sql = "SELECT T1.coursename, T1.style, T1.entries, T1.username "
								"FROM (SELECT coursename, style, entries FROM LocalRun WHERE style = ? GROUP BY coursename, style) T1 "
									"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE style = ? AND username = ? GROUP BY coursename, style) T2 "
									"ON T1.coursename = T2.coursename AND T1.style = T2.style "
								"WHERE T2.coursename IS NULL OR T2.style IS NULL "
					"ORDER BY entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, style));
					CALL_SQLITE (bind_int (stmt, 2, style));
					CALL_SQLITE (bind_text (stmt, 3, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 4, start));
				}
				else { //Style, user, season
					sql = "SELECT T1.coursename, T1.style, T1.season_entries, T1.username "
								"FROM (SELECT coursename, style, season_entries, username FROM LocalRun WHERE season_rank = 1 AND style = ? AND season = ? GROUP BY coursename, style) T1 "
									"LEFT JOIN (SELECT coursename, style FROM LocalRun WHERE style = ? AND season = ? AND username = ? GROUP BY coursename, style) T2 "
									"ON T1.coursename = T2.coursename AND T1.style = T2.style "
								"WHERE T2.coursename IS NULL OR T2.style IS NULL "
					"ORDER BY season_entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, style));
					CALL_SQLITE (bind_int (stmt, 2, season));
					CALL_SQLITE (bind_int (stmt, 3, style));
					CALL_SQLITE (bind_int (stmt, 4, season));
					CALL_SQLITE (bind_text (stmt, 5, username, -1, SQLITE_STATIC));
					CALL_SQLITE (bind_int (stmt, 6, start));
				}
			}
			else {
				if (season == -1) { //Style
					sql = "SELECT coursename, style, entries, username FROM LocalRun WHERE rank = 1 AND style = ? GROUP BY coursename, style ORDER BY entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, style));
					CALL_SQLITE (bind_int (stmt, 2, start));
				}
				else { //Style, season
					sql = "SELECT coursename, style, season_entries, username FROM LocalRun WHERE season_rank = 1 AND style = ? AND season = ? GROUP BY coursename, style ORDER BY season_entries DESC LIMIT ?, 10";
					CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
					CALL_SQLITE (bind_int (stmt, 1, style));
					CALL_SQLITE (bind_int (stmt, 2, season));
					CALL_SQLITE (bind_int (stmt, 3, start));
				}
			}
		}

		if (season == -1)
			trap->SendServerCommand(ent-g_entities, va("print \"Most popular courses for %s:\n    ^5Course                      Style      Entries      Winner\n\"", styleString));
		else
			trap->SendServerCommand(ent-g_entities, va("print \"Most popular courses for %s season %i:\n    ^5Course                      Style      Entries      Winner\n\"", styleString, season));

		while (1) {
			s = sqlite3_step(stmt);
			if (s == SQLITE_ROW) {
				char *tmpMsg = NULL;
				IntegerToRaceName(sqlite3_column_int(stmt, 1), styleStr, sizeof(styleStr));
				Com_sprintf(entriesStr, sizeof(entriesStr), "%i", sqlite3_column_int(stmt, 2));

				tmpMsg = va("^5%2i^3: ^3%-27s ^3%-10s ^3%-12s %s\n", row+start, sqlite3_column_text(stmt, 0), styleStr, entriesStr, sqlite3_column_text(stmt, 3));
				if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
					trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
					msg[0] = '\0';
				}
				Q_strcat(msg, sizeof(msg), tmpMsg);
				row++;
			}
			else if (s == SQLITE_DONE)
				break;
			else {
				G_ErrorPrint("ERROR: SQL Select Failed (Cmd_DFTodo_f)", s);
				break;
			}
		}
		trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
		CALL_SQLITE (finalize(stmt));

		CALL_SQLITE (close(db));
	}

}

#if !_NEWRACERANKING
void Cmd_DFRefresh_f(gentity_t *ent) {
	if (ent->client && ent->client->sess.fullAdmin) {//Logged in as full admin
		if (!(g_fullAdminLevel.integer & (1 << A_BUILDHIGHSCORES))) {
			trap->SendServerCommand( ent-g_entities, "print \"You are not authorized to use this command (dfRefresh).\n\"" );
			return;
		}
	}
	else if (ent->client && ent->client->sess.juniorAdmin) {//Logged in as junior admin
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
	//BuildMapHighscores(); //From db, built to memory
}
#endif

void Cmd_ACWhois_f( gentity_t *ent ) { //why does this crash sometimes..? conditional open/close issue??
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;
	qboolean	whois = qfalse, seeip = qfalse;
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	if (ent->client->sess.fullAdmin) {//Logged in as full admin
		if (g_fullAdminLevel.integer & (1 << A_WHOIS))
			whois = qtrue;
		if (g_fullAdminLevel.integer & (1 << A_SEEIP))
			seeip = qtrue;
	}
	else if (ent->client->sess.juniorAdmin) {//Logged in as junior admin
		if (g_juniorAdminLevel.integer & (1 << A_WHOIS))
			whois = qtrue;
		if (g_juniorAdminLevel.integer & (1 << A_SEEIP))
			seeip = qtrue;
	}
	
	//A_STATUS

	if (whois) {
		CALL_SQLITE (open (LOCAL_DB_PATH, & db));
		sql = "SELECT username FROM LocalAccount WHERE lastip = ?";
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	}

	if (whois && seeip) {
		if (g_raceMode.integer)
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            IP                Plugin  Admin  Race  Style    Jump  Hidden  Nickame\n\"");
		else
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            IP                Plugin  Admin  Nickname\n\"");
	}
	else if (whois) {
		if (g_raceMode.integer)
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            Plugin  Admin  Race  Style    Jump  Hidden  Nickame\n\"");
		else
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            Plugin  Admin  Nickname\n\"");
	}
	else {
		if (g_raceMode.integer)
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            Plugin  Race  Style    Jump  Hidden  Nickname\n\"");
		else
			trap->SendServerCommand(ent-g_entities, "print \"^5   Username            Plugin  Nickname\n\"");
	}

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clients
		char *tmpMsg = NULL;
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (cl->pers.netname[0]) { // && cl->pers.userName[0] ?
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strUser[20] = {0};
			char strIP[NET_ADDRSTRMAXLEN] = {0};
			char strAdmin[32] = {0};
			char strPlugin[32] = {0};
			char strRace[32] = {0};
			char strHidden[32] = {0};
			char strStyle[32] = {0};
			char jumpLevel[32] = {0};
			char *p = NULL;

			Q_strncpyz(strNum, va("^5%2i^3:", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));
			Com_sprintf(strUser, sizeof(strUser), "^7%s^7", cl->pers.userName);
			Q_strncpyz(strIP, cl->sess.IP, sizeof(strIP));

			if (cl->sess.sessionTeam != TEAM_SPECTATOR)
				Q_strncpyz(jumpLevel, va("%i", cl->ps.fd.forcePowerLevel[FP_LEVITATION]), sizeof(jumpLevel));

			if (whois || seeip) {
				p = strchr(strIP, ':');
				if (p) //loda - fix ip sometimes not printing in amstatus?
					*p = 0;
			}
			if (whois) {
				if (cl->sess.fullAdmin)
					Q_strncpyz( strAdmin, "^3Full^7", sizeof(strAdmin));
				else if (cl->sess.juniorAdmin)
					Q_strncpyz(strAdmin, "^3Junior^7", sizeof(strAdmin));
				else
					Q_strncpyz(strAdmin, "^7None^7", sizeof(strAdmin));
			}

			if (g_raceMode.integer) {
				if (cl->sess.sessionTeam == TEAM_SPECTATOR) {
					Q_strncpyz(strStyle, "^7^7", sizeof(strStyle));
					Q_strncpyz(strRace, "^7^7", sizeof(strRace));
					Q_strncpyz(strHidden, "^7^7", sizeof(strHidden));
				}
				else {
					char strStyleName[16] = {0};
					Q_strncpyz(strRace, (cl->sess.raceMode) ? "^2Yes^7" : "^1No^7", sizeof(strRace));
					Q_strncpyz(strHidden, (cl->pers.noFollow) ? "^2Yes^7" : "^1No^7", sizeof(strHidden));

					Q_strncpyz(strStyle, "^7", sizeof(strStyle));
					IntegerToRaceName(cl->ps.stats[STAT_MOVEMENTSTYLE],strStyleName, sizeof(strStyleName));
					Q_strcat(strStyle, sizeof(strStyle), va("%s^7", strStyleName));
				}
			}

			if (g_entities[i].r.svFlags & SVF_BOT)
				Q_strncpyz(strPlugin, "^7Bot^7", sizeof(strPlugin));
			else
				Q_strncpyz(strPlugin, (cl->pers.isJAPRO) ? "^2Yes^7" : "^1No^7", sizeof(strPlugin));

			if (whois) { //No username means not logged in, so check if they have an account tied to their ip
				if (!cl->pers.userName[0]) {
					unsigned int ip;

					ip = ip_to_int(strIP);

					CALL_SQLITE (bind_int64 (stmt, 1, ip));

					s = sqlite3_step(stmt);

					if (s == SQLITE_ROW) {
						if (ip)
							//Q_strncpyz(strUser, (char*)sqlite3_column_text(stmt, 0), sizeof(strUser));
							Com_sprintf(strUser, sizeof(strUser), "^3%s^7", (char*)sqlite3_column_text(stmt, 0));
					}
					else if (s != SQLITE_DONE) {
						G_ErrorPrint("ERROR: SQL Select Failed (Cmd_ACWhois_f)", s);
						CALL_SQLITE (finalize(stmt));
						CALL_SQLITE (close(db));
						return;
					}

					CALL_SQLITE (reset (stmt));
					CALL_SQLITE (clear_bindings (stmt));
				}

				if (seeip) {
					//Admin prints
					if (g_raceMode.integer)
						tmpMsg = va( "%-2s%-24s%-18s%-12s%-11s%-10s%-13s%-6s%-12s%s\n", strNum, strUser, strIP, strPlugin, strAdmin, strRace, strStyle, jumpLevel, strHidden, strName);
					else
						tmpMsg = va( "%-2s%-24s%-18s%-12s%-11s%s\n", strNum, strUser, strIP, strPlugin, strAdmin, strName);
				}
				else {
					//Admin prints
					if (g_raceMode.integer)
						tmpMsg = va( "%-2s%-24s%-12s%-11s%-10s%-13s%-6s%-12s%s\n", strNum, strUser, strPlugin, strAdmin, strRace, strStyle, jumpLevel, strHidden, strName);
					else
						tmpMsg = va( "%-2s%-24s%-12s%-11s%s\n", strNum, strUser, strPlugin, strAdmin, strName);
				}
			}
			else {//Not admin
				if (g_raceMode.integer)
					tmpMsg = va( "%-2s%-24s%-12s%-10s%-13s%-6s%-12s%s\n", strNum, strUser, strPlugin, strRace, strStyle, jumpLevel, strHidden, strName);
				else
					tmpMsg = va( "%-2s%-24s%-12s%s\n", strNum, strUser, strPlugin, strName);
			}
			
			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}

	if (whois) {
		CALL_SQLITE (finalize(stmt));
		CALL_SQLITE (close(db));
	}

	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));

	//DebugWriteToDB("Cmd_ACWhois_f");
}

void InitGameAccountStuff( void ) { //Called every mapload , move the create table stuff to something that gets called every srvr start.. eh?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
	int s;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));

	//sqlite_exec(db, "VACUUM;", 0, 0);
	//index LocalRun on RANK
	//use transactions
	//COUNT_CHANGES off

	sql = "CREATE TABLE IF NOT EXISTS LocalAccount(id INTEGER PRIMARY KEY, username VARCHAR(16), password VARCHAR(16), kills UNSIGNED SMALLINT, deaths UNSIGNED SMALLINT, "
		"suicides UNSIGNED SMALLINT, captures UNSIGNED SMALLINT, returns UNSIGNED SMALLINT, racetime UNSIGNED INTEGER, lastlogin UNSIGNED INTEGER, created UNSIGNED INTEGER, lastip UNSIGNED INTEGER, flags UNSIGNED TINYINT)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Create Failed (InitGameAccountStuff 1)", s);
	CALL_SQLITE (finalize(stmt));

#if 1//NEWRACERANKING
	sql = "CREATE TABLE IF NOT EXISTS LocalRun(id INTEGER PRIMARY KEY, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED TINYINT, season UNSIGNED TINYINT, end_time UNSIGNED INTEGER, rank UNSIGNED SMALLINT, entries UNSIGNED SMALLINT, season_rank UNSIGNED SMALLINT, season_entries UNSIGNED SMALLINT, last_update UNSIGNED INTEGER)";
#else
	sql = "CREATE TABLE IF NOT EXISTS LocalRun(id INTEGER PRIMARY KEY, username VARCHAR(16), coursename VARCHAR(40), duration_ms UNSIGNED INTEGER, topspeed UNSIGNED SMALLINT, "
		"average UNSIGNED SMALLINT, style UNSIGNED TINYINT, end_time UNSIGNED INTEGER)";
#endif
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Create Failed (InitGameAccountStuff 2)", s);
	CALL_SQLITE (finalize(stmt));

	sql = "CREATE TABLE IF NOT EXISTS LocalDuel(id INTEGER PRIMARY KEY, winner VARCHAR(16), loser VARCHAR(16), duration UNSIGNED SMALLINT, "
		"type UNSIGNED TINYINT, winner_hp UNSIGNED TINYINT, winner_shield UNSIGNED TINYINT, end_time UNSIGNED INTEGER, winner_elo DECIMAL(6,2), loser_elo DECIMAL(6,2), odds DECIMAL(9,2))";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	s = sqlite3_step(stmt);
	if (s != SQLITE_DONE)
		G_ErrorPrint("ERROR: SQL Create Failed (InitGameAccountStuff 3)", s);
	CALL_SQLITE (finalize(stmt));

#if _ELORANKING
	/*
	sql = "CREATE TABLE IF NOT EXISTS DuelRanks(id INTEGER PRIMARY KEY, username VARCHAR(16), type UNSIGNED SMALLINT, rank DECIMAL(6,2), TSSUM DECIMAL(9,2), count UNSIGNED INTEGER)"; //We only need like 2 decimal precision here so how do that in sqlite C? --todo
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));
	*/
#endif

#if 0//NEWRACERANKING
	sql = "CREATE TABLE IF NOT EXISTS RaceRanks(id INTEGER PRIMARY KEY, username VARCHAR(16), style UNSIGNED SMALLINT, score DECIMAL(6,2), percentilesum DECIMAL(6,2), ranksum DECIMAL(6,2), golds UNSIGNED SMALLINT, silvers UNSIGNED SMALLINT, bronzes UNSIGNED SMALLINT, count UNSIGNED SMALLINT)"; //We only need like 2 decimal precision here so how do that in sqlite C? --todo
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	CALL_SQLITE_EXPECT (step (stmt), DONE);
	CALL_SQLITE (finalize(stmt));
#endif

	CALL_SQLITE (close(db));

	CleanupLocalRun(); //Deletes useless shit from LocalRun database table
#if !_NEWRACERANKING
	G_AddToDBFromFile(); //Add last maps highscores
#endif
	//BuildMapHighscores();//Build highscores into memory from database

	//DebugWriteToDB("InitGameAccountStuff");
}

void G_SpawnWarpLocationsFromCfg(void) //loda fixme
{
	fileHandle_t f;	
	int		fLen = 0, i, MAX_FILESIZE = 4096, MAX_NUM_WARPS = 64, args = 1, row = 0;  //use max num warps idk
	char	filename[MAX_QPATH+4] = {0}, info[1024] = {0}, buf[4096] = {0};//eh
	char*	pch;

	trap->GetServerinfo(info, sizeof(info));
	Q_strncpyz(filename, Info_ValueForKey(info, "mapname"), sizeof(filename));
	Q_strlwr(filename);//dat linux
	Q_strcat(filename, sizeof(filename), "_warps.cfg");

	for(i = 0; i < strlen(filename); i++) {//Replace / in mapname with _ since we cant have a file named mp/duel1.cfg etc.
		if (filename[i] == '/')
			filename[i] = '_'; 
	} 

	fLen = trap->FS_Open(filename, &f, FS_READ);

	if (!f) {
		//Com_Printf ("Couldn't load tele locations from %s\n", filename);
		return;
	}
	if (fLen >= MAX_FILESIZE) {
		trap->FS_Close(f);
		Com_Printf ("Couldn't load tele locations from %s, file is too large\n", filename);
		return;
	}

	trap->FS_Read(buf, fLen, f);
	buf[fLen] = 0;
	trap->FS_Close(f);

	pch = strtok (buf," \n\t");  //loda fixme why is this broken
	while (pch != NULL && row < MAX_NUM_WARPS)
	{
		if ((args % 5) == 1)
			Q_strncpyz(warpList[row].name, pch, sizeof(warpList[row].name));
		else if ((args % 5) == 2)
			warpList[row].x = atoi(pch);
		else if ((args % 5) == 3)
			warpList[row].y = atoi(pch);
		else if ((args % 5) == 4)
			warpList[row].z = atoi(pch);
		else if ((args % 5) == 0) {
			warpList[row].yaw = atoi(pch);
			//trap->Print("Warp added: %s, <%i, %i, %i, %i>\n", warpList[row].name, warpList[row].x, warpList[row].y, warpList[row].z, warpList[row].yaw);
			row++;
		}
    	pch = strtok (NULL, " \n\t");
		args++;
	}

	Com_Printf ("Loaded warp locations from %s\n", filename);
}

#if 0
void AddRunToWebServer(RaceRecord_t record) 
{ 

	//fetch_response();

	CURL *curl;
	char address[128], data[256], password[64];
	CURLcode res;


	Q_strncpyz(address, sv_webServerPath.string, sizeof(address));
	Q_strncpyz(password, sv_webServerPassword.string, sizeof(password));

	//Case, special chars matter? clean??  Encode coursename / username for html ?


	Q_strncpyz(record.username, "testuser", sizeof(record.username));
	Q_strncpyz(record.coursename, "testcourse", sizeof(record.coursename));
	record.duration_ms = 123456;
	record.topspeed = 835;
	record.average = 652;
	record.style = 3;
	record.end_timeInt = 26246234;


	Com_sprintf(data, sizeof(data), "username=%s&coursename=%s&duration_ms=%i&topspeed=%i&average=%i&style=%i&end_time=%i", 
		record.username, record.coursename, record.duration_ms, record.topspeed, record.average, record.style, record.end_time);

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_URL, address);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		res = curl_easy_perform(curl); 
	
		if(res != CURLE_OK) 
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res)); 
		else 
			trap->Print("cURL Worked?\n"); //de fuck izzat

		curl_easy_cleanup(curl);
	}
	else 
		trap->Print("ERROR: Libcurl failed\n"); //de fuck izzat


} 
#endif