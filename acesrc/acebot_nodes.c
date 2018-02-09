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
//  acebot_nodes.c -   This file contains all of the 
//                     pathing routines for the ACE bot.
// 
///////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "acebot.h"

// flags
qboolean newmap=true;

// Total number of nodes that are items
int numitemnodes; 

// Total number of nodes
short numnodes;

//#define NULL    ((void *)0)
#define NODE0 ((short)0)

//short NODE0 = (short)0;

//hypo add command to stop file beint written to
int stopNodeUpdate;

// For debugging paths
short show_path_from = -1;
short show_path_to = -1;

// array for node data
botnode_t nodes[MAX_BOTNODES]; 
short path_table[MAX_BOTNODES][MAX_BOTNODES];

///////////////////////////////////////////////////////////////////////
// NODE INFORMATION FUNCTIONS
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Determin cost of moving from one node to another
///////////////////////////////////////////////////////////////////////
int ACEND_FindCost(short from, short to)
{
	short curnode;
	int cost=1; // Shortest possible is 1

	// If we can not get there then return invalid
	if (path_table[from][to] == INVALID) //hypo skiped? why? added short to int
		return INVALID;

	// Otherwise check the path and return the cost
	curnode = path_table[from][to];

	// Find a path (linear time, very fast)
	while(curnode != to)
	{
		curnode = path_table[curnode][to];
		if(curnode == INVALID) // something has corrupted the path abort
			return INVALID;
		cost++;

		if (cost > 999) // add hypov8. something has corrupted the path abort
			return INVALID;
	}
	
	return cost;
}

#if 0
///////////////////////////////////////////////////////////////////////
// Find a close node to the player within dist.
//
// Faster than looking for the closest node, but not very 
// accurate.
///////////////////////////////////////////////////////////////////////
short ACEND_FindCloseReachableNode(edict_t *self, int range, short type)
{
	vec3_t v;
	int i;
	trace_t tr;
	float dist;

	range *= range;

	for(i=0;i<numnodes;i++)
	{
		if(type == BOTNODE_ALL || type == nodes[i].type) // check node type
		{
		
			VectorSubtract(nodes[i].origin,self->s.origin,v); // subtract first

			dist = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

			if(dist < range) // square range instead of sqrt
			{
				// make sure it is visible
				//trace = gi.trace (self->s.origin, vec3_origin, vec3_origin, nodes[i].origin, self, MASK_OPAQUE);
				tr = gi.trace (self->s.origin, self->mins, self->maxs, nodes[i].origin, self, MASK_OPAQUE);

				if(tr.fraction == 1.0)
					return i;
			}
		}
	}

	return -1;
}
#endif
///////////////////////////////////////////////////////////////////////
// Find the closest node to the player within a certain range
///////////////////////////////////////////////////////////////////////
short ACEND_FindClosestReachableNode(edict_t *self, int range, short type)
{
	short i;
	float closest = 99999;
	float dist;
	short node=INVALID;
	vec3_t v;
	trace_t tr;
	float rng;
	vec3_t maxs,mins;

	VectorCopy(self->mins,mins);
	VectorCopy(self->maxs,maxs);
	
	// For Ladders, do not worry so much about reachability
	if(type == BOTNODE_LADDER)
	{
		VectorCopy(vec3_origin,maxs);
		VectorCopy(vec3_origin,mins);
	}
	else
		mins[2] += 18; // Stepsize

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)	
	
	for (i = NODE0; i<numnodes; i++)
	{		
		if(type == BOTNODE_ALL || type == nodes[i].type) // check node type
		{
			VectorSubtract(nodes[i].origin, self->s.origin,v); // subtract first

			dist = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
		
			if(dist < closest && dist < rng) 
			{
				// make sure it is visible
				tr = gi.trace(self->s.origin, mins, maxs, nodes[i].origin, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
				if(tr.fraction == 1.0)
				{
					node = i;
					closest = dist;
				}
				else //hypo check from higher up?
				{
					vec3_t viewHeightUp;
					VectorCopy(self->s.origin, viewHeightUp);
					viewHeightUp[2] += 36;
					self->viewheight;

					// make sure it is visible
					tr = gi.trace(viewHeightUp, vec3_origin, vec3_origin, nodes[i].origin, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
					if (tr.fraction == 1.0)
					{
						node = i;
						closest = dist;
					}

				}
			}
		}
	}
	
	return node;
}

///////////////////////////////////////////////////////////////////////
// BOT NAVIGATION ROUTINES
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Set up the goal
///////////////////////////////////////////////////////////////////////
void ACEND_SetGoal(edict_t *self, short goal_node)
{
	short node;

	self->acebot.goal_node = goal_node;
	node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY*3, BOTNODE_ALL);
	
	if(node == INVALID)
		return;
	
	if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
		debug_printf("%s new start node selected %d\n",self->client->pers.netname,node);
	
	
	self->acebot.current_node = node;
	self->acebot.next_node = self->acebot.current_node; // make sure we get to the nearest node first
	self->acebot.node_timeout = 0;

}

///////////////////////////////////////////////////////////////////////
// Move closer to goal by pointing the bot to the next node
// that is closer to the goal
///////////////////////////////////////////////////////////////////////
qboolean ACEND_FollowPath(edict_t *self)
{
	vec3_t v;

#if 1 //def HYPODEBUG //defined in project DEBUG
	//////////////////////////////////////////
	// Show the path (uncomment for debugging)
	if (debug_mode && !debug_mode_origin_ents) //hypov8 disable path lines, tomany overflows
	{
		show_path_from = self->acebot.current_node;
		show_path_to = self->acebot.goal_node;
		ACEND_DrawPath();
	}
	//////////////////////////////////////////
#endif

	// Try again?
	if(self->acebot.node_timeout ++ > 30)
	{
		if(self->acebot.tries++ > 3)
			return false;
		else
			ACEND_SetGoal(self, self->acebot.goal_node);
	}
		
	// Are we there yet?
#if 0
	VectorSubtract(self->s.origin, nodes[self->acebot.next_node].origin, v);
	if (VectorLength(v) < 32) //hypov8 ToDo: will turn on spot if node is above player
	{
		// reset timeout
		self->acebot.node_timeout = 0;

		if (self->acebot.next_node == self->acebot.goal_node)
		{
			if (debug_mode)
				debug_printf("%s reached goal!\n", self->client->pers.netname);

			ACEAI_PickLongRangeGoal(self); // Pick a new goal
		}
		else
		{
			self->acebot.current_node = self->acebot.next_node;
			self->acebot.next_node = path_table[self->acebot.current_node][self->acebot.goal_node];
		}
	}

#else
	//hypov8 reached goal at higher ground
	VectorSubtract(self->s.origin, nodes[self->acebot.next_node].origin, v);
	if (VectorLength(v) < 64)
	{
		vec3_t movedOrigin;
		VectorCopy(self->s.origin, movedOrigin);
		movedOrigin[2] = nodes[self->acebot.next_node].origin[2];
		VectorSubtract(movedOrigin, nodes[self->acebot.next_node].origin, v);
		if (VectorLength(v) < 32) //hypov8 ToDo: will turn on spot if node is above player
		{
			// reset timeout
			self->acebot.node_timeout = 0;

			if (self->acebot.next_node == self->acebot.goal_node)
			{
				if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on 
					debug_printf("%s reached goal!\n",self->client->pers.netname);	
			
				ACEAI_PickLongRangeGoal(self); // Pick a new goal
			}
			else
			{
				self->acebot.current_node = self->acebot.next_node;
				self->acebot.next_node = path_table[self->acebot.current_node][self->acebot.goal_node];
			}
		}
	


	}

#endif
	

	if (self->acebot.current_node == INVALID || self->acebot.next_node == INVALID)
		return false;
	
	// Set bot's movement vector
	//ACE_Look_Straight(nodes[self->acebot.next_node].origin, self->s.origin, ACE_look_out); //hypov8 nodes shouldent look up/down
	//VectorSubtract(/*nodes[self->acebot.next_node].origin*/ACE_look_out, self->s.origin, self->acebot.move_vector);
	VectorSubtract(nodes[self->acebot.next_node].origin, self->s.origin, self->acebot.move_vector);
	return true;
}


///////////////////////////////////////////////////////////////////////
// MAPPING CODE
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Capture when the grappling hook has been fired for mapping purposes.
///////////////////////////////////////////////////////////////////////
#if 1
void ACEND_GrapFired(edict_t *self) //hypo todo: hitmen
{
	//int closest_node;
	
	if(!self->owner)
		return; // should not be here

#ifdef NOT_ZOID
	// Check to see if the grapple is in pull mode
	if(self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
	{
		// Look for the closest node of type grapple
		closest_node = ACEND_FindClosestReachableNode(self,NODE_DENSITY,NODE_GRAPPLE);
		if(closest_node == -1 ) // we need to drop a node
		{	
			closest_node = ACEND_AddNode(self,NODE_GRAPPLE);
			 
			// Add an edge
			ACEND_UpdateNodeEdge(self->owner->last_node,closest_node);
		
			self->owner->last_node = closest_node;
		}
		else
			self->owner->last_node = closest_node; // zero out so other nodes will not be linked
	}
#endif
}
#endif

///////////////////////////////////////////////////////////////////////
// Check for adding ladder nodes
///////////////////////////////////////////////////////////////////////
static qboolean ACEND_CheckForLadder(edict_t *self)
{
	int closest_node;

	// If there is a ladder and we are moving up, see if we should add a ladder node
	if (gi.pointcontents(self->s.origin) & CONTENTS_LADDER && self->velocity[2] > 0)
	{
		//debug_printf("contents: %x\n",tr.contents);

		closest_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_LADDER); 
		if(closest_node == -1)
		{
			closest_node = ACEND_AddNode(self,BOTNODE_LADDER);
	
			// Now add link
			ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, false);
			
			// Set current to last
			self->acebot.last_node = closest_node;
		}
		else
		{
			ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, false);
			self->acebot.last_node = closest_node; // set visited to last
		}
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////
// This routine is called to hook in the pathing code and sets
// the current node if valid.
///////////////////////////////////////////////////////////////////////
void ACEND_PathMap(edict_t *self)
{
	short closest_node;
	//short closest_nodeTmp;
	static float last_update=0; // start off low
	vec3_t v;

	if(level.time < last_update)
		return;

	last_update = level.time + 0.15; // slow down updates a bit

	// Special node drawing code for debugging
    if(show_path_to != -1)
		ACEND_DrawPath();
	
	////////////////////////////////////////////////////////
	// Special check for ladder nodes
	///////////////////////////////////////////////////////
	if(ACEND_CheckForLadder(self)) // check for ladder nodes
		return;

	// Not on ground, and not in the water, so bail
    if(!self->groundentity && !self->waterlevel)
		return;

	////////////////////////////////////////////////////////
	// Lava/Slime
	////////////////////////////////////////////////////////
	VectorCopy(self->s.origin,v);
	v[2] -= 18;
	if(gi.pointcontents(v) & (CONTENTS_LAVA|CONTENTS_SLIME))
		return; // no nodes in slime
	
    ////////////////////////////////////////////////////////
	// Jumping
	///////////////////////////////////////////////////////
	if (self->acebot.is_jumping && sv_botjump->value == 1.0) //hypov8 bug. was not called previously. uncommented in ace source
	{														//disable it by default, bunnyhop makes lots of nodes.
	   // See if there is a closeby jump landing node (prevent adding too many)
		closest_node = ACEND_FindClosestReachableNode(self, 64, BOTNODE_JUMP);

		if(closest_node == INVALID)
			closest_node = ACEND_AddNode(self,BOTNODE_JUMP);
		
		// Now add link
		if (self->acebot.last_node != -1)
			ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, false);

		self->acebot.is_jumping = false;
		return;
	}

	////////////////////////////////////////////////////////////
	// Grapple
	// Do not add nodes during grapple, added elsewhere manually
	////////////////////////////////////////////////////////////


	// Iterate through all nodes to make sure far enough apart
	closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);

	////////////////////////////////////////////////////////
	// Special Check for Platforms
	////////////////////////////////////////////////////////
	if(self->groundentity && self->groundentity->use == Use_Plat)
	{
		if(closest_node == INVALID)
			return; // Do not want to do anything here.

		// Here we want to add links
		if (closest_node != self->acebot.last_node && self->acebot.last_node != INVALID)
			ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, false);

		self->acebot.last_node = closest_node; // set visited to last
		return;
	}
	 
	 ////////////////////////////////////////////////////////
	 // Add Nodes as needed
	 ////////////////////////////////////////////////////////
	 if(closest_node == INVALID)
	 {
		// Add nodes in the water as needed
		 if (self->waterlevel)
		 {
			closest_node = ACEND_AddNode(self, BOTNODE_WATER);

			// Now add link
			if (self->acebot.last_node != -1)
			ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, false);
		 }
		 else
		 {
			closest_node = ACEND_AddNode(self, BOTNODE_MOVE);

			// Now add link
			if (self->acebot.last_node != INVALID)
				ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, true);
		 }	
		 self->acebot.last_node = closest_node; // set visited to last
		 //debug_printf("LASTNODE= %i (closest_node %i=INVALID)\n", self->acebot.last_node, BOTNODE_DENSITY);
	 }
	 else //hypo only join when we are closer than default 92 units
	 {	
		 closest_node = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY_LOCAL, BOTNODE_ALL);
		
		 //found node withing 48 units
		 if (closest_node > INVALID)
		 {
			  BOTNODE_DENSITY_LOCAL;


			 if (closest_node != self->acebot.last_node && self->acebot.last_node != INVALID)
			 {
				float dist;
				vec3_t from2, to2,v2;
				VectorCopy(nodes[self->acebot.last_node].origin, from2);
				VectorCopy(nodes[closest_node].origin, to2);
				from2[2] = to2[2] = 0; //hypo remove height from distance

				VectorSubtract(from2, to2, v2);
				dist = VectorLength(v2);
				//if (dist < BOTNODE_DENSITY * 3) //reject long paths.. didnt get close enough to path
				{
					ACEND_UpdateNodeEdge(self->acebot.last_node, closest_node, true);
					//debug_printf("LASTNODE= %i CLOSENODE= %i (closest_node %i)\n", self->acebot.last_node, closest_node, BOTNODE_DENSITY_LOCAL);
					self->acebot.last_node = closest_node; // set visited to last
				}
				//else
				//{
					//debug_printf("LASTNODE= %i CLOSENODE= %i (path to far > %i)\n", self->acebot.last_node, closest_node, BOTNODE_DENSITY * 3);
					self->acebot.last_node = closest_node;
				//}
				
			 }
			 else if (self->acebot.last_node == INVALID)
			 {
				 //debug_printf("LASTNODE= %i CLOSENODE= %i (last_node %i=INVALID)\n", self->acebot.last_node, closest_node, BOTNODE_DENSITY_LOCAL);
				self->acebot.last_node = closest_node;
				
			 }
		 }
	 }
}

///////////////////////////////////////////////////////////////////////
// Init node array (set all to INVALID)
///////////////////////////////////////////////////////////////////////
void ACEND_InitNodes(void)
{
	numnodes = (short)1;
	numitemnodes = 1;
	memset(nodes,0,sizeof(botnode_t) * MAX_BOTNODES);
	memset(path_table,INVALID,sizeof(short)*MAX_BOTNODES*MAX_BOTNODES);
			
}

///////////////////////////////////////////////////////////////////////
// Show the node for debugging (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_ShowNode(short node, int isTmpNode)
{

#if 0 //ndef HYPODEBUG //defined in project DEBUG
	return; // commented out for now. uncommend to show nodes during debugging,
	        // but too many will cause overflows. You have been warned.
#endif
	edict_t *ent;

	if (!debug_mode)
		return;

		//stop showing other nodes if in "localnode" mode
	if (debug_mode_origin_ents && isTmpNode != 2)
		return;

	ent = G_Spawn();

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;

	ent->s.effects = EF_COLOR_SHELL;
	ent->s.renderfx2 = RF2_NOSHADOW;
	ent->s.renderfx = RF_FULLBRIGHT;

	if (nodes[node].type == BOTNODE_MOVE)
		ent->s.renderfx |= RF_SHELL_GREEN;
	else if (nodes[node].type == BOTNODE_LADDER)
		ent->s.renderfx |= RF_SHELL_RED;
	else if (nodes[node].type == BOTNODE_JUMP)
		ent->s.renderfx |= RF_SHELL_BLUE;
	else if (nodes[node].type == BOTNODE_MOVE)
		ent->s.renderfx &= (RF_SHELL_RED | RF_SHELL_GREEN);
	else if (nodes[node].type == BOTNODE_PLATFORM)
		ent->s.renderfx |= (RF_SHELL_BLUE | RF_SHELL_GREEN);
	else 
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN);


	//ent->s.modelindex = gi.modelindex ("models/items/ammo/grenades/medium/tris.md2");
	ent->s.modelindex = gi.modelindex("models/props/cash/tris.md2");
	//ent->s.modelindex = gi.modelindex("models/weapons/e_pistol/tris.md2");
	//ent->s.modelindex = gi.modelindex("models/props/crate/stillcrate32_1.mdx");

	ent->owner = ent;
	if (isTmpNode == 1)
		ent->nextthink = level.time + 20.0;
	else if (isTmpNode == 2)
		ent->nextthink = level.time + 0.1;
	else
		ent->nextthink = level.time + 200;

	ent->think = G_FreeEdict;                
	ent->dmg = 0;

	VectorCopy(nodes[node].origin,ent->s.origin);
	gi.linkentity (ent);

}

///////////////////////////////////////////////////////////////////////
// Draws the current path (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_DrawPath()
{
	short current_node, goal_node, next_node;
	int i = 0;

	current_node = show_path_from;
	goal_node = show_path_to;

	next_node = path_table[current_node][goal_node];

	// Now set up and display the path
	while(current_node != goal_node && current_node != -1)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_LASER);
		gi.WritePosition (nodes[current_node].origin);
		gi.WritePosition (nodes[next_node].origin);
		gi.multicast (nodes[current_node].origin, MULTICAST_PVS);
		current_node = next_node;
		next_node = path_table[current_node][goal_node];
		i++;
		if (i > 15) //add hypov8 draw short paths
			break;
	}
}


///////////////////////////////////////////////////////////////////////
// Turns on showing of the path, set goal to -1 to 
// shut off. (utility function)
///////////////////////////////////////////////////////////////////////
void ACEND_ShowPath(edict_t *self, short goal_node)
{
	show_path_from = ACEND_FindClosestReachableNode(self, BOTNODE_DENSITY, BOTNODE_ALL);
	show_path_to = goal_node;
}

///////////////////////////////////////////////////////////////////////
// Add a node of type ?
//note hypov8 nodes initialised with 1. should be [0]. but may be used else where?
// so added -1 to everything
///////////////////////////////////////////////////////////////////////
short ACEND_AddNode(edict_t *self, short type)
{
	vec3_t v1,v2;
	
	// Block if we exceed maximum
	if (numnodes + (short)1 > MAX_BOTNODES)
		return false;

	if (stopNodeUpdate)
		return self->acebot.last_node;

	// Set location
	VectorCopy(self->s.origin, nodes[numnodes - (short)1].origin);

	// Set type
	nodes[numnodes - (short)1].type = type;

	/////////////////////////////////////////////////////
	// ITEMS
	// Move the z location up just a bit.
	if(type == BOTNODE_ITEM)
	{
		nodes[numnodes - (short)1].origin[2] += 16;
		numitemnodes++;
	}

	// Teleporters
	if(type == BOTNODE_TELEPORTER)
	{
		// Up 32
		nodes[numnodes - (short)1].origin[2] += 32;
	}

	if(type == BOTNODE_LADDER)
	{
		nodes[numnodes - (short)1].type = BOTNODE_LADDER;
				
		if(debug_mode)
		{
			debug_printf("Node added %d type: Ladder\n",numnodes-1);
			ACEND_ShowNode(numnodes - (short)1, 0);
		}
		
		numnodes++;
		return numnodes - (short)2; // return the node added

	}

	// For platforms drop two nodes one at top, one at bottom
	if(type == BOTNODE_PLATFORM) //hypo
	{
		VectorCopy(self->maxs,v1);
		VectorCopy(self->mins,v2);
		
		// To get the center
		nodes[numnodes - (short)1].origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
		nodes[numnodes - (short)1].origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
		nodes[numnodes - (short)1].origin[2] = self->maxs[2] + 32;
			
		if(debug_mode)	
			ACEND_ShowNode(numnodes - (short)1, 0);
		
		numnodes++;

		nodes[numnodes - (short)1].origin[0] = nodes[numnodes - (short)2].origin[0];
		nodes[numnodes - (short)1].origin[1] = nodes[numnodes - (short)2].origin[1];
		nodes[numnodes - (short)1].origin[2] = self->mins[2] + 32;
		
		nodes[numnodes - (short)1].type = BOTNODE_PLATFORM;

		// Add a link
		ACEND_UpdateNodeEdge(numnodes - (short)1, numnodes - (short)2, false);
		
		if(debug_mode)
		{
			debug_printf("Node added %d type: Platform\n", numnodes - (short)1);
			ACEND_ShowNode(numnodes - (short)1, 0);
		}

		numnodes++;

		return numnodes - (short)2;
	}
		
	if(debug_mode)
	{
		if (nodes[numnodes - (short)1].type == BOTNODE_MOVE)
			debug_printf("Node added %d type: Move\n",numnodes-1);
		else if (nodes[numnodes - (short)1].type == BOTNODE_TELEPORTER)
			debug_printf("Node added %d type: Teleporter\n",numnodes-1);
		else if (nodes[numnodes - (short)1].type == BOTNODE_ITEM)
			debug_printf("Node added %d type: Item\n",numnodes-1);
		else if (nodes[numnodes - (short)1].type == BOTNODE_PLATFORM)
			debug_printf("Node added %d type: platform\n", numnodes-1);
		else if (nodes[numnodes - (short)1].type == BOTNODE_WATER)
			debug_printf("Node added %d type: Water\n",numnodes-1);
/*		else if(nodes[numnodes].type == BOTNODE_GRAPPLE)
			debug_printf("Node added %d type: Grapple\n",numnodes);*/

		ACEND_ShowNode(numnodes - (short)1, 0);
	}
	//if (type != BOTNODE_TELEPORTER) //hypov8 will get counted later on
		numnodes++;
	
	return numnodes- (short)2; // return the node added
}

///////////////////////////////////////////////////////////////////////
// Add/Update node connections (paths)
///////////////////////////////////////////////////////////////////////
void ACEND_UpdateNodeEdge(short from, short to, qboolean check)
{
	short i;
	vec3_t v, from2, to2, boxMins, boxMaxs;
	float distToTarget;
	trace_t tr;

	if (stopNodeUpdate)
		return;

	if (from == INVALID || to == INVALID || from == to)
		return; // safety

	//if (path_table[from][to] == to)
	//		return; //add hypov8 no need?

#ifdef HYPODEBUG
	if (from < INVALID || to < INVALID)
		debug_printf(" ****ERROR**** from:%d to:%d\n", from, to);
#endif

#if 1 //hypov8 change link code to not jump up walls, posible rj was used with player?
	if (check && (nodes[to].origin[2] - nodes[from].origin[2]) > 60) //taller than crate
	{
		VectorSet(boxMins, -2, -2, -2);
		VectorSet(boxMaxs, 2, 2, 2);

		VectorCopy(nodes[from].origin, from2);
		VectorCopy(nodes[to].origin, to2);
		from2[2] = to2[2] = 0;

		VectorSubtract(from2, to2, v);
		distToTarget = VectorLength(v);
		if (distToTarget >= 64)
		{
			tr = gi.trace(nodes[from].origin, boxMins, boxMaxs, nodes[to].origin, NULL, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
			//tr.surface->flags; //surface flag
			if (!(tr.contents & CONTENTS_LADDER)) //content flag
			{
				if (!(tr.fraction == 1.0)) //1.0 = nothing in the way
				{
					if (debug_mode)
						debug_printf(" *REJECTED* Link %d -> %d\n", from, to);

					return; //add hypov8. node link not added because its to far from edge and way above our head
				}
			}
		}
	}
#if 0
	if(distToTarget > 92) //check any node for visual connection
	{
		VectorCopy(nodes[from].origin, from2);
		VectorCopy(nodes[to].origin, to2);

		tr = gi.trace(from2, vec3_origin, vec3_origin, to2, NULL, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
		//tr.surface->flags; //surface flag

		if (!(tr.fraction == 1.0)) //1.0 = nothing in the way
		{
			from2[2] += 60;
			to2[2] += 60;
			tr = gi.trace(from2, vec3_origin, vec3_origin, to2, NULL, MASK_BOT_SOLID_FENCE | CONTENTS_LADDER);
			if (!(tr.fraction == 1.0)) //1.0 = nothing in the way
			{
				if (debug_mode)
					debug_printf(" *REJECTED* Link %d -> %d\n", from, to);

				return; //add hypov8. node link not added because its to far from edge and way above our head
			}
		}

	}
#endif
#endif
#if 1

#endif

	if (debug_mode)
	{
		if (!(path_table[from][to] == to)) //hypov8 dont write created link if it existed
			debug_printf(" *CREATED* Link %d -> %d **was=%d**\n", from, to, path_table[from][to]);
	}

	// Add the link
	path_table[from][to] = to;

	// Now for the self-referencing part, linear time for each link added
#if 1
	for (i = NODE0; i < numnodes; i++)
	{
		if (path_table[i][from] != INVALID)
		{
			if (i == to)
			{
				path_table[i][to] = INVALID; // make sure we terminate
			}
			else
			{
				path_table[i][to] = path_table[i][from];
			}
		}
	}
		
#endif
}

///////////////////////////////////////////////////////////////////////
// Remove a node edge
///////////////////////////////////////////////////////////////////////
void ACEND_RemoveNodeEdge(edict_t *self, short from, short to)
{
	short i;

	if (stopNodeUpdate)
		return;

	if(debug_mode) 
		debug_printf("%s: Removing Edge %d -> %d\n", self->client->pers.netname, from, to);
		
	path_table[from][to] = INVALID; // set to invalid			

	// Make sure this gets updated in our path array
	for (i = NODE0; i<numnodes; i++)
		if(path_table[from][i] == to)
			path_table[from][i] = INVALID;
}

///////////////////////////////////////////////////////////////////////
// Remove a node edge
///////////////////////////////////////////////////////////////////////
void ACEND_RemovePaths(edict_t *self, short from)
{
	short i;
	
	if (stopNodeUpdate)
		return;

	if(debug_mode) 
		debug_printf("%s: Removing paths %d\n", self->client->pers.netname, from);
		
	//path_table[from][to] = INVALID; // set to invalid			

	// Make sure this gets updated in our path array
	for (i = NODE0; i<numnodes; i++)
		path_table[from][i] = INVALID;
}


///////////////////////////////////////////////////////////////////////
// This function will resolve all paths that are incomplete
// usually called before saving to disk
///////////////////////////////////////////////////////////////////////
static void ACEND_ResolveAllPaths()
{
	short i, from, to;
	int num=0;

	//short NODE0 = (short)0
	//gi.bprintf();
	//gi.cprintf();

	gi.dprintf("Resolving all paths...");
	//safe_bprintf(PRINT_HIGH, "Resolving all paths...");

	for (from = NODE0; from < numnodes; from++)
	{
		for (to = NODE0; to < numnodes; to++)
		{
#ifdef HYPODEBUG
			if (path_table[from][to] < INVALID)
				debug_printf(" ****ERROR**** from:%d to:%d\n", from, to);
#endif
			// update unresolved paths
			// Not equal to itself, not equal to -1 and equal to the last link
			if (from != to && path_table[from][to] == to)
			{
				num++;
				// Now for the self-referencing part linear time for each link added
				for (i = NODE0; i < numnodes; i++)
				{
					if (path_table[i][from] != INVALID)
						if (i == to)
							path_table[i][to] = INVALID; // make sure we terminate
						else
							path_table[i][to] = path_table[i][from];
				}
			}
		}
	}
	gi.dprintf("done (%d updated)\n",num);
	//safe_bprintf(PRINT_MEDIUM,"done (%d updated)\n",num);
}



///////////////////////////////////////////////////////////////////////
// Only called once per level, when saved will not be called again
//
// Downside of the routine is that items can not move about. If the level
// has been saved before and reloaded, it could cause a problem if there
// are items that spawn at random locations.
//
#if HYPODEBUG
#define DEBUG_ACE // uncomment to write out items to a file.
#endif
///////////////////////////////////////////////////////////////////////
static void ACEND_BuildItemNodeTable(qboolean reLinkEnts)
{
	edict_t *items;
	int i, item_index;
	vec3_t v, v1, v2;

#ifdef DEBUG_ACE
	FILE *pOut; // for testing
	cvar_t	*game_dir;
	char buf[32];

	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s\\items.txt", game_dir->string);

	if ((pOut = fopen(buf, "wt")) == NULL) //hypov8 //comp\\items.txt
		return;
#endif

	num_items = 0;

	// Add game items
	for (items = g_edicts; items < &g_edicts[globals.num_edicts]; items++)
	{
		// filter out crap
		if (items->solid == SOLID_NOT)
			continue;

		if (!items->classname)
			continue;

		/////////////////////////////////////////////////////////////////
		// Items
		/////////////////////////////////////////////////////////////////
		item_index = ACEIT_ClassnameToIndex(items->classname, items->style); //hypov8 add safe styles

		////////////////////////////////////////////////////////////////
		// SPECIAL NAV NODE DROPPING CODE
		////////////////////////////////////////////////////////////////
		// Special node dropping for platforms
		if (strcmp(items->classname, "func_plat") == 0)
		{
			if (!reLinkEnts)
				ACEND_AddNode(items, BOTNODE_PLATFORM);
			item_index = 99; // to allow to pass the item index test
			continue; //add hypov8
		}

		// Special node dropping for teleporters
		if (strcmp(items->classname, "misc_teleporter") == 0)
		{
			if (!reLinkEnts)
				ACEND_AddNode(items, BOTNODE_TELEPORTER);
			item_index = 99;
			continue; //add hypov8
		}

#ifdef DEBUG_ACE
		if (item_index == INVALID)
			fprintf(pOut, "Rejected item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
		else
			fprintf(pOut, "item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif		

		if (item_index == INVALID)
			continue;

		// add a pointer to the item entity
		item_table[num_items].ent = items;
		item_table[num_items].item = item_index;

		// If new, add nodes for items
		if (!reLinkEnts)
		{
			// Add a new node at the item's location.
			item_table[num_items].node = ACEND_AddNode(items, BOTNODE_ITEM);
			num_items++;
		}
		else // Now if rebuilding, just relink ent structures 
		{
			// Find stored location
			for (i = 0; i<(int)numnodes; i++)
			{
				if (nodes[i].type == BOTNODE_ITEM ||
					nodes[i].type == BOTNODE_PLATFORM ||
					nodes[i].type == BOTNODE_TELEPORTER) // valid types
				{
					VectorCopy(items->s.origin, v);

					// Add 16 to item type nodes
					if (nodes[i].type == BOTNODE_ITEM)
						v[2] += 16;

					// Add 32 to teleporter
					if (nodes[i].type == BOTNODE_TELEPORTER)
						v[2] += 32;

					if (nodes[i].type == BOTNODE_PLATFORM)
					{
						VectorCopy(items->maxs, v1);
						VectorCopy(items->mins, v2);

						// To get the center
						v[0] = (v1[0] - v2[0]) / 2 + v2[0];
						v[1] = (v1[1] - v2[1]) / 2 + v2[1];
						v[2] = items->mins[2] + 64;
					}

					if (v[0] == nodes[i].origin[0] &&
						v[1] == nodes[i].origin[1] &&
						v[2] == nodes[i].origin[2])
					{
						// found a match now link to facts
						item_table[num_items].node = i;

#ifdef DEBUG_ACE
						fprintf(pOut, "Relink item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif							
						num_items++;
						break; //add hypov8. stop it serching for new items. will get stuck if item is at same origin
					}
				}
			}
		}


	}

#ifdef DEBUG_ACE
	fclose(pOut);
#endif

}



///////////////////////////////////////////////////////////////////////
// Save to disk file
//
// Since my compression routines are one thing I did not want to
// release, I took out the compressed format option. Most levels will
// save out to a node file around 50-200k, so compression is not really
// a big deal.
///////////////////////////////////////////////////////////////////////
void ACEND_SaveNodes()
{
	FILE *pOut;
	char filename[60];
	short i,j;
	int version = 3; //file version 3 now has nodefinal
	cvar_t	*game_dir;
	char buf[32];
	int nodefinal = 0;
	
	if (!(level.modeset == TEAM_MATCH_RUNNING || level.modeset == DM_MATCH_RUNNING))
		return;

	if (!level.bots_spawned) //hypov8. no nodes loaded. so dont save, it will be blank
		return;

	// Resolve paths
	ACEND_ResolveAllPaths();

	gi.dprintf("Saving node table...");
	//safe_bprintf(PRINT_MEDIUM,"Saving node table...");

//hypo mod folder for bots dir
	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s\\nav\\", game_dir->string);
	strcpy(filename, buf);
	strcat(filename,level.mapname);
	strcat(filename,".nod");

	//stop map being updated
	if (stopNodeUpdate == 1)
	{
		nodefinal = 1;
		gi.dprintf("ACE: Node table *WRITE PROTECTED*\n ");
	}

	if((pOut = fopen(filename, "wb" )) == NULL)
		return; // bail
	
	fwrite(&version,sizeof(int),1,pOut); // write version
	fwrite(&nodefinal, sizeof(int), 1, pOut); //hypo if 1. will never get updated
	fwrite(&numnodes,sizeof(int),1,pOut); // write count
	fwrite(&num_items,sizeof(int),1,pOut); // write facts count
	
	fwrite(nodes,sizeof(botnode_t),numnodes,pOut); // write nodes
	
	for (i = NODE0; i<numnodes; i++)
		for (j = NODE0; j<numnodes; j++)
			fwrite(&path_table[i][j],sizeof(short),1,pOut); // write count
		
	fwrite(item_table,sizeof(item_table_t),num_items,pOut); 		// write out the fact table

	fclose(pOut);

	gi.dprintf(" <nodes=%i items=%i Version=%i> ", numnodes, num_items, version);
	gi.dprintf("done.\n");
	//safe_bprintf(PRINT_MEDIUM,"done.\n");
}

///////////////////////////////////////////////////////////////////////
// Read from disk file
///////////////////////////////////////////////////////////////////////
void ACEND_LoadNodes(void)
{
	FILE *pIn;
	short i,j;
	char filename[60];
	int version;
	cvar_t	*game_dir;
	char buf[32];
	int nodefinal = 0; //hypo dont update nodes

//hypo mod folder for bots dir
	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s\\nav\\", game_dir->string);
	strcpy(filename,buf); 
	strcat(filename,level.mapname);
	strcat(filename,".nod");

	stopNodeUpdate = 0; //hypo add


	if((pIn = fopen(filename, "rb" )) == NULL)
    {
		// Create item table
		gi.dprintf("ACE: No node file found, creating new one...");
		//safe_bprintf(PRINT_MEDIUM, "ACE: No node file found, creating new one...");
		ACEND_BuildItemNodeTable(false);
		gi.dprintf("done.\n");
		//safe_bprintf(PRINT_MEDIUM, "done.\n");
		return; 
	}

	// determin version
	fread(&version,sizeof(int),1,pIn); // read version
	if(version == 2) 
	{
		gi.dprintf("ACE: Loading node table...");
		//safe_bprintf(PRINT_MEDIUM,"ACE: Loading node table...");

		fread(&numnodes,sizeof(int),1,pIn); // read count
		fread(&num_items,sizeof(int),1,pIn); // read facts count
		fread(nodes,sizeof(botnode_t),numnodes,pIn);

		for (i = NODE0; i<numnodes; i++)
			for (j = NODE0; j<numnodes; j++)
				fread(&path_table[i][j],sizeof(short),1,pIn); // write count
	
		fread(item_table,sizeof(item_table_t),num_items,pIn);
		fclose(pIn);
	}
	else if (version == 3) //ver 3. added nodefinal
	{
		fread(&nodefinal, sizeof(int), 1, pIn);
		if (nodefinal == 1)
		{
			gi.dprintf(" ACE: Node table *WRITE PROTECTED*\n");
			stopNodeUpdate = 1;
		}

		gi.dprintf(" ACE: Loading node table...");
		//safe_bprintf(PRINT_MEDIUM,"ACE: Loading node table...");

		fread(&numnodes, sizeof(int), 1, pIn); // read count
		fread(&num_items, sizeof(int), 1, pIn); // read facts count
		fread(nodes, sizeof(botnode_t), numnodes, pIn);

		for (i = NODE0; i<numnodes; i++)
			for (j = NODE0; j<numnodes; j++)
				fread(&path_table[i][j], sizeof(short), 1, pIn); // write count

		fread(item_table, sizeof(item_table_t), num_items, pIn);
		fclose(pIn);

	}
	else
	{
		// Create item table
		gi.dprintf("ACE: No node file found, creating new one...");
		//safe_bprintf(PRINT_MEDIUM, "ACE: No node file found, creating new one...");
		ACEND_BuildItemNodeTable(false);
		gi.dprintf("done.\n");
		//safe_bprintf(PRINT_MEDIUM, "done.\n");
		return; // bail
	}

	gi.dprintf(" <nodes=%i items=%i Version=%i> ", numnodes, num_items, version);
	gi.dprintf("done.\n");
	
	ACEND_BuildItemNodeTable(true);
}

//hypov8
//remove short range goals and set teleporter node as hit todo:
void ACEND_TeleporterUpdate(edict_t *bot)
{
	short tmpNode;
	float dist;
	vec3_t from2, to2, v2;

	bot->acebot.trigPushTimer = level.framenum + 5;
	bot->acebot.isMovingUpPushed = true;
	bot->goalentity = NULL;
	bot->goal_ent;
	bot->last_goal;
	//find closest node and link to next node
	bot->movetarget = NULL;

	//set next node
	if (bot->acebot.goal_node != bot->acebot.next_node && bot->acebot.state == BOTSTATE_MOVE)
	{
		VectorCopy(bot->s.origin, from2);
		VectorCopy(nodes[bot->acebot.next_node].origin, to2);
		from2[2] = to2[2] = 0; //hypo remove height from distance
		VectorSubtract(from2, to2, v2);
		dist = VectorLength(v2);

		if (dist < 64)
		{
			bot->acebot.current_node = bot->acebot.next_node;
			bot->acebot.next_node = path_table[bot->acebot.current_node][bot->acebot.goal_node];
		}
		else
		{
			tmpNode = ACEND_FindClosestReachableNode(bot, BOTNODE_DENSITY, BOTNODE_TELEPORTER);
			if (tmpNode == bot->acebot.next_node)
			{
				bot->acebot.current_node = tmpNode;
				bot->acebot.next_node = path_table[bot->acebot.current_node][bot->acebot.goal_node];
			}
		}
	}


}

//hypov8
//remove short range goals and set jump pad node as hit, incase in bad location location
void ACEND_JumpPadUpdate(edict_t *bot)
{
	short tmpNode;
	float dist;
	vec3_t from2, to2, v2;

	bot->acebot.trigPushTimer = level.framenum + 5;
	bot->acebot.isMovingUpPushed = true;
	bot->goalentity = NULL;
	bot->goal_ent;
	bot->last_goal;
	//find closest node and link to next node
	bot->movetarget = NULL;

	//set next node
	if (bot->acebot.goal_node != bot->acebot.next_node && bot->acebot.state == BOTSTATE_MOVE)
	{
		VectorCopy(bot->s.origin, from2);
		VectorCopy(nodes[bot->acebot.next_node].origin, to2);
		from2[2] = to2[2] = 0; //hypo remove height from distance
		VectorSubtract(from2, to2, v2);
		dist = VectorLength(v2);

		if (dist < 64)
		{
			bot->acebot.current_node = bot->acebot.next_node;
			bot->acebot.next_node = path_table[bot->acebot.current_node][bot->acebot.goal_node];
		}
		else
		{
			tmpNode = ACEND_FindClosestReachableNode(bot, BOTNODE_DENSITY, BOTNODE_ALL);
			if (tmpNode == bot->acebot.next_node)
			{
				bot->acebot.current_node = tmpNode;
				bot->acebot.next_node = path_table[bot->acebot.current_node][bot->acebot.goal_node];
			}
		}
	}

	//bot->acebot.current_node = ;
	//bot->acebot.next_node;
	//bot->acebot.

}


//////////////////////////////////////////////////////////////////////
//ACEND_DebugNodesLocal
// Draws local path (utility function) 
//hypo
//will draw a path from closest nodes to there routable paths
///////////////////////////////////////////////////////////////////////
void ACEND_DebugNodesLocal(void)
{
	vec3_t v;
	float distToTarget;
	edict_t *firstPlayer;
	short j, k = NODE0, i, m = NODE0, iPlyr;
	short current_node, next_node, i2, n;
	static short count[15];

	if (debug_mode) //"botdebug"
	{
		if (debug_mode_origin_ents)
		{
			memset(count, INVALID, sizeof(count));

			for_each_player_not_bot(firstPlayer, iPlyr)
			//firstPlayer = &g_edicts[1];
			//if (for_each_player(firstPlayer)) //hypoo todo: fix player used
			{

				for (j = NODE0; j < numnodes; j++)
				{
					VectorSubtract(firstPlayer->s.origin, nodes[j].origin, v);
					distToTarget = VectorLength(v);
					if (distToTarget <= 192)
					{
						ACEND_ShowNode(j, 2); //hypov8 show closest node
						k++;
						if (k == 15) //only do 15 nodes
						{
							safe_cprintf(firstPlayer, PRINT_MEDIUM, "*ERROR* more than 15 nodes in your area\n");
							break;
						}
					}
				}

				current_node = ACEND_FindClosestReachableNode(firstPlayer, 64, BOTNODE_ALL);
				if (current_node != INVALID)
				{
					m = NODE0;
					n = NODE0;
					for (i = NODE0; i < 1000; i++)
					{
						if (path_table[current_node][i] != -1)
						{
							next_node = path_table[current_node][i];
							if (next_node <= -2)
								break;
							for (i2 = NODE0; i2 < (short)15; i2++)
							{
								if (count[i2] == next_node)
									break;
							}
							if (i2 == (short)15)
							{
								count[n] = next_node;
								gi.WriteByte(svc_temp_entity);
								gi.WriteByte(TE_BFG_LASER);
								gi.WritePosition(nodes[current_node].origin);
								gi.WritePosition(nodes[next_node].origin);
								gi.multicast(nodes[current_node].origin, MULTICAST_PVS);
								//current_node = next_node;
								//next_node = path_table[current_node][goal_node];
								m++;
								n++;
								if (m > (short)15) //add hypov8 draw short paths
									break;
							}
						}
					}
				}

				break; //only use first valid player "for_each_player_not_bot"
			}
		}
	}
}
