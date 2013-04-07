// cg_text.c -- 

// this line must stay at top so the whole PCH thing works...
#include "cg_headers.h"

#include "cg_media.h"


//int precacheWav_i;	// Current high index of precacheWav array
//precacheWav_t precacheWav[MAX_PRECACHEWAV];


//int precacheText_i;	// Current high index of precacheText array
//precacheText_t precacheText[MAX_PRECACHETEXT];


extern vec4_t textcolor_caption;
extern vec4_t textcolor_center;
extern vec4_t textcolor_scroll;


#define	GAMETEXT_X_START	75.0f
#define	GAMETEXT_X_END		600.0f

#define	MAX_NUM_GAMELINES 4

void CG_GameText(int y )
{
	CG_Printf("CG_GameText() being called. Tell Ste\n");

/*	const char	*s,*holds;
	int i, len;
	float	x, w;
	int		numChars;
	int text_i;
	char str[MAX_QPATH];
	int	holdCnt,playingTime;
	int totalLength,sound,max;

	Q_strncpyz (str, CG_Argv( 1 ), MAX_QPATH );

	cg.gameTextSpeaker = atoi(CG_Argv(2));
	cg.gameTextEntNum = atoi(CG_Argv(3));

	sound = cgs.sound_precache[atoi(CG_Argv(4))];

	text_i = CG_SearchTextPrecache(str);
	//ensure we found a match
	if (text_i == -1) 
	{	
		Com_Printf("WARNING: CG_GameText given invalid text key :'%s'",str);
		return; 
	}

	cg.gameTextTime = cg.time;
	cg.printTextY = 5 + SMALLCHAR_HEIGHT;

	cg.gameTextCurrentLine = 0;

	// count the number of lines for centering
	cg.scrollTextLines = 1;


	memset (cg.printText, 0, sizeof(cg.printText));

	// Break into individual lines
	i = 0;
	len = 0;
	s = precacheText[text_i].text;
	holds = s;

	playingTime = cgi_S_GetSampleLength(sound);
	totalLength = strlen(s);
	if (totalLength == 0)
	{
		totalLength = 1;
	}
	cg.gameLetterTime = playingTime / totalLength;

	//We start at column 75 according to DrawGameText
	x = GAMETEXT_X_START;
	w = GAMETEXT_X_END - GAMETEXT_X_START;
	numChars = floor(w/SMALLCHAR_WIDTH);

	while( *s ) 
	{
		len++;
		if (*s == '\n')
		{//Being told explicitly to start a new line
			Q_strncpyz( cg.printText[i], holds, len);
			i++;
			len = 0;
			holds = s;
			holds++;
			cg.scrollTextLines++;
		}
		else if ( len == numChars )
		{//Reached max length of this line
			//step back until we find a space
			while( len && *s != ' ' )
			{
				s--;
				len--;
			}
			//break the line here
			Q_strncpyz( cg.printText[i], holds, len);
			i++;
			len = 0;
			holds = s;
			holds++;
			cg.scrollTextLines++;
		}
		s++;
	}

	len++;  // So the NULL will be properly placed at the end of the string of Q_strncpyz
	Q_strncpyz( cg.printText[i], holds, len); // To get the last line

	//NOTE: This might be able to use the VoiceVolume or TID_VOICE info from the cg.gameTextEntNum
	//		to decide when to drop the text...
	max = MAX_NUM_GAMELINES;
	if (max >cg.scrollTextLines)
	{
		max = cg.scrollTextLines;
	}

	holdCnt = 0;
	for (i=0;i<max;++i)
	{
		holdCnt += strlen(cg.printText[i]);
	}
	cg.gameNextTextTime = cg.time + (holdCnt * cg.gameLetterTime);	


	cg.scrollTextTime = 0;	// No scrolling during captions
*/
}

void CG_DrawGameText(void)
{
	if ( !cg.gameTextTime ) 
	{
		return;
	}
	CG_Printf("CG_DrawGameText() being called. Tell Ste\n");
/*
	char	*start;
	int		l;
	int		i,max;
	int		x, y;
	char linebuffer[1024], string[1024];
	int	holdCnt;
	vec4_t		color; 


	// Advance to next line (if there are any) and calculate time to show
	if ( cg.gameNextTextTime < cg.time ) 
	{
		cg.gameTextCurrentLine += MAX_NUM_GAMELINES;

		if (cg.gameTextCurrentLine >= cg.scrollTextLines)
		{
			cg.gameTextTime = 0;
			return;
		}
		else
		{
			max = MAX_NUM_GAMELINES;
			if ((cg.scrollTextLines - cg.gameTextCurrentLine) < max)
			{
				max = cg.scrollTextLines - cg.gameTextCurrentLine;
			}

			// Loop through next lines to calc how long to show 'em
			holdCnt = 0;
			for (i=cg.gameTextCurrentLine;i<(cg.gameTextCurrentLine + max);++i)
			{
				holdCnt += strlen(cg.printText[i]);
			}
			cg.gameNextTextTime = cg.time + (holdCnt * cg.gameLetterTime);	
		}
	}

	// Give a color if one wasn't given
	if((textcolor_caption[0] == 0) && (textcolor_caption[1] == 0) && 
		(textcolor_caption[2] == 0) && (textcolor_caption[3] == 0))
	{
		Vector4Copy( colorTable[CT_WHITE], textcolor_caption );
	}


	color[0] = colorTable[CT_BLACK][0];
	color[1] = colorTable[CT_BLACK][1];
	color[2] = colorTable[CT_BLACK][2];
	color[3] = 0.350f;

	// Set Y of the first line
	y = cg.printTextY;
	x = GAMETEXT_X_START;

	// Background
	cgi_R_SetColor(color);	// Background, CLAMP TO 4 LINES
	CG_DrawPic( x - 4, y - SMALLCHAR_HEIGHT - 2, (70 * SMALLCHAR_WIDTH),(( ((cg.scrollTextLines>MAX_NUM_GAMELINES)?MAX_NUM_GAMELINES:cg.scrollTextLines) + 1) * SMALLCHAR_HEIGHT) + 4,	cgs.media.ammoslider );

	sprintf(string, "%s:", speakerTable[cg.gameTextSpeaker].stringID);

	CG_DrawStringExt( x, y - SMALLCHAR_HEIGHT, string, colorTable[CT_LTPURPLE1], qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT );

	for (i=	cg.gameTextCurrentLine;i< cg.gameTextCurrentLine + MAX_NUM_GAMELINES;++i)
	{
		start = cg.printText[i];
		while ( 1 ) 
		{

			for ( l = 0; l < 80; l++ ) 
			{
				if ( !start[l] || start[l] == '\n' ) 
				{
					break;
				}
				linebuffer[l] = start[l];
			}
			linebuffer[l] = 0;


			CG_DrawStringExt( x, y, linebuffer, textcolor_caption, qfalse, qtrue,
				SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT );

			y += SMALLCHAR_HEIGHT;

			while ( *start && ( *start != '\n' ) ) 
			{
				start++;
			}

			if ( !*start ) 
			{
				break;
			}
			start++;
		}
	}

	cgi_R_SetColor( NULL );
*/
}


// display text in a supplied box, start at top left and going down by however many pixels I feel like internally, 
//	return value is NULL if all fitted, else char * of next char to continue from that didn't fit.
//
// (coords are in the usual 640x480 virtual space)...
//
// ( if you get the same char * returned as what you passed in, then none of it fitted at all (box too small) )
//
const char *CG_DisplayBoxedText(int iBoxX, int iBoxY, int iBoxWidth, int iBoxHeight, 
								const char *psText, int iFontHandle, float fScale,
								const vec4_t v4Color)
{
	cgi_R_SetColor( v4Color );

	// Setup a reasonable vertical spacing (taiwanese & japanese need 1.5 fontheight, so use that for all)...
	//
	const int iFontHeight		 = cgi_R_Font_HeightPixels(iFontHandle, fScale);
	const int iFontHeightAdvance = (int) (1.5f * (float) iFontHeight);
	int iYpos = iBoxY;	// start print pos

	// findmeste	// test stuff, remove later
//	psText = "�ɨ������դh�w�g�w���F�A�ڤ]��Ҧ��o�{���i���u�ө��v�C�ܤ����a�A��hĵ�����ǥ�è�o�{�F�@�Ǫ��p�A�ǳƦb�����e���Ⱦ���E�ǧJ��o�C�L���˦��~��ϸ`�A��L�F�h�h���ơC�{�b�L�����H��A�åB�¯٭n�����f�r�C�ھڳ̷psCurrentTextReadPos�����i�A�ǧJ��o�H�ΥL���ҦФw�g�������ڤF�����C�ڨ��R�Ӱl���ǧJ��o�H�αϥX�Ҧ��H��C�o�ä��e���C";

	// this could probably be simplified now, but it was converted from something else I didn't originally write, 
	//	and it works anyway so wtf...
	//
	const char *psCurrentTextReadPos = psText;	
	const char *psReadPosAtLineStart = psCurrentTextReadPos;	
	const char *psBestLineBreakSrcPos = psCurrentTextReadPos;
	const char *psLastGood_s;	// needed if we get a full screen of chars with no punctuation or space (see usage notes)
	while( *psCurrentTextReadPos && (iYpos + iFontHeight < (iBoxY + iBoxHeight)) )
	{
		char sLineForDisplay[2048];	// ott

		// construct a line...
		//
		psCurrentTextReadPos = psReadPosAtLineStart;
		sLineForDisplay[0] = '\0';
		while ( *psCurrentTextReadPos )
		{
			psLastGood_s = psCurrentTextReadPos;

			// read letter...
			//
			qboolean bIsTrailingPunctuation;
			unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(&psCurrentTextReadPos, &bIsTrailingPunctuation);

			// concat onto string so far...
			//
			if (uiLetter == 32 && sLineForDisplay[0] == '\0')
			{
				psReadPosAtLineStart++;
				continue;	// unless it's a space at the start of a line, in which case ignore it.
			}

			if (uiLetter > 255)
			{
				Q_strcat(sLineForDisplay, sizeof(sLineForDisplay),va("%c%c",uiLetter >> 8, uiLetter & 0xFF));
			}
			else
			{
				Q_strcat(sLineForDisplay, sizeof(sLineForDisplay),va("%c",uiLetter & 0xFF));
			}

			if (uiLetter == '\n')
			{
				// explicit new line...
				//
				sLineForDisplay[ strlen(sLineForDisplay)-1 ] = '\0';	// kill the CR
				psReadPosAtLineStart = psCurrentTextReadPos;
				psBestLineBreakSrcPos = psCurrentTextReadPos;
				break;	// print this line
			}
			else 
			if ( cgi_R_Font_StrLenPixels(sLineForDisplay, iFontHandle, fScale) >= iBoxWidth )
			{					
				// reached screen edge, so cap off string at bytepos after last good position...
				//
				if (uiLetter > 255 && bIsTrailingPunctuation && !cgi_Language_UsesSpaces())
				{
					// Special case, don't consider line breaking if you're on an asian punctuation char of
					//	a language that doesn't use spaces...
					//
				}
				else
				{
					if (psBestLineBreakSrcPos == psReadPosAtLineStart)
					{
						//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
						//		since it doesn't have a single space or punctuation mark right the way across one line
						//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese 
						//		string into this function while the game was running in english (which should NEVER happen 
						//		normally).  On the other hand I suppose it'psCurrentTextReadPos entirely possible that some taiwanese string 
						//		might have no punctuation at all, so...
						//
						psBestLineBreakSrcPos = psLastGood_s;	// force a break after last good letter
					}

					sLineForDisplay[ psBestLineBreakSrcPos - psReadPosAtLineStart ] = '\0';
					psReadPosAtLineStart = psCurrentTextReadPos = psBestLineBreakSrcPos;
					break;	// print this line
				}
			}

			// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
			//
			if (bIsTrailingPunctuation || uiLetter == ' ' || (uiLetter > 255 && !cgi_Language_UsesSpaces()))
			{
				psBestLineBreakSrcPos = psCurrentTextReadPos;
			}
		}

		// ... and print it...
		//
		cgi_R_Font_DrawString(iBoxX, iYpos, sLineForDisplay, v4Color, iFontHandle, -1, fScale);
		iYpos += iFontHeightAdvance;

		// and echo to console in dev mode...
		//
		if ( cg_developer.integer )
		{
//			Com_Printf( "%psCurrentTextReadPos\n", sLineForDisplay );
		}
	}
	return psReadPosAtLineStart;
}



/*
===============================================================================

CAPTION TEXT

===============================================================================
*/
void CG_CaptionTextStop(void) 
{
	cg.captionTextTime = 0;
}

// try and get the correct StripEd text (with retry) for a given reference...
//
// returns 0 if failed, else strlen...
//
static int cg_SP_GetStringTextStringWithRetry( LPCSTR psReference, char *psDest, int iSizeofDest)
{
	int iReturn;

	if (psReference[0] == '#')
	{
		// then we know the striped package name is already built in, so do NOT try prepending anything else...
		//
		return cgi_SP_GetStringTextString( va("%s",psReference+1), psDest, iSizeofDest );
	}

	for (int i=0; i<STRIPED_LEVELNAME_VARIATIONS; i++)
	{
		if (cgs.stripLevelName[i][0])	// entry present?
		{
			iReturn = cgi_SP_GetStringTextString( va("%s_%s",cgs.stripLevelName[i],psReference), psDest, iSizeofDest );
			if (iReturn)
			{
				return iReturn;
			}
		}
	}

	return 0;
}

// slightly confusingly, the char arg for this function is an audio filename of the form "path/path/filename",
//	the "filename" part of which should be the same as the StripEd reference we're looking for in the current 
//	level's string package...
//
void CG_CaptionText( const char *str, int sound, int y ) 
{
	const char	*s, *holds;
	int i;
	int	holdTime;
	char text[8192]={0};

	const float fFontScale = cgi_Language_IsAsian() ? 0.8f : 1.0f;

	holds = strrchr(str,'/');
	if (!holds)
	{
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_CaptionText given audio filename with no '/':'%s'\n",str);
#endif
		return; 
	}
	i = cg_SP_GetStringTextStringWithRetry( holds+1, text, sizeof(text) );
	//ensure we found a match
	if (!i) 
	{	
#ifndef FINAL_BUILD
		// we only care about some sound dirs...
		if (!strnicmp(str,"sound/chars/",12))	// whichever language it is, it'll be pathed as english at this point
		{
			Com_Printf("WARNING: CG_CaptionText given invalid text key :'%s'\n",str);
		}
		else
		{
			// anything else is probably stuff we don't care about. It certainly shouldn't be speech, anyway
		}
#endif
		return; 
	}

	const int fontHeight = (int) ((cgi_Language_IsAsian() ? 1.4f : 1.0f) * (float) cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, fFontScale));	// taiwanese & japanese need 1.5 fontheight spacing

	cg.captionTextTime = cg.time;
	if (in_camera) {
		cg.captionTextY = SCREEN_HEIGHT - (client_camera.bar_height_dest/2);	// ths is now a centre'd Y, not a start Y
	} else {	//get above the hud
		cg.captionTextY = (int) (0.88f * ((float)SCREEN_HEIGHT - (float)fontHeight * 1.5f));	// do NOT move this, it has to fit in between the weapon HUD and the datapad update.
	}
	cg.captionTextCurrentLine = 0;

	// count the number of lines for centering
	cg.scrollTextLines = 1;

	memset (cg.captionText, 0, sizeof(cg.captionText));

	// Break into individual lines
	i = 0;	// this could be completely removed and replace by "cg.scrollTextLines-1", but wtf?

	// findmeste	// test stuff, remove later
	s=(const char*)&text;
	// tai...
//	s="�ɨ������դh�w�g�w���F�A�ڤ]��Ҧ��o�{���i���u�ө��v�C�ܤ����a�A��hĵ�����ǥ�è�o�{�F�@�Ǫ��p�A�ǳƦb�����e���Ⱦ���E�ǧJ��o�C�L���˦��~��ϸ`�A��L�F�h�h���ơC�{�b�L�����H��A�åB�¯٭n�����f�r�C�ھڳ̷s�����i�A�ǧJ��o�H�ΥL���ҦФw�g�������ڤF�����C�ڨ��R�Ӱl���ǧJ��o�H�αϥX�Ҧ��H��C�o�ä��e���C";
	// kor...
//	s="Wp:��Ÿ���̴� �ָ�. �׵��� ���Ѵ�� �װ� ������ ����ϰڴ�.��Ÿ���̴� �ָ�. �׵��� ���Ѵ�� �װ� ������ ����ϰڴ�.";
	holds = s;

	int iPlayingTimeMS	= cgi_S_GetSampleLength(sound);
	int iLengthInChars	= strlen(s);//cgi_R_Font_StrLenChars(s);	// strlen is also good for MBCS in this instance, since it's for timing
	if (iLengthInChars == 0)
	{
		iLengthInChars = 1;
	}
	cg.captionLetterTime = iPlayingTimeMS / iLengthInChars;

	const char *psBestLineBreakSrcPos = s;
	const char *psLastGood_s;	// needed if we get a full screen of chars with no punctuation or space (see usage notes)
	while( *s )
	{
		psLastGood_s = s;

		// read letter...
		//
		qboolean bIsTrailingPunctuation;
		unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(&s, &bIsTrailingPunctuation);

		// concat onto string so far...
		//
		if (uiLetter == 32 && cg.captionText[i][0] == '\0')
		{
			holds++;
			continue;	// unless it's a space at the start of a line, in which case ignore it.
		}

		if (uiLetter > 255)
		{
			Q_strcat(cg.captionText[i],sizeof(cg.captionText[i]),va("%c%c",uiLetter >> 8, uiLetter & 0xFF));
		}
		else
		{
			Q_strcat(cg.captionText[i],sizeof(cg.captionText[i]),va("%c",uiLetter & 0xFF));
		}

		if (uiLetter == '\n')
		{
			// explicit new line...
			//
			cg.captionText[i][ strlen(cg.captionText[i])-1 ] = '\0';	// kill the CR
			i++;
			holds = s;
			psBestLineBreakSrcPos = s;
			cg.scrollTextLines++;
		}
		else 
		if ( cgi_R_Font_StrLenPixels(cg.captionText[i], cgs.media.qhFontMedium, fFontScale) >= SCREEN_WIDTH)
		{
			// reached screen edge, so cap off string at bytepos after last good position...
			//
			if (uiLetter > 255 && bIsTrailingPunctuation && !cgi_Language_UsesSpaces())
			{
				// Special case, don't consider line breaking if you're on an asian punctuation char of
				//	a language that doesn't use spaces...
				//
			}
			else
			{
				if (psBestLineBreakSrcPos == holds)
				{
					//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
					//		since it doesn't have a single space or punctuation mark right the way across one line
					//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese 
					//		string into this function while the game was running in english (which should NEVER happen 
					//		normally).  On the other hand I suppose it's entirely possible that some taiwanese string 
					//		might have no punctuation at all, so...
					//
					psBestLineBreakSrcPos = psLastGood_s;	// force a break after last good letter
				}

				cg.captionText[i][ psBestLineBreakSrcPos - holds ] = '\0';
				holds = s = psBestLineBreakSrcPos;
				i++;
				cg.scrollTextLines++;
			}
		}

		// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
		//
		if (bIsTrailingPunctuation || uiLetter == ' ' || (uiLetter > 255 && !cgi_Language_UsesSpaces()))
		{
			psBestLineBreakSrcPos = s;
		}
	}


	// calc the length of time to hold each 2 lines of text on the screen.... presumably this works?
	//
	holdTime = strlen(cg.captionText[0]);
	if (cg.scrollTextLines > 1)
	{
		holdTime += strlen(cg.captionText[1]);	// strlen is also good for MBCS in this instance, since it's for timing
	}
	cg.captionNextTextTime = cg.time + (holdTime * cg.captionLetterTime);	

	cg.scrollTextTime = 0;	// No scrolling during captions

	//Echo to console in dev mode
	if ( cg_developer.integer )
	{
		Com_Printf( "%s\n", cg.captionText[0] );	// ste:  was [i], but surely sentence 0 is more useful than last?
	}
}


void CG_DrawCaptionText(void)
{
	int		i;
	int		x, y, w;
	int	holdTime;

	if ( !cg.captionTextTime ) 
	{
		return;
	}

	const float fFontScale = cgi_Language_IsAsian() ? 0.8f : 1.0f;

	if (cg_skippingcin.integer != 0)
	{
		cg.captionTextTime = 0;
		return;
	}

	if ( cg.captionNextTextTime < cg.time ) 
	{
		cg.captionTextCurrentLine += 2;

		if (cg.captionTextCurrentLine >= cg.scrollTextLines)
		{
			cg.captionTextTime = 0;
			return;
		}
		else
		{
			holdTime = strlen(cg.captionText[cg.captionTextCurrentLine]);
			if (cg.scrollTextLines >= cg.captionTextCurrentLine)
			{
				// ( strlen is also good for MBCS in this instance, since it's for timing -ste)
				//
				holdTime += strlen(cg.captionText[cg.captionTextCurrentLine + 1]);
			}

			cg.captionNextTextTime = cg.time + (holdTime * cg.captionLetterTime);//50);
		}
	}

	// Give a color if one wasn't given
	if((textcolor_caption[0] == 0) && (textcolor_caption[1] == 0) && 
		(textcolor_caption[2] == 0) && (textcolor_caption[3] == 0))
	{
		Vector4Copy( colorTable[CT_WHITE], textcolor_caption );
	}

	cgi_R_SetColor(textcolor_caption);

	// Set Y of the first line (varies if only printing one line of text)
	// (this all works, please don't mess with it)
	const int fontHeight = (int) ((cgi_Language_IsAsian() ? 1.4f : 1.0f) * (float) cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, fFontScale));
	const bool bPrinting2Lines = !!(cg.captionText[ cg.captionTextCurrentLine+1 ][0]);
	y = cg.captionTextY - ( (float)fontHeight * (bPrinting2Lines ? 1 : 0.5f));	// captionTextY was a centered Y pos, not a top one
	y -= cgi_Language_IsAsian() ? 0 : 4;

	for (i=	cg.captionTextCurrentLine;i< cg.captionTextCurrentLine + 2;++i)
	{
		w = cgi_R_Font_StrLenPixels(cg.captionText[i], cgs.media.qhFontMedium, fFontScale);	
		if (w)
		{
			x = (SCREEN_WIDTH-w) / 2;
			cgi_R_Font_DrawString(x, y, cg.captionText[i], textcolor_caption, cgs.media.qhFontMedium, -1, fFontScale);
			y += fontHeight;
		}
	}

	cgi_R_SetColor( NULL );
}

/*
===============================================================================

SCROLL TEXT

===============================================================================

CG_ScrollText - split text up into seperate lines

 'str' arg is StripEd string reference, eg "CREDITS_RAVEN"

*/
int giScrollTextPixelWidth = SCREEN_WIDTH;
void CG_ScrollText( const char *str, int iPixelWidth )
{
	const char	*s,*holds;
	int i;//, len;//, numChars;

	giScrollTextPixelWidth = iPixelWidth;

	// first, ask the strlen of the final string...
	//	
	i = cgi_SP_GetStringTextString( str, NULL, 0 );

	//ensure we found a match
	if (!i)
	{
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_ScrollText given invalid text key :'%s'\n",str);
#endif
		return; 
	}
	//
	// malloc space to hold it...
	//
	char *psText = (char *) cgi_Z_Malloc( i+1, TAG_STRING );
	//
	// now get the string...
	//	
	i = cgi_SP_GetStringTextString( str, psText, i+1 );
	//ensure we found a match
	if (!i)
	{
		assert(0);	// should never get here now, but wtf?
		cgi_Z_Free(psText);
#ifndef FINAL_BUILD
		Com_Printf("WARNING: CG_ScrollText given invalid text key :'%s'\n",str);
#endif
		return; 
	}

	cg.scrollTextTime = cg.time;
	cg.printTextY = SCREEN_HEIGHT;
	cg.scrollTextLines = 1;
	
	s = psText;
	i = 0;
	holds = s;

	const char *psBestLineBreakSrcPos = s;
	const char *psLastGood_s;	// needed if we get a full screen of chars with no punctuation or space (see usage notes)
	while( *s )
	{
		psLastGood_s = s;

		// read letter...
		//
		qboolean bIsTrailingPunctuation;
		unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(&s, &bIsTrailingPunctuation);

		// concat onto string so far...
		//
		if (uiLetter == 32 && cg.printText[i][0] == '\0')
		{
			holds++;
			continue;	// unless it's a space at the start of a line, in which case ignore it.
		}

		if (uiLetter > 255)
		{
			Q_strcat(cg.printText[i],sizeof(cg.printText[i]),va("%c%c",uiLetter >> 8, uiLetter & 0xFF));
		}
		else
		{
			Q_strcat(cg.printText[i],sizeof(cg.printText[i]),va("%c",uiLetter & 0xFF));
		}

		// record last-good linebreak pos...  (ie if we've just concat'd a punctuation point (western or asian) or space)
		//
		if (bIsTrailingPunctuation || uiLetter == ' ')
		{
			psBestLineBreakSrcPos = s;
		}

		if (uiLetter == '\n')
		{
			// explicit new line...
			//
			cg.printText[i][ strlen(cg.printText[i])-1 ] = '\0';	// kill the CR
			i++;
			assert (i < (sizeof(cg.printText)/sizeof(cg.printText[0])) );
			if (i >= (sizeof(cg.printText)/sizeof(cg.printText[0])) )
			{
				break;
			}
			holds = s;
			cg.scrollTextLines++;
		}
		else 
		if ( cgi_R_Font_StrLenPixels(cg.printText[i], cgs.media.qhFontMedium, 1.0f) >= iPixelWidth)
		{
			// reached screen edge, so cap off string at bytepos after last good position...
			//
			if (psBestLineBreakSrcPos == holds)
			{
				//  aarrrggh!!!!!   we'll only get here is someone has fed in a (probably) garbage string,
				//		since it doesn't have a single space or punctuation mark right the way across one line
				//		of the screen.  So far, this has only happened in testing when I hardwired a taiwanese 
				//		string into this function while the game was running in english (which should NEVER happen 
				//		normally).  On the other hand I suppose it's entirely possible that some taiwanese string 
				//		might have no punctuation at all, so...
				//
				psBestLineBreakSrcPos = psLastGood_s;	// force a break after last good letter
			}

			cg.printText[i][ psBestLineBreakSrcPos - holds ] = '\0';
			holds = s = psBestLineBreakSrcPos;
			i++;
			assert (i < (sizeof(cg.printText)/sizeof(cg.printText[0])) );
			cg.scrollTextLines++;
		}
	}

	cg.captionTextTime = 0;		// No captions during scrolling
	cgi_Z_Free(psText);
}


// draws using [textcolor_scroll]...
//
#define SCROLL_LPM (1/50.0) // 1 line per 50 ms
void CG_DrawScrollText(void)
{		
	char	*start;
	int		i;
	int		x,y;	
	const int fontHeight = (int) (1.5f * (float) cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f));	// taiwanese & japanese need 1.5 fontheight spacing

	if ( !cg.scrollTextTime ) 
	{
		return;
	}

	cgi_R_SetColor( textcolor_scroll );

	y = cg.printTextY - (cg.time - cg.scrollTextTime) * SCROLL_LPM;

//	cgi_R_Font_DrawString(320, 200, va("Scrolltext printing @ %d",y), colorTable[CT_LTGOLD1], cgs.media.qhFontMedium, -1, 1.0f);

	// See if text has finished scrolling off screen
	if ((y + cg.scrollTextLines * fontHeight) < 1)
	{
		cg.scrollTextTime = 0;
		return;
	}

	for (i=0;i<cg.scrollTextLines;++i)
	{

		// Is this line off top of screen?
		if ((y + ((i +1) * fontHeight)) < 1)
		{
			y += fontHeight;
			continue;
		}
		// or past bottom of screen?
		else if (y > SCREEN_HEIGHT)
		{
			break;
		}

		start = cg.printText[i];

//		w = cgi_R_Font_StrLenPixels(cg.printText[i], cgs.media.qhFontMedium, 1.0f);	
//		if (w)
		{
			x = (SCREEN_WIDTH - giScrollTextPixelWidth) / 2;
			cgi_R_Font_DrawString(x,y, cg.printText[i], textcolor_scroll, cgs.media.qhFontMedium, -1, 1.0f);
			y += fontHeight;
		}		
	}

	cgi_R_SetColor( NULL );
}


/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y) {
	char	*s;
	
	// Find text to match the str given
	if (*str == '@')
	{
		int i;

		i = cgi_SP_GetStringTextString( str+1, cg.centerPrint, sizeof(cg.centerPrint) );

		if (!i)
		{
			Com_Printf (S_COLOR_RED"CG_CenterPrint: cannot find reference '%s' in StringPackage!\n",str);	
			Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );
		}
	}
	else
	{
		Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );
	}

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}

}


/*
===================
CG_DrawCenterString
===================
*/
void CG_DrawCenterString( void ) 
{
	char	*start;
	int		l;
	int		x, y, w;
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	if((textcolor_center[0] == 0) && (textcolor_center[1] == 0) && 
		(textcolor_center[2] == 0) && (textcolor_center[3] == 0))
	{
		Vector4Copy( colorTable[CT_WHITE], textcolor_center );
	}

	start = cg.centerPrint;

	const int fontHeight = cgi_R_Font_HeightPixels(cgs.media.qhFontMedium, 1.0f);
	y = cg.centerPrintY - (cg.centerPrintLines * fontHeight) / 2;

	while ( 1 ) {
		char linebuffer[1024];

		// this is kind of unpleasant when dealing with MBCS, but...
		//		
		const char *psString = start;
		int iOutIndex = 0;
		int nope = 0;
		for ( l = 0; l < sizeof(linebuffer)-1; l++ ) {
			unsigned int uiLetter = cgi_AnyLanguage_ReadCharFromString(&psString, 0);
			if (!uiLetter || uiLetter == '\n'){
				break;
			}
			if (uiLetter > 255)
			{
				linebuffer[iOutIndex++] = uiLetter >> 8;
				linebuffer[iOutIndex++] = uiLetter & 0xFF;
			}
			else
			{
				linebuffer[iOutIndex++] = uiLetter & 0xFF;
			}
		}
		linebuffer[iOutIndex++] = '\0';

		w = cgi_R_Font_StrLenPixels(linebuffer, cgs.media.qhFontMedium, 1.0f);	

		x = ( SCREEN_WIDTH - w ) / 2;

		cgi_R_Font_DrawString(x,y,linebuffer, textcolor_center, cgs.media.qhFontMedium, -1, 1.0f);

		y += fontHeight;

		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

}
