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
//  acebot_ai.c -      This file contains all of the 
//                     AI routines for the ACE II bot.
//
//
// NOTE: I went back and pulled out most of the brains from
//       a number of these functions. They can be expanded on 
//       to provide a "higher" level of AI. 
////////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "..\m_player.h"

#include "acebot.h"

///////////////////////////////////////////////////////////////////////
// Main Think function for bot
///////////////////////////////////////////////////////////////////////
void ACEAI_Think (edict_t *self)
{
	usercmd_t	ucmd;
	qboolean warmupTime =0;
	/*
	if (level.time > 30 && (level.framenum > (level.startframe + ((int)timelimit->value) * 600 - 1)))
	{
		//self->client->ps.pmove.pm_type = PM_NORMAL;
	//	self->client->pers.spectator == PLAYING;
	}
	else
	{
		//self->client->ps.pmove.pm_type = PM_SPECTATOR;
		self->client->pers.spectator == SPECTATING;
		self->nextthink = level.time + 50; //hypov8 delay think
		warmupTime = 1;
	}
	*/

	// Set up client movement
	VectorCopy(self->client->ps.viewangles,self->s.angles);
	VectorSet (self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset (&ucmd, 0, sizeof (ucmd));
	self->enemy = NULL;
	self->movetarget = NULL;
	
	if (warmupTime == 1)
		return;

	// Force respawn 
	if (self->deadflag)
	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
	}
	
	if(self->state == BOTSTATE_WANDER && self->wander_timeout < level.time)
	  ACEAI_PickLongRangeGoal(self); // pick a new long range goal

	// Kill the bot if completely stuck somewhere
	if(VectorLength(self->velocity) > 37) //
		self->suicide_timeout = level.time + 10.0;

	if(self->suicide_timeout < level.time)
	{
		self->health = 0;
		player_die (self, self, self, 100000, vec3_origin, 0, 0); //hypov8 add null
	}
	
	// Find any short range goal
	ACEAI_PickShortRangeGoal(self);
	
	// Look for enemies

	if (ACEAI_FindEnemy(self) && self->acebot.botNewTargetTime <= level.framenum) //hypo add timer for bot to not own so much
	{	
		ACEAI_ChooseWeapon(self);
		ACEMV_Attack (self, &ucmd);
	}
	else
	{
		// Execute the move, or wander
		if(self->state == BOTSTATE_WANDER)
			ACEMV_Wander(self,&ucmd);
		else if(self->state == BOTSTATE_MOVE)
			ACEMV_Move(self,&ucmd);
	}
	
	//debug_printf("State: %d\n",self->state);

	// set approximate ping
	ucmd.msec = 75 + floor (random () * 25) + 1;

	// show random ping values in scoreboard
	//self->client->ping = ucmd.msec;
	self->client->ping = 0; //hypo

	// set bot's view angle
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);
	
	// send command through id's code
	ClientThink (self, &ucmd);
	
	self->nextthink = level.time + FRAMETIME;
}

///////////////////////////////////////////////////////////////////////
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickLongRangeGoal(edict_t *self)
{

	int i;
	short int node; //hypo
	float weight,best_weight=0.0;
	int current_node,goal_node;
	edict_t *goal_ent;
	float cost;
	
	// look for a target 
	current_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_ALL);

	self->current_node = current_node;
	
	if(current_node == -1)
	{
		self->state = BOTSTATE_WANDER;
		self->wander_timeout = level.time + 1.0;
		self->goal_node = -1;
		return;
	}

	///////////////////////////////////////////////////////
	// Items
	///////////////////////////////////////////////////////
	for(i=0;i<num_items;i++)
	{
		if(item_table[i].ent == NULL) // ignore items that are not there.
			continue;
		if (!item_table[i].ent->solid)
			continue;
		if (item_table[i].ent->solid == SOLID_NOT) // ignore items that are not there.
			continue;

		cost = ACEND_FindCost(current_node,item_table[i].node);
		
		if (node == INVALID) 
			continue;

		if(cost == INVALID || cost < 2) // ignore invalid and very short hops
			continue;
	
		weight = ACEIT_ItemNeed(self, item_table[i].item);
#ifdef NOT_ZOID
		// If I am on team one and I have the flag for the other team....return it
		if(ctf->value && (item_table[i].item == ITEMLIST_FLAG2 || item_table[i].item == ITEMLIST_FLAG1) &&
		  (self->client->resp.ctf_team == CTF_TEAM1 && self->client->pers.inventory[ITEMLIST_FLAG2] ||
		   self->client->resp.ctf_team == CTF_TEAM2 && self->client->pers.inventory[ITEMLIST_FLAG1]))
			weight = 10.0;
#endif
		weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there
				
		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = item_table[i].node;
			goal_ent = item_table[i].ent;
		}
	}

	///////////////////////////////////////////////////////
	// Players
	///////////////////////////////////////////////////////
	// This should be its own function and is for now just
	// finds a player to set as the goal.
	for(i=0;i<num_players;i++)
	{
		if(players[i] == self)
			continue;

		node = ACEND_FindClosestReachableNode(players[i],BOTNODE_DENSITY,BOTNODE_ALL);
		if (node == INVALID) ////hypo added to stop below returning crapola 
			continue;

		cost = ACEND_FindCost(current_node, node);
		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;
#ifdef NOT_ZOID
		// Player carrying the flag?
		if(ctf->value && (players[i]->client->pers.inventory[ITEMLIST_FLAG2] || players[i]->client->pers.inventory[ITEMLIST_FLAG1]))
		  weight = 2.0;
		else
#endif
		  weight = 0.3; 
		
		weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there
		
		if(weight > best_weight)
		{		
			best_weight = weight;
			goal_node = node;
			goal_ent = players[i];
		}	
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == INVALID)
	{
		self->goal_node = INVALID;
		self->state = BOTSTATE_WANDER;
		self->wander_timeout = level.time + 1.0;
		if(debug_mode)
			debug_printf("%s did not find a LR goal, wandering.\n",self->client->pers.netname);
		return; // no path? 
	}
	
	// OK, everything valid, let's start moving to our goal.
	self->state = BOTSTATE_MOVE;
	self->tries = 0; // Reset the count of how many times we tried this goal
	 
	if(goal_ent != NULL && debug_mode)
		debug_printf("%s selected a %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node);

}

///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
void ACEAI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float weight,best_weight=0.0;
	edict_t *best;
	int index;
	
	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);
	
	while(target)
	{
		if(target->classname == NULL)
			return;
		
		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if(strcmp(target->classname,"rocket")==0 || strcmp(target->classname,"grenade")==0)
		{
			if(debug_mode) 
				debug_printf("ROCKET ALERT!\n");

// acebot ToDo: hypo add player as rocket  target, so strafe/dodge is correct
			//need to work out when to dodge left or right?
			//self->acebot.rocketOwner = target->owner>
			//

			self->movetarget = target;
			return;
		}
	
		if (ACEIT_IsReachable(self,target->s.origin))
		{
			if (infront(self, target))
			{
				index = ACEIT_ClassnameToIndex(target->classname);
				weight = ACEIT_ItemNeed(self, index);
				
				if(weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, 200);
	}

	if(best_weight)
	{
		self->movetarget = best;
		
		if(debug_mode && self->goalentity != self->movetarget)
			debug_printf("%s selected a %s for SR goal.\n",self->client->pers.netname, self->movetarget->classname);
		
		self->goalentity = best;

	}

}

///////////////////////////////////////////////////////////////////////
// Scan for enemy (simplifed for now to just pick any visible enemy)
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_FindEnemy(edict_t *self)
{
	int i;
	int	j = -1;
	//char *namex;
	float range, range_tmp;
	vec3_t v;

#if 1 //hypo used to test players info
	for (i = 0; i < num_players; i++)
	{
		if (players[i] == NULL || players[i] == self ||
			players[i]->solid == SOLID_NOT)

			//namex = players[i]->client->pers.netname;

			//safe_bprintf(PRINT_HIGH, "player =s.\n", namex);
			continue;
	}
#endif


	
	for (i = 0; i < num_players; i++)
	{
		if(players[i] == NULL || players[i] == self || 
		   players[i]->solid == SOLID_NOT)
		   continue;

#ifdef NOT_ZOID	
		if(ctf->value && 
		   self->client->resp.ctf_team == players[i]->client->resp.ctf_team)
		   continue;
#endif
		if (teamplay->value && self->client->pers.team == players[i]->client->pers.team)
			continue;

		if (self->acebot.old_target == i && players[i]->health < 1)
		{
			self->acebot.old_target = -1; //reset to no players stop bot hunting same player after death
			continue;
		}


		if(!players[i]->deadflag && visible(self, players[i]) && gi.inPVS (self->s.origin, players[i]->s.origin))
		{
			//self->enemy = players[i];
//hypov8

//end
			// Base selection on distance.
			VectorSubtract(self->s.origin, players[i]->s.origin, v);
			range = VectorLength(v);
			
			if (range_tmp)
			{
				if (range < range_tmp)
					j = i; //j is new closer player
			}
			else
			{
				range_tmp = range;
				j = i;
			}

			//return true;
		}
		//continue;
	}

	if (j != -1)
	{
		self->acebot.new_target = j;
		if (self->acebot.new_target != self->acebot.old_target)
		{
			self->acebot.old_target = j;
			self->acebot.botNewTargetTime = level.framenum + 20; //give player 2 seconds leway between seeing n being shot
			return false;
		}



		self->enemy = players[j];
		return true;
	}
	return false;
  
}

///////////////////////////////////////////////////////////////////////
// Hold fire with RL/BFG?
///////////////////////////////////////////////////////////////////////
qboolean ACEAI_CheckShot(edict_t *self)
{
	trace_t tr;

	tr = gi.trace (self->s.origin, tv(-8,-8,-8), tv(8,8,8), self->enemy->s.origin, self, MASK_OPAQUE);
	
	// Blocked, do not shoot
	if (tr.fraction != 1.0)
		return false; 
	
	return true;
}

///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (simplified)
///////////////////////////////////////////////////////////////////////
void ACEAI_ChooseWeapon(edict_t *self)
{	
	float range;
	vec3_t v;
	
	// if no enemy, then what are we doing here?
	if(!self->enemy)
		return;
	
	// always favor the HMG
	if(ACEIT_ChangeWeapon(self,FindItem("Heavy machinegun")))
		return;

	// Base selection on distance.
	VectorSubtract (self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);
		
	// Longer range 
	if(range > 150)
	{
/*		// choose BFG if enough ammo
		if(self->client->pers.inventory[ITEMLIST_CELLS] > 50)
			if(ACEAI_CheckShot(self) && ACEIT_ChangeWeapon(self, FindItem("bfg10k")))
				return;*/

		if(ACEAI_CheckShot(self) && ACEIT_ChangeWeapon(self,FindItem("Bazooka")))
			return;
	}
	
	// Only use GL in certain ranges and only on targets at or below our level
	if(range > 100 && range < 500 && self->enemy->s.origin[2] - 20 < self->s.origin[2])
		if(ACEIT_ChangeWeapon(self,FindItem("Grenade Launcher")))
			return;

	// Flamethrower wasn't in so I've sorted it.
	if (range < 256 && self->enemy->s.origin[2] - 20 < self->s.origin[2])
	if(ACEIT_ChangeWeapon(self,FindItem("FlameThrower")))
		return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("Tommygun")))
		return;
	
	// Only use FT at short range and only on targets at or below our level
	if(range < 512 && self->enemy->s.origin[2] - 20 < self->s.origin[2])
		if(ACEIT_ChangeWeapon(self,FindItem("Grenade Launcher")))
			return;

	if(ACEIT_ChangeWeapon(self,FindItem("Shotgun")))
		return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("Pistol")))
   	   return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("Crowbar")))
   	   return;
	
	if(ACEIT_ChangeWeapon(self,FindItem("Pipe")))
   	   return;
	
	return;

}
