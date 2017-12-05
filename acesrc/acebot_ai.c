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
// Hold fire with RL/BFG?
//hypov8 match ACEAI_VisibleBot(). except for glass/fence
///////////////////////////////////////////////////////////////////////
static qboolean ACEAI_CheckShot(edict_t *self)
{
	trace_t tr;
	vec3_t	spot1;
	vec3_t	spot2;

	self->acebot.aimHead = 0;

	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(self->enemy->s.origin, spot2);
	spot2[2] += self->enemy->viewheight; //36 (60 hight)

	tr = gi.trace(spot1, tv(-8, -8, -60), tv(8, 8, 0), spot2, self, MASK_BOT_ATTACK_SOLID_FENCE);

	// Blocked, do not shoot
	if (tr.fraction != 1.0)
	{
		tr = gi.trace(spot1, tv(-8, -8, -12), tv(8, 8, 12), spot2, self, MASK_BOT_ATTACK_SOLID_FENCE);
		if (tr.fraction == 1.0)
		{
			self->acebot.aimHead = true;
			return true;
		}
		else
			return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (simplified)
///////////////////////////////////////////////////////////////////////
static void ACEAI_ChooseWeapon(edict_t *self)
{
	float range;
	vec3_t v;

	// if no enemy, then what are we doing here?
	if (!self->enemy)
		return;

	// Base selection on distance.
	VectorSubtract(self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);

	switch (self->acebot.randomWeapon)
	{
	case 0:
	{
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckShot(self)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)// Flamethrower.
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		//Tommy
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;
	}
	break;

	case 1:
	{
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckShot(self)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;

		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
	}
	break;

	case 2:
	{
		//Tommy.
		if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
			return;
		//HMG.
		if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
			return;
		//RL.
		/*if (range > 80)*/// Longer range 
		if (ACEAI_CheckShot(self)  //check if fence infront
			&& ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
			return;
		//GL. Only use in certain ranges and only on targets at or below our level
		if (range > 100 && range < 920 && (self->enemy->s.origin[2] - 20) < self->s.origin[2])
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
		//Flamer.
		if (range < 720)
			if (ACEAI_CheckShot(self)  //check if fence infront
				&& ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
	}
	break;
	}//end switch

	if (ACEIT_ChangeWeapon(self, FindItem("Shotgun")))
		return;

	if (ACEIT_ChangeWeapon(self, FindItem("Pistol")))
		return;

	if (ACEAI_CheckShot(self)  //check if fence infront
		&& ACEIT_ChangeWeapon(self, FindItem("Crowbar")))
		return;

	if (/*ACEAI_CheckShot(self)  //check if fence infront
		&& */ACEIT_ChangeWeapon(self, FindItem("Pipe"))) //hypo last resort no checks
		return;

	return;

}


/*
=============
infrontBot
hypov8
returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
qboolean ACEAI_InfrontBot(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	//hypov8 make safe/cash look in 360 deg
	if (strcmp(other->classname, "item_cashroll") == 0
		|| strcmp(other->classname, "item_cashbagsmall") == 0
		|| strcmp(other->classname, "dm_safebag") == 0
		)
		return true;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > -0.25)
		return true;
	return false;
}





///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
static void ACEAI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float weight, best_weight = 0.0;
	edict_t *best = NULL;
	int index;

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, 200);

	while (target)
	{
		if (target->classname == NULL)
			return;

		// Missle avoidance code
		// Set our movetarget to be the rocket or grenade fired at us. 
		if (strcmp(target->classname, "rocket") == 0 || strcmp(target->classname, "grenade") == 0)
		{
			//if(debug_mode) 
			//debug_printf("ROCKET ALERT!\n");

			// acebot ToDo: hypo add player as rocket  target, so strafe/dodge is correct
			//need to work out when to dodge left or right?
			//self->acebot.rocketOwner = target->owner>
			//

			self->movetarget = target;
			return;
		}

		if (ACEIT_IsReachable(self, target->s.origin) && target->solid != SOLID_NOT)
		{
			if (ACEAI_InfrontBot(self, target))
			{
				index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeed(self, index, target->timestamp, target->spawnflags);

				if (weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, 200); //true=bot
	}

	if (best_weight)
	{

		self->movetarget = best;

		if (debug_mode && self->goalentity != self->movetarget && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s selected a %s for SR goal.\n", self->client->pers.netname, self->movetarget->classname);

		self->goalentity = best;
	}

}



///////////////////////////////////////////////////////////////////////
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
///////////////////////////////////////////////////////////////////////
static qboolean ACEAI_PickShortRangeGoalSpawned(edict_t *self)
{
	edict_t *target;
	float weight, best_weight = 0.0;
	edict_t *best = NULL;
	int index;
	qboolean goal = false;
	int localWeaponDist = 416;

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, localWeaponDist);

	while (target)
	{
		if (target->classname == NULL)
			return goal;

		if (ACEIT_IsReachable(self, target->s.origin) && target->solid != SOLID_NOT)
		{
			//if (ACEAI_InfrontBot(self, target))
			{
				index = ACEIT_ClassnameToIndex(target->classname, target->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeedSpawned(self, index, target->timestamp, target->spawnflags);

				if (weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}

		// next target
		target = findradius(target, self->s.origin, localWeaponDist); //true=bot
	}

	if (best_weight)
	{

		self->movetarget = best;

		if (debug_mode && self->goalentity != self->movetarget && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s selected a %s for SR Spawn goal.\n", self->client->pers.netname, self->movetarget->classname);

		self->goalentity = best;
		goal = true;
	}
	return goal;
}










//hypov8 add
//count if we picked up a new wep. should we change to it?
static qboolean ACEAI_WeaponCount(edict_t *self)
{
	int			i;
	gitem_t		*it;
	int wepCount = 0;

	for (i = 1; i <= 20/*MAX_ITEMS*/; i++) //hypov8 last wep is item #16, change if we get more weps 
	{
		if (!self->client->pers.inventory[i])
			continue;

		it = &itemlist[i];
		if (!it->use)
			continue;

		if (!(it->flags & IT_WEAPON))
			continue;

		wepCount++;
	}

	if (wepCount > self->acebot.num_weps)
	{
		self->acebot.num_weps = wepCount;
		return true;
	}
	return false;
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
	int current_node, goal_node = INVALID;
	edict_t *goal_ent = NULL;
	float cost;
	
	// look for a target 
	current_node = ACEND_FindClosestReachableNode(self,BOTNODE_DENSITY,BOTNODE_ALL);

	self->acebot.current_node = current_node;
	
	if(current_node == -1)
	{
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		self->acebot.goal_node = -1;
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
		
		if (cost == INVALID) 
			continue;

		if(cost < 2) // ignore invalid and very short hops
			continue;
	
		weight = ACEIT_ItemNeed(self, item_table[i].item, 0.0, 0);
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
		self->acebot.goal_node = INVALID;
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
			debug_printf("%s did not find a LR goal, wandering.\n",self->client->pers.netname);
		return; // no path? 
	}
	
	// OK, everything valid, let's start moving to our goal.
	self->acebot.state = BOTSTATE_MOVE;
	self->acebot.tries = 0; // Reset the count of how many times we tried this goal
	 
	if (goal_ent != NULL && debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
		debug_printf("%s selected a %s at node %d for LR goal.\n",self->client->pers.netname, goal_ent->classname, goal_node);

	ACEND_SetGoal(self,goal_node);

}

//hypov8 add
/*
=============
ACEAI_VisibleBot

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
static qboolean ACEAI_VisibleBot(edict_t *self, edict_t *other)
{
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace_trans;// , trace_alpha;
	//vec3_t vec3_org = { 0, 0, 0 };

	VectorCopy(self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy(other->s.origin, spot2);
	spot2[2] += other->viewheight;

	trace_trans = gi.trace(spot1, tv(-8, -8, -8), tv(8, 8, 8), spot2, self, MASK_OPAQUE | CONTENTS_TRANSLUCENT); //hypov8
	
	if (trace_trans.fraction == 1.0)
	{
		return true;
	}
	else
	{
		if (trace_trans.contents & CONTENTS_ALPHA)
			return true;
		//trace_alpha = gi.trace(spot1, vec3_org, vec3_org, spot2, self, MASK_OPAQUE | CONTENTS_ALPHA); //hypov8
		//if (trace_alpha.fraction == 1.0)
		//	return true;
	}

	//if (self )
	//{
		
	//	self->client->weaponstate = WEAPON_READY;
		//self->client->pers.weapon ==wea;
	//}


	return false;
}

static int botRankPlayerNum[MAX_CLIENTS];
static int botRankScores[MAX_CLIENTS];

static void ACEAI_GetScores()
{
	int		i, j, k;
	int		score, total;
	
	// sort the clients by score
	total = 0;
	for (i = 0; i < num_players; i++)
	{
		if (players[i] == NULL
			|| players[i]->solid == SOLID_NOT
			|| players[i]->movetype == MOVETYPE_NOCLIP
			|| players[i]->client->invincible_framenum > level.framenum)
			continue;

		if (players[i]->client->pers.spectator == SPECTATING)
			continue;

		score = players[i]->client->resp.score;

		for (j = 0; j<total; j++)
		{
			if (score > botRankScores[j])
				break;
		}
		for (k = total; k>j; k--)
		{
			botRankPlayerNum[k] = botRankPlayerNum[k - 1];
			botRankScores[k] = botRankScores[k - 1];
		}
		botRankPlayerNum[j] = i;
		botRankScores[j] = score;
		total++;
	}

}

static qboolean ACEAI_PickOnBestPlayer(int playerNum)
{
	int tmpScore;
	if (playerNum == botRankPlayerNum[0])
	{
		tmpScore = botRankScores[0] - botRankScores[1];
		if (tmpScore > 3)
		{
			if (debug_mode && level.framenum %10 == 0)
				debug_printf("    ******Best Player****** Targeted by bots.\n");
	
			return true;
		}
	}
	return false;
}


/*
=============
infrontEnem

returns 1 if other is in front (in sight) of self
hypov8 look more to side for items was 0.2
0 must be 90 deg???
=============
*/
qboolean ACEAI_InfrontEnemy(edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot, fov;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(other->s.origin, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);
	//hypov8
	fov = (4 - sv_botskill->value) / 2.25 - 1;	//-1 to 0.77 
	//1.0 is dead ahead
	if (dot > fov)
		return true;
	return false;
}

///////////////////////////////////////////////////////////////////////
// Scan for enemy (simplifed for now to just pick any visible enemy)
///////////////////////////////////////////////////////////////////////
static qboolean ACEAI_FindEnemy(edict_t *self)
{
	int i;
	int	j = -1;
	//char *namex;
	float range, range_tmp = 0;
	vec3_t v;
	//int scores[MAX_CLIENTS];

	if (sv_botskill->value < 0.0f) sv_botskill->value = 0.0f;
	if (sv_botskill->value > 4.0f) sv_botskill->value = 4.0f;

	//use player score to attack them harder
	 ACEAI_GetScores();

	for (i = 0; i < num_players; i++)
	{
		if(players[i] == NULL )
			continue;

		players[i]->acebot.hunted = false;

		if( players[i] == self 
			|| players[i]->solid == SOLID_NOT
			|| players[i]->movetype == MOVETYPE_NOCLIP
			|| players[i]->flags & FL_GODMODE //addhypov8
			|| players[i]->client->invincible_framenum > level.framenum)
		   continue;

		//on same team
		if (teamplay->value && self->client->pers.team == players[i]->client->pers.team)
			continue;

		//reset to no players stop bot hunting same player after death
		if (self->acebot.old_target == i && players[i]->health < 1)
		{
			self->acebot.old_target = -1;
			continue;
		}

		if(!players[i]->deadflag 
			&& ACEAI_VisibleBot(self, players[i])
			&& gi.inPVS(self->s.origin, players[i]->s.origin)
			)
		{
			if (players[i]->acebot.is_bot == 0 && ACEAI_PickOnBestPlayer(i)) //hypov8 add. stop top players :)
			{
				self->enemy = players[i];
				self->acebot.botNewTargetTime = 0;
				players[i]->acebot.hunted = true;
				return true;
			}
			else
			//just keep shooting last target for now
			if (self->acebot.old_target == i)
			{
				self->enemy = players[i];
				if (self->client->weaponstate == WEAPON_FIRING)
					return true;
				else if (!ACEAI_InfrontEnemy(self, players[i])) //hypo stop bot turning a full 360? using skill %))
					return false;
				return true;
			}

			//hypo dont add a new target if not infront
			if (!ACEAI_InfrontEnemy(self, players[i])) //hypo stop bot turning a full 360? using skill %))
				continue;

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
			if (self->client->weaponstate == WEAPON_FIRING /* || underAttack*/) //hypov8 ToDo: shoot enemy faster if being attacked
				self->acebot.botNewTargetTime = level.time +((4 - sv_botskill->value) / 4);//hypov8 shoot next closer player sooner
			else
				self->acebot.botNewTargetTime = level.time +(4 - sv_botskill->value); //give player 2 seconds(default) leway between seeing n being shot
			return false; //self->client->weaponstate == WEAPON_FIRING;
		}

		self->enemy = players[j];
		return true;
	}
	return false;
  
}

///////////////////////////////////////////////////////////////////////
// Choose the best weapon for bot (hypo)
///////////////////////////////////////////////////////////////////////
static void ACEAI_PreChooseWeapon(edict_t *self)
{	
	switch (self->acebot.randomWeapon)
	{
	case 0:
		{
			// always favor the HMG
			if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
		}
		break;
	case 1:
		{
			// always favor the boozooka
			if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		}
		break;
	case 2:
	default:
		{
			// always favor the tommy
			if (ACEIT_ChangeWeapon(self, FindItem("Tommygun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Heavy machinegun")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Bazooka")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("Grenade Launcher")))
				return;
			else if (ACEIT_ChangeWeapon(self, FindItem("FlameThrower")))
				return;
		}
		break;
	} //end switch

	//no weps so chose shotty?
	if (ACEIT_ChangeWeapon(self, FindItem("Shotgun")))
		return;

}

//hypov8 bot debug, stops goal move etc
//#define HYPODEBUG_BOTS

///////////////////////////////////////////////////////////////////////
// Main Think function for bot
///////////////////////////////////////////////////////////////////////
void ACEAI_Think(edict_t *self)
{
	usercmd_t	ucmd;
	vec3_t		v;
	float		tmpTimeout, velo;

	VectorCopy(self->client->ps.viewangles, self->s.angles);
	VectorSet(self->client->ps.pmove.delta_angles, 0, 0, 0);
	memset(&ucmd, 0, sizeof(ucmd));
	self->enemy = NULL;
	self->movetarget = NULL;


	if (self->acebot.isMovingUpPushed)
	{ 
		if (self->groundentity && self->acebot.trigPushTimer < level.framenum) //allow some time for lunch
			self->acebot.isMovingUpPushed = false;
	}
#if 0

	if (!self->deadflag && !self->acebot.isMovingUpPushed /*nodes[self->acebot.last_node].type != BOTNODE_TELEPORTER*/)
	{	//hypov8 add set velocity to last frame difference, getting it wrong for some reason
		int vel;
		float distFromLastFrame;
		vec3_t		forward, right;
		vec3_t		offset;
		vec3_t tmp1, tmp2, tmp3;

		distFromLastFrame = VectorDistance(self->acebot.oldOrigin, self->s.origin);
		VectorSubtract(self->acebot.oldOrigin, self->s.origin, tmp2);
		vectoangles(tmp2, tmp3);
		AngleVectors(tmp3, forward, right, NULL);

		VectorSet(offset, 0, 0, 0);
		G_ProjectSource(self->acebot.oldOrigin, self->s.origin, offset, forward, right); //start?
		VectorScale(forward, -distFromLastFrame * 10, tmp1);

		VectorCopy(tmp1, self->velocity);
		//SV_CheckVelocity(self); // dont mover to fast

		for (vel = 0; vel<3; vel++)
		{
			if (self->velocity[vel] > sv_maxvelocity->value)
			{
				debug_printf("cap vel\n");

				self->velocity[vel] = sv_maxvelocity->value;
			}
			else if (self->velocity[vel] < -sv_maxvelocity->value)
			{
				debug_printf("cap vel\n");
				self->velocity[vel] = -sv_maxvelocity->value;
			}
		}

	}

#endif

	//hypov8
	if (self->acebot.is_crate)
	{
		if (level.framenum > self->acebot.crate_time)
			self->acebot.is_crate = false;
	}

	if (level.framenum > self->acebot.ladder_time)
		self->acebot.isOnLadder = false; //hypo stop bot attacking on ladders


	// Force respawn 
	if (self->deadflag)
	{
		self->client->buttons = 0;
		ucmd.buttons = BUTTON_ATTACK;
		//VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement
		if (self->velocity[2] > 0)
			self->velocity[2] = 0;
		//hypo add this to stop bots moving while dead. may need to reset some values?
		ClientThink(self, &ucmd);
		self->nextthink = level.time + BOTFRAMETIME;
		return;
	}

#ifndef HYPODEBUG_BOTS
	if (self->acebot.state == BOTSTATE_WANDER && self->acebot.wander_timeout < level.time)
		ACEAI_PickLongRangeGoal(self); // pick a new long range goal
#endif
	//check if bot is moving.

	//if (VectorCompare(self->s.origin, self->acebot.oldOrigin) == 0)
	VectorSubtract(self->acebot.oldOrigin, self->s.origin, v);
	velo = VectorLength(v);
	if ( velo > 2) //if (VectorLength(self->velocity) > 37) //
		self->acebot.suicide_timeout = level.time + 10.0;

	tmpTimeout = self->acebot.suicide_timeout - level.time;

	if (tmpTimeout < 6 && self->acebot.isOnLadder)
	{
		self->acebot.suicide_timeout = level.time + 10.0;
		self->s.angles[YAW] += 180.0;
		ucmd.forwardmove = BOT_FORWARD_VEL;
		self->acebot.isOnLadder = false;

		ClientThink(self, &ucmd);
		self->nextthink = level.time + BOTFRAMETIME;
		return;
	}
	// Kill the bot if completely stuck somewhere
	if (tmpTimeout < 0)
	{
		self->flags &= ~FL_GODMODE; //hypov8 added. shown as player killed them selves now
		self->health = 0;
		meansOfDeath = MOD_BOT_SUICIDE;		//hypov8 added. shown as player killed them selves now
		VectorSet(self->velocity, 0, 0, 0);	//hypo stop movement

		player_die(self, self, self, 100000, vec3_origin, 0, 0); //hypov8 add null
	}
#ifndef HYPODEBUG_BOTS
	// Find any short range goal
	if (!self->acebot.isMovingUpPushed)
		ACEAI_PickShortRangeGoal(self);


	if (ACEAI_WeaponCount(self) /*self->client->pers.inventory != self->acebot.num_weps*/)
		ACEAI_PreChooseWeapon(self);


	//allow some extra time to serch for a better wep
	if (strcmp(self->client->pers.weapon->classname,"weapon_pistol") == 0 
		&& self->acebot.spawnedTime >level.framenum
		&& !self->acebot.isMovingUpPushed)
	{
		if (/*self->client->pers.inventory[ITEMLIST_SPISTOL] == 0
			|| */self->client->pers.inventory[ITEMLIST_SHOTGUN] == 0
			&& self->client->pers.inventory[ITEMLIST_TOMMYGUN] == 0
			&& self->client->pers.inventory[ITEMLIST_GRENADELAUNCHER] == 0
			&& self->client->pers.inventory[ITEMLIST_FLAMETHROWER] == 0
			&& self->client->pers.inventory[ITEMLIST_BAZOOKA] == 0
			&& self->client->pers.inventory[ITEMLIST_HEAVYMACHINEGUN] == 0
			)
		{
			if (ACEAI_PickShortRangeGoalSpawned(self))
			{
				self->acebot.botNewTargetTime = level.time + .1;
			}
		}
	}

	// Look for enemies //hypo serch weps if reloading etc. can make it serch while fireing
	if ((self->client->weaponstate == WEAPON_READY || self->client->weaponstate == WEAPON_FIRING)
		&& ACEAI_FindEnemy(self)
		&& self->acebot.botNewTargetTime <= level.time //hypo add timer for bot to not own so much
		&& !self->acebot.isOnLadder)  //hypo stop bot attacking on ladders
	{
		ACEAI_ChooseWeapon(self);
		ACEMV_Attack(self, &ucmd);
	}
	else //if (!self->acebot.isMovingUpPushed)
	{
		// Execute the move, or wander
		if (self->acebot.state == BOTSTATE_WANDER)
			ACEMV_Wander(self, &ucmd);
		else if (self->acebot.state == BOTSTATE_MOVE)
			ACEMV_Move(self, &ucmd);
	}
	//debug_printf("State: %d\n",self->state);
#endif
	// set approximate ping
	ucmd.msec = 100;// 75 + floor(random() * 25) + 1;

	// show random ping values in scoreboard
	//self->client->ping = ucmd.msec;
	self->client->ping = 0; //hypo

	//hypo set old origin for movement calculations
	VectorCopy(self->s.origin, self->acebot.oldOrigin);

	// set bot's view angle
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);

	if (self->acebot.isMovingUpPushed)
	{
		ucmd.forwardmove = 0;
		ucmd.sidemove = 0;
		ucmd.upmove = 0;
	}

#ifdef HYPODEBUG_BOTS
	{
		VectorCopy( self->acebot.oldAngles,self->s.angles);
		self->s.angles[YAW] += 4.2;
		if (self->s.angles[YAW] >= 180)
			self->s.angles[YAW] = self->s.angles[YAW] - 360;

		VectorCopy( self->s.angles,self->acebot.oldAngles);
		ucmd.forwardmove = BOT_FORWARD_VEL;
		ucmd.sidemove = 0;
		ucmd.upmove = 0;
		ucmd.angles[PITCH] = ANGLE2SHORT(0.0f);
		ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
		ucmd.angles[ROLL] = ANGLE2SHORT(0.0f);
	}
#endif

	// send command through id's code
	ClientThink(self, &ucmd);

	self->nextthink = level.time + BOTFRAMETIME;
	//self->nextthink = (level.framenum*FRAMETIME) + FRAMETIME;
}
