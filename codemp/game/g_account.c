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
"Run" Entity
+------------+-------------+------+-----+---------+-------+
| Field		 | Type        | Null | Key | Default | Extra |
+------------+-------------+------+-----+---------+-------+
| mapname    | varchar(32) | YES  |     | NULL    |       | //Dunno what the longest jka map would be, 32 seems more than enough
| coursename | varchar(16) | YES  |     | NULL    |       | //longest coursename atm is .. lotiymens? only 9 long.  If it goes over its not gamebraeking, it will still be unique most likely.
| user_id	 | int		   | YES  |     | NULL    |       |
| duration_ms| int		   | YES  |     | NULL    |       | //24 bits would allow 4+ hours of time to be recorded, should be plenty? or maybe go to 26bits for people with autism.
| end_time	 | datetime    | YES  |     | NULL    |       | 
| id         | int         | YES  |     | NULL    |       | 
| style      | int         | YES  |     | NULL    |       | //max value atm is 6, future proof to 16?
+------------+-------------+------+-----+---------+-------+

"PlayerServerAccount" Entity, account of player specific to this server (though he logs in using IP matching tied to his website account)
+------------+-------------+------+-----+---------+-------+
| Field		 | Type        | Null | Key | Default | Extra |
+------------+-------------+------+-----+---------+-------+
| username   | varchar(32) | YES  |     | NULL    |       |
| id		 | int		   | YES  |     | NULL    |       |
//The following (rest of fields in gameaccount, and the Duel entity) will be in v2 update, after defrag functionality is complete
//Simple stuff that does not need to be described or have its own entity
| playtime	 | int		   | YES  |     | NULL    |       |
| kills      | int         | YES  |     | NULL    |       |
| deaths     | int         | YES  |     | NULL    |       |
| captures   | int         | YES  |     | NULL    |       |
| returns    | int         | YES  |     | NULL    |       | //I guess all these could be 16bits or something really
+------------+-------------+------+-----+---------+-------+

"Duel" Entity, can we just user winner_id, loser_id, and ignore ties? or do we have to do id1, id2, and make 'outcome' to store wether id1>id2, id2>id1, or tie?
+---------------+---------=----+------+-----+---------+-------+
| Field		    | Type         | Null | Key | Default | Extra |
+---------------+--------------+------+-----+---------+-------+
| winner_id		| int		   | YES  |     | NULL    |       |
| loser_id		| int		   | YES  |     | NULL    |       |
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

void Cmd_ACLogin_f( gentity_t *ent ) {
	//Client inputs account name, server querys user database on website, checks last known IP for that username, if match, client is logged in.
	//This will require client to revisit the master website if their IP changes (and be logged in there).
	//But it stops client from having to send password over JKA, and lets global accounts work nice

	if (trap->Argc() != 2) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: /login <username>\n\"" );
		return;
	}

	TestInsert();
	TestSelect();

	//Connect to master acounts db, check lastKnownIP of username, compare to ent->client->ip, if match, log player in.
	//If not, print message and quit (no account found, invalid pass, etc).

}

static void Cmd_ACLogout_f( gentity_t *ent ) {
	//If logged in, print logout msg, remove login status.
}

static void DFBuildTop10( gentity_t *ent ) {
	//Get mapname
	//Get all courses for mapname? how?
	//Build the top10lists for each course
		//Use selection algorithm to get 10th smallest time
}
static void Cmd_DFTop10_f( gentity_t *ent ) {

	//This is a server specific top 10, not global.

	//if more than 1 course per map, require usage "/dftop10 <coursename>"
	//else display map top10

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
			char strAccount[32] = {0};

			Q_strncpyz(strNum, va("(%i)", i), sizeof(strNum));
			Q_strncpyz(strName, cl->pers.netname, sizeof(strName));

			if (1)//they are logged in ?
				Q_strncpyz( strAccount, "dehhh", sizeof(strAccount));	
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

	//Send the following to database:
		//Player account name
		// ?? Player IP ??
		//Mapname
		//Course name
		//Time duration of run
		//Avg speed
		//Max speed
		//Physics mode
		//Time right now
		//Player netname ?
		//Server IP?
}

static void SyncToGlobalDB( gentity_t *ent ) {
	//every map load, local DB is sends changes to global DB, hosted on master website.
}
