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

//No register function, thats done on master website.
//ent->client->accountID ? if 0 , not logged in ?, use this in interaction with db?

//When user logs in, if no PlayerServerAccount is created for them yet, create it.  If it already exists, just update it.

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

//In the v2 update, maybe also add ingame passwords, so people sharing a net connection (at university etc) wont be able to login to eachothers accounts ingame.

void TestInsert ()
{
    sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;

    CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "INSERT INTO t (xyz) VALUES (?)";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
    CALL_SQLITE (bind_text (stmt, 1, "testtesttest", 6, SQLITE_STATIC));
    CALL_SQLITE_EXPECT (step (stmt), DONE);
    trap->Print("row id was %d\n", (int)sqlite3_last_insert_rowid(db));
}

void G_AddRunToDB(char *account, char *courseName, float time, int style, int topspeed, int average) {//should be short.. but have to change elsewhere? is it worth it?
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;

	int datetime = 0;//yeah use rawtime or something?

    CALL_SQLITE (open (LOCAL_DB_PATH, & db));
	sql = va("INSERT INTO Run (account, courseName, duration, style, topspeed, average) VALUES (%i, %s, %i, %i, %i, %i, %i)", //the last %i should be datetime or something 
		account, courseName, time, style, topspeed, average, datetime);
	CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));//what does this do, send the insert query?
    CALL_SQLITE (bind_text (stmt, 1, "testtesttest", 6, SQLITE_STATIC));//what does this do?
    CALL_SQLITE_EXPECT (step (stmt), DONE); //what does this do?
}

void TestSelect ()
{
    sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;
    int row = 0;

    CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "SELECT * FROM t";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));

    while (1) {
        int s;

        s = sqlite3_step (stmt);
        if (s == SQLITE_ROW) {
            int bytes;
            const unsigned char *text;
            bytes = sqlite3_column_bytes(stmt, 0);
            text  = sqlite3_column_text (stmt, 0);
            trap->Print("%d: %s\n", row, text);
            row++;
        }
        else if (s == SQLITE_DONE)
            break;
        else {
            fprintf (stderr, "ERROR: SQL Select Failed.\n");//trap print?
			break;
        }
    }
}

void InitGameAccountStuff( void ) {
	/*
	sqlite3 * db;
    char * sql;
    sqlite3_stmt * stmt;

	CALL_SQLITE (open (LOCAL_DB_PATH, & db));
    sql = "SELECT COUNT(*) FROM LocalAccount";
    CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));
	*/

	//if (atoi((const char *)sqlite3_column_text (stmt, 0)) == 0) //Table not created yet..? actually no it could be created but empty :S

	//if (sqlite3FindTable(db, "sqlite_stat1", pDb->zName))
		//return;

	//If no "LocalAccount" table exists, create one
	//If no "LocalRun" table exists, create one
	//If no "LocalDuel" table exists, create one

	//Check if stuff needs to be sent to global db? idk
}

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
	if (Q_stricmp(ent->client->pers.accountName, "")) {
		trap->SendServerCommand(ent-g_entities, "print \"You are already logged in!\n\"");
		return;
	}

	while (ent->client->pers.accountName[i]) {
		accountName[i] = tolower(ent->client->pers.accountName[i]);
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
		sql = va("INSERT INTO LocalAccount (accountname) VALUES (%s)", accountName);
		CALL_SQLITE (prepare_v2 (db, sql, strlen (sql) + 1, & stmt, NULL));//what does this do, send the insert query?
		CALL_SQLITE (bind_text (stmt, 1, "testtesttest", 6, SQLITE_STATIC));//what does this do?
		CALL_SQLITE_EXPECT (step (stmt), DONE); //what does this do?
	}


	Q_strncpyz(ent->client->pers.accountName, accountName, sizeof(ent->client->pers.accountName));
}

static void Cmd_ACLogout_f( gentity_t *ent ) {
	//If logged in, print logout msg, remove login status.
	//if (ent->client->pers.accountName) {
	if (Q_stricmp(ent->client->pers.accountName, "")) {
		Q_strncpyz(ent->client->pers.accountName, "", sizeof(ent->client->pers.accountName));
		trap->SendServerCommand(ent-g_entities, "print \"Logged out.\n\"");
	}
	else
		trap->SendServerCommand(ent-g_entities, "print \"You are not logged in!\n\"");
}

static void DFBuildTop10( gentity_t *ent ) {
	//Get mapname
	//Get all courses for mapname? how?
	//Build the top10lists for each course
		//Use selection algorithm to get 10th smallest time
}
static void Cmd_DFTop10_f( gentity_t *ent ) {

	//This is a server specific top 10, not global.

	//if more than 1 course per map, require usage "/dftop10 <coursename>", how to find this out?  or do it based on how many courses there are for this map in the db?
	//else display map top10

	//dftop10 <mapname> <coursename> <style> is full syntax... how deal with partial entries?

	//At mapload, get 10thfastest time for that map
	//Every time someone completes course, check if it was faster than 10thfastest
	//If so, rebuild top10 list?  And redefine 10thfastest?
}

static void Cmd_ACWhois_f( gentity_t *ent ) {
	int			i;
	char		msg[1024-128] = {0};
	gclient_t	*cl;

	for (i=0; i<MAX_CLIENTS; i++) {//Build a list of clients
		char *tmpMsg = NULL;
		if (!g_entities[i].inuse)
			continue;
		cl = &level.clients[i];
		if (cl->pers.netname[0]) {
			char strNum[12] = {0};
			char strName[MAX_NETNAME] = {0};
			char strAccount[16] = {0};

			Q_strncpyz(strNum, va("(%i)", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));

			Q_strncpyz( strAccount, cl->pers.accountName, sizeof(strAccount));	
			tmpMsg = va("%-5s%-18s^7%-14s^7\n", strNum, strName, strAccount);

			if (strlen(msg) + strlen(tmpMsg) >= sizeof( msg)) {
				trap->SendServerCommand( ent-g_entities, va("print \"%s\"", msg));
				msg[0] = '\0';
			}
			Q_strcat(msg, sizeof(msg), tmpMsg);
		}
	}
	trap->SendServerCommand(ent-g_entities, va("print \"%s\"", msg));
}

static void AddRaceTime( gentity_t *ent ) {

	//Create db connection TO LOCAL DB or whatever? or is it already created?

	//Send the following to database LocalRun table:
		//Player account id
		//Map + course name
		//Time duration of run
		//Avg speed
		//Max speed
		//Physics mode
		//Time right now
}

static void SyncToGlobalDB( gentity_t *ent ) {
	//every map load, local DB is sends changes to global DB, hosted on master website.
}
