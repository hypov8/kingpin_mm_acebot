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
//  acebot_spawn.c - This file contains all of the 
//                   spawing support routines for the ACE bot.
//
///////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "..\m_player.h"
#include "acebot.h"

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// save the bots out to a file. 
//
// NOTE: There is still a bug when the bots are saved for
//       a dm game and then reloaded into a CTF game.
///////////////////////////////////////////////////////////////////////
void ACESP_SaveBots()
{
    edict_t *bot;
    FILE *pOut;
	int i,count = 0;
	cvar_t	*game_dir;
	char buf[32];
	bot_skin_t player;

	if (level.customSkinsUsed)
		return; //dont write to temp skins file if a custom player file is specified in map

#if 1
	//hypo mod folder for bots dir
	game_dir = gi.cvar("game", "", 0);
	sprintf(buf, "%s\\bots\\_bots.tmp", game_dir->string);
	
	
	//strcpy(filename, buf);
	if ((pOut = fopen(buf, "wb")) == NULL) //wb
		//end
#else		
	if((pOut = fopen("comp\\bots.tmp", "wb" )) == NULL)
#endif
		return; // bail
	
	// Get number of bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			count++;
	}
	gi.dprintf("Saved %i Bots to Disk\n", count);
	
	fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			fwrite(bot->client->pers.userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut); 
	}
		
    fclose(pOut);
}

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// load the bots from a file.
//
// Side effect/benifit are that the bots persist between games.
///////////////////////////////////////////////////////////////////////
void ACESP_LoadBots()
{
    FILE *pIn;
	char userinfo[MAX_INFO_STRING];
	char buffer[MAX_STRING_LENGTH];
	int i, count;
	cvar_t	*game_dir, *map_name;
	char buf[32];

	char *line, *token;
	bot_skin_t player; // name, skin, team;

	level.bots_spawned = true;
	level.customSkinsUsed = false;

	//hypo mod folder for bots dir
	map_name = gi.cvar("mapname", "", 0);
	game_dir = gi.cvar("game", "", 0);




	//check for individual bot config
	sprintf(buf, "%s\\bots\\%s.cfg", game_dir->string, map_name->string); // comp\bots\mapname.cfg

	if ((pIn = fopen(buf, "r")) == NULL)
	{	
		//use a custom config for every map thats not in the pre map.cfg
		//will not use the bot.tmp
		if (sv_botcfg->value == 1) //sv_botcfg "1"
		{
			//check for individual bot config
			sprintf(buf, "%s\\bots\\_default.cfg", game_dir->string); // comp\bots\mapname.cfg

			if ((pIn = fopen(buf, "r")) == NULL)return; // bail
			goto customcfg;
		}
		
		//hypo mod folder for bots dir
		sprintf(buf, "%s\\bots\\_bots.tmp", game_dir->string);

		//strcpy(filename, buf);
		if ((pIn = fopen(buf, "rb")) == NULL) //	if((pIn = fopen("comp\\bots.tmp", "rb" )) == NULL)
			return; // bail
	}
	else
	{
customcfg:
		level.customSkinsUsed = true; //hypo dont save custom players per map

		fgetline(pIn, buffer);
		while (!feof(pIn))
		{
		line = buffer;
			//token = player.name = player->skin = 
			//strcpy(player.name, "");
			//strcpy(player.skin, "");
			//strcpy(player.team, "");

			for (i = 1; i <= 3; i++)
			{
				token = COM_Parse(&line);
				if (token[0] == '\0')
					break;

				switch (i)
					{
					case 1: strcpy(player.name, token);
					case 2: strcpy(player.skin, token);
					case 3: strcpy(player.team, token);
						//case 4: skill = token; break;
					}


				if (i == 3)
				{
					if (teamplay->value) // name, skin, team 
						ACESP_SpawnBot(player.team, player.name, player.skin, NULL); //sv addbot thugBot "male_thug/009 031 031" dragon
					else // name, skin			
						ACESP_SpawnBot("\0", player.name, player.skin, NULL); //sv addbot thugBot "male_thug/009 031 031"
				}
			}
			fgetline(pIn, buffer);
			continue;
		}

		fclose(pIn);
	return;
	}

	fread(&count,sizeof (int),1,pIn); 

	for(i=0;i<count;i++)
	{
		fread(userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn); 
		ACESP_SpawnBot("\0", "\0", "\0", userinfo);
	}
		
    fclose(pIn);
}

///////////////////////////////////////////////////////////////////////
// Called by PutClient in Server to actually release the bot into the game
// Keep from killin' each other when all spawned at once
///////////////////////////////////////////////////////////////////////
void ACESP_HoldSpawn(edict_t *self)
{
	if (!KillBox (self))
	{	// could't spawn in?
	}

	gi.linkentity (self);

	self->think = ACEAI_Think;
	self->nextthink = level.time + FRAMETIME;

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (self->s.origin, MULTICAST_PVS);
#ifdef NOT_ZOID
	if(ctf->value)
	safe_bprintf(PRINT_MEDIUM, "%s joined the %s team.\n",
		self->client->pers.netname, CTFTeamName(self->client->resp.ctf_team));
	else
#endif

		safe_bprintf (PRINT_MEDIUM, "%s entered the game\n", self->client->pers.netname);

}

///////////////////////////////////////////////////////////////////////
// Modified version of id's code
///////////////////////////////////////////////////////////////////////
void ACESP_PutClientInServer (edict_t *bot, qboolean respawn, int team)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 48};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	client_persistant_t	saved;
	client_respawn_t	resp;
	char *s;
	
	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (bot, spawn_origin, spawn_angles);
	
	index = bot-g_edicts-1;
	client = bot->client;

	// deathmatch wipes most client data every spawn
	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = bot->client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		//add
		bot->move_frame = 0;
		bot->name_change_frame = -80;  //just to be sure
		//end add
		ClientUserinfoChanged (bot, userinfo);
	}
	else
	{
		if (bot != level.characters[0])
		{
			AddCharacterToGame(bot);
		}

		memset(&resp, 0, sizeof(resp));
	}
	
	//KP_ADD
	bot->name_index = -1;
	//KP_END

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	if( client->pers.health <=0 )
		InitClientPersistant( client);
	client->resp = resp;
	
	// copy some data from the client to the entity
	FetchClientEntData (bot);
	
	// clear entity values
	bot->groundentity = NULL;
	bot->client = &game.clients[index];
	bot->takedamage = DAMAGE_AIM;

	//hypov8 shouldent be needed anymore
#if 0
	if (((teamplay->value) && ((level.modeset == MATCHSETUP) || (level.modeset == TEAM_PRE_MATCH)))
		|| (level.modeset == DM_PRE_MATCH) || (bot->client->pers.spectator == SPECTATING))
	{
		bot->movetype = MOVETYPE_NOCLIP;
		bot->solid = SOLID_NOT;
		bot->svflags |= SVF_NOCLIENT;
		bot->client->pers.weapon = NULL;
		bot->client->pers.spectator = SPECTATING;
	}
	else
#endif
	{
		bot->movetype = MOVETYPE_WALK;
		bot->solid = SOLID_BBOX;
		bot->svflags &= ~(SVF_DEADMONSTER | SVF_NOCLIENT);

		//give 3 seconds of imortality on each spawn (anti-camp) 
		if (anti_spawncamp->value)
			client->invincible_framenum = level.framenum + 30;  //3 seconds 
	}

	// RAFAEL
	bot->viewheight = 40;
	bot->inuse = true;

	//bot->classname = "player";
	bot->classname = "bot"; //"player" ?
	bot->mass = 200;
	bot->solid = SOLID_BBOX;
	bot->deadflag = DEAD_NO;
	bot->air_finished = level.time + 12;
	bot->clipmask = MASK_PLAYERSOLID;
//	bot->model = "players/male/tris.md2";
	bot->pain = player_pain;
	bot->die = player_die;
	bot->waterlevel = 0;
	bot->watertype = 0;
	bot->flags &= ~FL_NO_KNOCKBACK;
	bot->svflags &= ~(SVF_DEADMONSTER|SVF_NOCLIENT);
	bot->is_jumping = false;
	bot->acebot.old_target = -1; //hypo add

#ifdef NOT_ZOID
	if(ctf->value)
	{
		client->resp.ctf_team = team;
		client->resp.ctf_state = CTF_STATE_START;
		s = Info_ValueForKey (client->pers.userinfo, "skin");
		CTFAssignSkin(bot, s);
	}
#endif

	if (teamplay->value)
	{
		client->pers.team = team;
		//client->pers.spectator = PLAYING; // CTF_STATE_START;
		s = Info_ValueForKey(client->pers.userinfo, "skin");
		//////CTFAssignSkin(bot, s);
	}

client->pers.spectator = PLAYING; // CTF_STATE_START;


	//KP_ADD
	bot->s.renderfx2 = 0;
	bot->onfiretime = 0;

	bot->cast_info.aiflags |= AI_GOAL_RUN;	// make AI run towards us if in pursuit
	//KP_END


	VectorCopy (mins, bot->mins);
	VectorCopy (maxs, bot->maxs);
	VectorClear (bot->velocity);

	//KP_ADD
	bot->cast_info.standing_max_z = bot->maxs[2];

	bot->cast_info.scale = MODEL_SCALE;
	bot->s.scale = bot->cast_info.scale - 1.0;
	//KP_END

	// clear playerstate values
	memset (&bot->client->ps, 0, sizeof(client->ps));
	
	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

//ZOID
//	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
//ZOID

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	// RAFAEL
	// weapon mdx
	{
		int i;
	
		memset(&(client->ps.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);

		client->ps.num_parts++;
	// JOSEPH 22-JAN-99
		if (client->pers.weapon)
			client->ps.model_parts[PART_HEAD].modelindex = gi.modelindex(client->pers.weapon->view_model);
		
		for (i=0; i<MAX_MODELPART_OBJECTS; i++)
			client->ps.model_parts[PART_HEAD].skinnum[i] = 0; // will we have more than one skin???
	}

	if (client->pers.weapon)
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	// END JOSEPH


	// clear entity state values
	bot->s.effects = 0;
	bot->s.skinnum = bot - g_edicts - 1;
	bot->s.modelindex = 255;		// will use the skin specified model
// KINGPIN_X	bot->s.modelindex2 = 255;		// custom gun model
	bot->s.frame = 0;
	VectorCopy (spawn_origin, bot->s.origin);
	bot->s.origin[2] += 1;	// make sure off ground

	//KP_ADD
	VectorCopy (bot->s.origin, bot->s.old_origin);

	// bikestuff
	bot->biketime = 0;
	bot->bikestate = 0;


// Ridah, Hovercars
	if (g_vehicle_test->value)
	{
		if (g_vehicle_test->value == 3)
			bot->s.modelindex = gi.modelindex ("models/props/moto/moto.mdx");
		else
			bot->s.modelindex = gi.modelindex ("models/vehicles/cars/viper/tris_test.md2");

//		ent->s.modelindex2 = 0;
		bot->s.skinnum = 0;
		bot->s.frame = 0;

		if ((int)g_vehicle_test->value == 1)
			bot->flags |= FL_HOVERCAR_GROUND;
		else if ((int)g_vehicle_test->value == 2)
			bot->flags |= FL_HOVERCAR;
		else if ((int)g_vehicle_test->value == 3)
			bot->flags |= FL_BIKE;
		else if ((int)g_vehicle_test->value == 4)
			bot->flags |= FL_CAR;
	}
// done.
	else if (dm_locational_damage->value)	// deathmatch, note models must exist on server for client's to use them, but if the server has a model a client doesn't that client will see the default male model
	{
		char	*s;
		char	modeldir[MAX_QPATH];//, *skins;
		int		len;
		int		did_slash;
		char	modelname[MAX_QPATH];
//		int		skin;

		// NOTE: this is just here for collision detection, modelindex's aren't actually set

		bot->s.num_parts = 0;		// so the client's setup the model for viewing

		s = Info_ValueForKey (client->pers.userinfo, "skin");

//		skins = strstr( s, "/" ) + 1;

		// converts some characters to NULL's
		len = strlen( s );
		did_slash = 0;
		for (i=0; i<len; i++)
		{
			if (s[i] == '/')
			{
				s[i] = '\0';
				did_slash = true;
			}
			else if (s[i] == ' ' && did_slash)
			{
				s[i] = '\0';
			}
		}

		if (strlen(s) > MAX_QPATH-1)
			s[MAX_QPATH-1] = '\0';

		strcpy(modeldir, s);
		
		if (strlen(modeldir) < 1)
			strcpy( modeldir, "male_thug" );
		
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		
		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/head.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/head.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/legs.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/legs.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		strcpy( modelname, "players/" );
		strcat( modelname, modeldir );
		strcat( modelname, "/body.mdx" );
		bot->s.model_parts[bot->s.num_parts-1].modelindex = 255;
		gi.GetObjectBounds( modelname, &bot->s.model_parts[bot->s.num_parts-1] );
		if (!bot->s.model_parts[bot->s.num_parts-1].object_bounds[0])
			gi.GetObjectBounds( "players/male_thug/body.mdx", &bot->s.model_parts[bot->s.num_parts-1] );

		bot->s.num_parts++;
		bot->s.model_parts[PART_GUN].modelindex = 255;
	}
	else	// make sure we can see their weapon
	{
		memset(&(bot->s.model_parts[0]), 0, sizeof(model_part_t) * MAX_MODEL_PARTS);
		bot->s.model_parts[PART_GUN].modelindex = 255;
		bot->s.num_parts = PART_GUN+1;	// make sure old clients recieve the view weapon index
	}

	//KP_END


	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	bot->s.angles[PITCH] = 0;
	bot->s.angles[YAW] = spawn_angles[YAW];
	bot->s.angles[ROLL] = 0;
	VectorCopy (bot->s.angles, client->ps.viewangles);
	VectorCopy (bot->s.angles, client->v_angle);
	
	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (bot);

	bot->enemy = NULL;
	bot->movetarget = NULL; 
	bot->state = BOTSTATE_MOVE;

	// Set the current node
	bot->current_node = ACEND_FindClosestReachableNode(bot,BOTNODE_DENSITY, BOTNODE_ALL);
	bot->goal_node = bot->current_node;
	bot->next_node = bot->current_node;
	bot->next_move_time = level.time;		
	bot->suicide_timeout = level.time + 15.0;

	// If we are not respawning hold off for up to three seconds before releasing into game
    if(!respawn)
	{
		bot->think = ACESP_HoldSpawn;
		bot->nextthink = level.time + 0.1;
		bot->nextthink = level.time + random()*3.0; // up to three seconds
	}
	else
	{
		if (!KillBox (bot))
		{	// could't spawn in?
		}

		gi.linkentity (bot);

		bot->think = ACEAI_Think;
		bot->nextthink = level.time + FRAMETIME;

/*			// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (bot-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (bot->s.origin, MULTICAST_PVS);*/
	}
	
}

///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_Respawn (edict_t *self)
{
	if (!((level.modeset == TEAM_MATCH_RUNNING) || (level.modeset == DM_MATCH_RUNNING)))
	{
		self->deadflag = 0;
		gi.dprintf("bot respawned after match\n");
		return; //hypov8 dont respawn, fixes last person dying loosing there mouse pitch
	}


	CopyToBodyQue (self);

/*	if(ctf->value)
		ACESP_PutClientInServer (self,true, self->client->resp.ctf_team);
	else*/
	if (teamplay->value)
		ACESP_PutClientInServer(self, true, self->client->pers.team);
	else
		ACESP_PutClientInServer (self,true,0);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time; // +5;//hypov8 add 5 secs to respawn
	
}

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
edict_t *ACESP_FindFreeClient (void)
{
	edict_t *bot = NULL;
	int	i;
	int max_count=0;
	
	// This is for the naming of the bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;
		
		if(bot->count > max_count)
			max_count = bot->count;
	}

	// Check for free spot
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (!bot->inuse)
			break;
	}

	bot->count = max_count + 1; // Will become bot name...

	if (bot->inuse)
		bot = NULL;
	
	return bot;
}

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
void ACESP_SetName(edict_t *bot, char *name, char *skin/*, char *team*/)
{
	float rnd;
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];

	// Set the name for the bot.
	// name
	if(strlen(name) == 0)
		sprintf(bot_name,"ThugBot_%d",bot->count);
	else
		strcpy(bot_name,name);
	
	// skin
	if(strlen(skin) == 0)
	{
		// randomly choose skin 
		rnd = random();
		if(rnd  < 0.05)
			sprintf(bot_skin,"female_chick/005 005 005");
		else if(rnd < 0.1)
			sprintf(bot_skin,"male_thug/010 010 010");
		else if(rnd < 0.15)
			sprintf(bot_skin,"male_thug/011 011 011");
		else if(rnd < 0.2)
			sprintf(bot_skin,"male_thug/012 012 012");
		else if(rnd < 0.25)
			sprintf(bot_skin,"female_chick/006 006 006");
		else if(rnd < 0.3)
			sprintf(bot_skin,"male_thug/013 013 013");
		else if(rnd < 0.35)
			sprintf(bot_skin,"male_thug/013 013 013");
		else if(rnd < 0.4)
			sprintf(bot_skin,"female_chick/020 020 020");
		else if(rnd < 0.45)
			sprintf(bot_skin,"male_thug/014 014 014");
		else if(rnd < 0.5)
			sprintf(bot_skin,"male_thug/015 015 015");
		else if(rnd < 0.55)
			sprintf(bot_skin,"male_thug/016 016 016");
		else if(rnd < 0.6)
			sprintf(bot_skin,"male_thug/017 017 017");
		else if(rnd < 0.65)
			sprintf(bot_skin,"female_chick/056 056 056");
		else if(rnd < 0.7)
			sprintf(bot_skin,"male_thug/300 300 300");
		else if(rnd < 0.75)
			sprintf(bot_skin,"male_thug/010 010 010");
		else if(rnd < 0.8)
			sprintf(bot_skin,"male_thug/008 008 008");
		else if(rnd < 0.85)
			sprintf(bot_skin,"male_thug/009 019 017");
		else if(rnd < 0.9)
			sprintf(bot_skin,"female_chick/032 032 032");
		else if(rnd < 0.95)
			sprintf(bot_skin,"male_thug/001 001 001");
		else 
			sprintf(bot_skin,"male_thug/004 004 004");
	}
	else
		strcpy(bot_skin,skin);

	// initialise userinfo
	memset (userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey(userinfo, "ver", "121");
	Info_SetValueForKey(userinfo, "fov", "90");
	Info_SetValueForKey(userinfo, "rate", "7500");
	Info_SetValueForKey(userinfo, "extras", "0000");
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "name", bot_name);

	Info_SetValueForKey(userinfo, "gl_mode", "1");
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!
	Info_SetValueForKey(userinfo, "ip", "loopback");
	Info_SetValueForKey(userinfo, "msg", "0");

	

	ClientConnect (bot, userinfo);

	ACESP_SaveBots(); // make sure to save the bots
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_SpawnBot (char *team, char *name, char *skin, char *userinfo)
{
	edict_t	*bot;
	
	bot = ACESP_FindFreeClient ();
	
	if (!bot)
	{
		gi.dprintf("Server is full, increase Maxclients.\n");
		return;
	}

// hypo add
	//bot->client->resp.is_spawn = true;
	//bot->client->pers.spectator = PLAYING;
	bot->flags &= ~FL_GODMODE;
	bot->health = 0;
	meansOfDeath = MOD_RESTART;
	bot->acebot.old_target = -1;
	bot->acebot.new_target = -1;
	bot->acebot.old_target = -1;
	//bot->client->buttons &= ~BUTTON_ATTACK; //

//end

	bot->yaw_speed = 100; // yaw speed
	bot->inuse = true;
	bot->is_bot = true;

	bot->client->pers.team = 0; //hypo set default

	if (teamplay->value)
	{
		//if (strlen(team) > 0)
		//if (team[0] != '\0') //NULL
		//if (team == NULL)
		//strcpy( team[0],'\0');

		if (team[0] != '\0') // && team[0] != '0') //hypo console spits out '\0'
		{
		//if (team[0] == team_names[1][0]) // "d" //hypo 1st letter team_name[team_1][char_0]
			if (team[0] == 'd' || team[0] == 'D' || team[0] == '1')
				bot->client->pers.team = TEAM_1;
			else
				bot->client->pers.team = TEAM_2;
		}
		else //null
		{
			if (level.lastTeamSpawned != TEAM_1)
			{
				level.lastTeamSpawned = TEAM_1;
				bot->client->pers.team = TEAM_1;
			}
			else
			{
				level.lastTeamSpawned = TEAM_2;
				bot->client->pers.team = TEAM_2;
			}
		}
	}



	// To allow bots to respawn
	if(userinfo == NULL)
		ACESP_SetName(bot, name, skin/*, team*/);
	else
		ClientConnect (bot, userinfo);
	
	G_InitEdict (bot);

	//InitClientResp (bot->client);
	//hypov8 reset scores?
	InitClientRespClear(bot->client);
	
	// locate ent at a spawn point
#ifdef NOT_ZOID
	if(ctf->value)
	{
		if (team != NULL && strcmp(team,"red")==0)
			ACESP_PutClientInServer (bot,false, CTF_TEAM1);
		else
			ACESP_PutClientInServer (bot,false, CTF_TEAM2);
	}
	else

#endif
#if 0
		if (teamplay->value)
		{
			if (team != NULL)
			{
				if (strcmp(team[0], "d") == 0) //hypo 1st letter
					ACESP_PutClientInServer(bot, false, TEAM_1);
				else
					ACESP_PutClientInServer(bot, false, TEAM_2);
			}
			else //null
			{
				if (level.lastTeamSpawned != TEAM_1)
				{
					level.lastTeamSpawned = TEAM_1;
					ACESP_PutClientInServer(bot, false, TEAM_1);
				}
				else
				{
					level.lastTeamSpawned = TEAM_2;
					ACESP_PutClientInServer(bot, false, TEAM_2);
				}
			}
		}
		else //end teamplay

#endif
			ACESP_PutClientInServer(bot, false, bot->client->pers.team /*TEAM_NONE*/);

	//safe_bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);
//hypo
	bot->client->resp.is_spawn = true;
	bot->inuse = true;
	bot->is_bot = true;
//end
	// make sure all view stuff is valid
	ClientEndServerFrame (bot);
	
	ACEIT_PlayerAdded (bot); // let the world know we added another

	ACEAI_PickLongRangeGoal(bot); // pick a new goal

}

///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void ACESP_RemoveBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if(bot->is_bot && (strcmp(bot->client->pers.netname,name)==0 || strcmp(name,"all")==0))
			{
				bot->health = 0;
				player_die (bot, bot, bot, 100000, vec3_origin,0,0); //hypov8 add null
				// don't even bother waiting for death frames
				bot->deadflag = DEAD_DEAD;
				bot->inuse = false;
				freed = true;
				///////ACEIT_PlayerRemoved (bot); // hypo now in client disconect
				safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
//add hypo
				ClientDisconnect(bot);
//end

			}
		}
	}

	if(!freed)	
		gi.dprintf("%s not found\n", name);
		//safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
	
	ACESP_SaveBots(); // Save them again

}

