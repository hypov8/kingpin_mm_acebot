///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//	
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the 
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for 
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//  
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////
	
///////////////////////////////////////////////////////////////////////
//
//  acebot_cmds.c - Main internal command processor
//
///////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "acebot.h"

qboolean debug_mode=false;
qboolean debug_mode_origin_ents = false; //local node

///////////////////////////////////////////////////////////////////////
// Special command processor
///////////////////////////////////////////////////////////////////////
qboolean ACECM_Commands(edict_t *ent)
{
	char	*cmd;
	//char *arg1;
	short int node;

	if (!debug_mode)
		return false;

	cmd = gi.argv(0);

	if(Q_stricmp (cmd, "addnode") == 0)
		ent->acebot.last_node = ACEND_AddNode(ent,atoi(gi.argv(1))); 
	
	else if(Q_stricmp (cmd, "removelink") == 0)
		ACEND_RemoveNodeEdge(ent,atoi(gi.argv(1)), atoi(gi.argv(2)));

	else if(Q_stricmp (cmd, "addlink") == 0)
		ACEND_UpdateNodeEdge(atoi(gi.argv(1)), atoi(gi.argv(2)),false);
	
	else if(Q_stricmp (cmd, "showpath") == 0)
    	ACEND_ShowPath(ent,atoi(gi.argv(1)));

	else if (Q_stricmp(cmd, "localnode") == 0) //hypov8 add. show nodes close by
	{
		if (!dedicated->value)
		{
			//arg1 = gi.argv(1);
			//if (Q_stricmp(arg1, "on") == 0)
			if (debug_mode_origin_ents == 0)
			{
				safe_cprintf(ent, PRINT_MEDIUM, "findlocalnode ON\n");
				debug_mode_origin_ents = 1;
			}
			else
			{
				safe_cprintf(ent, PRINT_MEDIUM, "findlocalnode OFF\n");
				debug_mode_origin_ents = 0;
			}
		}
	}
	else if(Q_stricmp (cmd, "findnode") == 0)
	{
		char strWrite[MAX_INFO_STRING];

		node = ACEND_FindClosestReachableNode(ent,BOTNODE_DENSITY, BOTNODE_ALL);
		if (dedicated->value)
			gi.dprintf(				  "node: %d type: %d x: %f y: %f z %f\n",node,nodes[node].type,nodes[node].origin[0],nodes[node].origin[1],nodes[node].origin[2]);
		safe_cprintf(ent,PRINT_MEDIUM,"node: %d type: %d x: %f y: %f z %f\n",node,nodes[node].type,nodes[node].origin[0],nodes[node].origin[1],nodes[node].origin[2]);
	
		ACEND_ShowNode(node, 1); //hypov8 show closest node

		sprintf(strWrite, "bind 9 movenode %d\n$Bound key 9 to %d\n", node, node);

		gi.WriteByte(13);
		gi.WriteString(strWrite);
		gi.unicast(ent, true);
	}

	else if (Q_stricmp(cmd, "movenode") == 0)
	{
		int cmd2, cmd3, cmd4;

		if (stopNodeUpdate)
			return true;

		node = atoi(gi.argv(1));

		cmd2 = atof(gi.argv(2));
		cmd3 = atof(gi.argv(3));
		cmd4 = atof(gi.argv(4));

		//hypov8 no node location specified, will use current player location
		if (!cmd2 && !cmd3 && !cmd4)
		{
			VectorCopy(ent->s.origin, nodes[node].origin);
			if (dedicated->value)
				gi.dprintf("node: %d moved to x: %f y: %f z %f\n", node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
			safe_cprintf(ent, PRINT_MEDIUM, "node: %d moved to x: %f y: %f z %f\n", node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
		}
		else
		{
			nodes[node].origin[0] = atof(gi.argv(2));
			nodes[node].origin[1] = atof(gi.argv(3));
			nodes[node].origin[2] = atof(gi.argv(4));
			gi.dprintf("node: %d moved to x: %f y: %f z %f\n", node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
			safe_cprintf(ent, PRINT_MEDIUM, "node: %d moved to x: %f y: %f z %f\n", node, nodes[node].origin[0], nodes[node].origin[1], nodes[node].origin[2]);
		}

	}
	else if (Q_stricmp(cmd, "clearnode") == 0) //add hypov8 clear all paths to a node(cant be removed?)
		ACEND_RemovePaths(ent, atoi(gi.argv(1)));

	else if (Q_stricmp(cmd, "nodefinal") == 0) //add hypov8 finalise node table
		stopNodeUpdate = 1;

	else
		return false;

	return true;
}


///////////////////////////////////////////////////////////////////////
// Called when the level changes, store maps and bots (disconnected)
///////////////////////////////////////////////////////////////////////
void ACECM_Store()
{
	ACEND_SaveNodes();
}

///////////////////////////////////////////////////////////////////////
// These routines are bot safe print routines, all id code needs to be 
// changed to these so the bots do not blow up on messages sent to them. 
// Do a find and replace on all code that matches the below criteria. 
//
// (Got the basic idea from Ridah)
//	
//  change: gi.cprintf to safe_cprintf
//  change: gi.bprintf to safe_bprintf
//  change: gi.centerprintf to safe_centerprintf
// 
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Debug print, could add a "logging" feature to print to a file
///////////////////////////////////////////////////////////////////////
void debug_printf(char *fmt, ...)
{
	int     i;
	char	bigbuffer[0x10000];
	int		len;
	va_list	argptr;
	edict_t	*cl_ent;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_MEDIUM, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->acebot.is_bot)
			continue;

		gi.cprintf(cl_ent,  PRINT_MEDIUM, bigbuffer);
	}

}

///////////////////////////////////////////////////////////////////////
// botsafe cprintf
///////////////////////////////////////////////////////////////////////
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
#if 1
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (ent && (!ent->inuse || ent->acebot.is_bot))
		return;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.cprintf(ent, printlevel, bigbuffer);
#endif
}

///////////////////////////////////////////////////////////////////////
// botsafe centerprintf
///////////////////////////////////////////////////////////////////////
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
#if 1
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (!ent->inuse || ent->acebot.is_bot)
		return;
	
	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);
	
	gi.centerprintf(ent, bigbuffer); //hypo to keep
#endif
}

///////////////////////////////////////////////////////////////////////
// botsafe bprintf
///////////////////////////////////////////////////////////////////////
void safe_bprintf (int printlevel, char *fmt, ...)
{
#if 1
	int i;
	char	bigbuffer[0x10000];
	int		len;
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		gi.cprintf(NULL, printlevel, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse) 
			continue;
		if (cl_ent->acebot.is_bot)
			continue;

		gi.cprintf(cl_ent, printlevel, bigbuffer);
	}
#endif
}

