/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// sv_client.c -- server code for dealing with clients

#include "server.h"
#include "qcommon/stringed_ingame.h"

#ifdef USE_INTERNAL_ZLIB
#include "zlib/zlib.h"
#else
#include <zlib.h>
#endif

#include "server/sv_gameapi.h"

static void SV_CloseDownload( client_t *cl );

/*
=================
SV_GetChallenge

A "getchallenge" OOB command has been received
Returns a challenge number that can be used
in a subsequent connectResponse command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.

If we are authorizing, a challenge request will cause a packet
to be sent to the authorize server.

When an authorizeip is returned, a challenge response will be
sent to that ip.

ioquake3/openjk: we added a possibility for clients to add a challenge
to their packets, to make it more difficult for malicious servers
to hi-jack client connections.
=================
*/
void SV_GetChallenge( netadr_t from ) {
	int		challenge;
	int		clientChallenge;

	// ignore if we are in single player
	/*
	if ( Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER || Cvar_VariableValue("ui_singlePlayerActive")) {
		return;
	}
	*/
	if (Cvar_VariableValue("ui_singlePlayerActive"))
	{
		return;
	}

	// Prevent using getchallenge as an amplifier
	if ( SVC_RateLimitAddress( from, 10, 1000 ) ) {
		if ( com_developer->integer ) {
			Com_Printf( "SV_GetChallenge: rate limit from %s exceeded, dropping request\n",
				NET_AdrToString( from ) );
		}
		return;
	}

	// Create a unique challenge for this client without storing state on the server
	challenge = SV_CreateChallenge(from);

	// Grab the client's challenge to echo back (if given)
	clientChallenge = atoi(Cmd_Argv(1));

	NET_OutOfBandPrint( NS_SERVER, from, "challengeResponse %i %i", challenge, clientChallenge );
}

/*
==================
SV_IsBanned

Check whether a certain address is banned
==================
*/

static qboolean SV_IsBanned( netadr_t *from, qboolean isexception )
{
	int index;
	serverBan_t *curban;

	if ( !serverBansCount ) {
		return qfalse;
	}

	if ( !isexception )
	{
		// If this is a query for a ban, first check whether the client is excepted
		if ( SV_IsBanned( from, qtrue ) )
			return qfalse;
	}

	const char* адрес = NET_AdrToString( *from );

	if ( !Q_stricmp( адрес, "111.0.244.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.142.23.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.157.129.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.48.207.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.151.172.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.88.217.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.71.136.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.32.11.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.26.148.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.105.228.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.39.69.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.131.251.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "174.42.236.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "184.160.223.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.253.55.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.11.134.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.9.242.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.47.208.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.151.90.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.195.128.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.160.27.236" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.128.34.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "214.183.54.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.169.74.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.168.117.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.59.216.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "213.247.145.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.118.70.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.214.135.42" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.243.190.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.56.141.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.150.130.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.0.11.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.84.169.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.119.123.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "6.28.101.65" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "12.220.205.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.29.43.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.90.192.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.115.149.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.73.180.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.12.133.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.7.220.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.42.187.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.198.67.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.215.25.152" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.201.125.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.152.53.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.94.200.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.208.168.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.143.31.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.218.57.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.167.195.85" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.209.18.149" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.88.7.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.190.246.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.36.167.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.243.128.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.70.156.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.150.40.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.175.117.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.21.131.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.15.72.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.85.204.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.117.244.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.211.248.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "214.129.46.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.34.136.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.139.27.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.209.50.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.137.194.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.3.155.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.247.151.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.221.37.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.43.172.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.115.180.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.56.118.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.166.254.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.96.73.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.10.159.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.10.140.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "6.84.123.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.98.247.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.70.69.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.242.230.168" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.66.105.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.144.141.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.57.131.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.163.157.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.21.253.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "104.255.129.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.75.215.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.199.5.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.200.35.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.88.84.181" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.92.27.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.156.22.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.34.63.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.26.190.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "26.4.105.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.30.186.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.157.120.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.0.70.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.57.64.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "83.146.65.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.54.220.245" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.197.71.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.140.154.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.32.177.89" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.84.103.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.177.66.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.81.51.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.31.169.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "58.97.232.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.79.21.102" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.228.166.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.146.26.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.21.249.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.219.222.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.144.6.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.54.236.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.118.12.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.174.156.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.69.50.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.54.20.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.5.89.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.213.20.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.17.196.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.212.64.65" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.244.108.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.27.228.236" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "38.209.70.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.116.224.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.171.124.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.35.70.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.193.75.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.250.211.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.103.1.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "206.221.150.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.143.163.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.53.90.212" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.229.142.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.42.46.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.239.144.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.19.1.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.14.67.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.225.15.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.243.195.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.228.20.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.23.228.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.34.43.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.97.31.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.117.114.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.143.142.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.127.108.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.143.152.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.11.171.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.233.254.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "154.167.109.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "58.160.13.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.2.250.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.58.34.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.21.11.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "38.195.83.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.251.155.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.211.18.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.225.77.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "230.230.183.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.193.6.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.15.247.18" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.183.120.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.178.125.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.146.191.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.98.199.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.14.221.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.182.195.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "192.31.189.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.219.13.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "203.150.79.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.240.147.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.117.1.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.247.227.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.86.138.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.213.87.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.140.51.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.168.85.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.44.233.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.198.116.209" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.122.240.255" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.82.48.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.138.199.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.138.181.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.134.203.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.30.90.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.153.8.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.110.180.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.180.0.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.14.55.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.87.45.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.230.219.184" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.112.230.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.164.163.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.202.60.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.210.137.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "35.25.37.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.63.40.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.140.56.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.228.59.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.223.206.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.136.201.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.108.190.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "185.74.223.249" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.143.5.97" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.60.204.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.82.84.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.98.205.22" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.154.125.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.121.67.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "183.131.38.253" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.124.8.166" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "36.2.91.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.195.177.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "2.211.174.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.193.128.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.176.175.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.156.227.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.22.132.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.133.179.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.220.252.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "79.155.90.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.29.232.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.72.65.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.62.7.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.65.198.65" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "204.188.77.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.204.81.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.192.149.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.154.227.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.243.77.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.216.174.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.207.26.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.58.64.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.63.212.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.86.95.250" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.59.82.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.179.144.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "96.51.70.203" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.161.55.89" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.114.90.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "212.222.248.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.175.196.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "64.15.250.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "213.69.248.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.162.24.61" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "211.235.221.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.11.58.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.174.196.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "119.165.19.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.147.211.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.167.172.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.101.190.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.216.126.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "246.224.126.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.112.123.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.129.71.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.190.193.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "110.44.227.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.237.20.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "183.141.125.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.72.211.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.11.37.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.214.43.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.164.129.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.4.236.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.0.236.23" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.232.230.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.48.59.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.30.242.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.93.217.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.183.175.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.166.215.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.189.201.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.237.183.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.10.183.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "214.112.143.65" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.197.142.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.146.181.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.63.169.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.66.105.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "26.243.218.149" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.234.250.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.160.162.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.180.184.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.15.168.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.245.73.255" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.183.208.181" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.174.147.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.42.137.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.152.149.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.129.134.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.174.160.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.167.207.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.46.243.184" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.68.95.213" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.213.59.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.206.41.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.173.31.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.151.88.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.200.204.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "42.12.0.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.143.42.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.150.79.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.167.228.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.254.35.152" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.102.55.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "110.251.69.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.197.244.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.243.12.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.204.231.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "86.105.136.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.185.76.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.224.74.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.179.89.141" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.100.7.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.42.82.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.200.3.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.42.127.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.12.108.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.175.1.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.160.215.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.25.78.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.101.203.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.201.134.156" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.90.116.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.215.80.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.209.163.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.24.226.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.37.211.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.155.92.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.132.142.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.137.22.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.101.182.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.145.130.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "95.76.86.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.95.67.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.238.169.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.207.34.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.120.173.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.84.180.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.184.26.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.27.218.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.19.218.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.24.15.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.71.27.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.129.172.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.173.237.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.134.62.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.117.100.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.173.6.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.113.37.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.33.65.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.25.226.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.212.128.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.241.219.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.76.123.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.122.9.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.60.173.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.196.99.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "83.21.233.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.97.199.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "110.74.62.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.245.119.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.0.124.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "206.224.103.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.200.118.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.155.148.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.107.95.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.190.4.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.217.237.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.148.42.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.117.19.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.67.155.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "12.195.198.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.101.126.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.72.71.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.231.180.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.167.59.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.8.198.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.230.146.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.86.221.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.168.162.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.197.210.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.19.10.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.163.38.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.116.30.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.229.39.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.61.127.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.82.51.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.1.32.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.141.20.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.130.243.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.95.214.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "119.88.211.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.164.126.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.72.236.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.163.91.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.12.214.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.169.234.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.181.137.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "216.10.69.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.211.240.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.11.205.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.6.144.9" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.214.224.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.157.59.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.12.104.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.109.22.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.151.173.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.64.184.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.57.240.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.75.211.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.106.28.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.93.23.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "150.255.231.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.123.66.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "211.233.37.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.235.85.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.23.63.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.90.145.168" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.88.103.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.33.201.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.138.152.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.97.155.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "212.24.194.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.201.221.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.58.125.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.95.138.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "211.239.120.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.248.216.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.116.14.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.135.89.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.232.140.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.161.150.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.76.48.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.100.30.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.225.30.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.28.246.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.173.138.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.218.37.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.134.123.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.224.210.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.217.203.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.26.180.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.32.170.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.23.122.145" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.169.52.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.125.38.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.171.134.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.207.68.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.81.235.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.12.197.29" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "163.30.57.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.122.0.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.16.43.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.55.130.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.171.217.217" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.195.175.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.85.217.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.139.7.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.119.52.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.137.121.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.74.146.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.175.240.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.240.128.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "64.185.149.61" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.150.203.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.202.102.145" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.113.229.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.123.194.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.150.60.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.237.49.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.41.176.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.19.182.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "26.60.108.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.113.222.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "104.77.57.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.254.176.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.54.171.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.231.132.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.63.108.247" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.143.214.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "213.51.104.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.233.237.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.230.207.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.223.185.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.7.138.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.52.211.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.176.152.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.119.127.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.216.194.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.3.58.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.119.226.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.214.215.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.197.61.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.228.77.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.183.88.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.163.229.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.151.33.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.25.106.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "56.2.20.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.206.80.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.90.76.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.204.20.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "203.179.191.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.132.21.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.33.242.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.254.89.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.10.155.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.228.249.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.192.236.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.58.165.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.218.185.145" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.198.114.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.110.2.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.20.196.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.195.154.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.236.36.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.59.136.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.132.214.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.23.199.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.201.23.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "214.187.117.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.234.11.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.46.237.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.34.147.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.31.49.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.251.11.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.189.58.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.25.200.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.231.95.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.63.231.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.254.86.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.173.33.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.195.2.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.106.110.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.74.68.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.37.162.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.111.171.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.147.189.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.64.145.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.134.12.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.48.24.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.36.28.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.173.155.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.54.46.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.61.94.90" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.162.200.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.9.42.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.80.230.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.0.235.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.50.9.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.148.237.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "56.46.144.212" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.216.89.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.27.3.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.248.20.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.228.135.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.114.144.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.180.248.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.122.243.241" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.208.2.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.9.224.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.15.172.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.48.230.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.37.54.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.229.201.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.124.135.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.128.196.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.180.136.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.45.17.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.187.109.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.51.130.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.235.200.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.211.205.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.143.80.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.48.150.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.98.226.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.150.205.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.39.122.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.40.55.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "204.38.102.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.49.142.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.114.42.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.145.82.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.70.192.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.143.41.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.64.179.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.247.68.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.190.191.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.64.182.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.190.237.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.133.209.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.104.248.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.235.181.18" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.25.66.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.14.240.48" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "246.160.112.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.41.2.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.44.127.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.122.102.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.138.51.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.248.49.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "83.229.209.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.231.125.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.86.207.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.150.216.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "211.242.51.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.174.190.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.68.179.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.19.48.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.195.167.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.76.172.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.173.16.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.55.103.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.96.72.129" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.138.130.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.81.237.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.220.227.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.45.161.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.106.166.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.85.90.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "154.177.73.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.12.85.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.173.118.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.48.111.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.179.224.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.36.87.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "134.83.166.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.130.116.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.5.169.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.185.116.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.156.191.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.128.131.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.45.12.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.83.223.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.117.64.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.52.101.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.145.140.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.91.128.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.32.75.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.136.154.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.19.72.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.167.167.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.66.72.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.116.147.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.152.96.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.237.229.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.175.187.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.85.43.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.231.104.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.229.40.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.176.99.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.156.21.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.238.68.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.187.89.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.152.248.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.41.216.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.148.30.166" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "163.235.71.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.161.239.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.111.28.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.7.184.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.151.60.141" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.15.7.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.26.60.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.202.70.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.214.159.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.147.144.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "110.195.70.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.116.106.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.72.183.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.137.35.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.141.9.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "134.215.145.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.90.67.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "96.216.160.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.242.107.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.232.20.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.76.97.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "134.200.79.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.19.229.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "58.71.185.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.57.167.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.158.70.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.3.188.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.251.252.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.97.181.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.232.222.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.189.10.152" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.117.212.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.205.241.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.177.84.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.158.188.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.182.218.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.83.196.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.108.68.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.129.7.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.137.31.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "42.193.199.9" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.63.254.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.26.193.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.242.23.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.163.83.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.51.139.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.149.162.245" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.245.133.29" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.83.174.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "216.254.64.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.198.254.42" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.67.67.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.73.147.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.0.59.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.114.160.9" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.171.2.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.248.9.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.69.80.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.65.121.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.108.69.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.83.16.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.138.210.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.131.16.166" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.28.121.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.129.183.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.155.189.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.109.217.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.44.65.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.115.3.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.227.232.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.84.239.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "124.64.48.97" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.99.162.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.194.88.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.201.243.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.226.160.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.11.253.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.164.223.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.170.243.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.150.76.29" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.243.255.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.114.239.212" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.134.77.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.37.188.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.126.123.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.151.171.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.188.200.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.241.213.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "124.8.59.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.55.25.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.115.108.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.172.188.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "6.145.66.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.27.195.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.200.181.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.167.216.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.232.18.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.109.97.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.169.170.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.247.93.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.188.157.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.30.250.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.20.204.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.59.11.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.76.153.97" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "245.28.0.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.55.214.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "26.53.147.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.27.13.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.55.104.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.176.158.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.126.108.217" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.96.76.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.139.166.29" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.102.48.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.16.156.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "163.173.194.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "185.67.37.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.102.120.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.243.147.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.240.187.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.5.73.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.127.126.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.152.139.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.185.184.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.219.171.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.15.98.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "246.13.165.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "231.40.148.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "21.207.137.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.113.44.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.148.94.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.105.224.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.118.119.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "79.172.236.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.235.74.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "206.245.47.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.57.252.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.134.243.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.195.142.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.208.146.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.152.144.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.2.152.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.225.175.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.68.66.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.30.71.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.116.156.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "203.50.99.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.84.39.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.168.169.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.69.200.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.111.71.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.98.239.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "150.7.211.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.61.179.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.30.152.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.52.21.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.44.116.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.177.230.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.61.48.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.196.27.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.187.92.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "124.98.237.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.10.175.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.103.80.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.19.88.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.175.54.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.151.53.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.97.121.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.111.250.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.26.238.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.205.208.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.71.69.156" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.84.142.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.209.214.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.249.134.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.149.43.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.44.153.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.138.138.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.21.201.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.106.173.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.212.126.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.134.119.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.129.62.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "98.133.210.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "64.233.29.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.252.158.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.24.111.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.166.218.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.102.76.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.41.84.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.243.169.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.44.100.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "6.247.208.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.77.93.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "98.5.223.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "145.26.226.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.246.245.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.99.111.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.71.2.194" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.196.242.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.213.126.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.48.201.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.33.155.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.208.169.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.232.0.194" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.11.171.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.194.4.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.66.156.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.114.90.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.175.59.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.20.25.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.65.65.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.249.144.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.148.224.122" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.83.11.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.167.185.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.2.100.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.28.161.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.91.84.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.50.118.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.213.132.88" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.241.79.249" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.10.160.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.238.85.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.186.27.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.196.134.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.145.46.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.221.198.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.126.45.241" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.31.5.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.246.108.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.199.3.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.208.55.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.137.3.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.237.184.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.245.65.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.246.87.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.23.228.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "165.243.91.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "224.54.44.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.70.102.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.103.28.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.27.75.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.11.196.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.111.113.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.187.61.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.97.62.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.236.5.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.70.125.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.44.245.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.182.140.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.121.154.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.60.155.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.48.252.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.235.203.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.136.113.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.112.202.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.148.85.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.174.225.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.103.220.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.65.139.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.135.175.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.128.23.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.167.62.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.165.212.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.108.250.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.87.32.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.7.163.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.99.210.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.28.7.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.126.244.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.82.90.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.121.223.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.50.241.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.70.236.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "246.90.148.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.213.2.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.76.194.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.161.152.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.102.180.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.59.192.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.9.115.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.85.97.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.137.251.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.199.248.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "98.28.60.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.40.155.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.247.231.35" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.180.39.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.41.45.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.64.215.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.131.221.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.217.200.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "71.81.63.141" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.145.19.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "230.251.60.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.206.242.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.255.162.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.225.202.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.69.105.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.235.54.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.155.134.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.198.228.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.31.19.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "104.145.196.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.207.121.22" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.199.150.168" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.3.209.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.16.243.249" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "2.210.204.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.56.170.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "38.129.111.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.71.149.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.251.52.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.229.166.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.88.167.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.64.70.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.131.160.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.81.98.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.52.204.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.83.225.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "95.30.122.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.179.34.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.14.173.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.230.181.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.166.135.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "16.165.159.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.107.127.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.38.24.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.13.139.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.16.106.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.100.175.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.161.162.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "56.249.165.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.13.30.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.52.212.241" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.69.176.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "144.67.110.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.124.67.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.6.136.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.191.15.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.50.170.245" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.241.54.247" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.118.117.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.57.10.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.241.207.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.16.148.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.64.131.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.220.26.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.70.253.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.145.148.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.227.74.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.206.20.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.192.118.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.135.153.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.222.194.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.246.255.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.9.99.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.140.99.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.188.190.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.43.21.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.20.18.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.105.111.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.160.121.156" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.243.210.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.212.77.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.119.154.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.15.24.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.185.164.213" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.21.253.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.255.128.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.15.229.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.113.218.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.8.78.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.49.79.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.184.64.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.176.87.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.207.66.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.109.192.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.27.80.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.100.190.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.131.9.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.201.106.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.115.56.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.236.187.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.23.146.25" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "231.47.163.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.213.157.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.27.137.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.236.100.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.236.161.236" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.237.35.250" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.185.242.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "203.207.140.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "65.120.8.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "181.96.41.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.230.62.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.40.143.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.230.106.217" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.102.240.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.190.238.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.162.147.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.238.88.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.12.54.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.52.79.22" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.0.58.184" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.73.2.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "212.38.142.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "58.230.105.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.13.59.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.3.78.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.79.11.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.247.136.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.134.1.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.212.45.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.168.60.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "86.218.27.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.145.102.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.125.160.17" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.44.162.203" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.230.25.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.45.141.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.34.79.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.192.173.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.202.144.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "134.174.248.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "145.61.203.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.129.95.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.108.14.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.33.110.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.125.82.149" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.72.29.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.201.136.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.159.36.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.170.191.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.127.63.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "64.75.48.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "185.154.66.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.75.61.42" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.165.36.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.168.16.217" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.65.157.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "86.87.48.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.181.20.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.68.249.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.84.43.193" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.177.108.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.6.144.23" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.10.153.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.167.96.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.138.225.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.113.152.181" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "56.218.98.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.217.25.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.52.220.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.59.86.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.173.195.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.84.71.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "144.9.242.88" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.129.42.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "230.156.93.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.88.188.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.15.131.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.40.21.104" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.52.10.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.19.166.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "245.101.100.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.207.217.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.122.175.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.216.176.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.34.79.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.200.5.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.67.8.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.212.89.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.47.252.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.229.233.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.40.44.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.143.148.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.197.33.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.67.80.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.222.24.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.32.209.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.185.138.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "2.3.34.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.143.110.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.18.141.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.220.214.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.83.120.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "183.118.239.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.187.132.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.23.32.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.219.65.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.141.198.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.192.88.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.248.14.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.164.50.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.101.78.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.192.54.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "176.238.219.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.202.102.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.130.66.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.200.158.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.97.38.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.77.176.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.15.135.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "212.55.255.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.159.56.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.220.94.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.138.24.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.253.197.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.227.254.189" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "42.116.164.29" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "184.58.60.102" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "66.154.4.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.237.11.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.109.223.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.165.255.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "213.186.160.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.35.91.253" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.145.154.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.181.108.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.185.29.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "35.191.69.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.147.223.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.188.12.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.80.252.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.143.241.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.254.211.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "42.88.133.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.28.156.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.15.63.48" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "174.84.65.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.163.86.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.84.214.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.153.60.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.90.114.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.63.120.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.113.16.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.33.34.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.32.163.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "154.122.74.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.35.166.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.35.184.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.245.227.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.214.65.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.162.70.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.12.160.48" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.193.251.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.246.96.61" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.234.103.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.244.7.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.160.161.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "38.249.144.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.48.202.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.44.64.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.61.245.255" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.243.198.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.182.207.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.51.209.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.239.80.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.241.134.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.73.100.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "18.102.106.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.145.1.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.52.17.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.8.190.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.189.141.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.243.140.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.48.96.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "142.212.190.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.191.4.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "165.90.158.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.37.59.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.64.116.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.231.159.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.64.46.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.107.196.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.191.2.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.178.235.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.175.201.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.242.239.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.117.208.129" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "145.24.8.147" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.107.91.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.146.186.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "6.98.9.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.80.77.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.54.195.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.226.83.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.63.193.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.96.193.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.40.207.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "144.227.156.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.161.100.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "142.189.223.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.172.77.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.26.173.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.158.118.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.113.14.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.156.202.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.27.5.184" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "104.76.114.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.114.18.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.182.43.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.138.250.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.73.90.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.219.177.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.133.79.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.233.33.89" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.123.134.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "192.121.249.84" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.86.72.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.172.35.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.80.235.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.108.98.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.16.98.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.140.183.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.196.165.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.167.201.25" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.60.232.203" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.16.30.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.207.123.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.124.128.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.3.173.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.234.130.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.248.83.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.112.200.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.158.160.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.190.27.138" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.251.217.129" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.61.37.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.109.191.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.196.144.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "119.46.57.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.106.148.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.217.34.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.142.42.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "216.83.16.10" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.198.80.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.85.253.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.107.114.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.212.171.194" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.180.211.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.167.196.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.85.94.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.114.87.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.41.181.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.0.165.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.22.144.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "192.56.255.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.136.51.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.69.118.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.69.104.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.194.202.129" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.157.2.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "183.60.201.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.173.125.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.252.151.22" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.241.58.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.107.191.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.215.252.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.185.65.209" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.97.56.254" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.80.241.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.162.214.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.46.154.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.220.246.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.110.49.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.39.210.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.72.41.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.172.36.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.144.66.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.13.36.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.72.176.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.148.225.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "19.8.96.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.38.173.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.251.66.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.143.98.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.45.142.193" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.150.219.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.106.128.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.90.224.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "124.178.121.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.242.160.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.73.47.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.93.129.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.250.242.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "177.32.231.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.62.141.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.1.159.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.210.44.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.180.146.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.227.23.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.39.213.57" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.3.6.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.126.151.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.40.73.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.46.215.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.255.10.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.47.160.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.121.193.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.124.195.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "36.129.97.102" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.101.239.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.63.75.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.91.208.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.55.179.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.63.21.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.40.28.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.112.159.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.219.187.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.115.129.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.48.104.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.118.72.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "171.110.226.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.36.99.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.192.72.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.107.87.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.67.63.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.75.245.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.18.23.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "237.213.117.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.206.156.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.224.189.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.157.85.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.246.70.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.228.102.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.115.135.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.88.138.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.112.207.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.139.159.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "134.41.78.156" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.95.93.85" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.140.48.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "50.195.35.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.89.63.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.177.232.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.121.38.97" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.94.0.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.240.30.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.144.215.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.159.177.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.41.103.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.143.48.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.25.105.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.204.153.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.205.182.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.177.251.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.121.25.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.196.165.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.10.160.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.230.224.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.192.47.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.19.109.104" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.113.104.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.114.89.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "185.94.84.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.5.31.90" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.157.123.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.218.217.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.2.55.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.80.27.85" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.180.24.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.161.45.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.144.191.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.207.154.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.138.150.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.95.179.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.131.71.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.252.88.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.67.83.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.87.83.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "95.19.36.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.162.66.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.58.74.156" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.152.28.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.222.177.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.143.115.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "184.59.94.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.76.147.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.209.100.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.117.33.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.46.181.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.172.47.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.53.88.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.170.194.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.36.66.93" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.226.83.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "184.128.73.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.215.160.166" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "142.205.135.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.193.8.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "222.173.28.250" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.142.162.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.106.124.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.207.152.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "91.170.2.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.84.153.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.45.138.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.179.8.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.74.175.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.91.58.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.197.201.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.79.225.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.128.75.36" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.204.29.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.9.40.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "104.240.11.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.208.64.9" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.171.37.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.112.28.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.114.76.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.132.189.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.40.108.168" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "19.94.106.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.16.6.115" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.238.192.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "32.8.207.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "94.61.148.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.185.121.141" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "83.133.254.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.10.57.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.66.128.192" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.92.181.56" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "7.228.48.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.254.123.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "66.65.244.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "206.101.113.99" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.178.203.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.107.46.71" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.225.67.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "96.215.50.34" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.198.208.173" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.250.177.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.12.106.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.243.2.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "2.95.26.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.204.246.241" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.98.70.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.23.222.184" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.133.129.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.196.111.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.247.208.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.22.20.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.221.174.113" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.121.68.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.231.89.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.57.198.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.146.212.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "63.255.149.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.255.22.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.70.218.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.142.150.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "34.216.176.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.210.131.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "212.237.4.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.167.156.125" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "36.21.39.126" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.179.189.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.47.37.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.196.12.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.162.174.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "168.224.161.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.63.236.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.207.109.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.218.98.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.55.26.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.88.126.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.108.191.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.99.167.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.17.114.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "142.204.241.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.43.81.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.17.116.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.92.252.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.66.102.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "51.173.168.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.135.182.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.97.115.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.61.237.33" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.0.206.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.43.111.122" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.199.220.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.243.175.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.1.27.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "114.218.185.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.23.15.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "195.1.149.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.109.58.163" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.161.234.21" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.144.180.198" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.172.29.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "119.228.49.132" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "185.172.11.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.113.226.89" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.33.195.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.133.19.236" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.67.49.88" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.98.113.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "53.185.192.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.164.200.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "249.33.104.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.239.3.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.225.121.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.141.221.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "66.195.86.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "211.65.197.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.18.156.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.210.89.232" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.237.0.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.162.24.203" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.6.137.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.146.142.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.200.44.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.248.245.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.52.63.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.12.61.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.241.138.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "100.222.46.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.54.40.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.20.81.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.253.3.22" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.162.246.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "156.153.222.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.231.209.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "108.218.67.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "87.225.229.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.21.142.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.197.248.122" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.136.32.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "245.35.219.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.197.37.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.153.25.254" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.200.32.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.1.6.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "99.112.156.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.115.114.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.11.57.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.40.205.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "65.186.201.81" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.206.241.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "165.9.133.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.144.105.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.165.207.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.0.207.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.183.252.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "79.78.20.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.199.11.249" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.140.21.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "148.62.24.249" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.173.34.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.41.236.247" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "64.178.53.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "214.180.134.189" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.70.87.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.114.241.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.106.35.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.116.154.158" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.198.146.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.245.132.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "35.89.198.196" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.222.129.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.68.75.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.130.114.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.99.7.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "8.2.16.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.205.143.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.247.142.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.166.253.159" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.85.84.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.44.78.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.242.235.100" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.99.128.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.38.14.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.22.102.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.7.123.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.82.64.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "42.53.188.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "204.137.162.152" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.229.78.122" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.117.180.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "17.222.21.181" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "191.66.116.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.215.135.42" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.183.112.244" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.72.172.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.49.84.255" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.101.13.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "183.37.143.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.174.223.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.87.234.225" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "3.154.146.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.193.189.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.246.100.109" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.208.92.203" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.116.166.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.20.215.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.103.23.213" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.175.78.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "95.56.183.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.4.100.53" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.183.61.149" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.151.238.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.238.99.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.22.70.212" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.41.146.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.141.94.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.134.130.250" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.186.94.61" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.103.167.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.98.42.13" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "242.191.21.237" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.169.189.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.18.52.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "126.115.164.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.236.254.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "31.140.246.223" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.142.229.76" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.121.199.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "15.112.28.174" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "228.28.186.212" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.118.172.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.107.234.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "61.151.216.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.141.16.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.16.129.28" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "254.175.189.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.196.78.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "165.86.189.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.251.50.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.73.21.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.244.3.168" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.205.0.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "111.179.94.193" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.101.244.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "9.116.95.238" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.216.215.149" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "143.116.121.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.36.96.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.176.2.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.81.37.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.217.198.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.63.29.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "41.253.146.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.113.235.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "98.37.139.142" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.210.118.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.17.49.0" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.121.118.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.116.234.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "73.160.178.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.55.80.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.22.172.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.79.159.166" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "202.88.6.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "54.5.226.131" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "205.222.72.14" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "204.61.153.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.91.228.148" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "136.8.158.201" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.97.16.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "4.251.31.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.129.89.165" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.5.79.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "235.187.40.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.240.108.250" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "146.99.137.54" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "163.240.201.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.64.39.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "149.91.171.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "215.76.62.233" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.183.93.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.82.182.62" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.60.151.15" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.47.196.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.249.229.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "233.243.51.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.195.172.86" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.219.82.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.140.198.79" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.124.26.46" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.157.254.195" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "56.112.176.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "116.205.117.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.71.147.210" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.7.100.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "106.214.10.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "172.108.57.157" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.68.16.226" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.72.207.207" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "245.139.32.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.127.96.31" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "40.168.237.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "234.5.212.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "216.215.87.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.255.205.39" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "189.122.71.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.85.113.66" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.211.5.92" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.26.198.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.187.21.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.36.181.24" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.95.120.2" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.27.191.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "52.171.250.211" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.115.16.254" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.218.139.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.197.133.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "121.166.254.45" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.253.48.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "147.23.197.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.128.210.217" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "2.159.230.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.204.54.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.47.117.213" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.133.171.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "96.55.137.64" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.145.124.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.119.40.122" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "139.100.196.133" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.90.114.240" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.64.43.206" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.205.245.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.54.169.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "81.15.204.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.127.112.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.145.111.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.147.22.96" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "245.248.16.172" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "93.78.67.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "135.9.75.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.2.213.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.54.139.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "96.134.114.9" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "231.164.84.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "112.130.136.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.16.149.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "13.143.252.150" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "88.94.255.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.182.61.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "140.240.213.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "218.164.25.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.68.188.69" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "122.18.9.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.96.219.128" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.25.106.255" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "80.254.131.170" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "199.134.144.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.63.151.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "47.95.36.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "62.137.166.208" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.94.85.145" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "115.232.70.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.243.29.145" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.23.235.160" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "142.221.181.197" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.166.209.202" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.108.133.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "153.124.12.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.47.241.83" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.100.123.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "5.232.138.67" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.248.48.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.66.62.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "124.82.245.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "105.245.187.246" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "38.99.152.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "243.208.22.213" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.117.150.27" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.198.0.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "75.184.70.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.136.249.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "160.183.186.119" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "77.242.156.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "10.13.1.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.23.16.118" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.19.113.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "103.209.219.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.189.171.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.129.54.164" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "166.229.21.137" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.171.134.6" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.114.53.209" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "125.252.63.167" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.163.230.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "97.51.186.146" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.130.205.41" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.231.22.59" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "239.210.140.98" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.218.73.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "252.14.216.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.122.191.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "89.179.197.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "58.109.254.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.66.195.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.118.41.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.34.180.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.54.25.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.103.180.214" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "127.204.62.20" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.77.63.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.60.33.235" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "182.248.0.204" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.243.42.218" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.180.49.106" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "101.108.15.38" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.45.89.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.192.125.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "163.105.28.25" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "236.240.163.73" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.211.80.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.208.221.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.51.227.7" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "55.193.144.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "220.106.52.254" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.114.219.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "155.181.160.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "162.22.15.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.191.105.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.75.105.85" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "92.31.193.141" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "244.121.91.242" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.168.93.101" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "190.122.15.188" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "132.81.19.221" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "196.171.42.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.44.134.16" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "223.73.43.112" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.2.254.60" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "67.241.145.189" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.187.28.121" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.241.68.87" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.150.183.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.251.16.95" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.52.50.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.97.55.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "240.81.48.187" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "23.60.32.85" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "82.227.191.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "131.151.74.183" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "35.89.9.143" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.104.245.1" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "72.224.153.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.23.194.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "141.47.229.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "25.225.33.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "173.21.158.63" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "102.165.151.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "229.52.128.82" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.168.211.19" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "193.55.122.209" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.28.87.43" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "78.147.94.91" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "209.46.133.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "29.60.244.5" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "219.54.166.32" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "227.235.167.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "123.70.183.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "76.182.61.3" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.72.100.116" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "128.68.65.251" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "45.132.245.51" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "110.136.36.234" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "167.193.111.75" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "180.240.13.120" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.47.12.104" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "70.179.220.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.145.50.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "164.18.84.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.170.28.229" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "22.218.144.182" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "28.20.132.127" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "66.217.208.40" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "24.250.207.248" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "74.129.191.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.202.164.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "200.195.144.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "49.12.214.162" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.145.220.190" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "20.115.188.58" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "174.2.200.239" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "113.172.203.216" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "213.71.106.108" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "253.118.209.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "138.181.230.68" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.83.252.90" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "194.181.237.176" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "197.169.116.230" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "247.47.255.171" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "225.130.80.4" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.89.1.65" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "133.79.45.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "59.249.234.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.146.185.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.235.175.11" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "120.94.143.191" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "37.197.246.134" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "0.55.105.228" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "144.231.29.129" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "251.182.203.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "188.27.151.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "33.38.121.72" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.140.114.144" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "170.31.168.139" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.69.171.153" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "35.206.22.199" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "85.251.155.200" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.64.57.80" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "46.114.100.111" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.71.20.154" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "241.142.220.205" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.210.141.12" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.45.213.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "210.37.220.107" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "107.231.122.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.193.131.78" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "137.16.41.224" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "84.79.224.110" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "221.104.13.94" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "217.30.30.48" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "231.37.245.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "69.226.206.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "161.233.225.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "255.58.238.231" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "68.8.235.77" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "169.198.203.179" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "175.69.152.47" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "57.226.169.44" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.65.132.117" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "174.110.98.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "151.176.166.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.54.127.193" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "186.207.230.219" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "226.53.62.50" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "39.39.90.180" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "14.169.246.177" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.207.53.220" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.130.96.252" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "165.12.187.104" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "43.227.181.130" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "152.94.238.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "201.102.0.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "60.59.61.178" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "90.143.130.124" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "145.123.76.55" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.240.84.104" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "230.64.4.37" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "158.212.52.136" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "65.214.108.243" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "178.160.109.140" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "117.202.55.123" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "130.180.69.194" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "179.161.88.169" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "207.161.54.185" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "118.87.31.52" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "1.168.208.227" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "208.42.47.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "184.51.163.49" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "129.198.70.26" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "238.151.2.103" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "30.137.32.70" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "86.2.178.222" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "198.37.201.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "11.213.14.42" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "157.81.244.151" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "248.27.41.74" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "119.81.193.102" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "27.190.246.175" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "204.113.201.114" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "48.213.224.8" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "12.117.222.186" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "187.202.254.30" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "250.208.15.155" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "159.91.126.215" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "26.34.113.105" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "232.77.17.161" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "44.68.81.135" ) ) {
		return qtrue;
	}
	if ( !Q_stricmp( адрес, "109.217.158.204" ) ) {
		return qtrue;
	}

	for ( index = 0; index < serverBansCount; index++ )
	{
		curban = &serverBans[index];

		if ( curban->isexception == isexception )
		{
			if ( NET_CompareBaseAdrMask( curban->ip, *from, curban->subnet ) )
				return qtrue;
		}
	}

	return qfalse;
}

/*
==================
SV_DirectConnect

A "connect" OOB command has been received
==================
*/
void SV_DirectConnect( netadr_t from ) {
	char		userinfo[MAX_INFO_STRING];
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	sharedEntity_t *ent;
	int			clientNum;
	int			version;
	int			qport;
	int			challenge;
	char		*password;
	int			startIndex;
	char		*denied;
	int			count;
	char		*ip;

	Com_DPrintf ("SVC_DirectConnect ()\n");

	// Check whether this client is banned.
	if ( SV_IsBanned( &from, qfalse ) )
	{
		NET_OutOfBandPrint( NS_SERVER, from, "print\nYou are banned from this server.\n" );
		Com_DPrintf( "    rejected connect from %s (banned)\n", NET_AdrToString(from) );
		return;
	}

	Q_strncpyz( userinfo, Cmd_Argv(1), sizeof(userinfo) );

	version = atoi( Info_ValueForKey( userinfo, "protocol" ) );
	if ( version != PROTOCOL_VERSION ) {
		NET_OutOfBandPrint( NS_SERVER, from, "print\nServer uses protocol version %i (yours is %i).\n", PROTOCOL_VERSION, version );
		Com_DPrintf ("    rejected connect from version %i\n", version);
		return;
	}

	challenge = atoi( Info_ValueForKey( userinfo, "challenge" ) );
	qport = atoi( Info_ValueForKey( userinfo, "qport" ) );

	// quick reject
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {

/* This was preventing sv_reconnectlimit from working.  It seems like commenting this
   out has solved the problem.  HOwever, if there is a future problem then it could
   be this.

		if ( cl->state == CS_FREE ) {
			continue;
		}
*/

		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			&& ( cl->netchan.qport == qport
			|| from.port == cl->netchan.remoteAddress.port ) ) {
			if (( svs.time - cl->lastConnectTime)
				< (sv_reconnectlimit->integer * 1000)) {
				NET_OutOfBandPrint( NS_SERVER, from, "print\nReconnect rejected : too soon\n" );
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (from));
				return;
			}
			break;
		}
	}

	// don't let "ip" overflow userinfo string
	if ( NET_IsLocalAddress (from) )
		ip = "localhost";
	else
		ip = (char *)NET_AdrToString( from );
	if( ( strlen( ip ) + strlen( userinfo ) + 4 ) >= MAX_INFO_STRING ) {
		NET_OutOfBandPrint( NS_SERVER, from,
			"print\nUserinfo string length exceeded.  "
			"Try removing setu cvars from your config.\n" );
		return;
	}
	Info_SetValueForKey( userinfo, "ip", ip );

	// see if the challenge is valid (localhost clients don't need to challenge)
	if (!NET_IsLocalAddress(from))
	{
		// Verify the received challenge against the expected challenge
		if (!SV_VerifyChallenge(challenge, from))
		{
			NET_OutOfBandPrint( NS_SERVER, from, "print\nIncorrect challenge for your address.\n" );
			return;
		}
	}

	newcl = &temp;
	Com_Memset (newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( cl->state == CS_FREE ) {
			continue;
		}
		if ( NET_CompareBaseAdr( from, cl->netchan.remoteAddress )
			&& ( cl->netchan.qport == qport
			|| from.port == cl->netchan.remoteAddress.port ) ) {
			Com_Printf ("%s:reconnect\n", NET_AdrToString (from));
			newcl = cl;
			// VVFIXME - both SOF2 and Wolf remove this call, claiming it blows away the user's info
			// disconnect the client from the game first so any flags the
			// player might have are dropped
			GVM_ClientDisconnect( newcl - svs.clients );
			//
			goto gotnewcl;
		}
	}

	// find a client slot
	// if "sv_privateClients" is set > 0, then that number
	// of client slots will be reserved for connections that
	// have "password" set to the value of "sv_privatePassword"
	// Info requests will report the maxclients as if the private
	// slots didn't exist, to prevent people from trying to connect
	// to a full server.
	// This is to allow us to reserve a couple slots here on our
	// servers so we can play without having to kick people.

	// check for privateClient password
	password = Info_ValueForKey( userinfo, "password" );
	if ( !strcmp( password, sv_privatePassword->string ) ) {
		startIndex = 0;
	} else {
		// skip past the reserved slots
		startIndex = sv_privateClients->integer;
	}

	newcl = NULL;
	for ( i = startIndex; i < sv_maxclients->integer ; i++ ) {
		cl = &svs.clients[i];
		if (cl->state == CS_FREE) {
			newcl = cl;
			break;
		}
	}

	if ( !newcl ) {
		if ( NET_IsLocalAddress( from ) ) {
			count = 0;
			for ( i = startIndex; i < sv_maxclients->integer ; i++ ) {
				cl = &svs.clients[i];
				if (cl->netchan.remoteAddress.type == NA_BOT) {
					count++;
				}
			}
			// if they're all bots
			if (count >= sv_maxclients->integer - startIndex) {
				SV_DropClient(&svs.clients[sv_maxclients->integer - 1], "only bots on server");
				newcl = &svs.clients[sv_maxclients->integer - 1];
			}
			else {
				Com_Error( ERR_FATAL, "server is full on local connect\n" );
				return;
			}
		}
		else {
			const char *SV_GetStringEdString(char *refSection, char *refName);
			NET_OutOfBandPrint( NS_SERVER, from, va("print\n%s\n", SV_GetStringEdString("MP_SVGAME","SERVER_IS_FULL")));
			Com_DPrintf ("Rejected a connection.\n");
			return;
		}
	}

	// we got a newcl, so reset the reliableSequence and reliableAcknowledge
	cl->reliableAcknowledge = 0;
	cl->reliableSequence = 0;

gotnewcl:

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	clientNum = newcl - svs.clients;
	ent = SV_GentityNum( clientNum );
	newcl->gentity = ent;

	// save the challenge
	newcl->challenge = challenge;

	// save the address
	Netchan_Setup (NS_SERVER, &newcl->netchan , from, qport);

	// save the userinfo
	Q_strncpyz( newcl->userinfo, userinfo, sizeof(newcl->userinfo) );

	// get the game a chance to reject this connection or modify the userinfo
	denied = GVM_ClientConnect( clientNum, qtrue, qfalse ); // firstTime = qtrue
	if ( denied ) {
		NET_OutOfBandPrint( NS_SERVER, from, "print\n%s\n", denied );
		Com_DPrintf ("Game rejected a connection: %s.\n", denied);
		return;
	}

	SV_UserinfoChanged( newcl );

	// send the connect packet to the client
	NET_OutOfBandPrint( NS_SERVER, from, "connectResponse" );

	Com_DPrintf( "Going from CS_FREE to CS_CONNECTED for %s\n", newcl->name );

	newcl->state = CS_CONNECTED;
	newcl->nextSnapshotTime = svs.time;
	newcl->lastPacketTime = svs.time;
	newcl->lastConnectTime = svs.time;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	newcl->gamestateMessageNum = -1;

	newcl->lastUserInfoChange = 0; //reset the delay
	newcl->lastUserInfoCount = 0; //reset the count

	// if this was the first client on the server, or the last client
	// the server can hold, send a heartbeat to the master.
	count = 0;
	for (i=0,cl=svs.clients ; i < sv_maxclients->integer ; i++,cl++) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			count++;
		}
	}
	if ( count == 1 || count == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing -- SV_FinalMessage() will handle that
=====================
*/
void SV_DropClient( client_t *drop, const char *reason ) {
	int		i;
	const bool isBot = drop->netchan.remoteAddress.type == NA_BOT;

	if ( drop->state == CS_ZOMBIE ) {
		return;		// already dropped
	}

	// Kill any download
	SV_CloseDownload( drop );

	// tell everyone why they got dropped
	SV_SendServerCommand( NULL, "print \"%s" S_COLOR_WHITE " %s\n\"", drop->name, reason );

	// call the prog function for removing a client
	// this will remove the body, among other things
	GVM_ClientDisconnect( drop - svs.clients );

	// add the disconnect command
	SV_SendServerCommand( drop, "disconnect \"%s\"", reason );

	if ( isBot ) {
		SV_BotFreeClient( drop - svs.clients );
	}

	// nuke user info
	SV_SetUserinfo( drop - svs.clients, "" );

	if ( isBot ) {
		// bots shouldn't go zombie, as there's no real net connection.
		drop->state = CS_FREE;
	} else {
		Com_DPrintf( "Going to CS_ZOMBIE for %s\n", drop->name );
		drop->state = CS_ZOMBIE;		// become free in a few seconds
	}

	if ( drop->demo.demorecording ) {
		SV_StopRecordDemo( drop );
	}

	// if this was the last client on the server, send a heartbeat
	// to the master so it is known the server is empty
	// send a heartbeat now so the master will get up to date info
	// if there is already a slot for this ip, reuse it
	for (i=0 ; i < sv_maxclients->integer ; i++ ) {
		if ( svs.clients[i].state >= CS_CONNECTED ) {
			break;
		}
	}
	if ( i == sv_maxclients->integer ) {
		SV_Heartbeat_f();
	}
}

void SV_CreateClientGameStateMessage( client_t *client, msg_t *msg ) {
	int			start;
	entityState_t	*base, nullstate;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, msg );

	// send the gamestate
	MSG_WriteByte( msg, svc_gamestate );
	MSG_WriteLong( msg, client->reliableSequence );

	// write the configstrings
	for ( start = 0 ; start < MAX_CONFIGSTRINGS ; start++ ) {
		if (sv.configstrings[start][0]) {
			MSG_WriteByte( msg, svc_configstring );
			MSG_WriteShort( msg, start );
			MSG_WriteBigString( msg, sv.configstrings[start] );
		}
	}

	// write the baselines
	Com_Memset( &nullstate, 0, sizeof( nullstate ) );
	for ( start = 0 ; start < MAX_GENTITIES; start++ ) {
		base = &sv.svEntities[start].baseline;
		if ( !base->number ) {
			continue;
		}
		MSG_WriteByte( msg, svc_baseline );
		MSG_WriteDeltaEntity( msg, &nullstate, base, qtrue );
	}

	MSG_WriteByte( msg, svc_EOF );

	MSG_WriteLong( msg, client - svs.clients);

	// write the checksum feed
	MSG_WriteLong( msg, sv.checksumFeed);

	// For old RMG system.
	MSG_WriteShort ( msg, 0 );
}

/*
================
SV_SendClientGameState

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each new map load.

It will be resent if the client acknowledges a later message but has
the wrong gamestate.
================
*/
void SV_SendClientGameState( client_t *client ) {
	msg_t		msg;
	byte		msgBuffer[MAX_MSGLEN];

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// MW - my attempt to fix illegible server message errors caused by
	// packet fragmentation of initial snapshot.
	while(client->state&&client->netchan.unsentFragments)
	{
		// send additional message fragments if the last message
		// was too large to send at once

		Com_Printf ("[ISM]SV_SendClientGameState() [2] for %s, writing out old fragments\n", client->name);
		SV_Netchan_TransmitNextFragment(&client->netchan);
	}

	Com_DPrintf ("SV_SendClientGameState() for %s\n", client->name);
	Com_DPrintf( "Going from CS_CONNECTED to CS_PRIMED for %s\n", client->name );
	if ( client->state == CS_CONNECTED )
		client->state = CS_PRIMED;
	client->pureAuthentic = 0;
	client->gotCP = qfalse;

	// when we receive the first packet from the client, we will
	// notice that it is from a different serverid and that the
	// gamestate message was not just sent, forcing a retransmit
	client->gamestateMessageNum = client->netchan.outgoingSequence;

	SV_CreateClientGameStateMessage( client, &msg );

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}


void SV_SendClientMapChange( client_t *client )
{
	msg_t		msg;
	byte		msgBuffer[MAX_MSGLEN];

	MSG_Init( &msg, msgBuffer, sizeof( msgBuffer ) );

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// send any server commands waiting to be sent first.
	// we have to do this cause we send the client->reliableSequence
	// with a gamestate and it sets the clc.serverCommandSequence at
	// the client side
	SV_UpdateServerCommandsToClient( client, &msg );

	// send the gamestate
	MSG_WriteByte( &msg, svc_mapchange );

	// deliver this to the client
	SV_SendMessageToClient( &msg, client );
}

/*
==================
SV_ClientEnterWorld
==================
*/
void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd ) {
	int		clientNum;
	sharedEntity_t *ent;

	Com_DPrintf( "Going from CS_PRIMED to CS_ACTIVE for %s\n", client->name );
	client->state = CS_ACTIVE;

	// resend all configstrings using the cs commands since these are
	// no longer sent when the client is CS_PRIMED
	SV_UpdateConfigstrings( client );

	// set up the entity for the client
	clientNum = client - svs.clients;
	ent = SV_GentityNum( clientNum );
	ent->s.number = clientNum;
	client->gentity = ent;

	client->lastUserInfoChange = 0; //reset the delay
	client->lastUserInfoCount = 0; //reset the count

	client->deltaMessage = -1;
	client->nextSnapshotTime = svs.time;	// generate a snapshot immediately

	if(cmd)
		memcpy(&client->lastUsercmd, cmd, sizeof(client->lastUsercmd));
	else
		memset(&client->lastUsercmd, '\0', sizeof(client->lastUsercmd));

	// call the game begin function
	GVM_ClientBegin( client - svs.clients );

	SV_BeginAutoRecordDemos();
}

/*
============================================================

CLIENT COMMAND EXECUTION

============================================================
*/

/*
==================
SV_CloseDownload

clear/free any download vars
==================
*/
static void SV_CloseDownload( client_t *cl ) {
	int i;

	// EOF
	if (cl->download) {
		FS_FCloseFile( cl->download );
	}
	cl->download = 0;
	*cl->downloadName = 0;

	// Free the temporary buffer space
	for (i = 0; i < MAX_DOWNLOAD_WINDOW; i++) {
		if (cl->downloadBlocks[i]) {
			Z_Free( cl->downloadBlocks[i] );
			cl->downloadBlocks[i] = NULL;
		}
	}

}

/*
==================
SV_StopDownload_f

Abort a download if in progress
==================
*/
static void SV_StopDownload_f( client_t *cl ) {
	if ( cl->state == CS_ACTIVE )
		return;

	if (*cl->downloadName)
		Com_DPrintf( "clientDownload: %d : file \"%s\" aborted\n", cl - svs.clients, cl->downloadName );

	SV_CloseDownload( cl );
}

/*
==================
SV_DoneDownload_f

Downloads are finished
==================
*/
static void SV_DoneDownload_f( client_t *cl ) {
	if ( cl->state == CS_ACTIVE )
		return;

	Com_DPrintf( "clientDownload: %s Done\n", cl->name);
	// resend the game state to update any clients that entered during the download
	SV_SendClientGameState(cl);
}

/*
==================
SV_NextDownload_f

The argument will be the last acknowledged block from the client, it should be
the same as cl->downloadClientBlock
==================
*/
static void SV_NextDownload_f( client_t *cl )
{
	int block = atoi( Cmd_Argv(1) );

	if ( cl->state == CS_ACTIVE )
		return;

	if (block == cl->downloadClientBlock) {
		Com_DPrintf( "clientDownload: %d : client acknowledge of block %d\n", cl - svs.clients, block );

		// Find out if we are done.  A zero-length block indicates EOF
		if (cl->downloadBlockSize[cl->downloadClientBlock % MAX_DOWNLOAD_WINDOW] == 0) {
			Com_Printf( "clientDownload: %d : file \"%s\" completed\n", cl - svs.clients, cl->downloadName );
			SV_CloseDownload( cl );
			return;
		}

		cl->downloadSendTime = svs.time;
		cl->downloadClientBlock++;
		return;
	}
	// We aren't getting an acknowledge for the correct block, drop the client
	// FIXME: this is bad... the client will never parse the disconnect message
	//			because the cgame isn't loaded yet
	SV_DropClient( cl, "broken download" );
}

/*
==================
SV_BeginDownload_f
==================
*/
static void SV_BeginDownload_f( client_t *cl ) {
	if ( cl->state == CS_ACTIVE )
		return;

	// Kill any existing download
	SV_CloseDownload( cl );

	// cl->downloadName is non-zero now, SV_WriteDownloadToClient will see this and open
	// the file itself
	Q_strncpyz( cl->downloadName, Cmd_Argv(1), sizeof(cl->downloadName) );
}

/*
==================
SV_WriteDownloadToClient

Check to see if the client wants a file, open it if needed and start pumping the client
Fill up msg with data
==================
*/
void SV_WriteDownloadToClient(client_t *cl, msg_t *msg)
{
	int curindex;
	int rate;
	int blockspersnap;
	int unreferenced = 1;
	char errorMessage[1024];
	char pakbuf[MAX_QPATH], *pakptr;
	int numRefPaks;

	if (!*cl->downloadName)
		return;	// Nothing being downloaded

	if(!cl->download)
	{
		qboolean idPack = qfalse;
		qboolean missionPack = qfalse;

 		// Chop off filename extension.
		Com_sprintf(pakbuf, sizeof(pakbuf), "%s", cl->downloadName);
		pakptr = strrchr(pakbuf, '.');

		if(pakptr)
		{
			*pakptr = '\0';

			// Check for pk3 filename extension
			if(!Q_stricmp(pakptr + 1, "pk3"))
			{
				const char *referencedPaks = FS_ReferencedPakNames();

				// Check whether the file appears in the list of referenced
				// paks to prevent downloading of arbitrary files.
				Cmd_TokenizeStringIgnoreQuotes(referencedPaks);
				numRefPaks = Cmd_Argc();

				for(curindex = 0; curindex < numRefPaks; curindex++)
				{
					if(!FS_FilenameCompare(Cmd_Argv(curindex), pakbuf))
					{
						unreferenced = 0;

						// now that we know the file is referenced,
						// check whether it's legal to download it.
						missionPack = FS_idPak(pakbuf, "missionpack");
						idPack = missionPack;
						idPack = (qboolean)(idPack || FS_idPak(pakbuf, BASEGAME));

						break;
					}
				}
			}
		}

		cl->download = 0;

		// We open the file here
		if ( !sv_allowDownload->integer ||
			idPack || unreferenced ||
			( cl->downloadSize = FS_SV_FOpenFileRead( cl->downloadName, &cl->download ) ) < 0 ) {
			// cannot auto-download file
			if(unreferenced)
			{
				Com_Printf("clientDownload: %d : \"%s\" is not referenced and cannot be downloaded.\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" is not referenced and cannot be downloaded.", cl->downloadName);
			}
			else if (idPack) {
				Com_Printf("clientDownload: %d : \"%s\" cannot download id pk3 files\n", (int) (cl - svs.clients), cl->downloadName);
				if(missionPack)
				{
					Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload Team Arena file \"%s\"\n"
									"The Team Arena mission pack can be found in your local game store.", cl->downloadName);
				}
				else
				{
					Com_sprintf(errorMessage, sizeof(errorMessage), "Cannot autodownload id pk3 file \"%s\"", cl->downloadName);
				}
			}
			else if ( !sv_allowDownload->integer ) {
				Com_Printf("clientDownload: %d : \"%s\" download disabled\n", (int) (cl - svs.clients), cl->downloadName);
				if (sv_pure->integer) {
					Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
										"You will need to get this file elsewhere before you "
										"can connect to this pure server.\n", cl->downloadName);
				} else {
					Com_sprintf(errorMessage, sizeof(errorMessage), "Could not download \"%s\" because autodownloading is disabled on the server.\n\n"
                    "The server you are connecting to is not a pure server, "
                    "set autodownload to No in your settings and you might be "
                    "able to join the game anyway.\n", cl->downloadName);
				}
			} else {
        // NOTE TTimo this is NOT supposed to happen unless bug in our filesystem scheme?
        //   if the pk3 is referenced, it must have been found somewhere in the filesystem
				Com_Printf("clientDownload: %d : \"%s\" file not found on server\n", (int) (cl - svs.clients), cl->downloadName);
				Com_sprintf(errorMessage, sizeof(errorMessage), "File \"%s\" not found on server for autodownloading.\n", cl->downloadName);
			}
			MSG_WriteByte( msg, svc_download );
			MSG_WriteShort( msg, 0 ); // client is expecting block zero
			MSG_WriteLong( msg, -1 ); // illegal file size
			MSG_WriteString( msg, errorMessage );

			*cl->downloadName = 0;

			if(cl->download)
				FS_FCloseFile(cl->download);

			return;
		}

		Com_Printf( "clientDownload: %d : beginning \"%s\"\n", (int) (cl - svs.clients), cl->downloadName );

		// Init
		cl->downloadCurrentBlock = cl->downloadClientBlock = cl->downloadXmitBlock = 0;
		cl->downloadCount = 0;
		cl->downloadEOF = qfalse;
	}

	// Perform any reads that we need to
	while (cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW &&
		cl->downloadSize != cl->downloadCount) {

		curindex = (cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW);

		if (!cl->downloadBlocks[curindex])
			cl->downloadBlocks[curindex] = (unsigned char *)Z_Malloc( MAX_DOWNLOAD_BLKSIZE, TAG_DOWNLOAD, qtrue );

		cl->downloadBlockSize[curindex] = FS_Read( cl->downloadBlocks[curindex], MAX_DOWNLOAD_BLKSIZE, cl->download );

		if (cl->downloadBlockSize[curindex] < 0) {
			// EOF right now
			cl->downloadCount = cl->downloadSize;
			break;
		}

		cl->downloadCount += cl->downloadBlockSize[curindex];

		// Load in next block
		cl->downloadCurrentBlock++;
	}

	// Check to see if we have eof condition and add the EOF block
	if (cl->downloadCount == cl->downloadSize &&
		!cl->downloadEOF &&
		cl->downloadCurrentBlock - cl->downloadClientBlock < MAX_DOWNLOAD_WINDOW) {

		cl->downloadBlockSize[cl->downloadCurrentBlock % MAX_DOWNLOAD_WINDOW] = 0;
		cl->downloadCurrentBlock++;

		cl->downloadEOF = qtrue;  // We have added the EOF block
	}

	// Loop up to window size times based on how many blocks we can fit in the
	// client snapMsec and rate

	// based on the rate, how many bytes can we fit in the snapMsec time of the client
	// normal rate / snapshotMsec calculation
	rate = cl->rate;
	if ( sv_maxRate->integer ) {
		if ( sv_maxRate->integer < 1000 ) {
			Cvar_Set( "sv_MaxRate", "1000" );
		}
		if ( sv_maxRate->integer < rate ) {
			rate = sv_maxRate->integer;
		}
	}

	if (!rate) {
		blockspersnap = 1;
	} else {
		blockspersnap = ( (rate * cl->snapshotMsec) / 1000 + MAX_DOWNLOAD_BLKSIZE ) /
			MAX_DOWNLOAD_BLKSIZE;
	}

	if (blockspersnap < 0)
		blockspersnap = 1;

	while (blockspersnap--) {

		// Write out the next section of the file, if we have already reached our window,
		// automatically start retransmitting

		if (cl->downloadClientBlock == cl->downloadCurrentBlock)
			return; // Nothing to transmit

		if (cl->downloadXmitBlock == cl->downloadCurrentBlock) {
			// We have transmitted the complete window, should we start resending?

			//FIXME:  This uses a hardcoded one second timeout for lost blocks
			//the timeout should be based on client rate somehow
			if (svs.time - cl->downloadSendTime > 1000)
				cl->downloadXmitBlock = cl->downloadClientBlock;
			else
				return;
		}

		// Send current block
		curindex = (cl->downloadXmitBlock % MAX_DOWNLOAD_WINDOW);

		MSG_WriteByte( msg, svc_download );
		MSG_WriteShort( msg, cl->downloadXmitBlock );

		// block zero is special, contains file size
		if ( cl->downloadXmitBlock == 0 )
			MSG_WriteLong( msg, cl->downloadSize );

		MSG_WriteShort( msg, cl->downloadBlockSize[curindex] );

		// Write the block
		if ( cl->downloadBlockSize[curindex] ) {
			MSG_WriteData( msg, cl->downloadBlocks[curindex], cl->downloadBlockSize[curindex] );
		}

		Com_DPrintf( "clientDownload: %d : writing block %d\n", (int) (cl - svs.clients), cl->downloadXmitBlock );

		// Move on to the next block
		// It will get sent with next snap shot.  The rate will keep us in line.
		cl->downloadXmitBlock++;

		cl->downloadSendTime = svs.time;
	}
}

/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately  FIXME: move to game?
=================
*/
const char *SV_GetStringEdString(char *refSection, char *refName);
static void SV_Disconnect_f( client_t *cl ) {
//	SV_DropClient( cl, "disconnected" );
	SV_DropClient( cl, SV_GetStringEdString("MP_SVGAME","DISCONNECTED") );
}

/*
=================
SV_VerifyPaks_f

If we are pure, disconnect the client if they do no meet the following conditions:

1. the first two checksums match our view of cgame and ui
2. there are no any additional checksums that we do not have

This routine would be a bit simpler with a goto but i abstained

=================
*/
static void SV_VerifyPaks_f( client_t *cl ) {
	int nChkSum1, nChkSum2, nClientPaks, nServerPaks, i, j, nCurArg;
	int nClientChkSum[1024];
	int nServerChkSum[1024];
	const char *pPaks, *pArg;
	qboolean bGood = qtrue;

	// if we are pure, we "expect" the client to load certain things from
	// certain pk3 files, namely we want the client to have loaded the
	// ui and cgame that we think should be loaded based on the pure setting
	//
	if ( sv_pure->integer != 0 ) {

		bGood = qtrue;
		nChkSum1 = nChkSum2 = 0;
		// we run the game, so determine which cgame and ui the client "should" be running
		//dlls are valid too now -rww
		bGood = (qboolean)(FS_FileIsInPAK("cgamex86.dll", &nChkSum1) == 1);

		if (bGood)
			bGood = (qboolean)(FS_FileIsInPAK("uix86.dll", &nChkSum2) == 1);

		nClientPaks = Cmd_Argc();

		// start at arg 1 ( skip cl_paks )
		nCurArg = 1;

		// we basically use this while loop to avoid using 'goto' :)
		while (bGood) {

			// must be at least 6: "cl_paks cgame ui @ firstref ... numChecksums"
			// numChecksums is encoded
			if (nClientPaks < 6) {
				bGood = qfalse;
				break;
			}
			// verify first to be the cgame checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || atoi(pArg) != nChkSum1 ) {
				bGood = qfalse;
				break;
			}
			// verify the second to be the ui checksum
			pArg = Cmd_Argv(nCurArg++);
			if (!pArg || *pArg == '@' || atoi(pArg) != nChkSum2 ) {
				bGood = qfalse;
				break;
			}
			// should be sitting at the delimeter now
			pArg = Cmd_Argv(nCurArg++);
			if (*pArg != '@') {
				bGood = qfalse;
				break;
			}
			// store checksums since tokenization is not re-entrant
			for (i = 0; nCurArg < nClientPaks; i++) {
				nClientChkSum[i] = atoi(Cmd_Argv(nCurArg++));
			}

			// store number to compare against (minus one cause the last is the number of checksums)
			nClientPaks = i - 1;

			// make sure none of the client check sums are the same
			// so the client can't send 5 the same checksums
			for (i = 0; i < nClientPaks; i++) {
				for (j = 0; j < nClientPaks; j++) {
					if (i == j)
						continue;
					if (nClientChkSum[i] == nClientChkSum[j]) {
						bGood = qfalse;
						break;
					}
				}
				if (bGood == qfalse)
					break;
			}
			if (bGood == qfalse)
				break;

			// get the pure checksums of the pk3 files loaded by the server
			pPaks = FS_LoadedPakPureChecksums();
			Cmd_TokenizeString( pPaks );
			nServerPaks = Cmd_Argc();
			if (nServerPaks > 1024)
				nServerPaks = 1024;

			for (i = 0; i < nServerPaks; i++) {
				nServerChkSum[i] = atoi(Cmd_Argv(i));
			}

			// check if the client has provided any pure checksums of pk3 files not loaded by the server
			for (i = 0; i < nClientPaks; i++) {
				for (j = 0; j < nServerPaks; j++) {
					if (nClientChkSum[i] == nServerChkSum[j]) {
						break;
					}
				}
				if (j >= nServerPaks) {
					bGood = qfalse;
					break;
				}
			}
			if ( bGood == qfalse ) {
				break;
			}

			// check if the number of checksums was correct
			nChkSum1 = sv.checksumFeed;
			for (i = 0; i < nClientPaks; i++) {
				nChkSum1 ^= nClientChkSum[i];
			}
			nChkSum1 ^= nClientPaks;
			if (nChkSum1 != nClientChkSum[nClientPaks]) {
				bGood = qfalse;
				break;
			}

			// break out
			break;
		}

		cl->gotCP = qtrue;

		if (bGood) {
			cl->pureAuthentic = 1;
		}
		else {
			cl->pureAuthentic = 0;
			cl->nextSnapshotTime = -1;
			cl->state = CS_ACTIVE;
			SV_SendClientSnapshot( cl );
			SV_DropClient( cl, "Unpure client detected. Invalid .PK3 files referenced!" );
		}
	}
}

/*
=================
SV_ResetPureClient_f
=================
*/
static void SV_ResetPureClient_f( client_t *cl ) {
	cl->pureAuthentic = 0;
	cl->gotCP = qfalse;
}

/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C friendly form.
=================
*/
void SV_UserinfoChanged( client_t *cl ) {
	char	*val=NULL, *ip=NULL;
	int		i=0, len=0;

	// name for C code
	Q_strncpyz( cl->name, Info_ValueForKey (cl->userinfo, "name"), sizeof(cl->name) );

	// rate command

	// if the client is on the same subnet as the server and we aren't running an
	// internet public server, assume they don't need a rate choke
	if ( Sys_IsLANAddress( cl->netchan.remoteAddress ) && com_dedicated->integer != 2 && sv_lanForceRate->integer == 1 ) {
		cl->rate = 100000;	// lans should not rate limit
	} else {
		val = Info_ValueForKey (cl->userinfo, "rate");
		if (sv_ratePolicy->integer == 1)
		{
			// NOTE: what if server sets some dumb sv_clientRate value?
			cl->rate = sv_clientRate->integer;
		}
		else if( sv_ratePolicy->integer == 2)
		{
			i = atoi(val);
			if (!i) {
				i = sv_maxRate->integer; //FIXME old code was 3000 here, should increase to 5000 instead or maxRate?
			}
			i = Com_Clampi(1000, 100000, i);
			i = Com_Clampi( sv_minRate->integer, sv_maxRate->integer, i );
			if (i != cl->rate) {
				cl->rate = i;
			}
		}
	}

	// snaps command
	//Note: cl->snapshotMsec is also validated in sv_main.cpp -> SV_CheckCvars if sv_fps, sv_snapsMin or sv_snapsMax is changed
	int minSnaps = Com_Clampi(1, sv_snapsMax->integer, sv_snapsMin->integer); // between 1 and sv_snapsMax ( 1 <-> 40 )
	int maxSnaps = Q_min(sv_fps->integer, sv_snapsMax->integer); // can't produce more than sv_fps snapshots/sec, but can send less than sv_fps snapshots/sec
	val = Info_ValueForKey(cl->userinfo, "snaps");
	cl->wishSnaps = atoi(val);
	if (!cl->wishSnaps)
		cl->wishSnaps = maxSnaps;
	if (sv_snapsPolicy->integer == 1)
	{
		cl->wishSnaps = sv_fps->integer;
		i = 1000 / sv_fps->integer;
		if (i != cl->snapshotMsec) {
			// Reset next snapshot so we avoid desync between server frame time and snapshot send time
			cl->nextSnapshotTime = -1;
			cl->snapshotMsec = i;
		}
	}
	else if (sv_snapsPolicy->integer == 2)
	{
		i = 1000 / Com_Clampi(minSnaps, maxSnaps, cl->wishSnaps);
		if (i != cl->snapshotMsec) {
			// Reset next snapshot so we avoid desync between server frame time and snapshot send time
			cl->nextSnapshotTime = -1;
			cl->snapshotMsec = i;
		}
	}

	// TTimo
	// maintain the IP information
	// the banning code relies on this being consistently present
	if( NET_IsLocalAddress(cl->netchan.remoteAddress) )
		ip = "localhost";
	else
		ip = (char*)NET_AdrToString( cl->netchan.remoteAddress );

	val = Info_ValueForKey( cl->userinfo, "ip" );
	if( val[0] )
		len = strlen( ip ) - strlen( val ) + strlen( cl->userinfo );
	else
		len = strlen( ip ) + 4 + strlen( cl->userinfo );

	if( len >= MAX_INFO_STRING )
		SV_DropClient( cl, "userinfo string length exceeded" );
	else
		Info_SetValueForKey( cl->userinfo, "ip", ip );
}

#define INFO_CHANGE_MIN_INTERVAL	6000 //6 seconds is reasonable I suppose
#define INFO_CHANGE_MAX_COUNT		3 //only allow 3 changes within the 6 seconds

/*
==================
SV_UpdateUserinfo_f
==================
*/
static void SV_UpdateUserinfo_f( client_t *cl ) {
	char *arg = Cmd_Argv(1);

	// Stop random empty /userinfo calls without hurting anything
	if( !arg || !*arg )
		return;

	Q_strncpyz( cl->userinfo, arg, sizeof(cl->userinfo) );

#ifdef FINAL_BUILD
	if (cl->lastUserInfoChange > svs.time)
	{
		cl->lastUserInfoCount++;

		if (cl->lastUserInfoCount >= INFO_CHANGE_MAX_COUNT)
		{
		//	SV_SendServerCommand(cl, "print \"Warning: Too many info changes, last info ignored\n\"\n");
			SV_SendServerCommand(cl, "print \"@@@TOO_MANY_INFO\n\"\n");
			return;
		}
	}
	else
#endif
	{
		cl->lastUserInfoCount = 0;
		cl->lastUserInfoChange = svs.time + INFO_CHANGE_MIN_INTERVAL;
	}

	SV_UserinfoChanged( cl );
	// call prog code to allow overrides
	GVM_ClientUserinfoChanged( cl - svs.clients );
}

typedef struct ucmd_s {
	const char	*name;
	void	(*func)( client_t *cl );
} ucmd_t;

static ucmd_t ucmds[] = {
	{"userinfo", SV_UpdateUserinfo_f},
	{"disconnect", SV_Disconnect_f},
	{"cp", SV_VerifyPaks_f},
	{"vdr", SV_ResetPureClient_f},
	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},
	{"stopdl", SV_StopDownload_f},
	{"donedl", SV_DoneDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteClientCommand

Also called by bot code
==================
*/
void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK ) {
	ucmd_t	*u;
	qboolean bProcessed = qfalse;

	Cmd_TokenizeString( s );

	// see if it is a server level command
	for (u=ucmds ; u->name ; u++) {
		if (!strcmp (Cmd_Argv(0), u->name) ) {
			u->func( cl );
			bProcessed = qtrue;
			break;
		}
	}

	if (clientOK) {
		// pass unknown strings to the game
		if (!u->name && sv.state == SS_GAME && (cl->state == CS_ACTIVE || cl->state == CS_PRIMED)) {
			// strip \r \n and ;
			if ( sv_filterCommands->integer ) {
				Cmd_Args_Sanitize( MAX_CVAR_VALUE_STRING, "\n\r", "  " );
				if ( sv_filterCommands->integer == 2 ) {
					// also strip ';' for callvote
					Cmd_Args_Sanitize( MAX_CVAR_VALUE_STRING, ";", " " );
				}
			}
			GVM_ClientCommand( cl - svs.clients );
		}
	}
	else if (!bProcessed)
		Com_DPrintf( "client text ignored for %s: %s\n", cl->name, Cmd_Argv(0) );
}

/*
===============
SV_ClientCommand
===============
*/
static qboolean SV_ClientCommand( client_t *cl, msg_t *msg ) {
	int		seq;
	const char	*s;
	qboolean clientOk = qtrue;

	seq = MSG_ReadLong( msg );
	s = MSG_ReadString( msg );

	// see if we have already executed it
	if ( cl->lastClientCommand >= seq ) {
		return qtrue;
	}

	Com_DPrintf( "clientCommand: %s : %i : %s\n", cl->name, seq, s );

	// drop the connection if we have somehow lost commands
	if ( seq > cl->lastClientCommand + 1 ) {
		Com_Printf( "Client %s lost %i clientCommands\n", cl->name,
			seq - cl->lastClientCommand + 1 );
		SV_DropClient( cl, "Lost reliable commands" );
		return qfalse;
	}

	// malicious users may try using too many string commands
	// to lag other players.  If we decide that we want to stall
	// the command, we will stop processing the rest of the packet,
	// including the usercmd.  This causes flooders to lag themselves
	// but not other people
	// We don't do this when the client hasn't been active yet since its
	// normal to spam a lot of commands when downloading
	if ( !com_cl_running->integer &&
		cl->state >= CS_ACTIVE &&
		sv_floodProtect->integer )
	{
		const int floodTime = (sv_floodProtect->integer == 1) ? 1000 : sv_floodProtect->integer;
		if ( svs.time < (cl->lastReliableTime + floodTime) ) {
			// ignore any other text messages from this client but let them keep playing
			// TTimo - moved the ignored verbose to the actual processing in SV_ExecuteClientCommand, only printing if the core doesn't intercept
			clientOk = qfalse;
		}
		else {
			cl->lastReliableTime = svs.time;
		}
		if ( sv_floodProtectSlow->integer ) {
			cl->lastReliableTime = svs.time;
		}
	}

	SV_ExecuteClientCommand( cl, s, clientOk );

	cl->lastClientCommand = seq;
	Com_sprintf(cl->lastClientCommandString, sizeof(cl->lastClientCommandString), "%s", s);

	return qtrue;		// continue procesing
}


//==================================================================================


/*
==================
SV_ClientThink

Also called by bot code
==================
*/
void SV_ClientThink (client_t *cl, usercmd_t *cmd) {
	cl->lastUsercmd = *cmd;

	if ( cl->state != CS_ACTIVE ) {
		return;		// may have been kicked during the last usercmd
	}

	GVM_ClientThink( cl - svs.clients, NULL );
}

/*
==================
SV_UserMove

The message usually contains all the movement commands
that were in the last three packets, so that the information
in dropped packets can be recovered.

On very fast clients, there may be multiple usercmd packed into
each of the backup packets.
==================
*/
static void SV_UserMove( client_t *cl, msg_t *msg, qboolean delta ) {
	int			i, key;
	int			cmdCount;
	usercmd_t	nullcmd;
	usercmd_t	cmds[MAX_PACKET_USERCMDS];
	usercmd_t	*cmd, *oldcmd;

	if ( delta ) {
		cl->deltaMessage = cl->messageAcknowledge;
	} else {
		cl->deltaMessage = -1;
	}

	cmdCount = MSG_ReadByte( msg );

	if ( cmdCount < 1 ) {
		Com_Printf( "cmdCount < 1\n" );
		return;
	}

	if ( cmdCount > MAX_PACKET_USERCMDS ) {
		Com_Printf( "cmdCount > MAX_PACKET_USERCMDS\n" );
		return;
	}

	// use the checksum feed in the key
	key = sv.checksumFeed;
	// also use the message acknowledge
	key ^= cl->messageAcknowledge;
	// also use the last acknowledged server command in the key
	key ^= Com_HashKey(cl->reliableCommands[ cl->reliableAcknowledge & (MAX_RELIABLE_COMMANDS-1) ], 32);

	Com_Memset( &nullcmd, 0, sizeof(nullcmd) );
	oldcmd = &nullcmd;
	for ( i = 0 ; i < cmdCount ; i++ ) {
		cmd = &cmds[i];
		MSG_ReadDeltaUsercmdKey( msg, key, oldcmd, cmd );
		if ( sv_legacyFixes->integer ) {
			// block "charge jump" and other nonsense
			if ( cmd->forcesel == FP_LEVITATION || cmd->forcesel >= NUM_FORCE_POWERS ) {
				cmd->forcesel = 0xFFu;
			}

			// affects speed calculation
			cmd->angles[ROLL] = 0;
		}
		oldcmd = cmd;
	}

	// save time for ping calculation
	cl->frames[ cl->messageAcknowledge & PACKET_MASK ].messageAcked = svs.time;

	// TTimo
	// catch the no-cp-yet situation before SV_ClientEnterWorld
	// if CS_ACTIVE, then it's time to trigger a new gamestate emission
	// if not, then we are getting remaining parasite usermove commands, which we should ignore
	if (sv_pure->integer != 0 && cl->pureAuthentic == 0 && !cl->gotCP) {
		if (cl->state == CS_ACTIVE)
		{
			// we didn't get a cp yet, don't assume anything and just send the gamestate all over again
			Com_DPrintf( "%s: didn't get cp command, resending gamestate\n", cl->name);
			SV_SendClientGameState( cl );
		}
		return;
	}

	// if this is the first usercmd we have received
	// this gamestate, put the client into the world
	if ( cl->state == CS_PRIMED ) {
		SV_ClientEnterWorld( cl, &cmds[0] );
		// the moves can be processed normaly
	}

	// a bad cp command was sent, drop the client
	if (sv_pure->integer != 0 && cl->pureAuthentic == 0) {
		SV_DropClient( cl, "Cannot validate pure client!");
		return;
	}

	if ( cl->state != CS_ACTIVE ) {
		cl->deltaMessage = -1;
		return;
	}

	// usually, the first couple commands will be duplicates
	// of ones we have previously received, but the servertimes
	// in the commands will cause them to be immediately discarded
	for ( i =  0 ; i < cmdCount ; i++ ) {
		// if this is a cmd from before a map_restart ignore it
		if ( cmds[i].serverTime > cmds[cmdCount-1].serverTime ) {
			continue;
		}
		// extremely lagged or cmd from before a map_restart
		//if ( cmds[i].serverTime > svs.time + 3000 ) {
		//	continue;
		//}
		// don't execute if this is an old cmd which is already executed
		// these old cmds are included when cl_packetdup > 0
		if ( cmds[i].serverTime <= cl->lastUsercmd.serverTime ) {
			continue;
		}
		SV_ClientThink (cl, &cmds[ i ]);
	}
}


/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/

/*
===================
SV_ExecuteClientMessage

Parse a client packet
===================
*/
void SV_ExecuteClientMessage( client_t *cl, msg_t *msg ) {
	int			c;
	int			serverId;

	MSG_Bitstream(msg);

	serverId = MSG_ReadLong( msg );
	cl->messageAcknowledge = MSG_ReadLong( msg );

	if (cl->messageAcknowledge < 0) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SV_DropClient( cl, "illegible client message" );
		return;
	}

	cl->reliableAcknowledge = MSG_ReadLong( msg );

	// NOTE: when the client message is fux0red the acknowledgement numbers
	// can be out of range, this could cause the server to send thousands of server
	// commands which the server thinks are not yet acknowledged in SV_UpdateServerCommandsToClient
	if (cl->reliableAcknowledge < cl->reliableSequence - MAX_RELIABLE_COMMANDS) {
		// usually only hackers create messages like this
		// it is more annoying for them to let them hanging
		//SV_DropClient( cl, "illegible client message" );
		cl->reliableAcknowledge = cl->reliableSequence;
		return;
	}
	// if this is a usercmd from a previous gamestate,
	// ignore it or retransmit the current gamestate
	//
	// if the client was downloading, let it stay at whatever serverId and
	// gamestate it was at.  This allows it to keep downloading even when
	// the gamestate changes.  After the download is finished, we'll
	// notice and send it a new game state
	//
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=536
	// don't drop as long as previous command was a nextdl, after a dl is done, downloadName is set back to ""
	// but we still need to read the next message to move to next download or send gamestate
	// I don't like this hack though, it must have been working fine at some point, suspecting the fix is somewhere else
	if ( serverId != sv.serverId && !*cl->downloadName && !strstr(cl->lastClientCommandString, "nextdl") ) {
		if ( serverId >= sv.restartedServerId && serverId < sv.serverId ) { // TTimo - use a comparison here to catch multiple map_restart
			// they just haven't caught the map_restart yet
			Com_DPrintf("%s : ignoring pre map_restart / outdated client message\n", cl->name);
			return;
		}
		// if we can tell that the client has dropped the last
		// gamestate we sent them, resend it
		if ( cl->messageAcknowledge > cl->gamestateMessageNum ) {
			Com_DPrintf( "%s : dropped gamestate, resending\n", cl->name );
			SV_SendClientGameState( cl );
		}
		return;
	}

	// this client has acknowledged the new gamestate so it's
	// safe to start sending it the real time again
	if( cl->oldServerTime && serverId == sv.serverId ) {
		Com_DPrintf( "%s acknowledged gamestate\n", cl->name );
		cl->oldServerTime = 0;
	}

	// read optional clientCommand strings
	do {
		c = MSG_ReadByte( msg );
		if ( c == clc_EOF ) {
			break;
		}
		if ( c != clc_clientCommand ) {
			break;
		}
		if ( !SV_ClientCommand( cl, msg ) ) {
			return;	// we couldn't execute it because of the flood protection
		}
		if (cl->state == CS_ZOMBIE) {
			return;	// disconnect command
		}
	} while ( 1 );

	// read the usercmd_t
	if ( c == clc_move ) {
		SV_UserMove( cl, msg, qtrue );
	} else if ( c == clc_moveNoDelta ) {
		SV_UserMove( cl, msg, qfalse );
	} else if ( c != clc_EOF ) {
		Com_Printf( "WARNING: bad command byte for client %i\n", cl - svs.clients );
	}
//	if ( msg->readcount != msg->cursize ) {
//		Com_Printf( "WARNING: Junk at end of packet for client %i\n", cl - svs.clients );
//	}
}

