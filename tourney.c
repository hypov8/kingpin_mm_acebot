#include "g_local.h"

int	 vote_set[9];        // stores votes for next map

char admincode[16];		 // the admincode
char default_map[32];    // default settings
char default_teamplay[16];
char default_dmflags[16];
char default_password[16];
char default_timelimit[16];
char default_cashlimit[16];
char default_fraglimit[16];
char default_dm_realmode[16];
char custom_map_filename[32];  // stores where various files can be found
char ban_name_filename[32];
char ban_ip_filename[32];
int allow_map_voting;
int disable_admin_voting;
int scoreboard_first;
int fph_scoreboard;
int total_rank;          // used in calculating maps picks based on weight
int num_custom_maps;
int num_netnames;
int num_ips;

int fixed_gametype;
int enable_password;
char rconx_file[32];
//char server_url[64];
int num_rconx_pass;
int keep_admin_status;
int default_random_map;
int disable_anon_text;
int disable_curse;
// BEGIN HITMEN
int enable_hitmen;
// END
//int enable_asc;
int unlimited_curse;
int enable_killerhealth;

MOTD_t	MOTD[20];

player_t playerlist[64];

ban_t	netname[100];
ban_t	ip[100];

ban_t	rconx_pass[100];

// HYPOV8_ADD??
edict_t *GetAdmin()
{
	int		i;
	edict_t	*doot;

	for_each_player_not_bot(doot, i)// ACEBOT_ADD
	{
		if (doot->client->pers.admin > NOT_ADMIN)
			return doot;
	}
	return NULL;
}
// HYPOV8_END

//==============================================================
//
// Papa - This file contains all the functions that control the 
//        modes a server may be in.
//
//===============================================================


// HYPOV8_ADD seems to cause issues
qboolean for_each_player(edict_t *JOE_BLOGGS)
{
	if (JOE_BLOGGS->inuse
		&& JOE_BLOGGS->client
		&& JOE_BLOGGS->client->pers.connected) //hypov8 and !bot?
		return true;

	return false;
}
// HYPOV8_END
/*
================
PublicSetup
server cmd "publicsetup"
================
*/
void PublicSetup ()  // returns the server into ffa mode and resets all the cvars (settings)
{
	edict_t		*self;
	int			i;

	gi.dprintf("--== PublicSetup ==-- command has been used\n"); // HYPOV8_ADD

// ACEBOT_ADD
	//ACEND_SaveNodes(); //save file nodes
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
	level.bots_spawned = false;
// ACEBOT_END

	level.modeset = DM_PRE_MATCH;  // HYPOV8 FFA
	gi.cvar_set("dmflags",default_dmflags);
	// BEGIN HITMEN
#if 0
	if (enable_hitmen && (int)dmflags->value & DF_INFINITE_AMMO)
	{
		int flagTmp;
		flagTmp = (int)dmflags->value;
		flagTmp &= ~(DF_INFINITE_AMMO);
		gi.cvar_set("dmflags", va("%i", flagTmp));
	}
#endif
	// END
	//gi.cvar_set("teamplay",default_teamplay);
	gi.cvar_set("teamplay", "0"); // HYPOV8_ADD
	gi.cvar_set("password",default_password);
	gi.cvar_set("timelimit",default_timelimit);
	gi.cvar_set("fraglimit",default_fraglimit);
	gi.cvar_set("cashlimit",default_cashlimit);
	gi.cvar_set("dm_realmode",default_dm_realmode);
	level.startframe = level.framenum;
	for_each_player_inc_bot(self, i)
	{
// ACEBOT_ADD
		if (self->acebot.is_bot){
			ACESP_RemoveBot(self->client->pers.netname); //disco
			continue;
		}
// ACEBOT_END

		self->flags &= ~FL_GODMODE;
		self->health = 0;
		meansOfDeath = MOD_RESTART;
//		player_die (self, self, self, 1, vec3_origin, 0, 0);

		ClientBeginDeathmatch( self );	
	}
	
	safe_bprintf(PRINT_HIGH, "The server is once again public.\n Load a new map for effect.\n");
}

/*
================
MatchSetup
level.modeset = MATCHSETUP
teamplay. ONLY used when console command "matchsetup" is used

Places the server in prematch mode
================
*/
void MatchSetup ()
{
	edict_t		*self;
	int			i;

	gi.dprintf("--== MatchSetup ==-- command has been used\n"); // HYPOV8_ADD

// ACEBOT_ADD
	//ACEND_SaveNodes(); //save file nodes
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
	ACESP_RemoveBot("all");
	level.bots_spawned = false;

	//if (self->acebot.is_bot)
	//{
	//	ACESP_RemoveBot(self->client->pers.netname); //disco
	//	continue;
	//} 
// ACEBOT_END

	level.modeset = MATCHSETUP;
	level.startframe = level.framenum;

	for_each_player_inc_bot(self, i)
	{
		if (self->client->pers.spectator == SPECTATING)
			continue;
		meansOfDeath = MOD_RESTART;
		// self->client->pers.spectator = SPECTATING;// HYPOV8_DISABLED
		self->flags &= ~FL_GODMODE;
		self->health = 0;
//		player_die (self, self, self, 1, vec3_origin, 0, 0);

		ClientBeginDeathmatch( self );	
	}

	safe_bprintf(PRINT_HIGH, "The server is now ready to setup a match.\n");
	safe_bprintf(PRINT_HIGH, "Players need to join the correct teams.\n");
	
}
int memalloced[3] = {0,0,0};

void ResetServer() // completely resets the server including map
{
	char command[64];

	gi.dprintf("--== ResetServer ==-- command has been used\n"); // HYPOV8_ADD

	gi.cvar_set("dmflags", default_dmflags);
	// BEGIN HITMEN
#if 0
	if (enable_hitmen && (int)dmflags->value & DF_INFINITE_AMMO)
	{
		int flagTmp;
		flagTmp = (int)dmflags->value;
		flagTmp &= ~(DF_INFINITE_AMMO);
		gi.cvar_set("dmflags", va("%i", flagTmp));
	}
#endif
	// END
	gi.cvar_set("teamplay",default_teamplay);
	gi.cvar_set("password",default_password);
	gi.cvar_set("timelimit",default_timelimit);
	gi.cvar_set("fraglimit",default_fraglimit);
	gi.cvar_set("cashlimit",default_cashlimit);
	gi.cvar_set("dm_realmode",default_dm_realmode);
	gi.cvar_set("cheats","0");
	memalloced[1]=0;memalloced[2]=0;

	if (default_random_map && num_custom_maps)
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", custom_list[rand()%num_custom_maps].custom_map);
	else
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", default_map);
	gi.AddCommandString (command);
}

int team_startcash[2]={0,0};

/*
================
MatchStart
level.modeset = TEAM_PRE_MATCH;
ONLY server command "matchstart"

start the match
================
*/
void MatchStart()
{
	int			i;
	edict_t		*self;

// ACEBOT_ADD
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
	level.bots_spawned = false;
// ACEBOT_END	

	level.player_num=0;
	level.modeset = TEAM_PRE_MATCH;
	level.startframe = level.framenum;
	safe_bprintf(PRINT_HIGH, "FINAL COUNTDOWN STARTED.  15 SECONDS TO MATCH.\n");
	team_cash[1]=team_startcash[0]; team_startcash[0]=0;
	team_cash[2]=team_startcash[1]; team_startcash[1]=0;
	UPDATESCORE

    G_ClearUp (NULL, FOFS(classname));

	for_each_player_inc_bot(self, i)
	{

// ACEBOT_ADD
		if (self->acebot.is_bot){
			ACESP_RemoveBot(self->client->pers.netname); //disco
			continue;
		}
// ACEBOT_END

		self->client->pers.bagcash = 0;
		self->client->resp.deposited = 0;
		self->client->resp.score = 0;
		self->client->pers.currentcash = 0;
		self->client->resp.acchit = self->client->resp.accshot = 0;
		memset(self->client->resp.fav,0,8*sizeof(int));
        
        if (self->client->pers.spectator == SPECTATING)
            continue;

        meansOfDeath = MOD_RESTART;
        self->client->pers.spectator = SPECTATING;
        self->flags &= ~FL_GODMODE;
        self->health = 0;
        ClientBeginDeathmatch( self );
    }

	gi.WriteByte( svc_stufftext );
	gi.WriteString( va("play world/cypress%i.wav", 2+(rand()%4)) );
	gi.multicast (vec3_origin, MULTICAST_ALL);
}


void SpawnPlayer () // Here I spawn players - 1 per server frame in hopes of reducing overflows
{
	edict_t		*self;
	int			i;
	int			ClientFound=false;

	for (i = 1 ; i <= maxclients->value; i++)
	{
		self = g_edicts + i;
		if (!self->inuse)
			continue;
		if (!self->client->resp.is_spawn)
		{
			self->flags &= ~FL_GODMODE;
			self->health = 0;
			meansOfDeath = MOD_RESTART;
			ClientFound = true;
			self->client->pers.spectator = PLAYING;
//			player_die (self, self, self, 1, vec3_origin, 0, 0);

// ACEBOT_ADD				
			if (!self->acebot.is_bot)
// ACEBOT_END				
				ClientBeginDeathmatch(self);

			self->client->resp.is_spawn = true;
			break;
		}
	}

	/* no more players to join match, so run game */
	if (!ClientFound)
		level.is_spawn = true;
}



void SpawnTeamPlayers ()  //HYPOV8 team.. Same idea but 1 player per team
{
	edict_t		*self;
	int			i;
	int			team1,team2;

	team1 = false;
	team2 = false;

	for (i=0 ; i<maxclients->value && (!team1 || !team2); i++)
	{
		self = g_edicts + 1 + i;
		if (!self->inuse)
			continue;
		if ((self->client->pers.team == 1) && (!team1) && (!self->client->resp.is_spawn))
		{
			self->flags &= ~FL_GODMODE;
			self->health = 0;
			meansOfDeath = MOD_RESTART;
			team1 = true;
			self->client->pers.spectator = PLAYING;
//			player_die (self, self, self, 1, vec3_origin, 0, 0);

// ACEBOT_ADD
			if (!self->acebot.is_bot)
// ACEBOT_END				
			ClientBeginDeathmatch(self);

			self->client->resp.is_spawn = true;
		}
		if ((self->client->pers.team == 2) && (!team2) && (!self->client->resp.is_spawn))
		{
			self->flags &= ~FL_GODMODE;
			self->health = 0;
			meansOfDeath = MOD_RESTART;
			team2 = true;
			self->client->pers.spectator = PLAYING;
//			player_die (self, self, self, 1, vec3_origin, 0, 0);

// ACEBOT_ADD	
			if (!self->acebot.is_bot)
// ACEBOT_END				
			ClientBeginDeathmatch(self);

			self->client->resp.is_spawn = true;
		}
	}
	if ((!team1) && (!team2))
		level.is_spawn = true;
}
/*
================
Start_TeamMatch
level.modeset = TEAM_MATCH_SPAWNING;

game will now load clients as active
================
*/
void Start_TeamMatch () // HYPOV8 team.. Starts the match
{
	edict_t		*self;
	int			i;

	level.startframe = level.framenum;
	level.modeset = TEAM_MATCH_SPAWNING;
	level.is_spawn = false;
	for_each_player_inc_bot(self, i)
	{
		safe_centerprintf(self, "The Match has begun.\n");
		self->client->resp.is_spawn = false;
		self->client->resp.enterframe = level.framenum;
	}

	gi.WriteByte( svc_stufftext );
	gi.WriteString("play world/pawnbuzz_out.wav");
	gi.multicast (vec3_origin, MULTICAST_ALL);
}

/*
================
Start_FFA
level.modeset = DM_MATCH_SPAWNING;

starting team game
================
*/
void Start_FFA () // Starts a pub
{
	edict_t		*self;
	int			i;

	level.startframe = level.framenum;
	level.modeset = DM_MATCH_SPAWNING;
	level.is_spawn = false;
	for_each_player_inc_bot(self, i)
	{
		safe_centerprintf(self, "Let the Fun Begin!\n");
		self->client->resp.is_spawn = false;
		self->client->resp.enterframe = level.framenum;
	}

	gi.WriteByte( svc_stufftext );
	gi.WriteString("play world/pawnbuzz_out.wav");
	gi.multicast (vec3_origin, MULTICAST_ALL);
}

/*
================
SetupMapVote
ENDMATCHVOTING
game ended. vote
================
*/
void SetupMapVote () // at the end of a level - starts the vote for the next map
{
	edict_t *self,*intermision;
	int i;
/*	int		found;
	int		i,j,k;
	int		unique;
	int		selection;
*/	

// ACEBOT_ADD
	ACEND_SaveNodes(); //save file nodes
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
// ACEBOT_END

	level.modeset = ENDMATCHVOTING;
	level.startframe = level.framenum;

// HYPOV8_ADD LOTS of differneces
	{
		// find an intermission spot
		intermision = G_Find(NULL, FOFS(classname), "info_player_intermission");
		if (!intermision)
		{	// the map creator forgot to put in an intermission point...
			intermision = G_Find(NULL, FOFS(classname), "info_player_start");
			if (!intermision)
				intermision = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
		}
		else
		{	// chose one of four spots
			i = rand() & 3;
			while (i--)
			{
				intermision = G_Find(intermision, FOFS(classname), "info_player_intermission");
				if (!intermision)	// wrap around the list
					intermision = G_Find(intermision, FOFS(classname), "info_player_intermission");
			}
		}
		VectorCopy(intermision->s.origin, level.intermission_origin);
		VectorCopy(intermision->s.angles, level.intermission_angle);
	}




	for_each_player_inc_bot(self, i)
	{	
		HideWeapon(self);
		if (self->client->flashlight) self->client->flashlight = false;
		self->movetype = MOVETYPE_NOCLIP;
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	//	self->client->pers.weapon = NULL;
		self->client->pers.spectator = SPECTATING;
//add hypov8
		self->client->ps.pmove.pm_flags = PM_NORMAL; //fixes specing player on game end
		self->client->ps.pmove.pm_type = PM_SPECTATOR; // switch bots to spec. elsewhere?
		self->flags = 0;
		self->client->buttons = 0;
		self->client->latched_buttons = 0;
		//self->client->respawn_time
		self->onfireent = NULL;
		if (self->onfiretime != 0)
		{
			gi.dprintf( "ent on fire RESET\n");
			self->onfiretime = 0;
		}
		self->client->pers.fakeThief = 0;
		meansOfDeath = MOD_RESTART;
		self->flags &= ~FL_GODMODE;
		self->health = 0;

		self->flags &= ~FL_CHASECAM; //hypov8 turn off togglecam

		//change wep to none
		self->client->newweapon = NULL;
		ChangeWeapon(self);

//hypov8 move player to intermision
		intermision = G_Find(NULL, FOFS(classname), "info_player_intermission");
		if (intermision)
		{

			MoveClientToIntermission(self);

			VectorCopy(intermision->s.origin, self->s.origin);
			self->client->ps.pmove.origin[0] = intermision->s.origin[0] * 8;
			self->client->ps.pmove.origin[1] = intermision->s.origin[1] * 8;
			self->client->ps.pmove.origin[2] = intermision->s.origin[2] * 8;
			VectorCopy(level.intermission_angle, self->client->ps.viewangles);
		}
//hypov8 end
	}

	gi.WriteByte( svc_stufftext );
	gi.WriteString( va("play world/cypress%i.wav", 2+(rand()%4)) );
	gi.multicast (vec3_origin, MULTICAST_ALL);

}

/*
================
MatchEnd
level.modeset =  ENDMATCHVOTING?
ONLY used for server cmd "matchend"
move players to spec
================
*/
void MatchEnd() // end of the match
{
	int		i, count = 0;
	edict_t	*doot;

	gi.dprintf("--== MatchEnd ==-- command has been used\n"); //add hypo

	safe_bprintf(PRINT_HIGH, "Admin ended match.\n");

	for_each_player_not_bot(doot, i)
	{
		count++; //hypov8 bots dont count
	}

	if (count == 0)
		ResetServer();
	else
	{
		if (!allow_map_voting)
			EndDMLevel(); //hypo MatchEnd(); // 
		else
			SetupMapVote();

	}
}

/*
================
CheckAllPlayersSpawned
TEAM_MATCH_SPAWNING = level.modeset = TEAM_MATCH_RUNNING
DM_MATCH_SPAWNING = level.modeset = DM_MATCH_RUNNING;

when starting a match this function is called until all the players are in the game
then sets mode dm or bm
================
*/
void CheckAllPlayersSpawned()
{
	level.startframe = level.framenum; // delay clock until all players have spawned
	if (teamplay->value) //hypov8 ToDo: acebot teamplay
		SpawnTeamPlayers();
	else
		SpawnPlayer();

	if (level.is_spawn)
	{
		if (level.is_spawn && level.modeset == TEAM_MATCH_SPAWNING)
			level.modeset = TEAM_MATCH_RUNNING;
		else	//DM_MATCH_SPAWNING
			level.modeset = DM_MATCH_RUNNING;
	}

		
}
/*
========
CheckIdleMatchSetup
command "matchsetup"
========
*/
void CheckIdleMatchSetup () // restart the server if its empty in matchsetup mode
{
	int		count=0;
	int		i;
	edict_t	*doot;

// ACEBOT_ADD
	level.bots_spawned = 0;
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
// ACEBOT_END

	for_each_player_not_bot(doot, i){ //hypov8 bots count!!
		count++;
	}

	if (count == 0)
		ResetServer (); //hypov8 bots count!!
}

/*
================
CheckStartTeamMatch
calls Start_TeamMatch() when time is up
level.modeset = TEAM_MATCH_SPAWNING;

15 countdown before matches
================
*/
void CheckStartTeamMatch()
{
// ACEBOT_ADD
	level.bots_spawned = 0; //hypov8 
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
// ACEBOT_END

	if (level.framenum >= level.startframe + (PRE_MATCH_TIME -5))
	{
		Start_TeamMatch ();
		return;
	}

	if ((level.framenum % 10 == 0) && (level.framenum > level.startframe + (PRE_MATCH_TIME -99)))
		safe_bprintf(PRINT_HIGH, "The Team Game will start in %d seconds!\n", ((PRE_MATCH_TIME-10) - (level.framenum - level.startframe)) / 10);
}

/*
================
CheckStartDM
calls Start_DM()
->level.modeset = DM_MATCH_SPAWNING;

15 second countdown before server starts
================
*/
void CheckStartDM () // HYPOV8_ADD dm... PRE_MATCH_TIME
{
	if (teamplay->value) //hypov8 make teamplay games use corect varables
	{
		level.modeset = TEAM_PRE_MATCH;
		return; 
	}


	//if (level.framenum >= 345)
	if (level.framenum >= level.startframe + (PRE_MATCH_TIME - 5)) //hypo global match start time PRE_MATCH_TIME
	{
// ACEBOT_ADD
	level.bots_spawned = 0; //hypov8 
	num_players = 0;
	botsRemoved = 0;
	num_bots = 0;
// ACEBOT_END
		Start_FFA ();
		return;
	}


	//if ((level.framenum % 10 == 0 ) && (level.framenum > 299))
	//	safe_bprintf(PRINT_HIGH, "The DM Server will start in %d seconds!\n", (340 - (level.framenum)) / 10);
	if ((level.framenum % 10 == 0) && (level.framenum > level.startframe + (PRE_MATCH_TIME - 99)))
		safe_bprintf(PRINT_HIGH, "The DM Game will start in %d seconds!\n", ((PRE_MATCH_TIME -10) - (level.framenum - level.startframe)) / 10);
}


void getTeamTags();
void CheckEndTeamMatch() // HYPOV8_ADD team... check if time,frag,cash limits have been reached in a match
{
	int			i;
	int		count = 0;
	edict_t	*doot;

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	// snap - team tags
	//hypo why reset team names?????
	//	if (level.framenum % 100 == 0 && !level.manual_tagset)		{
		//	getTeamTags();
		//}

	for_each_player_inc_bot(doot, i)
	{
	//if (doot->is_bot) continue; //hypov8 change to next map, instead of waiting for bots to vote
		count++;
	}

// ACEBOT_ADD
	if (count && !level.bots_spawned)
	{
		ACEND_InitNodes();
		ACEND_LoadNodes();
		ACESP_LoadBots();
	}
// ACEBOT_END

	if ((count == 0) && (level.framenum > 12000))	{
		ResetServer(); //hypov8 bots count?
		//return??
	}

	if ((int)fraglimit->value && (int)teamplay->value == 4)	
	{
		if (team_cash[1] >= (int)fraglimit->value || team_cash[2] >= (int)fraglimit->value) {
			safe_bprintf(PRINT_HIGH, "Fraglimit hit.\n");
			if (!allow_map_voting)
				EndDMLevel(); //hypov8 MatchEnd(); // 
			else
				SetupMapVote();
			return;
		}
	}


	if ((int)cashlimit->value)
	{
		if ((team_cash[1] >= (int)cashlimit->value) || (team_cash[2] >= (int)cashlimit->value))
		{
			safe_bprintf(PRINT_HIGH, "Cashlimit hit.\n");
			if (!allow_map_voting)
				EndDMLevel(); //hypov8 MatchEnd(); // 
			else
				SetupMapVote();
			return;
		}
	}

	if ((int)timelimit->value) 
	{
		if (level.framenum > (level.startframe + ((int)timelimit->value) * 600 - 1))
		{
			safe_bprintf(PRINT_HIGH, "Timelimit hit.\n");
			if (!allow_map_voting)
				EndDMLevel(); //hypov8 MatchEnd(); // 
			else
				SetupMapVote();
			return;
		}
		if (((level.framenum - level.startframe ) % 10 == 0 ) && (level.framenum > (level.startframe + (((int)timelimit->value  * 600) - 155))))  
		{
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d seconds\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 10);
			return;
		}
		if (((level.framenum - level.startframe ) % 600 == 0 ) && (level.framenum > (level.startframe + (((int)timelimit->value * 600) - 3000))))  
		{
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
			return;
		}
		if ((level.framenum - level.startframe ) % 3000 == 0 )
			safe_bprintf(PRINT_HIGH, "The Match will end in  %d minutes\n", (((int)timelimit->value * 600) + level.startframe - level.framenum) / 600);
	}

	

}

void CheckEndVoteTime () // check the timelimit for voting next level/start next map
{
	int		i,count[9];
	edict_t *player;
	char	command[64];
	int		wining_map;

	if (level.framenum == (level.startframe + 10))
	{
	for_each_player_not_bot(player,i)// ACEBOT_ADD
		{
			if (scoreboard_first)
				player->client->showscores = SCOREBOARD;
			else
				player->client->showscores = SCORE_MAP_VOTE;
			DeathmatchScoreboard (player);
		}
	}

	if (level.framenum > (level.startframe + 300))
	{
		memset (&count, 0, sizeof(count));

		for_each_player_not_bot(player,i)// ACEBOT_ADD
		{
			count[player->vote]++;
		}

		wining_map = 1;
		for (i = 2;i < 9 ; i++)
		{
			custom_list[vote_set[i]].rank += count[i];
			if (count[i] > count[wining_map])
				wining_map = i;
		}

		i = write_map_file();
		if (i != OK)
			gi.dprintf("Error writing custom map file!\n");

		Com_sprintf (command, sizeof(command), "gamemap \"%s\"\n", custom_list[vote_set[wining_map]].custom_map);
		gi.AddCommandString (command);
	}
}

// HYPOV8_ADD
void CheckEndMatchTime() // check the timelimit for voting next level/start next map
{
	int		i;
	edict_t *player;
	char	command[64];
	static int		wining_map;

	if (level.framenum == (level.startframe + 10))
	{
		for_each_player_not_bot(player, i)
		{
			player->client->showscores = SCOREBOARD;
			DeathmatchScoreboard(player);
		}
	}

	if (level.framenum > (level.startframe + 100))
	{
		if (num_custom_maps)
		{

			if (wining_map >= num_custom_maps)
				wining_map = 0;
			else
				wining_map += 1;

			Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", custom_list[wining_map].custom_map);
		}
		else
			Com_sprintf(command, sizeof(command), "gamemap \"%s\"\n", level.mapname);
		
		gi.AddCommandString(command);

	}
}
// HYPOV8_END

void CheckVote() // check the timelimit for an admin vote
{
	
	if (level.framenum > (level.voteframe + 1200))
	{
		switch (level.voteset)
		{
			case VOTE_ON_ADMIN:
				safe_bprintf(PRINT_HIGH, "The request for admin has failed!\n");
				break;
		}
		level.voteset = NO_VOTES;
	}
}



int	CheckNameBan (char *name)
{
	char n[64];
	int i;

	strcpy(n,name);
	for (i=0;i<strlen(n);i++) n[i]=tolower(n[i]);
	for (i=0;i<num_netnames;i++) {
		if (strstr(n,netname[i].value)) 
			return true;
	}
	return false;
}

int	CheckPlayerBan(char *userinfo)
{

	char	*value, *test;
	int		i;
	char    temp[22];

	if (num_netnames) //add hypov8, if missing
	{
		value = Info_ValueForKey(userinfo, "name");
		if (CheckNameBan(value))
			return true;
	}

	if (num_ips) //add hypov8, if missing
	{
		value = Info_ValueForKey(userinfo, "ip");

		strcpy(temp, value);
		test = strrchr(temp, ':');
		test[0] = '\0';

		for (i = 0; i < num_ips; i++)
			if (strstr(temp, ip[i].value))
				return true;
	}
    
/*    for (i=0;i<num_ips;i++) {
		isSame = true;
		j = 0;
		while ((isSame) && (value[j] != '\0') && (value[j] != ':'))
		{		
			if (ip[i].value[j] != value[j])
				isSame = false;
			j++;
		}
		if (isSame)
			return true;
	}*/

	return false;
}

/////////////////////////////////////////////////////
// snap - team tags
void setTeamName (int team, char *name) // tical's original code :D
{ 
	if (!name || !*name) return; 

	if (strlen(name)<16 && name[0]!=' ') 
	{
		if(memalloced[team])
			free(team_names[team]);

		team_names[team] = (char *)malloc(16);
		if(team_names[team]!=NULL){
			memalloced[team] = 1;
			sprintf(team_names[team], "%s", name);
		}
	}
}
// snap - new function.
void getTeamTags(){

	int			i;
	edict_t		*doot;
	char		names[2][64][16];
	int			namesLen[2] = { 0, 0 };
	char		teamTag[2][12];
	int			teamTagsFound[2]= { FALSE, FALSE };
	int team;

	for_each_player_inc_bot(doot, i)
	{
		team = doot->client->pers.team;
		if(team && namesLen[team-1] < 64){
			strcpy(names[team-1][namesLen[team-1]++], doot->client->pers.netname);
		}
	}


	for(i=0;i<2;i++){
		int	j;
		for(j=0;j<namesLen[i] && teamTagsFound[i] == FALSE;j++){
			int	k;
			for(k=0;k<namesLen[i] && j != k && teamTagsFound[i] == FALSE;k++){
				char theTag[12];
				int	theTagNum = 0;
				int	y = 0;
				char s = names[i][j][y];
					
				while(s != '\0' && theTagNum == 0){
					int	z = 0;
					char t = names[i][k][z];
					while(t != '\0'){
						if(s == t && s != ' '){ // we have a matched char
							int	posY = y+1;
							int	posZ = z+1;
							char ss = names[i][j][posY];
							char tt = names[i][k][posZ];

							while(ss != '\0' && tt != '\0' && ss == tt && theTagNum < 11){
								if(theTagNum == 0){ // we have two consecutive matches, this is a tag
									theTag[theTagNum++] = s;
									theTag[theTagNum++] = ss;
								}
								else{
									theTag[theTagNum++] = ss;
								}
								ss = names[i][j][++posY];
								tt = names[i][k][++posZ];
							}
						}
						t = names[i][k][++z];
					}
					s = names[i][j][++y];
				}
				if(theTagNum > 0){
					int	e;
					float howmany = 0.0;
					float percentage; 
					theTag[theTagNum] = '\0';
					
					for(e=0;e<namesLen[i];e++){
						if(strstr(names[i][e],theTag) != NULL){
							howmany += 1.0;
						}
					}
					percentage = howmany/(float)namesLen[i]*100.0;
					if(percentage > 75.0){
						strcpy(teamTag[i],theTag);
						teamTagsFound[i] = TRUE;
					}	
				}
			}
		}
	}


	for(i=0;i<2;i++){
		if(teamTagsFound[i] == TRUE){
			setTeamName(i+1,teamTag[i]);
		}
		else if(i==0){
			setTeamName(i+1, "Dragons");
		}
		else if(i==1){
			setTeamName(i+1, "Nikki's Boyz");
		}
	}
}

