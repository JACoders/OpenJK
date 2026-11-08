/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// cl_http.h -- a web request made on its own thread, out of the way of the game; see cl_http.cpp

#pragma once

#define HTTP_MAX_ANSWER		( 256 * 1024 )
#define HTTP_MAX_BIG		( 8 * 1024 * 1024 )		// a bigger answer (pictures, long pages), on the heap
#define HTTP_HEADERS_LEN	2048
#define HTTP_RESP_HEADERS	8192

typedef enum {
	HTTP_IDLE,
	HTTP_BUSY,
	HTTP_DONE
} httpState_t;

typedef struct {
	httpState_t		state;
	volatile long	finished;			// the thread is done with what follows
	int				status;				// the answer's HTTP status, 0 when none came
	char			url[512];
	char			*send;				// what's POSTed, NULL for a GET; freed when it's done
	int				sendLen;
	char			answer[HTTP_MAX_ANSWER];	// what came back, cut at its end
	int				answerLen;
	int				waitMs;				// how long the answer may take, 0 as usual: a poll held open longer

	// set before HTTP_Start, all optional (the datapad's web apps): kept until changed
	char			headers[HTTP_HEADERS_LEN];	// more request headers, each ending in \r\n (a Cookie: ...)
	qboolean		form;				// the body is a form (application/x-www-form-urlencoded), not JSON
	qboolean		noRedirect;			// a redirect is the answer, not followed: its Set-Cookie and Location seen
	int				maxAnswer;			// above HTTP_MAX_ANSWER: the answer on the heap, up to this (HTTP_MAX_BIG at most)

	// what came back besides the answer
	char			respHeaders[HTTP_RESP_HEADERS];	// the answer's headers, as sent, \r\n between
	char			*big;				// the answer, when maxAnswer asked for more: freed by the next start or HTTP_Release
	int				bigSize;
} httpRequest_t;

// sends it: a POST of body as JSON, or a GET when body is NULL. qfalse when it couldn't start
// (still busy, or no way to ask on this computer), error then says why
qboolean	HTTP_Start( httpRequest_t *req, const char *url, const char *body, char *error, int errorSize );
// qtrue once, when it's finished: status and answer are then its own
qboolean	HTTP_Poll( httpRequest_t *req );

// the answer, wherever it is (the heap's, or answer), answerLen long, binary or text, 0-terminated
const char	*HTTP_Answer( const httpRequest_t *req );
// a big answer let go of, once read
void		HTTP_Release( httpRequest_t *req );
