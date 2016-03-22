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

#include "client.h"
#include "cl_cgameapi.h"
#include "cl_uiapi.h"
#include "qcommon/stringed_ingame.h"
/*

key up events are sent even if in console mode

*/

// console
field_t		g_consoleField;
int			nextHistoryLine;	// the last line in the history buffer, not masked
int			historyLine;		// the line being displayed from history buffer will be <= nextHistoryLine
field_t		historyEditLines[COMMAND_HISTORY];

// chat
field_t		chatField;
qboolean	chat_team;
int			chat_playerNum;

keyGlobals_t	kg;

// do NOT blithely change any of the key names (3rd field) here, since they have to match the key binds
//	in the CFG files, they're also prepended with "KEYNAME_" when looking up StringEd references
//
keyname_t keynames[MAX_KEYS] =
{
	{ 0x00, 0x00, NULL, A_NULL, false									},
	{ 0x01, 0x01, "SHIFT", A_SHIFT, false 								},
	{ 0x02, 0x02, "CTRL", A_CTRL, false   								},
	{ 0x03, 0x03, "ALT", A_ALT, false									},
	{ 0x04, 0x04, "CAPSLOCK", A_CAPSLOCK, false							},
	{ 0x05, 0x05, "KP_NUMLOCK", A_NUMLOCK, false						},
	{ 0x06, 0x06, "SCROLLLOCK", A_SCROLLLOCK, false						},
	{ 0x07, 0x07, "PAUSE", A_PAUSE, false								},
	{ 0x08, 0x08, "BACKSPACE", A_BACKSPACE, false						},
	{ 0x09, 0x09, "TAB", A_TAB, false									},
	{ 0x0a, 0x0a, "ENTER", A_ENTER, false								},
	{ 0x0b, 0x0b, "KP_PLUS", A_KP_PLUS, false							},
	{ 0x0c, 0x0c, "KP_MINUS", A_KP_MINUS, false							},
	{ 0x0d, 0x0d, "KP_ENTER", A_KP_ENTER, false							},
	{ 0x0e, 0x0e, "KP_DEL", A_KP_PERIOD, false							},
	{ 0x0f, 0x0f, NULL, A_PRINTSCREEN, false							},
	{ 0x10, 0x10, "KP_INS", A_KP_0, false								},
	{ 0x11, 0x11, "KP_END", A_KP_1, false								},
	{ 0x12, 0x12, "KP_DOWNARROW", A_KP_2, false							},
	{ 0x13, 0x13, "KP_PGDN", A_KP_3, false								},
	{ 0x14, 0x14, "KP_LEFTARROW", A_KP_4, false							},
	{ 0x15, 0x15, "KP_5", A_KP_5, false									},
	{ 0x16, 0x16, "KP_RIGHTARROW", A_KP_6, false						},
	{ 0x17, 0x17, "KP_HOME", A_KP_7, false								},
	{ 0x18, 0x18, "KP_UPARROW", A_KP_8, false							},
	{ 0x19, 0x19, "KP_PGUP", A_KP_9, false								},
	{ 0x1a, 0x1a, "CONSOLE", A_CONSOLE, false 							},
	{ 0x1b, 0x1b, "ESCAPE", A_ESCAPE, false								},
	{ 0x1c, 0x1c, "F1", A_F1, true										},
	{ 0x1d, 0x1d, "F2", A_F2, true										},
	{ 0x1e, 0x1e, "F3", A_F3, true										},
	{ 0x1f, 0x1f, "F4", A_F4, true										},

	{ 0x20, 0x20, "SPACE", A_SPACE, false								},
	{ (word)'!', (word)'!', NULL, A_PLING, false		  				},
	{ (word)'"', (word)'"', NULL, A_DOUBLE_QUOTE, false  				},
	{ (word)'#', (word)'#', NULL, A_HASH, false		  					},
	{ (word)'$', (word)'$', NULL, A_STRING, false						},
	{ (word)'%', (word)'%', NULL, A_PERCENT, false						},
	{ (word)'&', (word)'&', NULL, A_AND, false							},
	{ 0x27, 0x27, NULL, A_SINGLE_QUOTE, false							},
	{ (word)'(', (word)'(', NULL, A_OPEN_BRACKET, false					},
	{ (word)')', (word)')', NULL, A_CLOSE_BRACKET, false				},
	{ (word)'*', (word)'*', NULL, A_STAR, false							},
	{ (word)'+', (word)'+', NULL, A_PLUS, false							},
	{ (word)',', (word)',', NULL, A_COMMA, false						},
	{ (word)'-', (word)'-', NULL, A_MINUS, false						},
	{ (word)'.', (word)'.', NULL, A_PERIOD, false						},
	{ (word)'/', (word)'/', NULL, A_FORWARD_SLASH, false				},
	{ (word)'0', (word)'0', NULL, A_0, false							},
	{ (word)'1', (word)'1', NULL, A_1, false							},
	{ (word)'2', (word)'2', NULL, A_2, false							},
	{ (word)'3', (word)'3', NULL, A_3, false							},
	{ (word)'4', (word)'4', NULL, A_4, false							},
	{ (word)'5', (word)'5', NULL, A_5, false							},
	{ (word)'6', (word)'6', NULL, A_6, false							},
	{ (word)'7', (word)'7', NULL, A_7, false							},
	{ (word)'8', (word)'8', NULL, A_8, false							},
	{ (word)'9', (word)'9', NULL, A_9, false							},
	{ (word)':', (word)':', NULL, A_COLON, false						},
	{ (word)';', (word)';', "SEMICOLON", A_SEMICOLON, false				},
	{ (word)'<', (word)'<', NULL, A_LESSTHAN, false						},
	{ (word)'=', (word)'=', NULL, A_EQUALS, false						},
	{ (word)'>', (word)'>', NULL, A_GREATERTHAN, false					},
	{ (word)'?', (word)'?', NULL, A_QUESTION, false						},

	{ (word)'@', (word)'@', NULL, A_AT, false							},
	{ (word)'A', (word)'a', NULL, A_CAP_A, false						},
	{ (word)'B', (word)'b', NULL, A_CAP_B, false						},
	{ (word)'C', (word)'c', NULL, A_CAP_C, false						},
	{ (word)'D', (word)'d', NULL, A_CAP_D, false						},
	{ (word)'E', (word)'e', NULL, A_CAP_E, false						},
	{ (word)'F', (word)'f', NULL, A_CAP_F, false						},
	{ (word)'G', (word)'g', NULL, A_CAP_G, false						},
	{ (word)'H', (word)'h', NULL, A_CAP_H, false						},
	{ (word)'I', (word)'i', NULL, A_CAP_I, false						},
	{ (word)'J', (word)'j', NULL, A_CAP_J, false						},
	{ (word)'K', (word)'k', NULL, A_CAP_K, false						},
	{ (word)'L', (word)'l', NULL, A_CAP_L, false						},
	{ (word)'M', (word)'m', NULL, A_CAP_M, false						},
	{ (word)'N', (word)'n', NULL, A_CAP_N, false						},
	{ (word)'O', (word)'o', NULL, A_CAP_O, false						},
	{ (word)'P', (word)'p', NULL, A_CAP_P, false						},
	{ (word)'Q', (word)'q', NULL, A_CAP_Q, false						},
	{ (word)'R', (word)'r', NULL, A_CAP_R, false						},
	{ (word)'S', (word)'s', NULL, A_CAP_S, false						},
	{ (word)'T', (word)'t', NULL, A_CAP_T, false						},
	{ (word)'U', (word)'u', NULL, A_CAP_U, false						},
	{ (word)'V', (word)'v', NULL, A_CAP_V, false						},
	{ (word)'W', (word)'w', NULL, A_CAP_W, false						},
	{ (word)'X', (word)'x', NULL, A_CAP_X, false						},
	{ (word)'Y', (word)'y', NULL, A_CAP_Y, false						},
	{ (word)'Z', (word)'z', NULL, A_CAP_Z, false						},
	{ (word)'[', (word)'[', NULL, A_OPEN_SQUARE, false					},
	{ 0x5c, 0x5c, NULL, A_BACKSLASH, false								},
	{ (word)']', (word)']', NULL, A_CLOSE_SQUARE, false 				},
	{ (word)'^', (word)'^', NULL, A_CARET, false		 				},
	{ (word)'_', (word)'_', NULL, A_UNDERSCORE, false					},

	{ 0x60, 0x60, NULL, A_LEFT_SINGLE_QUOTE, false						},
	{ (word)'A', (word)'a', NULL, A_LOW_A, false						},
	{ (word)'B', (word)'b', NULL, A_LOW_B, false						},
	{ (word)'C', (word)'c', NULL, A_LOW_C, false						},
	{ (word)'D', (word)'d', NULL, A_LOW_D, false						},
	{ (word)'E', (word)'e', NULL, A_LOW_E, false						},
	{ (word)'F', (word)'f', NULL, A_LOW_F, false						},
	{ (word)'G', (word)'g', NULL, A_LOW_G, false						},
	{ (word)'H', (word)'h', NULL, A_LOW_H, false						},
	{ (word)'I', (word)'i', NULL, A_LOW_I, false						},
	{ (word)'J', (word)'j', NULL, A_LOW_J, false						},
	{ (word)'K', (word)'k', NULL, A_LOW_K, false						},
	{ (word)'L', (word)'l', NULL, A_LOW_L, false						},
	{ (word)'M', (word)'m', NULL, A_LOW_M, false						},
	{ (word)'N', (word)'n', NULL, A_LOW_N, false						},
	{ (word)'O', (word)'o', NULL, A_LOW_O, false						},
	{ (word)'P', (word)'p', NULL, A_LOW_P, false						},
	{ (word)'Q', (word)'q', NULL, A_LOW_Q, false						},
	{ (word)'R', (word)'r', NULL, A_LOW_R, false						},
	{ (word)'S', (word)'s', NULL, A_LOW_S, false						},
	{ (word)'T', (word)'t', NULL, A_LOW_T, false						},
	{ (word)'U', (word)'u', NULL, A_LOW_U, false						},
	{ (word)'V', (word)'v', NULL, A_LOW_V, false						},
	{ (word)'W', (word)'w', NULL, A_LOW_W, false						},
	{ (word)'X', (word)'x', NULL, A_LOW_X, false						},
	{ (word)'Y', (word)'y', NULL, A_LOW_Y, false						},
	{ (word)'Z', (word)'z', NULL, A_LOW_Z, false						},
	{ (word)'{', (word)'{', NULL, A_OPEN_BRACE, false					},
	{ (word)'|', (word)'|', NULL, A_BAR, false							},
	{ (word)'}', (word)'}', NULL, A_CLOSE_BRACE, false					},
	{ (word)'~', (word)'~', NULL, A_TILDE, false						},
	{ 0x7f, 0x7f, "DEL", A_DELETE, false								},

	{ 0x80, 0x80, "EURO", A_EURO, false  								},
	{ 0x81, 0x81, "SHIFT", A_SHIFT2, false								},
	{ 0x82, 0x82, "CTRL", A_CTRL2, false								},
	{ 0x83, 0x83, "ALT", A_ALT2, false									},
	{ 0x84, 0x84, "F5", A_F5, true										},
	{ 0x85, 0x85, "F6", A_F6, true										},
	{ 0x86, 0x86, "F7", A_F7, true										},
	{ 0x87, 0x87, "F8", A_F8, true										},
	{ 0x88, 0x88, "CIRCUMFLEX", A_CIRCUMFLEX, false  					},
	{ 0x89, 0x89, "MWHEELUP", A_MWHEELUP, false							},
	{ 0x8a, 0x9a, NULL, A_CAP_SCARON, false								},	// ******
	{ 0x8b, 0x8b, "MWHEELDOWN", A_MWHEELDOWN, false						},
	{ 0x8c, 0x9c, NULL, A_CAP_OE, false									},	// ******
	{ 0x8d, 0x8d, "MOUSE1", A_MOUSE1, false								},
	{ 0x8e, 0x8e, "MOUSE2", A_MOUSE2, false								},
	{ 0x8f, 0x8f, "INS", A_INSERT, false								},
	{ 0x90, 0x90, "HOME", A_HOME, false									},
	{ 0x91, 0x91, "PGUP", A_PAGE_UP, false								},
	{ 0x92, 0x92, NULL, A_RIGHT_SINGLE_QUOTE, false						},
	{ 0x93, 0x93, NULL, A_LEFT_DOUBLE_QUOTE, false						},
	{ 0x94, 0x94, NULL, A_RIGHT_DOUBLE_QUOTE, false						},
	{ 0x95, 0x95, "F9", A_F9, true										},
	{ 0x96, 0x96, "F10", A_F10, true									},
	{ 0x97, 0x97, "F11", A_F11, true									},
	{ 0x98, 0x98, "F12", A_F12, true									},
	{ 0x99, 0x99, NULL, A_TRADEMARK, false								},
	{ 0x8a, 0x9a, NULL, A_LOW_SCARON, false								},	// ******
	{ 0x9b, 0x9b, "SHIFT_ENTER", A_ENTER, false							},
	{ 0x8c, 0x9c, NULL, A_LOW_OE, false									},	// ******
	{ 0x9d, 0x9d, "END", A_END, false									},
	{ 0x9e, 0x9e, "PGDN", A_PAGE_DOWN, false							},
	{ 0x9f, 0xff, NULL, A_CAP_YDIERESIS, false							},	// ******

	{ 0xa0, 0,	  "SHIFT_SPACE", A_SPACE, false							},
	{ 0xa1, 0xa1, NULL, A_EXCLAMDOWN, false								},	// upside down '!' - undisplayable
	{ L'\u00A2', L'\u00A2', NULL, A_CENT, false	  			}, // cent sign
	{ L'\u00A3', L'\u00A3', NULL, A_POUND, false	  		}, // pound (as in currency) symbol
	{ 0xa4, 0,    "SHIFT_KP_ENTER", A_KP_ENTER, false					},
	{ L'\u00A5', L'\u00A5', NULL, A_YEN, false		  		}, // yen symbol
	{ 0xa6, 0xa6, "MOUSE3", A_MOUSE3, false								},
	{ 0xa7, 0xa7, "MOUSE4", A_MOUSE4, false								},
	{ 0xa8, 0xa8, "MOUSE5", A_MOUSE5, false								},
	{ L'\u00A9', L'\u00A9', NULL, A_COPYRIGHT, false 		}, // copyright symbol
	{ 0xaa, 0xaa, "UPARROW", A_CURSOR_UP, false							},
	{ 0xab, 0xab, "DOWNARROW", A_CURSOR_DOWN, false						},
	{ 0xac, 0xac, "LEFTARROW", A_CURSOR_LEFT, false						},
	{ 0xad, 0xad, "RIGHTARROW", A_CURSOR_RIGHT, false					},
	{ L'\u00AE', L'\u00AE', NULL, A_REGISTERED, false		}, // registered trademark symbol
	{ 0xaf, 0,	  NULL, A_UNDEFINED_7, false							},
	{ 0xb0, 0,	  NULL, A_UNDEFINED_8, false							},
	{ 0xb1, 0,	  NULL, A_UNDEFINED_9, false							},
	{ 0xb2, 0,	  NULL, A_UNDEFINED_10, false							},
	{ 0xb3, 0,	  NULL, A_UNDEFINED_11, false							},
	{ 0xb4, 0,	  NULL, A_UNDEFINED_12, false							},
	{ 0xb5, 0,	  NULL, A_UNDEFINED_13, false							},
	{ 0xb6, 0,	  NULL, A_UNDEFINED_14, false							},
	{ 0xb7, 0,	  NULL, A_UNDEFINED_15, false							},
	{ 0xb8, 0,	  NULL, A_UNDEFINED_16, false							},
	{ 0xb9, 0,	  NULL, A_UNDEFINED_17, false							},
	{ 0xba, 0,	  NULL, A_UNDEFINED_18, false							},
	{ 0xbb, 0,	  NULL, A_UNDEFINED_19, false							},
	{ 0xbc, 0,	  NULL, A_UNDEFINED_20, false							},
	{ 0xbd, 0,	  NULL, A_UNDEFINED_21, false							},
	{ 0xbe, 0,	  NULL, A_UNDEFINED_22, false							},
	{ L'\u00BF', L'\u00BF', NULL, A_QUESTION_DOWN, false	}, // upside-down question mark

	{ L'\u00C0', L'\u00E0', NULL, A_CAP_AGRAVE, false		},
	{ L'\u00C1', L'\u00E1', NULL, A_CAP_AACUTE, false		},
	{ L'\u00C2', L'\u00E2', NULL, A_CAP_ACIRCUMFLEX, false	},
	{ L'\u00C3', L'\u00E3', NULL, A_CAP_ATILDE, false		},
	{ L'\u00C4', L'\u00E4', NULL, A_CAP_ADIERESIS, false	},
	{ L'\u00C5', L'\u00E5', NULL, A_CAP_ARING, false		},
	{ L'\u00C6', L'\u00E6', NULL, A_CAP_AE, false			},
	{ L'\u00C7', L'\u00E7', NULL, A_CAP_CCEDILLA, false		},
	{ L'\u00C8', L'\u00E8', NULL, A_CAP_EGRAVE, false		},
	{ L'\u00C9', L'\u00E9', NULL, A_CAP_EACUTE, false		},
	{ L'\u00CA', L'\u00EA', NULL, A_CAP_ECIRCUMFLEX, false	},
	{ L'\u00CB', L'\u00EB', NULL, A_CAP_EDIERESIS, false	},
	{ L'\u00CC', L'\u00EC', NULL, A_CAP_IGRAVE, false		},
	{ L'\u00CD', L'\u00ED', NULL, A_CAP_IACUTE, false		},
	{ L'\u00CE', L'\u00EE', NULL, A_CAP_ICIRCUMFLEX, false	},
	{ L'\u00CF', L'\u00EF', NULL, A_CAP_IDIERESIS, false	},
	{ L'\u00D0', L'\u00F0', NULL, A_CAP_ETH, false			},
	{ L'\u00D1', L'\u00F1', NULL, A_CAP_NTILDE, false		},
	{ L'\u00D2', L'\u00F2', NULL, A_CAP_OGRAVE, false		},
	{ L'\u00D3', L'\u00F3', NULL, A_CAP_OACUTE, false		},
	{ L'\u00D4', L'\u00F4', NULL, A_CAP_OCIRCUMFLEX, false	},
	{ L'\u00D5', L'\u00F5', NULL, A_CAP_OTILDE, false		},
	{ L'\u00D6', L'\u00F6', NULL, A_CAP_ODIERESIS, false	},
	{ L'\u00D7', L'\u00D7', "KP_STAR", A_MULTIPLY, false 	},
	{ L'\u00D8', L'\u00F8', NULL, A_CAP_OSLASH, false		},
	{ L'\u00D9', L'\u00F9', NULL, A_CAP_UGRAVE, false		},
	{ L'\u00DA', L'\u00FA', NULL, A_CAP_UACUTE, false		},
	{ L'\u00DB', L'\u00FB', NULL, A_CAP_UCIRCUMFLEX, false	},
	{ L'\u00DC', L'\u00FC', NULL, A_CAP_UDIERESIS, false	},
	{ L'\u00DD', L'\u00FD', NULL, A_CAP_YACUTE, false		},
	{ L'\u00DE', L'\u00FE', NULL, A_CAP_THORN, false		},
	{ L'\u00DF', L'\u00DF', NULL, A_GERMANDBLS, false 		},

	{ L'\u00C0', L'\u00E0', NULL, A_LOW_AGRAVE, false		},
	{ L'\u00C1', L'\u00E1', NULL, A_LOW_AACUTE, false		},
	{ L'\u00C2', L'\u00E2', NULL, A_LOW_ACIRCUMFLEX, false	},
	{ L'\u00C3', L'\u00E3', NULL, A_LOW_ATILDE, false		},
	{ L'\u00C4', L'\u00E4', NULL, A_LOW_ADIERESIS, false	},
	{ L'\u00C5', L'\u00E5', NULL, A_LOW_ARING, false		},
	{ L'\u00C6', L'\u00E6', NULL, A_LOW_AE, false			},
	{ L'\u00C7', L'\u00E7', NULL, A_LOW_CCEDILLA, false		},
	{ L'\u00C8', L'\u00E8', NULL, A_LOW_EGRAVE, false		},
	{ L'\u00C9', L'\u00E9', NULL, A_LOW_EACUTE, false		},
	{ L'\u00CA', L'\u00EA', NULL, A_LOW_ECIRCUMFLEX, false	},
	{ L'\u00CB', L'\u00EB', NULL, A_LOW_EDIERESIS, false	},
	{ L'\u00CC', L'\u00EC', NULL, A_LOW_IGRAVE, false		},
	{ L'\u00CD', L'\u00ED', NULL, A_LOW_IACUTE, false		},
	{ L'\u00CE', L'\u00EE', NULL, A_LOW_ICIRCUMFLEX, false	},
	{ L'\u00CF', L'\u00EF', NULL, A_LOW_IDIERESIS, false	},
	{ L'\u00D0', L'\u00F0', NULL, A_LOW_ETH, false			},
	{ L'\u00D1', L'\u00F1', NULL, A_LOW_NTILDE, false		},
	{ L'\u00D2', L'\u00F2', NULL, A_LOW_OGRAVE, false		},
	{ L'\u00D3', L'\u00F3', NULL, A_LOW_OACUTE, false		},
	{ L'\u00D4', L'\u00F4', NULL, A_LOW_OCIRCUMFLEX, false	},
	{ L'\u00D5', L'\u00F5', NULL, A_LOW_OTILDE, false		},
	{ L'\u00D6', L'\u00F6', NULL, A_LOW_ODIERESIS, false	},
	{ L'\u00F7', L'\u00F7', "KP_SLASH", A_DIVIDE, false 	},
	{ L'\u00D8', L'\u00F8', NULL, A_LOW_OSLASH, false		},
	{ L'\u00D9', L'\u00F9', NULL, A_LOW_UGRAVE, false		},
	{ L'\u00DA', L'\u00FA', NULL, A_LOW_UACUTE, false		},
	{ L'\u00DB', L'\u00FB', NULL, A_LOW_UCIRCUMFLEX, false	},
	{ L'\u00DC', L'\u00FC', NULL, A_LOW_UDIERESIS, false	},
	{ L'\u00DD', L'\u00FD', NULL, A_LOW_YACUTE, false		},
	{ L'\u00DE', L'\u00FE', NULL, A_LOW_THORN, false		},
	{ 0x9f, 0xff, NULL, A_LOW_YDIERESIS, false							},	// *******

	{ 0x100, 0x100, "JOY0", A_JOY0, false								},
	{ 0x101, 0x101, "JOY1", A_JOY1, false								},
	{ 0x102, 0x102, "JOY2", A_JOY2, false								},
	{ 0x103, 0x103, "JOY3", A_JOY3, false								},
	{ 0x104, 0x104, "JOY4", A_JOY4, false								},
	{ 0x105, 0x105, "JOY5", A_JOY5, false								},
	{ 0x106, 0x106, "JOY6", A_JOY6, false								},
	{ 0x107, 0x107, "JOY7", A_JOY7, false								},
	{ 0x108, 0x108, "JOY8", A_JOY8, false								},
	{ 0x109, 0x109, "JOY9", A_JOY9, false								},
	{ 0x10a, 0x10a, "JOY10", A_JOY10, false								},
	{ 0x10b, 0x10b, "JOY11", A_JOY11, false								},
	{ 0x10c, 0x10c, "JOY12", A_JOY12, false								},
	{ 0x10d, 0x10d, "JOY13", A_JOY13, false								},
	{ 0x10e, 0x10e, "JOY14", A_JOY14, false								},
	{ 0x10f, 0x10f, "JOY15", A_JOY15, false								},
	{ 0x110, 0x110, "JOY16", A_JOY16, false								},
	{ 0x111, 0x111, "JOY17", A_JOY17, false								},
	{ 0x112, 0x112, "JOY18", A_JOY18, false								},
	{ 0x113, 0x113, "JOY19", A_JOY19, false								},
	{ 0x114, 0x114, "JOY20", A_JOY20, false								},
	{ 0x115, 0x115, "JOY21", A_JOY21, false								},
	{ 0x116, 0x116, "JOY22", A_JOY22, false								},
	{ 0x117, 0x117, "JOY23", A_JOY23, false								},
	{ 0x118, 0x118, "JOY24", A_JOY24, false								},
	{ 0x119, 0x119, "JOY25", A_JOY25, false								},
	{ 0x11a, 0x11a, "JOY26", A_JOY26, false								},
	{ 0x11b, 0x11b, "JOY27", A_JOY27, false								},
	{ 0x11c, 0x11c, "JOY28", A_JOY28, false								},
	{ 0x11d, 0x11d, "JOY29", A_JOY29, false								},
	{ 0x11e, 0x11e, "JOY30", A_JOY30, false								},
	{ 0x11f, 0x11f, "JOY31", A_JOY31, false								},

	{ 0x120, 0x120, "AUX0", A_AUX0, false								},
	{ 0x121, 0x121, "AUX1", A_AUX1, false								},
	{ 0x122, 0x122, "AUX2", A_AUX2, false								},
	{ 0x123, 0x123, "AUX3", A_AUX3, false								},
	{ 0x124, 0x124, "AUX4", A_AUX4, false								},
	{ 0x125, 0x125, "AUX5", A_AUX5, false								},
	{ 0x126, 0x126, "AUX6", A_AUX6, false								},
	{ 0x127, 0x127, "AUX7", A_AUX7, false								},
	{ 0x128, 0x128, "AUX8", A_AUX8, false								},
	{ 0x129, 0x129, "AUX9", A_AUX9, false								},
	{ 0x12a, 0x12a, "AUX10", A_AUX10, false								},
	{ 0x12b, 0x12b, "AUX11", A_AUX11, false								},
	{ 0x12c, 0x12c, "AUX12", A_AUX12, false								},
	{ 0x12d, 0x12d, "AUX13", A_AUX13, false								},
	{ 0x12e, 0x12e, "AUX14", A_AUX14, false								},
	{ 0x12f, 0x12f, "AUX15", A_AUX15, false								},
	{ 0x130, 0x130, "AUX16", A_AUX16, false								},
	{ 0x131, 0x131, "AUX17", A_AUX17, false								},
	{ 0x132, 0x132, "AUX18", A_AUX18, false								},
	{ 0x133, 0x133, "AUX19", A_AUX19, false								},
	{ 0x134, 0x134, "AUX20", A_AUX20, false								},
	{ 0x135, 0x135, "AUX21", A_AUX21, false								},
	{ 0x136, 0x136, "AUX22", A_AUX22, false								},
	{ 0x137, 0x137, "AUX23", A_AUX23, false								},
	{ 0x138, 0x138, "AUX24", A_AUX24, false								},
	{ 0x139, 0x139, "AUX25", A_AUX25, false								},
	{ 0x13a, 0x13a, "AUX26", A_AUX26, false								},
	{ 0x13b, 0x13b, "AUX27", A_AUX27, false								},
	{ 0x13c, 0x13c, "AUX28", A_AUX28, false								},
	{ 0x13d, 0x13d, "AUX29", A_AUX29, false								},
	{ 0x13e, 0x13e, "AUX30", A_AUX30, false								},
	{ 0x13f, 0x13f, "AUX31", A_AUX31, false								}
};
static const size_t numKeynames = ARRAY_LEN( keynames );



/*
=============================================================================

EDIT FIELDS

=============================================================================
*/


/*
===================
Field_Draw

Handles horizontal scrolling and cursor blinking
x, y, amd width are in pixels
===================
*/
void Field_VariableSizeDraw( field_t *edit, int x, int y, int width, int size, qboolean showCursor, qboolean noColorEscape ) {
	int		len;
	int		drawLen;
	int		prestep;
	int		cursorChar;
	char	str[MAX_STRING_CHARS];
	int		i;

	drawLen = edit->widthInChars - 1; // - 1 so there is always a space for the cursor
	len = strlen( edit->buffer );

	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	if ( drawLen < 0 )
		return;

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		Com_Error( ERR_DROP, "drawLen >= MAX_STRING_CHARS" );
	}

	Com_Memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	// draw it
	if ( size == SMALLCHAR_WIDTH ) {
		float	color[4];

		color[0] = color[1] = color[2] = color[3] = 1.0;
		SCR_DrawSmallStringExt( x, y, str, color, qfalse, noColorEscape );
	} else {
		// draw big string with drop shadow
		SCR_DrawBigString( x, y, str, 1.0, noColorEscape );
	}

	// draw the cursor
	if ( showCursor ) {
		if ( (int)( cls.realtime >> 8 ) & 1 ) {
			return;		// off blink
		}

		if ( kg.key_overstrikeMode ) {
			cursorChar = 11;
		} else {
			cursorChar = 10;
		}

		i = drawLen - strlen( str );

		if ( size == SMALLCHAR_WIDTH ) {
			SCR_DrawSmallChar( x + ( edit->cursor - prestep - i ) * size, y, cursorChar );
		} else {
			str[0] = cursorChar;
			str[1] = 0;
			SCR_DrawBigString( x + ( edit->cursor - prestep - i ) * size, y, str, 1.0, qfalse );
		}
	}
}

void Field_Draw( field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape )
{
	Field_VariableSizeDraw( edit, x, y, width, SMALLCHAR_WIDTH, showCursor, noColorEscape );
}

void Field_BigDraw( field_t *edit, int x, int y, int width, qboolean showCursor, qboolean noColorEscape )
{
	Field_VariableSizeDraw( edit, x, y, width, BIGCHAR_WIDTH, showCursor, noColorEscape );
}

/*
================
Field_Paste
================
*/
void Field_Paste( field_t *edit ) {
	char	*cbd, *c;

	c = cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		return;
	}

	// send as if typed, so insert / overstrike works properly
	while( *c )
	{
		uint32_t utf32 = ConvertUTF8ToUTF32( c, &c );
		Field_CharEvent( edit, ConvertUTF32ToExpectedCharset( utf32 ) );
	}

	Z_Free( cbd );
}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent( field_t *edit, int key ) {
	int		len;

	// shift-insert is paste
	if ( ( ( key == A_INSERT ) || ( key == A_KP_0 ) ) && kg.keys[A_SHIFT].down ) {
		Field_Paste( edit );
		return;
	}

	key = tolower( key );
	len = strlen( edit->buffer );

	switch ( key ) {
		case A_DELETE:
			if ( edit->cursor < len ) {
				memmove( edit->buffer + edit->cursor,
					edit->buffer + edit->cursor + 1, len - edit->cursor );
			}
			break;

		case A_CURSOR_RIGHT:
			if ( edit->cursor < len ) {
				edit->cursor++;
			}
			break;

		case A_CURSOR_LEFT:
			if ( edit->cursor > 0 ) {
				edit->cursor--;
			}
			break;

		case A_HOME:
			edit->cursor = 0;
			break;

		case A_END:
			edit->cursor = len;
			break;

		case A_INSERT:
			kg.key_overstrikeMode = (qboolean)!kg.key_overstrikeMode;
			break;

		default:
			break;
 	}

	// Change scroll if cursor is no longer visible
	if ( edit->cursor < edit->scroll ) {
		edit->scroll = edit->cursor;
	} else if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len ) {
		edit->scroll = edit->cursor - edit->widthInChars + 1;
 	}
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent( field_t *edit, int ch ) {
	int		len;

	if ( ch == 'v' - 'a' + 1 ) {	// ctrl-v is paste
		Field_Paste( edit );
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {	// ctrl-c clears the field
		Field_Clear( edit );
		return;
	}

	len = strlen( edit->buffer );

	if ( ch == 'h' - 'a' + 1 )	{	// ctrl-h is backspace
		if ( edit->cursor > 0 ) {
			memmove( edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor, len + 1 - edit->cursor );
			edit->cursor--;
			if ( edit->cursor < edit->scroll )
			{
				edit->scroll--;
			}
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {	// ctrl-a is home
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {	// ctrl-e is end
		edit->cursor = len;
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch < 32 ) {
		return;
	}

	if ( kg.key_overstrikeMode ) {
		// - 2 to leave room for the leading slash and trailing \0
		if ( edit->cursor == MAX_EDIT_LINE - 2 )
			return;
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	} else {	// insert mode
		// - 2 to leave room for the leading slash and trailing \0
		if ( len == MAX_EDIT_LINE - 2 ) {
			return; // all full
		}
		memmove( edit->buffer + edit->cursor + 1,
			edit->buffer + edit->cursor, len + 1 - edit->cursor );
		edit->buffer[edit->cursor] = ch;
		edit->cursor++;
	}

	if ( edit->cursor >= edit->widthInChars ) {
		edit->scroll++;
	}

	if ( edit->cursor == len + 1) {
		edit->buffer[edit->cursor] = 0;
	}
}

/*
====================
Console_Key

Handles history and console scrollback
====================
*/
void Console_Key( int key ) {
	// ctrl-L clears screen
	if ( keynames[key].lower == 'l' && kg.keys[A_CTRL].down ) {
		Cbuf_AddText( "clear\n" );
		return;
	}

	// enter finishes the line
	if ( key == A_ENTER || key == A_KP_ENTER ) {
		// legacy hack: strip any prepended slashes. they're not necessary anymore
		if ( g_consoleField.buffer[0] &&
			(g_consoleField.buffer[0] == '\\' || g_consoleField.buffer[0] == '/') ) {
			char temp[MAX_EDIT_LINE-1];

			Q_strncpyz( temp, g_consoleField.buffer+1, sizeof( temp ) );
			Com_sprintf( g_consoleField.buffer, sizeof( g_consoleField.buffer ), "%s", temp );
			g_consoleField.cursor--;
		}
	//	else
	//		Field_AutoComplete( &g_consoleField );

		// print executed command
		Com_Printf( "%c%s\n", CONSOLE_PROMPT_CHAR, g_consoleField.buffer );

		// check if cgame wants to eat the command...?
		if ( cls.cgameStarted && cl.mSharedMemory ) {
			TCGIncomingConsoleCommand *icc = (TCGIncomingConsoleCommand *)cl.mSharedMemory;

			Q_strncpyz( icc->conCommand, g_consoleField.buffer, sizeof( icc->conCommand ) );

			if ( CGVM_IncomingConsoleCommand() ) {
				// valid command
				Cbuf_AddText( g_consoleField.buffer );
				Cbuf_AddText( "\n" );
			}
			else if ( icc->conCommand[0] ) {
				// cgame ate it and substituted their own
				Cbuf_AddText( icc->conCommand );
				Cbuf_AddText( "\n" );
			}
		}
		else {
			// cgame didn't eat it, execute it
			Cbuf_AddText( g_consoleField.buffer );
			Cbuf_AddText( "\n" );
		}

		if (!g_consoleField.buffer[0])
		{
			return; // empty lines just scroll the console without adding to history
		}

		// copy line to history buffer
		historyEditLines[nextHistoryLine % COMMAND_HISTORY] = g_consoleField;
		nextHistoryLine++;
		historyLine = nextHistoryLine;

		Field_Clear( &g_consoleField );

		g_consoleField.widthInChars = g_console_field_width;

	//	CL_SaveConsoleHistory();

		if ( cls.state == CA_DISCONNECTED )
			SCR_UpdateScreen ();	// force an update, because the command may take some time

		return;
	}

	// tab completion
	if ( key == A_TAB ) {
		Field_AutoComplete( &g_consoleField );
		return;
	}

	// history scrolling
	if ( key == A_CURSOR_UP || key == A_KP_8
		|| (kg.keys[A_SHIFT].down && key == A_MWHEELUP)
		|| (kg.keys[A_CTRL].down && keynames[key].lower == 'p') )
	{// scroll up: arrow-up, numpad-up, shift + mwheelup, ctrl + p
		if ( nextHistoryLine - historyLine < COMMAND_HISTORY && historyLine > 0 )
			historyLine--;
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];

		return;
	}

	if ( key == A_CURSOR_DOWN || key == A_KP_2
		|| (kg.keys[A_SHIFT].down && key == A_MWHEELDOWN)
		|| (kg.keys[A_CTRL].down && keynames[key].lower == 'n') )
	{// scroll down: arrow-down, numpad-down, shift + mwheeldown, ctrl + n
		historyLine++;
		if (historyLine >= nextHistoryLine) {
			historyLine = nextHistoryLine;
			Field_Clear( &g_consoleField );
			g_consoleField.widthInChars = g_console_field_width;
			return;
		}
		g_consoleField = historyEditLines[historyLine % COMMAND_HISTORY];
		return;
	}

	// console scrolling (ctrl to scroll fast)
	if ( key == A_PAGE_UP || key == A_MWHEELUP ) {
		int count = kg.keys[A_CTRL].down ? 5 : 1;
		for ( int i=0; i<count; i++ )
			Con_PageUp();
		return;
	}

	if ( key == A_PAGE_DOWN || key == A_MWHEELDOWN ) {
		int count = kg.keys[A_CTRL].down ? 5 : 1;
		for ( int i=0; i<count; i++ )
			Con_PageDown();
		return;
	}

	// ctrl-home = top of console
	if ( key == A_HOME && kg.keys[A_CTRL].down ) {
		Con_Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( key == A_END && kg.keys[A_CTRL].down ) {
		Con_Bottom();
		return;
	}

	// pass to the normal editline routine
	Field_KeyDownEvent( &g_consoleField, key );
}

//============================================================================


/*
================
Message_Key

In game talk message
================
*/
void Message_Key( int key ) {
	char buffer[MAX_STRING_CHARS] = {0};

	if ( key == A_ESCAPE ) {
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		Field_Clear( &chatField );
		return;
	}

	if ( key == A_ENTER || key == A_KP_ENTER ) {
		if ( chatField.buffer[0] && cls.state == CA_ACTIVE ) {
				 if ( chat_playerNum != -1 )	Com_sprintf( buffer, sizeof( buffer ), "tell %i \"%s\"\n", chat_playerNum, chatField.buffer );
			else if ( chat_team )				Com_sprintf( buffer, sizeof( buffer ), "say_team \"%s\"\n", chatField.buffer );
			else								Com_sprintf( buffer, sizeof( buffer ), "say \"%s\"\n", chatField.buffer );

			CL_AddReliableCommand( buffer, qfalse );
		}
		Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_MESSAGE );
		Field_Clear( &chatField );
		return;
	}

	Field_KeyDownEvent( &chatField, key );
}

//============================================================================


qboolean Key_GetOverstrikeMode( void ) {
	return kg.key_overstrikeMode;
}


void Key_SetOverstrikeMode( qboolean state ) {
	kg.key_overstrikeMode = state;
}


/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum ) {
	if ( keynum < 0 || keynum >= MAX_KEYS )
		return qfalse;

	return kg.keys[keynames[keynum].upper].down;
}


/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers
to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( char *str ) {
	if ( !VALIDSTRING( str ) )
		return -1;

	// If single char bind, presume ascii char bind
	if ( !str[1] )
		return keynames[(unsigned char)str[0]].upper;

	// scan for a text match
	for ( int i=0; i<MAX_KEYS; i++ ) {
		if ( keynames[i].name && !Q_stricmp( str, keynames[i].name ) )
			return keynames[i].keynum;
	}

	// check for hex code
	if ( strlen( str ) == 4 ) {
		int n = Com_HexStrToInt( str );

		if ( n >= 0 )
			return n;
	}

	return -1;
}

static char tinyString[16];
static const char *Key_KeynumValid( int keynum ) {
	if ( keynum == -1 )
		return "<KEY NOT FOUND>";
	if ( keynum < 0 || keynum >= MAX_KEYS )
		return "<OUT OF RANGE>";
	return NULL;
}

static const char *Key_KeyToName( int keynum ) {
	return keynames[keynum].name;
}


static const char *Key_KeyToAscii( int keynum ) {
	if ( !keynames[keynum].lower )
		return NULL;

		 if ( keynum == A_SPACE )		tinyString[0] = (char)A_SHIFT_SPACE;
	else if ( keynum == A_ENTER )		tinyString[0] = (char)A_SHIFT_ENTER;
	else if ( keynum == A_KP_ENTER )	tinyString[0] = (char)A_SHIFT_KP_ENTER;
	else								tinyString[0] = keynames[keynum].upper;

	tinyString[1] = '\0';
	return tinyString;
}

static const char *Key_KeyToHex( int keynum ) {
	int i = keynum >> 4;
	int j = keynum & 15;

	tinyString[0] = '0';
	tinyString[1] = 'x';
	tinyString[2] = i > 9 ? (i - 10 + 'A') : (i + '0');
	tinyString[3] = j > 9 ? (j - 10 + 'A') : (j + '0');
	tinyString[4] = '\0';

	return tinyString;
}

// Returns the ascii code of the keynum
const char *Key_KeynumToAscii( int keynum ) {
	const char *name = Key_KeynumValid(keynum);

	// check for printable ascii
	if ( !name && keynum > 0 && keynum < 256 )
		name = Key_KeyToAscii( keynum );

	// Check for name (for JOYx and AUXx buttons)
	if ( !name )
		name = Key_KeyToName( keynum );

	// Fallback to hex number
	if ( !name )
		name = Key_KeyToHex( keynum );

	return name;
}


/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
// Returns a console/config file friendly name for the key
const char *Key_KeynumToString( int keynum ) {
	const char *name;

	name = Key_KeynumValid( keynum );

	// Check for friendly name
	if ( !name )
		name = Key_KeyToName( keynum );

	// check for printable ascii
	if ( !name && keynum > 0 && keynum < 256)
		name = Key_KeyToAscii( keynum );

	// Fallback to hex number
	if ( !name )
		name = Key_KeyToHex( keynum );

	return name;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( int keynum, const char *binding ) {
	if ( keynum < 0 || keynum >= MAX_KEYS )
		return;

	// free old bindings
	if ( kg.keys[keynames[keynum].upper].binding ) {
		Z_Free( kg.keys[keynames[keynum].upper].binding );
		kg.keys[keynames[keynum].upper].binding = NULL;
	}

	// allocate memory for new binding
	if ( binding )
		kg.keys[keynames[keynum].upper].binding = CopyString( binding );

	// consider this like modifying an archived cvar, so the
	// file write will be triggered at the next oportunity
	cvar_modifiedFlags |= CVAR_ARCHIVE;
}

/*
===================
Key_GetBinding
===================
*/
char *Key_GetBinding( int keynum ) {
	if ( keynum < 0 || keynum >= MAX_KEYS )
		return "";

	return kg.keys[keynum].binding;
}

/*
===================
Key_GetKey
===================
*/
int Key_GetKey( const char *binding ) {
	if ( binding ) {
		for ( int i=0; i<MAX_KEYS; i++ ) {
			if ( kg.keys[i].binding && !Q_stricmp( binding, kg.keys[i].binding ) )
				return i;
		}
	}

	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void ) {
	if ( Cmd_Argc() != 2 ) {
		Com_Printf( "unbind <key> : remove commands from a key\n" );
		return;
	}

	int b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, "" );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void ) {
	for ( int i=0; i<MAX_KEYS; i++ ) {
		if ( kg.keys[i].binding )
			Key_SetBinding( i, "" );
	}
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void ) {
	int c = Cmd_Argc();

	if ( c < 2 ) {
		Com_Printf( "bind <key> [command] : attach a command to a key\n" );
		return;
	}

	int b = Key_StringToKeynum( Cmd_Argv( 1 ) );
	if ( b == -1 ) {
		Com_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	if ( c == 2 ) {
		if ( kg.keys[b].binding && kg.keys[b].binding[0] )
			Com_Printf( S_COLOR_GREY "Bind " S_COLOR_WHITE "%s = " S_COLOR_GREY "\"" S_COLOR_WHITE "%s" S_COLOR_GREY "\"" S_COLOR_WHITE "\n", Key_KeynumToString( b ), kg.keys[b].binding );
		else
			Com_Printf( "\"%s\" is not bound\n", Key_KeynumToString( b ) );
		return;
	}

	Key_SetBinding( b, Cmd_ArgsFrom( 2 ) );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f ) {
	FS_Printf( f, "unbindall\n" );
	for ( size_t i=0; i<MAX_KEYS; i++ ) {
		if ( kg.keys[i].binding && kg.keys[i].binding[0] ) {
			const char *name = Key_KeynumToString( i );

			// handle the escape character nicely
			if ( !strcmp( name, "\\" ) )
				FS_Printf( f, "bind \"\\\" \"%s\"\n", kg.keys[i].binding );
			else
				FS_Printf( f, "bind \"%s\" \"%s\"\n", name, kg.keys[i].binding );
		}
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void ) {
	for ( size_t i=0; i<MAX_KEYS; i++ ) {
		if ( kg.keys[i].binding && kg.keys[i].binding[0] )
			Com_Printf( S_COLOR_GREY "Key " S_COLOR_WHITE "%s (%s) = " S_COLOR_GREY "\"" S_COLOR_WHITE "%s" S_COLOR_GREY "\"" S_COLOR_WHITE "\n", Key_KeynumToAscii( i ), Key_KeynumToString( i ), kg.keys[i].binding );
	}
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( callbackFunc_t callback ) {
	for ( size_t i=0; i<numKeynames; i++ ) {
		if ( keynames[i].name )
			callback( keynames[i].name );
	}
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( char *args, int argNum ) {
	if ( argNum == 2 ) {
		// Skip "unbind "
		char *p = Com_SkipTokens( args, 1, " " );
		if ( p > args )
			Field_CompleteKeyname();
	}
}

/*
====================
Key_CompleteBind
====================
*/
static void Key_CompleteBind( char *args, int argNum ) {
	char *p;

	if ( argNum == 2 ) {
		// Skip "bind "
		p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
			Field_CompleteKeyname();
	}
	else if ( argNum >= 3 ) {
		// Skip "bind <key> "
		p = Com_SkipTokens( args, 2, " " );

		if ( p > args )
			Field_CompleteCommand( p, qtrue, qtrue );
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands( void ) {
	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f, "Bind a key to a console command" );
	Cmd_SetCommandCompletionFunc( "bind", Key_CompleteBind );
	Cmd_AddCommand( "unbind", Key_Unbind_f, "Unbind a key" );
	Cmd_SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f, "Delete all key bindings" );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f, "Show all bindings in the console" );
}

/*
===================
CL_BindUICommand

Returns qtrue if bind command should be executed while user interface is shown
===================
*/
static qboolean CL_BindUICommand( const char *cmd ) {
	if ( Key_GetCatcher( ) & KEYCATCH_CONSOLE )
		return qfalse;

	if ( !Q_stricmp( cmd, "toggleconsole" ) )
		return qtrue;
	if ( !Q_stricmp( cmd, "togglemenu" ) )
		return qtrue;

	return qfalse;
}

/*
===================
CL_ParseBinding

Execute the commands in the bind string
===================
*/
void CL_ParseBinding( int key, qboolean down, unsigned time )
{
	char buf[ MAX_STRING_CHARS ], *p = buf, *end;
	qboolean allCommands, allowUpCmds;

	if( cls.state == CA_DISCONNECTED && Key_GetCatcher( ) == 0 )
		return;
	if( !kg.keys[keynames[key].upper].binding || !kg.keys[keynames[key].upper].binding[0] )
		return;
	Q_strncpyz( buf, kg.keys[keynames[key].upper].binding, sizeof( buf ) );

	// run all bind commands if console, ui, etc aren't reading keys
	allCommands = (qboolean)( Key_GetCatcher( ) == 0 );

	// allow button up commands if in game even if key catcher is set
	allowUpCmds = (qboolean)( cls.state != CA_DISCONNECTED );

	while( 1 )
	{
		while( isspace( *p ) )
			p++;
		end = strchr( p, ';' );
		if( end )
			*end = '\0';
		if( *p == '+' )
		{
			// button commands add keynum and time as parameters
			// so that multiple sources can be discriminated and
			// subframe corrected
			if ( allCommands || ( allowUpCmds && !down ) ) {
				char cmd[1024];
				Com_sprintf( cmd, sizeof( cmd ), "%c%s %d %d\n",
					( down ) ? '+' : '-', p + 1, key, time );
				Cbuf_AddText( cmd );
			}
		}
		else if( down )
		{
			// normal commands only execute on key press
			if ( allCommands || CL_BindUICommand( p ) ) {
				// down-only command
				if ( cls.cgameStarted && cl.mSharedMemory ) {
					// don't do this unless cgame is inited and shared memory is valid
					TCGIncomingConsoleCommand *icc = (TCGIncomingConsoleCommand *)cl.mSharedMemory;

					Q_strncpyz( icc->conCommand, p, sizeof(icc->conCommand) );

					if ( CGVM_IncomingConsoleCommand() ) {
						//rww - let mod authors filter client console messages so they can cut them off if they want.
						Cbuf_AddText( p );
						Cbuf_AddText( "\n" );
					}
					else if ( icc->conCommand[0] ) {
						//the vm call says to execute this command in place
						Cbuf_AddText( icc->conCommand );
						Cbuf_AddText( "\n" );
					}
				}
				else {
					//otherwise just add it
					Cbuf_AddText( p );
					Cbuf_AddText( "\n" );
				}
			}
		}
		if( !end )
			break;
		p = end + 1;
	}
}

/*
===================
CL_KeyDownEvent

Called by CL_KeyEvent to handle a keypress
===================
*/
void CL_KeyDownEvent( int key, unsigned time )
{
	kg.keys[keynames[key].upper].down = qtrue;
	kg.keys[keynames[key].upper].repeats++;
	if( kg.keys[keynames[key].upper].repeats == 1 ) {
		kg.keyDownCount++;
		kg.anykeydown = qtrue;
	}

	if ( cl_allowAltEnter->integer && kg.keys[A_ALT].down && key == A_ENTER )
	{
		Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue( "r_fullscreen" ) );
		return;
	}

	// console key is hardcoded, so the user can never unbind it
	if ( key == A_CONSOLE || (kg.keys[A_SHIFT].down && key == A_ESCAPE) ) {
		Con_ToggleConsole_f();
		Key_ClearStates ();
		return;
	}

	// keys can still be used for bound actions
	if ( cls.state == CA_CINEMATIC && !Key_GetCatcher() ) {
		if ( !com_cameraMode->integer ) {
			Cvar_Set( "nextdemo", "" );
			key = A_ESCAPE;
		}
	}

	// escape is always handled special
	if ( key == A_ESCAPE ) {
		if ( !kg.keys[A_SHIFT].down && ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) ) {
			Con_ToggleConsole_f();
			Key_ClearStates();
			return;
		}

		if ( Key_GetCatcher() & KEYCATCH_MESSAGE ) {
			// clear message mode
			Message_Key( key );
			return;
		}

		// escape always gets out of CGAME stuff
		if ( Key_GetCatcher() & KEYCATCH_CGAME ) {
			Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CGAME );
			CGVM_EventHandling( CGAME_EVENT_NONE );
			return;
		}

		if ( !(Key_GetCatcher() & KEYCATCH_UI) ) {
			if ( cls.state == CA_ACTIVE && !clc.demoplaying )
				UIVM_SetActiveMenu( UIMENU_INGAME );
			else {
				CL_Disconnect_f();
				S_StopAllSounds();
				UIVM_SetActiveMenu( UIMENU_MAIN );
			}
			return;
		}

		UIVM_KeyEvent( key, qtrue );
		return;
	}

	// send the bound action
	CL_ParseBinding( key, qtrue, time );

	// distribute the key down event to the appropriate handler
	// console
	if ( Key_GetCatcher() & KEYCATCH_CONSOLE )
		Console_Key( key );
	// ui
	else if ( Key_GetCatcher() & KEYCATCH_UI ) {
		if ( cls.uiStarted )
			UIVM_KeyEvent( key, qtrue );
	}
	// cgame
	else if ( Key_GetCatcher() & KEYCATCH_CGAME ) {
		if ( cls.cgameStarted )
			CGVM_KeyEvent( key, qtrue );
	}
	// chatbox
	else if ( Key_GetCatcher() & KEYCATCH_MESSAGE )
		Message_Key( key );
	// console
	else if ( cls.state == CA_DISCONNECTED )
		Console_Key( key );
}

/*
===================
CL_KeyUpEvent

Called by CL_KeyEvent to handle a keyrelease
===================
*/
void CL_KeyUpEvent( int key, unsigned time )
{
	kg.keys[keynames[key].upper].repeats = 0;
	kg.keys[keynames[key].upper].down = qfalse;
	kg.keyDownCount--;

	if (kg.keyDownCount <= 0) {
		kg.anykeydown = qfalse;
		kg.keyDownCount = 0;
	}

	// don't process key-up events for the console key
	if ( key == A_CONSOLE || ( key == A_ESCAPE && kg.keys[A_SHIFT].down ) )
		return;

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	CL_ParseBinding( key, qfalse, time );

	if ( Key_GetCatcher( ) & KEYCATCH_UI && cls.uiStarted )
		UIVM_KeyEvent( key, qfalse );
	else if ( Key_GetCatcher( ) & KEYCATCH_CGAME && cls.cgameStarted )
		CGVM_KeyEvent( key, qfalse );
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
void CL_KeyEvent (int key, qboolean down, unsigned time) {
	if( down )
		CL_KeyDownEvent( key, time );
	else
		CL_KeyUpEvent( key, time );
}

/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int key ) {
	// delete is not a printable character and is otherwise handled by Field_KeyDownEvent
	if ( key == 127 )
		return;

	// distribute the key down event to the appropriate handler
		 if ( Key_GetCatcher() & KEYCATCH_CONSOLE )		Field_CharEvent( &g_consoleField, key );
	else if ( Key_GetCatcher() & KEYCATCH_UI )			UIVM_KeyEvent( key|K_CHAR_FLAG, qtrue );
	else if ( Key_GetCatcher() & KEYCATCH_CGAME )		CGVM_KeyEvent( key|K_CHAR_FLAG, qtrue );
	else if ( Key_GetCatcher() & KEYCATCH_MESSAGE )		Field_CharEvent( &chatField, key );
	else if ( cls.state == CA_DISCONNECTED )			Field_CharEvent( &g_consoleField, key );
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void ) {
	kg.anykeydown = qfalse;

	for ( int i=0; i<MAX_KEYS; i++ ) {
		if ( kg.keys[i].down )
			CL_KeyEvent( i, qfalse, 0 );
		kg.keys[i].down = qfalse;
		kg.keys[i].repeats = 0;
	}
}

static int keyCatchers = 0;

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher( void ) {
	return keyCatchers;
}

/*
====================
Key_SetCatcher
====================
*/
void Key_SetCatcher( int catcher ) {
	// If the catcher state is changing, clear all key states
	if ( catcher != keyCatchers )
		Key_ClearStates();

	keyCatchers = catcher;
}
