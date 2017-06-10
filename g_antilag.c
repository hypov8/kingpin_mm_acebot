/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//hypov8 anti lag file from et-xreal
//hypov8 todo: check teleporters?

// NET_ANTILAG	//et-xreal antilag

#include "g_local.h"
#define DEBUGLAG 0

/*
=================
LerpPosition
hypov8 calculate a historical origin 
with % between 2 frames
=================
*/
void LerpPosition(vec3_t start, vec3_t end, float frac, vec3_t out)
{
	vec3_t          dist;

	VectorSubtract(end, start, dist);
	VectorMA(start, frac, dist, out);
}

/*
================
G_StoreClientPosition
================
*/
void G_StoreClientPosition(edict_t * ent)
{
	int             top;
	
	if(!(ent->inuse
		&& ent->solid!=SOLID_NOT))
	{
		return;
	}

	ent->client->topMarker++;
	if(ent->client->topMarker >= MAX_CLIENT_MARKERS)
	{
		ent->client->topMarker = 0;
	}

	top = ent->client->topMarker;

	VectorCopy(ent->mins, ent->client->clientMarkers[top].mins);
	VectorCopy(ent->maxs, ent->client->clientMarkers[top].maxs);
	VectorCopy(ent->s.origin, ent->client->clientMarkers[top].origin); //origin ?
	ent->client->clientMarkers[top].time = level.framenum * 100;
}

/*
================
G_AdjustSingleClientPosition
move clients back in history to match an old recieved event
================
*/
static void G_AdjustSingleClientPosition(edict_t * ent, int time)
{
	int             i, j;
	float           frac;

	if (time > (level.framenum * 100 + 100)) //hypov8 allow prediction forward
	{
		#if DEBUGLAG
		Com_Printf("ERROR: time=%i > framenum=%i\n", time, level.framenum * 100);
		#endif
		return; //hypo use normal trace, dont move anyone. will cause print_debug error when moving clients back
	}	// no lerping forward....

	time -= 100; //hypov8 introduce 100ms. game seems to be lagging behind?

	// find a pair of markers which bound the requested time
	i = j = ent->client->topMarker;
	do
	{
		if(ent->client->clientMarkers[i].time <= time)
			break;

		j = i;
		i--;
		if(i < 0)
			i = MAX_CLIENT_MARKERS - 1;

	} while(i != ent->client->topMarker);
	
	// save current position to backup
	if (ent->client->backupMarker.time != level.framenum * 100)
	{
		VectorCopy(ent->s.origin, ent->client->backupMarker.origin);
		VectorCopy(ent->mins, ent->client->backupMarker.mins);
		VectorCopy(ent->maxs, ent->client->backupMarker.maxs);
		ent->client->backupMarker.time = level.framenum * 100;
	}
#if DEBUGLAG
	else
		Com_Printf("ERROR: backupMarker store for %s mkrTime=%i leveltime=%i ping=%i\n", ent->client->pers.netname, ent->client->backupMarker.time, level.framenum * 100, ent->client->ping);
#endif

	if(i != ent->client->topMarker)
	{
		frac = (float)(time - ent->client->clientMarkers[i].time) /
			(float)(ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time);

		LerpPosition(ent->client->clientMarkers[i].origin, ent->client->clientMarkers[j].origin, frac, ent->s.origin);
		LerpPosition(ent->client->clientMarkers[i].mins, ent->client->clientMarkers[j].mins, frac, ent->mins);
		LerpPosition(ent->client->clientMarkers[i].maxs, ent->client->clientMarkers[j].maxs, frac, ent->maxs);
	}
	else if (i == j) //predict forward, for low pingers
	{
		//j=	//hypo last posi
		i--;	//hypo 2nd last posi
		if (i < 0)
			i = MAX_CLIENT_MARKERS - 1;

		frac = (float)(time - ent->client->clientMarkers[i].time) /
			(float)(ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time);

#if DEBUGLAG
		Com_Printf("Time = %f ping = %i\n", frac, ent->client->ping);// ping = %i\n", ent->client->pers.netname, ent->client->backupMarker.time, level.framenum * 100, ent->client->ping
#endif


		//hypo use last 2 locations to predict location
		LerpPosition(ent->client->clientMarkers[i].origin, ent->client->clientMarkers[j].origin, frac, ent->s.origin);
		LerpPosition(ent->client->clientMarkers[i].mins, ent->client->clientMarkers[j].mins, frac, ent->mins);
		LerpPosition(ent->client->clientMarkers[i].maxs, ent->client->clientMarkers[j].maxs, frac, ent->maxs);
	}

	else // either head (topMarker) or tail (topMarker+1) of history
	{
		#if DEBUGLAG
		//Com_Printf("NOTE: USING ent->client->topMarker for %s %i %i\n", ent->client->pers.netname, time, level.framenum * 100);
		#endif
		VectorCopy(ent->client->clientMarkers[j].origin, ent->s.origin);
		VectorCopy(ent->client->clientMarkers[j].mins, ent->mins);
		VectorCopy(ent->client->clientMarkers[j].maxs, ent->maxs);
	}

	gi.linkentity(ent);//trap_LinkEntity

}

/*
================
G_ReAdjustSingleClientPosition
move clients back to real position
================
*/
static void G_ReAdjustSingleClientPosition(edict_t * ent)
{
	// restore from backup 
	if (ent->client->backupMarker.time == level.framenum * 100 /*(int)level.time * 1000*/)
	{
		VectorCopy(ent->client->backupMarker.origin, ent->s.origin);
		VectorCopy(ent->client->backupMarker.mins, ent->mins);
		VectorCopy(ent->client->backupMarker.maxs, ent->maxs);
		ent->client->backupMarker.time = 0;

		gi.linkentity(ent);//trap_LinkEntity
	}
	#if DEBUGLAG
	else
		Com_Printf("ERROR: backupMarker NOT leveltime for %s mkrTime=%i leveltime=%i ping=%i\n", ent->client->pers.netname, ent->client->backupMarker.time, level.framenum * 100, ent->client->ping);
	#endif
}

/*
================
G_AdjustClientPositions
================
*/
static void G_AdjustClientPositions(edict_t * ent, int time, qboolean forward, edict_t * owner)
{
	int             i;
	edict_t      *list;

	/* sv_antilag_botdelay not working 100%, it wont predict a player ahead of time */
	/* add some extra ping to all players. will add effects/value upto the players ping value */
	/* eg. value=75. if the lowest ping is 50 in server and rest +100. */
	/* 50 ping player will act as if 50ping. the rest will be like a 75 ping game */
	/* is aimed to make all players even while will reduce aimbot accuracy */
	if (sv_antilag_botdelay->value)
		time += (int)sv_antilag_botdelay->value;

	for (i = 0; i < game.maxclients; i++, list++)
	{
		//game.clients;
		list = &g_edicts[1 + i];

		// Gordon: ok lets test everything under the sun
		if (list->client
			&& list->inuse
			&& (list != ent)  //hypov8 dont shoot ur self in the foot!!!
			&& (list != owner) //hypo dont trace back person who created the entitiy
			&& list->solid != SOLID_NOT
			&&list->client->invincible_framenum < level.framenum)
		{
			if(forward)
			{
				G_AdjustSingleClientPosition(list, time);
			}
			else
			{
				G_ReAdjustSingleClientPosition(list);
			}
		}
	}
}

/*
================
G_ResetMarkers
reset players old frame data
for respawn etc
================
*/
void G_ResetMarkers(edict_t * ent)
{
	int             i, time;
	//char            buffer[MAX_STRING_CHARS];
	int           period;

	//hypov8 no way to change server framerates?
		period = 100; //kp seems to use 0.1 framerate  //100ms = 10fps  //50ms = sv_fps 20 

	ent->client->topMarker = MAX_CLIENT_MARKERS - 1;
	for (i = MAX_CLIENT_MARKERS - 1, time = level.framenum * 100; i >= 0; i--, time -= period)
	{
		VectorCopy(ent->mins, ent->client->clientMarkers[i].mins);
		VectorCopy(ent->maxs, ent->client->clientMarkers[i].maxs);
		VectorCopy(ent->s.origin, ent->client->clientMarkers[i].origin);
		ent->client->clientMarkers[i].time = time /* *1000 */;
	}
	/* hypov8 added to reset old trace if they died before G_HistoricalTraceEnd got to run */
	VectorCopy(ent->s.origin, ent->client->backupMarker.origin);
	VectorCopy(ent->mins, ent->client->backupMarker.mins);
	VectorCopy(ent->maxs, ent->client->backupMarker.maxs);
	ent->client->backupMarker.time = 0;	
}

/*
================
G_HistoricalTraceBegin
================
*/
void G_HistoricalTraceBegin(edict_t * ent, edict_t * owner)
{
	int frameMinusPing, mSec;

// ACEBOT_ADD
	if (ent->acebot.is_bot)
		return;
// ACEBOT_END


	/* calculate exact time client think was run between server frames */
	mSec =  Sys_Milliseconds() - level.RealTimeMSec;
	
	/*saninty check should never happen*/
	if (mSec > 99) mSec = 99;
	if (mSec < 0) mSec = 0;

#if DEBUGLAG
	gi.dprintf("shot %i ms afer server frame \n", mSec);
#endif

	if ((sv_antilag->value && !ent->cl_noAntiLag)) /* client don't want bullets predicted */
	{
		if (ent->antilagToTrace) //entity is not a player
		{
			if (!ent->owner->cl_noAntiLag) /* client don't want rocket predicted */
			{
				frameMinusPing = level.framenum * 100 - ent->antilagPingTimer;
				G_AdjustClientPositions(ent, frameMinusPing, true, owner); //set time relitive to explosives ping
			}
		}
		else if (ent->client) //skip dogs etc
		{
			frameMinusPing = level.framenum * 100 - ent->client->ping + mSec;
			G_AdjustClientPositions(ent, frameMinusPing, true, owner);
		}
	}
}

/*
================
G_HistoricalTraceEnd
================
*/
void G_HistoricalTraceEnd(edict_t * ent, edict_t * owner)
{									/* client don't want bullets predicted */
// ACEBOT_ADD
	if (ent->acebot.is_bot)
		return;
// ACEBOT_END

	if (ent->antilagToTrace) /* must b rocket */
	{
		if (!ent->owner->cl_noAntiLag)
			G_AdjustClientPositions(ent, 0, false, owner);
	}
	else if (sv_antilag->value && !ent->cl_noAntiLag)
		G_AdjustClientPositions(ent, 0, false, owner);
}



/*
===================
G_UnTimeShiftClient
MH:
Move a client back to where he was before the time shift
===================
*/
/*
void G_UnTimeShiftClient(edict_t *ent) {
	// if it was saved
	if (ent->client->saved.time == level.framenum * 100) {
		// move it back
		VectorCopy(ent->client->saved.mins, ent->mins);
		VectorCopy(ent->client->saved.maxs, ent->maxs);
		VectorCopy(ent->client->saved.origin, ent->s.origin);
		ent->client->saved.time = 0;

		gi.linkentity(ent);
	}
}

*/




// END_LAG
