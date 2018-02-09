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
//  acebot_items.c - This file contains all of the 
//                   item handling routines for the 
//                   ACE bot, including fact table support
//           
///////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "acebot.h"

int	num_players = 0;
int num_items = 0;
int nodeFileComplet =0;
int botsRemoved = 0;
int num_bots = 0;


int num_items_tmp = 0;				//add hypov8 store old item ammount, needed???
qboolean num_items_changed = false;	//add hypov8 changed when an item it droped etc

item_table_t item_table[MAX_EDICTS];
edict_t *players[MAX_CLIENTS];		// pointers to all players in the game

void ACEIT_PlayerCheckCount()
{
	edict_t	*bot;
	int i;
	int countBot = 0, countPlayer = 0;
	char botname[16];

	if ((int)sv_bot_max_players->value != 0)
	{
		for_each_player_inc_bot(bot, i)
		{
			if (bot->acebot.is_bot)
				countBot++;
			else
				countPlayer++;
		}

		if (countBot > 0 && countPlayer > 1)
		{
			//for (i = 1; i <= botsRemoved; i++)
			if ((countBot + countPlayer) > (int)sv_bot_max_players->value)
			{
				for_each_player_inc_bot(bot, i)
				{
					if (!bot->acebot.is_bot) 
						continue;

					if (bot->client)
					{
						strcpy(botname, bot->client->pers.netname);
						ACESP_RemoveBot(botname);
						botsRemoved++;
						return;

					}
				}
			}
		}
	}
}

void ACEIT_PlayerCheckAddCount()
{
	edict_t	*bot;
	int i;
	int countBot = 0, countPlayer = 0;

	if (botsRemoved > 0)
	{
		if (sv_bot_max_players->value > 0)
		{
			for_each_player_inc_bot(bot, i)
			{
				if (bot->acebot.is_bot)
					countBot++;
				else
					countPlayer++;
			}

			if ((int)sv_bot_max_players->value > (countBot + countPlayer))
			{
				botsRemoved--;
				if (botsRemoved < 0) botsRemoved = 0;
				ACESP_SpawnRandomBot('\0', "\0", "\0", NULL);
			}
		}
	}

}
///////////////////////////////////////////////////////////////////////
// Add the player to our list
///////////////////////////////////////////////////////////////////////
void ACEIT_PlayerAdded(edict_t *ent)
{
	players[num_players++] = ent;



	gi.dprintf(" Added: %s, Bot Enemy = %i\n", ent->client->pers.netname, num_players);
	//safe_bprintf(PRINT_HIGH, "Working ... 1\n");

	if (!ent->acebot.is_bot)
		ACEIT_PlayerCheckCount();
	else
		num_bots++;
}

///////////////////////////////////////////////////////////////////////
// Remove player from list
///////////////////////////////////////////////////////////////////////
void ACEIT_PlayerRemoved(edict_t *ent)
{
	int i;
	int pos =0;

	// watch for 0 players
	if(num_players == 0)
		return;

	// special cas for only one player
	if(num_players == 1)
	{	
		num_players = 0;
		gi.dprintf(" Removed: %s, Inuse = %i\n", ent->client->pers.netname, num_players);
		return;
	}

	// Find the player
	for(i=0;i<num_players;i++)
		if(ent == players[i])
			pos = i;

	// decrement
	for(i=pos;i<num_players-1;i++)
		players[i] = players[i+1];

	num_players--;
	gi.dprintf(" Removed: %s, Inuse = %i\n", ent->client->pers.netname, num_players);

	//hypo add back in a bot if it was removed
	if (!ent->acebot.is_bot)
		ACEIT_PlayerCheckAddCount();
}

///////////////////////////////////////////////////////////////////////
// Can we get there?
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsReachable(edict_t *self, vec3_t goal)
{
	trace_t trace;
	vec3_t goal_move_dn, player_move_up;
	vec_t jump_height;
	vec3_t minx, maxx;

	//hypo todo: check crouch?

	VectorCopy(self->mins,minx);
	VectorCopy(self->maxs, maxx);
	minx[2] += 18; // Stepsize //hypov8 jump height
	minx[0] = minx[1] = -15; //catching on walls, failed trace
	maxx[0] = maxx[1] = 15; //hypov8 was 15?
	
	//hypov8 calculate jumping
	//jumpv[0] = self->maxs[0];
	//jumpv[1] = self->maxs[1];
	//jumpv[2] = (self->maxs[2]);

	trace = gi.trace(self->s.origin, minx, maxx, goal, self, MASK_BOT_SOLID_FENCE); //hypo can we jump up? minus 12, jump to 60 units hypo: todo
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;

	//hypov8 also check ledges for items?

	jump_height = goal[2] - self->s.origin[2];
	if (jump_height >= 16 && jump_height <= 52 /*&& !self->acebot.is_crate*/)
	{
		VectorCopy(goal, goal_move_dn);
		goal_move_dn[2] -= 15;	//move items down 15 units
		VectorCopy(self->s.origin, player_move_up);
		player_move_up[2] = goal_move_dn[2];

		minx[2] -= 18;
		trace = gi.trace(player_move_up, minx, maxx, goal_move_dn, self, MASK_BOT_SOLID_FENCE); //hypo can we jump up? minus 12, jump to 60 units hypo: todo

		if (trace.allsolid == 0 && trace.startsolid == 0 && trace.fraction == 1.0)
		{
			self->acebot.is_crate= true;
			self->acebot.crate_time = level.framenum + 5;
			//self->nextthink = level.time + 0.03; //hypov8
			return true;

		}
	}

	return false;
}

#if 0 //hypov8 not used??
///////////////////////////////////////////////////////////////////////
// Visiblilty check 
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal)
{
	trace_t trace;
	
	trace = gi.trace (self->s.origin, vec3_origin, vec3_origin, goal, self, MASK_OPAQUE);
	
	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}
#endif


static int ACEIT_ClipNameIndex(gitem_t *item)
{

	if (!strcmp(item->pickup_name, "Pipe"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Crowbar"))
		return CLIP_NONE;
	else if (!strcmp(item->pickup_name, "Pistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "SPistol"))
		return CLIP_PISTOL;
	else if (!strcmp(item->pickup_name, "Shotgun"))
		return CLIP_SHOTGUN;
	else if (!strcmp(item->pickup_name, "Tommygun"))
		return CLIP_TOMMYGUN;
	else if (!strcmp(item->pickup_name, "FlameThrower"))
		return CLIP_FLAMEGUN;
	else if (!strcmp(item->pickup_name, "Bazooka"))
		return CLIP_ROCKETS;
	else if (!strcmp(item->pickup_name, "Grenade Launcher"))
		return CLIP_GRENADES;
	// JOSEPH 16-APR-99
	else if (!strcmp(item->pickup_name, "Heavy machinegun"))
		return CLIP_SLUGS;
	// END JOSEPH

	return (0);
}


///////////////////////////////////////////////////////////////////////
//  Weapon changing support
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
	int			clip_index; //ammon on wep

		
	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return true; 

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		clip_index = ACEIT_ClipNameIndex(item);


		if ((!ent->client->pers.inventory[ammo_index] && !ent->client->pers.weapon_clip[clip_index]) 
			&& !g_select_empty->value)
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;
	
	return true;
}


extern gitem_armor_t jacketarmor_info;
extern gitem_armor_t combatarmor_info;
extern gitem_armor_t bodyarmor_info;

#if 0
///////////////////////////////////////////////////////////////////////
// Check if we can use the armor
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (item->tag == ARMOR_SHARD)
		return true;
	
	// get info on old armor
	if (old_armor_index == ITEM_INDEX(FindItem("Jacket Armor")))
		oldinfo = &jacketarmor_info;
	else if (old_armor_index == ITEM_INDEX(FindItem("Combat Armor")))
		oldinfo = &combatarmor_info;
	else // (old_armor_index == body_armor_index)
		oldinfo = &bodyarmor_info;

	if (newinfo->normal_protection <= oldinfo->normal_protection)
	{
		// calc new armor values
		salvage = newinfo->normal_protection / oldinfo->normal_protection;
		salvagecount = salvage * newinfo->base_count;
		newcount = other->client->pers.inventory[old_armor_index] + salvagecount;

		if (newcount > oldinfo->max_count)
			newcount = oldinfo->max_count;

		// if we're already maxed out then we don't need the new armor
		if (other->client->pers.inventory[old_armor_index] >= newcount)
			return false;

	}

	return true;
}

#endif
///////////////////////////////////////////////////////////////////////
// Determins the NEED for an item
//
// This function can be modified to support new items to pick up
// Any other logic that needs to be added for custom decision making
// can be added here. For now it is very simple.
///////////////////////////////////////////////////////////////////////
float ACEIT_ItemNeed(edict_t *self, int item, float timestamp, int spawnflags)
{
	//gitem_t *itemArmor;

	// Make sure item is at least close to being valid
	if(item < 0 || item > 100)
		return 0.0;
	//return 0.0; //hypo debug wander

	//hypov8 make bot ignore all items if they have cash
	if (teamplay->value == 1)
		if ((self->client->pers.currentcash >= MAX_CASH_PLAYER || self->client->pers.bagcash >= 100))
		{
			if (item != ITEMLIST_SAFEBAG1 && self->client->pers.team == TEAM_1)
				return 0.0;
			if (item != ITEMLIST_SAFEBAG2 && self->client->pers.team == TEAM_2)
				return 0.0;
		}

	//hypov8 calculate if we need the ammo, then the gun
	if (spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
	{
		switch (item)
		{
		// Weapons that are droped. when player dies..
		case ITEMLIST_PISTOL:
		case ITEMLIST_SPISTOL:
		case ITEMLIST_TOMMYGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 0.3; break;
		case ITEMLIST_SHOTGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] < self->client->pers.max_shells) return 0.4; break;
		case ITEMLIST_GRENADELAUNCHER:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Grenades"))] < self->client->pers.max_grenades) return 0.3; break;
		case ITEMLIST_FLAMETHROWER:	if (self->client->pers.inventory[ITEM_INDEX(FindItem("Gas"))] < self->client->pers.max_cells) return 0.6; break;
		case ITEMLIST_BAZOOKA:if (self->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] < self->client->pers.max_rockets)	return 1.5;	break;
		case ITEMLIST_HEAVYMACHINEGUN:if (self->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))] < self->client->pers.max_slugs) return 1.5; break;
		}
	}


	switch (item)
	{
		// Health
	case ITEMLIST_HEALTH_SMALL:
	case ITEMLIST_HEALTH_LARGE:
	case ITEMLIST_ADRENALINE: if (self->health < 100) return 1.0 - (float)self->health / 100.0f; // worse off, higher priority
		break;

		//@@ Integrate
		//		case ITEMLIST_COIL:
		//		case ITEMLIST_LIZZYHEAD:
	case ITEMLIST_CASHROLL:
	case ITEMLIST_CASHBAGLARGE:				//hypov8 let cash fall for 5 sec first
	case ITEMLIST_CASHBAGSMALL:	if (/*timestamp &&*/ timestamp <= level.time + 55) return 4.0;break;

		// Weapons
	case ITEMLIST_CROWBAR: if (!self->client->pers.inventory[item])return 0.1; break;
	case ITEMLIST_PISTOL:
	case ITEMLIST_SPISTOL:
	case ITEMLIST_SHOTGUN:
	case ITEMLIST_TOMMYGUN:
	case ITEMLIST_GRENADELAUNCHER:
	case ITEMLIST_FLAMETHROWER:	if (!self->client->pers.inventory[item]) return 2.8; break; //was 1.8

	case ITEMLIST_BAZOOKA:
	case ITEMLIST_HEAVYMACHINEGUN: if (!self->client->pers.inventory[item]) return 3.0; break; //was 2.0

		// Ammo
	case ITEMLIST_GRENADES: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Grenades"))] < self->client->pers.max_grenades) return 0.3; break;
	case ITEMLIST_SHELLS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Shells"))] < self->client->pers.max_shells) return 0.4; break;
	case ITEMLIST_BULLETS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 0.3; break;
	case ITEMLIST_ROCKETS: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Rockets"))] < self->client->pers.max_rockets)	return 1.5;	break;
	case ITEMLIST_AMMO308: if (self->client->pers.inventory[ITEM_INDEX(FindItem("308cal"))] < self->client->pers.max_slugs) return 1.5; break;
	case ITEMLIST_CYLINDER: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Bullets"))] < self->client->pers.max_bullets) return 1.4; break;
	case ITEMLIST_FLAMETANK: if (self->client->pers.inventory[ITEM_INDEX(FindItem("Gas"))] < self->client->pers.max_cells) return 0.6; break;

		// Armor
	case ITEMLIST_ARMORHELMET:	
		if (self->client->ps.stats[STAT_ARMOR1] < 100) 	return 0.4; 
		else if (self->client->ps.stats[STAT_ARMOR1] > 100
			&&self->client->pers.inventory[ITEMLIST_ARMORHELMETHEAVY] < 100) return 0.4;	break;
	case ITEMLIST_ARMORHELMETHEAVY:	
		if (self->client->pers.inventory[ITEMLIST_ARMORHELMETHEAVY] < 100) return 0.8; break;

	case ITEMLIST_ARMORJACKET:
		if (self->client->ps.stats[STAT_ARMOR2] < 100) 	return 0.4;
		else if (self->client->ps.stats[STAT_ARMOR2] > 100 &&
			self->client->pers.inventory[ITEMLIST_ARMORJACKETHEAVY] < 100) return 0.4;	break;
	case ITEMLIST_ARMORJACKETHEAVY:
		if (self->client->pers.inventory[ITEMLIST_ARMORJACKETHEAVY] < 100) return 0.8; break;

	case ITEMLIST_ARMORLEGS:
		if (self->client->ps.stats[STAT_ARMOR3] < 100) 	return 0.4;
		else if (self->client->ps.stats[STAT_ARMOR3] > 100
			&& self->client->pers.inventory[ITEMLIST_ARMORLEGSHEAVY] < 100) return 0.4;	break;
	case ITEMLIST_ARMORLEGSHEAVY:
		if (self->client->pers.inventory[ITEMLIST_ARMORLEGSHEAVY] < 100) return 0.8; break;


/*		case ITEMLIST_FLAG1:
			// If I am on team one, I want team two's flag
			if(!self->client->pers.inventory[item] && self->client->resp.ctf_team == CTF_TEAM2)
				return 10.0;  
			else 
				return 0.0;

		case ITEMLIST_FLAG2:
			if(!self->client->pers.inventory[item] && self->client->resp.ctf_team == CTF_TEAM1)
				return 10.0;  
			else
				return 0.0;
		*/

		// Bagman
	case ITEMLIST_SAFEBAG1:
		if (teamplay->value == 1)
		{	//deposit cash
			if (self->client->pers.team == TEAM_1 && (self->client->pers.currentcash >= 50 || self->client->pers.bagcash >= 50))
			{
				//ToDo: set long term goal if cash was picked up
				return 4.0;
			}
			else if (self->client->pers.team == TEAM_2)
			{	//if enamy safe has cash and player is not full
				if (team_cash[TEAM_1] > 0 && self->client->pers.bagcash < MAX_BAGCASH_PLAYER)
					return 4.0;
			}
		}
		break;

	case ITEMLIST_SAFEBAG2:
		if (teamplay->value == 1)
		{	//deposit cash
			if (self->client->pers.team == TEAM_2 && (self->client->pers.currentcash >= 50 || self->client->pers.bagcash >= 50))
			{
				//ToDo: set long term goal if cash was picked up
				return 4.0;
			}
			else 
			if (self->client->pers.team == TEAM_1)
			{	//if enamy safe has cash and player is not full
				if (team_cash[TEAM_2] > 0 && self->client->pers.bagcash < MAX_BAGCASH_PLAYER)
					return 4.0;
			}
		}
		break;

		// Mods
	case ITEMLIST_HMG_COOL_MOD:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_COOLING_JACKET)) 
			return 1.2; 
		break;
	case ITEMLIST_PISTOLMOD_DAMAGE:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_DAMAGE))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_RELOAD:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_RELOAD))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_ROF:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_ROF))
			return 0.5;
		break;


//ITEMLIST_SAFEBAG2

/*
// Integrate
case ITEMLIST_FLASHLIGHT:
case ITEMLIST_WATCH:
case ITEMLIST_WHISKEY:

case ITEMLIST_PISTOLMODS:
*/			

	case ITEMLIST_PACK: 
		if (self->client->pers.max_bullets < 300) //must not have pack yet. does not check low ammo..
		return 0.5; break;

		default:
			return 0.0;
			
	}
	return 0.0;
		
}


//hypo add. get gun items after spawning
float ACEIT_ItemNeedSpawned(edict_t *self, int item, float timestamp, int spawnflags)
{
	//gitem_t *itemArmor;

	// Make sure item is at least close to being valid
	if (item < 0 || item > 100)
		return 0.0;

	switch (item)
	{
		// Weapons
	case ITEMLIST_CROWBAR: if (!self->client->pers.inventory[item])return 0.1; break;
	case ITEMLIST_PISTOL:
	case ITEMLIST_SPISTOL:
	case ITEMLIST_SHOTGUN:
	case ITEMLIST_TOMMYGUN:
	case ITEMLIST_GRENADELAUNCHER:
	case ITEMLIST_FLAMETHROWER:	if (!self->client->pers.inventory[item]) return 2.8; break; //was 1.8

	case ITEMLIST_BAZOOKA:
	case ITEMLIST_HEAVYMACHINEGUN: if (!self->client->pers.inventory[item]) return 3.0; break; //was 2.0


	case ITEMLIST_PISTOLMOD_DAMAGE:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_DAMAGE))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_RELOAD:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_RELOAD))
			return 0.5;
		break;
	case ITEMLIST_PISTOLMOD_ROF:
		if (!(self->client->pers.pistol_mods & WEAPON_MOD_ROF))
			return 0.5;
		break;

	default:
		return 0.0;

	}
	return 0.0;

}





///////////////////////////////////////////////////////////////////////
// Convert a classname to its index value
//
// I prefer to use integers/defines for simplicity sake. This routine
// can lead to some slowdowns I guess, but makes the rest of the code
// easier to deal with.
///////////////////////////////////////////////////////////////////////
int ACEIT_ClassnameToIndex(char *classname, int style)
{
	if(strcmp(classname,"item_armor_helmet")==0) 		return ITEMLIST_ARMORHELMET;
	
	if(strcmp(classname,"item_armor_jacket")==0)		return ITEMLIST_ARMORJACKET;

	if(strcmp(classname,"item_armor_legs")==0)		return ITEMLIST_ARMORLEGS;
	
	if(strcmp(classname,"item_armor_helmet_heavy")==0) 		return ITEMLIST_ARMORHELMETHEAVY;
	
	if(strcmp(classname,"item_armor_jacket_heavy")==0)		return ITEMLIST_ARMORJACKETHEAVY;

	if(strcmp(classname,"item_armor_legs_heavy")==0)		return ITEMLIST_ARMORLEGSHEAVY;

/////////////////////////////////////////////////	
	if(strcmp(classname,"weapon_crowbar")==0)		return ITEMLIST_CROWBAR;

	if(strcmp(classname,"weapon_pistol")==0)		return ITEMLIST_PISTOL;

	if(strcmp(classname,"weapon_spistol")==0)		return ITEMLIST_SPISTOL;

	if(strcmp(classname,"weapon_shotgun")==0)		return ITEMLIST_SHOTGUN;
	
	if(strcmp(classname,"weapon_tommygun")==0)		return ITEMLIST_TOMMYGUN;
	
	if(strcmp(classname,"weapon_heavymachinegun")==0)		return ITEMLIST_HEAVYMACHINEGUN;
	
	if(strcmp(classname,"weapon_grenadelauncher")==0)		return ITEMLIST_GRENADELAUNCHER;

	if(strcmp(classname,"weapon_bazooka")==0)		return ITEMLIST_BAZOOKA;

	if(strcmp(classname,"weapon_flamethrower")==0)		return ITEMLIST_FLAMETHROWER;

	///////////////////////////////////////
	if(strcmp(classname,"ammo_grenades")==0)		return ITEMLIST_GRENADES;

	if(strcmp(classname,"ammo_shells")==0)		return ITEMLIST_SHELLS;
	
	if(strcmp(classname,"ammo_bullets")==0)		return ITEMLIST_BULLETS;

	if(strcmp(classname,"ammo_rockets")==0)		return ITEMLIST_ROCKETS;

	if(strcmp(classname,"ammo_308")==0)		return ITEMLIST_AMMO308;
	
	if(strcmp(classname,"ammo_cylinder")==0)		return ITEMLIST_CYLINDER;

	if(strcmp(classname,"ammo_flametank")==0)		return ITEMLIST_FLAMETANK;
/////////////////////////////////////////////

	if(strcmp(classname,"item_cashroll")==0)		return ITEMLIST_CASHROLL;

	if(strcmp(classname,"item_cashbaglarge")==0)		return ITEMLIST_CASHBAGLARGE;

	if(strcmp(classname,"item_cashbagsmall")==0)		return ITEMLIST_CASHBAGSMALL;

	if(strcmp(classname,"dm_safebag")==0)
		if (style && style == 1)
			return ITEMLIST_SAFEBAG1;
		else
			return ITEMLIST_SAFEBAG2;

//////////////////////////////////////////////
	if(strcmp(classname,"item_health_sm")==0)		return ITEMLIST_HEALTH_SMALL;

	if(strcmp(classname,"item_health_lg")==0)		return ITEMLIST_HEALTH_LARGE;
	

	if(strcmp(classname,"item_pack")==0)		return ITEMLIST_PACK;

	if(strcmp(classname,"item_adrenaline")==0)		return ITEMLIST_ADRENALINE;

////////////////////////////////////////////////
	if(strcmp(classname,"pistol_mod_damage")==0)	
		return ITEMLIST_PISTOLMOD_DAMAGE;

	if (strcmp(classname, "pistol_mod_reload") == 0)
		return ITEMLIST_PISTOLMOD_RELOAD;

	if (strcmp(classname, "pistol_mod_rof") == 0)	
		return ITEMLIST_PISTOLMOD_ROF;



	if (strcmp(classname, "hmg_mod_cooling") == 0)
		//if (count && count == 1)
			return ITEMLIST_HMG_COOL_MOD;


/*
	if(strcmp(classname,"item_flag_team1")==0)
		return ITEMLIST_FLAG1;

	if(strcmp(classname,"item_flag_team2")==0)
		return ITEMLIST_FLAG2;
*/
	return INVALID;
}

