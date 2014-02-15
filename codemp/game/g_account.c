#include "g_local.h"

//No register function, thats done on master website.
//ent->client->accountID ? if 0 , not logged in ?, use this in interaction with db?

static void Cmd_ACLogin_f( gentity_t *ent ) {

	//Client inputs account name, server querys user database on website, checks last known IP for that username, if match, client is logged in.
	//This will require client to revisit the master website if their IP changes (and be logged in there).
	//But it stops client from having to send password over JKA, and lets global accounts work nice

	if (trap->Argc() != 2) {
		trap->SendServerCommand( ent-g_entities, "print \"Usage: /login <username>\n\"" );
		return;
	}

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