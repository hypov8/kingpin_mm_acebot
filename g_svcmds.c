
#include "g_local.h"

// BEGIN HITMEN
#if 0
#include "stdlog.h"    // StdLog
#include "gslog.h"    // StdLog
#endif
// END

void	Svcmd_Test_f (void)
{
	safe_cprintf(NULL, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

sv addip <ip>
sv removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

sv listip
Prints the current list of filters.

sv writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

typedef struct
{
	unsigned	mask;
	unsigned	compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int			numipfilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			safe_cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", s);
			return false;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;
	
	return true;
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket (char *from)
{
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}
	
	in = *(unsigned *)m;

	for (i=0 ; i<numipfilters ; i++)
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
			return (int)filterban->value;

	return (int)!filterban->value;
}


/*
=================
SV_AddIP_f
=================
*/
void SVCmd_AddIP_f (void)
{
	int		i;
	
	if (gi.argc() < 3) {
		safe_cprintf(NULL, PRINT_HIGH, "Usage:  addip <ip-mask>\n");
		return;
	}

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			safe_cprintf(NULL, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}
	
	if (!StringToFilter (gi.argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void SVCmd_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (gi.argc() < 3) {
		safe_cprintf(NULL, PRINT_HIGH, "Usage:  sv removeip <ip-mask>\n");
		return;
	}

	if (!StringToFilter (gi.argv(2), &f))
		return;

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].mask == f.mask
		&& ipfilters[i].compare == f.compare)
		{
			for (j=i+1 ; j<numipfilters ; j++)
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			safe_cprintf(NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	safe_cprintf(NULL, PRINT_HIGH, "Didn't find %s.\n", gi.argv(2));
}

/*
=================
SV_ListIP_f
=================
*/
void SVCmd_ListIP_f (void)
{
	int		i;
	byte	b[4];

	safe_cprintf(NULL, PRINT_HIGH, "Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		safe_cprintf(NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SVCmd_WriteIP_f (void)
{
	FILE	*f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;
	cvar_t	*game;

	game = gi.cvar("game", "", 0);

	if (!*game->string)
		strcpy (name, "main"DIR_SLASH"listip.cfg");
	else
		sprintf (name, "%s"DIR_SLASH"listip.cfg", game->string);

	safe_cprintf(NULL, PRINT_HIGH, "Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f)
	{
		safe_cprintf(NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}
	
	fprintf(f, "set filterban %d\n", (int)filterban->value);

	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		fprintf (f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	fclose (f);
}

// ACEBOT_ADD
void SVCmd_BotDebug(void)
{
	int		i;
	edict_t	*doot;
	//char string[10];

	if (debug_mode == false)
	{
		safe_bprintf(PRINT_MEDIUM, "ACE: Debug Mode On\n");
		debug_mode = true;

		for_each_player_not_bot(doot, i)
		{
			//if (doot->acebot.is_bot)
				//continue;
			//sv botdebug on
			//safe_cprintf(doot, PRINT_MEDIUM, "0=MOVE 1=LADDER 2=PLATFORM 3=TELEPORTER 4=ITEM 5=WATER 7=JUMP\n");
			//=======================================================
			safe_cprintf(doot, PRINT_MEDIUM, " \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†††††††††††††††††††††ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†===================†ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†  *KEYS REBOUND*   †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 5 = WATER     †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 6 = LADDER    †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 7 = JUMP      †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 8 = findnode  †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 9 = movenode  †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ† KEY 0 = MOVE      †ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†===================†ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒ†††††††††††††††††††††ƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, "ƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒƒ \n");
			safe_cprintf(doot, PRINT_MEDIUM, " \n");

			
		//	string[0] = 'a' + 'z'; 
		//	string[1] = 'b' + 'z';
			//string[2] = 'c' + 'z';
			//string[3] = 'd' + 'z';
			//string[4] = 'e' + 'z';
			//string[0] = 'a' + (char)127;
			//string[1] = 'a' + (char)127;
			//string[2] = 'a' + (char)128;
			//string[3] = 'a' + (char)129;
			//string[4] = (char)122;

			//string[5] = (char)122;
			//string[6] = (char)122;



			//safe_cprintf(doot, PRINT_MEDIUM, "-=( %s )=- \n", string);

//#define	PRINT_LOW			0		// pickup messages
//#define	PRINT_MEDIUM		1		// death messages
//#define	PRINT_HIGH			2		// critical messages
//#define	PRINT_CHAT			3		// chat messages
			//=======================================================	

			gi.WriteByte(13);
			gi.WriteString("bind 0 addnode 0;bind 5 addnode 5;bind 6 addnode 1; bind 7 addnode 7; bind 8 findnode; bind 9 movenode 999\n");
			gi.unicast(doot, true);
		}
	}
	else
	{
		safe_bprintf(PRINT_MEDIUM, "ACE: Debug Mode Off\n");
		debug_mode = false;
	}
}

void SVCmd_BotAdd(char *cmd2, char *cmd3, char *cmd4)

{
	/* bots need to be added between game start and end. less issues and for bot saves */
	if (level.modeset == TEAM_MATCH_RUNNING || level.modeset == DM_MATCH_RUNNING)
	{
		if (teamplay->value) // name, skin, team 
			ACESP_SpawnBot(cmd4, cmd2, cmd3, NULL); //sv addbot thugBot "male_thug/009 031 031" dragon
		else // name, skin			
			ACESP_SpawnBot("\0", cmd2, cmd3, NULL); //sv addbot thugBot "male_thug/009 031 031"
	}
}

// ACEBOT_END

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void	ServerCommand(void)
{
	char	*cmd; 
	char	*cmd2, *cmd3, *cmd4;// ACEBOT_ADD

	cmd = gi.argv(1);
// ACEBOT_ADD
	cmd2 = gi.argv(2);
	cmd3 = gi.argv(3);
	cmd4 = gi.argv(4);
// ACEBOT_END
	if (Q_stricmp(cmd, "test") == 0)
		Svcmd_Test_f();
	else if (Q_stricmp(cmd, "addip") == 0)
		SVCmd_AddIP_f();
	else if (Q_stricmp(cmd, "removeip") == 0)
		SVCmd_RemoveIP_f();
	else if (Q_stricmp(cmd, "listip") == 0)
		SVCmd_ListIP_f();
	else if (Q_stricmp(cmd, "writeip") == 0)
		SVCmd_WriteIP_f();

	// BEGIN HITMEN
#if 0
	else if (Q_stricmp (cmd, "log") == 0)  // Start the logging (if not active).
		{
		gi.cvar_set("stdlogfile", "1");	// force stdlogfile cvar on	
		//sl_GameStart( &gi, level );	// StdLog - Mark Davies
		}
	else if (Q_stricmp (cmd, "nolog") == 0)  // Terminates the log (if active).
		{
		gi.cvar_set("stdlogfile", "0");	// force stdlogfile cvar off	
		//sl_GameEnd( &gi, level );	// StdLog - Mark Davies
		}
#endif
	// END

// ACEBOT_ADD
	else if (Q_stricmp(cmd, "acedebug") == 0 || Q_stricmp(cmd, "botdebug") == 0 || Q_stricmp(cmd, "debugbot") == 0)
		SVCmd_BotDebug();
	else if (Q_stricmp(cmd, "addbot") == 0)
		SVCmd_BotAdd(cmd2, cmd3, cmd4);
	else if (Q_stricmp(cmd, "removebot") == 0 || Q_stricmp(cmd, "removebots") == 0)
			ACESP_RemoveBot(cmd2);
	else if (Q_stricmp(cmd, "savenodes") == 0 || Q_stricmp(cmd, "savenode") == 0)
		ACEND_SaveNodes();
// ACEBOT_END

	else if (!Q_stricmp(cmd, "banip"))
		Cmd_BanDicks_f(NULL, 1);
	else if (!Q_stricmp(cmd, "banname"))
		Cmd_BanDicks_f(NULL, 0);
    else
		safe_cprintf(NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}

