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
//  acebot.h - Main header file for ACEBOT
// 
// 
///////////////////////////////////////////////////////////////////////

#ifndef _ACEBOT_H
#define _ACEBOT_H

//add hypov8
#include "../voice_punk.h"
#include "../voice_bitch.h"

//hypov8 func to stop bot looking up/down when not needed
#define ACE_Look_Straight(target,player,out) (out[0]=target[0],out[1]=target[1],out[2]=player[2])

vec3_t ACE_look_out; //hypov8 global var
#define BOT_JUMP_VEL (200*2) //340
#define BOT_FORWARD_VEL (160*2) //340 //hypov8 kp default
#define BOT_SIDE_VEL (160*2) //cl_anglespeedkey->value)	//hypov8 kp default 1.5

//bot time. allow some errors in float. only run every 0.1 seconds anyway
#define BOTFRAMETIME 0.09f

// Only 100 allowed for now (probably never be enough edicts for 'em
#define MAX_BOTS 100

// Platform states
#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

// Maximum nodes
#define MAX_BOTNODES 1000

// Link types
#define INVALID -1

// Node types
#define BOTNODE_MOVE 0
#define BOTNODE_LADDER 1
#define BOTNODE_PLATFORM 2
#define BOTNODE_TELEPORTER 3
#define BOTNODE_ITEM 4
#define BOTNODE_WATER 5
//#define BOTNODE_GRAPPLE 6
#define BOTNODE_JUMP 7
#define BOTNODE_DRAGON_SAFE 8 //hypov8 todo:
#define BOTNODE_NIKKISAFE 9 //hypov8 todo:
#define BOTNODE_ALL 99 // For selecting all nodes

// Density setting for nodes
#define BOTNODE_DENSITY 128
#define BOTNODE_DENSITY_LOCAL (BOTNODE_DENSITY*0.75) //add hypov8 allow only very close node to add a link. 
													//needs to be shorter then 1/2 way betweeen BOTNODE_DENSITY

// Bot state types
#define BOTSTATE_STAND 0
#define BOTSTATE_MOVE 1
#define BOTSTATE_ATTACK 2
#define BOTSTATE_WANDER 3
#define BOTSTATE_FLEE 4

#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_FORWARD 2
#define MOVE_BACK 3


// KingPin Item defines 
#define ITEMLIST_NULL				0
	
#define ITEMLIST_ARMORHELMET		1
#define ITEMLIST_ARMORJACKET		2
#define ITEMLIST_ARMORLEGS			3
#define ITEMLIST_ARMORHELMETHEAVY	4
#define ITEMLIST_ARMORJACKETHEAVY	5
#define ITEMLIST_ARMORLEGSHEAVY		6

//weapon_blackjack"
#define ITEMLIST_CROWBAR			8
#define ITEMLIST_PISTOL				9
#define ITEMLIST_SPISTOL			10
#define ITEMLIST_SHOTGUN			11
#define ITEMLIST_TOMMYGUN			12
#define ITEMLIST_HEAVYMACHINEGUN	13
#define ITEMLIST_GRENADELAUNCHER	14
#define ITEMLIST_BAZOOKA			15
#define ITEMLIST_FLAMETHROWER		16

#define ITEMLIST_GRENADES			17
#define ITEMLIST_SHELLS				18
#define ITEMLIST_BULLETS			19
#define ITEMLIST_ROCKETS			20
#define ITEMLIST_AMMO308			21
#define ITEMLIST_CYLINDER			22
#define ITEMLIST_FLAMETANK			23

//item_coil					24
//item_lizzyhead			25

#define ITEMLIST_CASHROLL			26
#define ITEMLIST_CASHBAGLARGE		27
#define ITEMLIST_CASHBAGSMALL		28
//item_battery				29
//item_jetpack				30
//#define ITEMLIST_SAFEBAG			31 todo fix

#define ITEMLIST_HEALTH_SMALL		31
#define ITEMLIST_HEALTH_LARGE		32
//item_flashlight	33
//item_watch		34
//item_whiskey	35

#define ITEMLIST_PACK				36
#define ITEMLIST_ADRENALINE			37
/*
key_fuse	38
item_safedocs	39
item_valve	40
item_oilcan		41
key_key1 42
key_key2	43
key_key3	44
key_key4	45
key_key5	46
key_key6	47
key_key7	48
key_key8	49
key_key9	50
key_key10	51
*/

#define ITEMLIST_PISTOLMOD_DAMAGE	52
#define ITEMLIST_PISTOLMOD_RELOAD	53
#define ITEMLIST_PISTOLMOD_ROF		54
#define ITEMLIST_HMG_COOL_MOD		55
#define ITEMLIST_SAFEBAG1			56
#define ITEMLIST_SAFEBAG2			57


#define ITEMLIST_BOT				58
#define ITEMLIST_PLAYER				59

typedef struct gitem_s gitem_t;

// Node structure
typedef struct botnode_s
{
	vec3_t origin; // Using Id's representation
	short type;   // type of node

} botnode_t;

typedef struct item_table_s
{
	int item;
	float weight;
	edict_t *ent;
	int node;

} item_table_t;

typedef struct bot_skin_s
{
	char name[32];
	char skin[64];
	char team[32];
} bot_skin_t;

typedef struct //bot->acebot->xxx
{
	qboolean	is_bot;
	qboolean	is_jumping;

	// For movement
	vec3_t		move_vector;
	float		next_move_time;
	float		wander_timeout;
	float		suicide_timeout;

	short	current_node;		// current node
	short	goal_node;			// current goal node
	short	next_node;			// the node that will take us one step closer to our goal
	int		node_timeout;
	short	last_node;
	int			tries;
	int			state;

	//hypo new bot skill func
	int			new_target;			//if new target. dont shoot straight away
	int			old_target;			//old player target. shoot if more than xx seconds
	float		botNewTargetTime;	//timer to allow bot to start attacking
	//cvar_t		*game_dir;			//add game dir to bot libary
	qboolean is_crate; //hypov8 tryto get bot to jump upto item
	int crate_time; 
	int ladder_time; //server frame num bot was on a ladder
	qboolean isOnLadder; //hypov8 add. stop bots aiming when on ladders
	int dodge_time; // time bot last moved sideways from player

	int uTurnCount; //hypov8 count times bot got stuck n turned
	int uTurnTime; //hypov8 get last time bot turned

	int	num_weps; //hypov8 added to compare bots invitory changed. select weapon?
	int randomWeapon; //hypov8 select a random weapon to be there poirity, reset per level

	vec3_t oldOrigin; //hypov8 store last position for calculating velocity
	vec3_t oldAngles;

	qboolean hunted; //bot will attack this persone with brute force:)

	int tauntTime; //hypov8 random taunt timmer
	qboolean aimHead; //hypo aim for head with rl. used when a low fence/rail is blocking player
	vec3_t aimPlayerOrigin; //store origin for think while shooting, was out because of movement b4 shooting cause qwrong aim angles

	int trigPushTimer; // bot will free move with trigger push
	qboolean isMovingUpPushed; 

	int spawnedTime; //store time just spawned, so they can collect better weps

	int last_strafeTime; //frame since strafed. make strafe go for longer
	int last_strafeDir;
} acebot_t;


extern int num_players;
extern int botsRemoved;
extern int num_bots;
extern edict_t *players[MAX_CLIENTS];		// pointers to all players in the game

// extern decs
extern botnode_t nodes[MAX_BOTNODES]; 
extern item_table_t item_table[MAX_EDICTS];
extern qboolean debug_mode;
extern qboolean debug_mode_origin_ents; //add hypov8
extern short numnodes;
extern int num_items;
extern int stopNodeUpdate;		// add hypov8

bot_skin_t randomBotSkins[64];
char VoteBotRemoveName[8][32];
float VoteBotSkill;

void ClientDisconnect(edict_t *ent); //hypov8

// id Function Protos I need
void     LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker);
void     ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker);
void     TossClientWeapon (edict_t *self);
void     ClientThink (edict_t *ent, usercmd_t *ucmd);
void     SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles);
void     ClientUserinfoChanged (edict_t *ent, char *userinfo);
void     CopyToBodyQue (edict_t *ent);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void     Use_Plat (edict_t *ent, edict_t *other, edict_t *activator);

// acebot_ai.c protos
void     ACEAI_Think (edict_t *self);
void     ACEAI_PickLongRangeGoal(edict_t *self);
//void     ACEAI_PickShortRangeGoal(edict_t *self);
//qboolean ACEAI_FindEnemy(edict_t *self);
//void     ACEAI_ChooseWeapon(edict_t *self);
//void     ACEAI_PreChooseWeapon(edict_t *self);//add hypov8
//qboolean ACEAI_WeaponCount(edict_t *self); //add hypov8

// acebot_cmds.c protos
qboolean ACECM_Commands(edict_t *ent);
void     ACECM_Store();

// acebot_items.c protos
void     ACEIT_PlayerAdded(edict_t *ent);
void     ACEIT_PlayerRemoved(edict_t *ent);
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal);
qboolean ACEIT_IsReachable(edict_t *self,vec3_t goal);
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item);
//qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other);
float	 ACEIT_ItemNeed(edict_t *self, int item, float timestamp, int spawnflags); //hypo add spawnflags. for droped items
int		 ACEIT_ClassnameToIndex(char *classname, int style);
//void     ACEIT_BuildItemNodeTable (qboolean rebuild);
//qboolean infrontBot(edict_t *self, edict_t *other); //add hypov8
//qboolean infrontEnemy(edict_t *self, edict_t *other); //add hypov8
float ACEIT_ItemNeedSpawned(edict_t *self, int item, float timestamp, int spawnflags); //add hypov8


// acebot_movement.c protos
//qboolean ACEMV_SpecialMove(edict_t *self,usercmd_t *ucmd);
void     ACEMV_Move(edict_t *self, usercmd_t *ucmd);
void     ACEMV_Attack (edict_t *self, usercmd_t *ucmd);
void     ACEMV_Wander (edict_t *self, usercmd_t *ucmd);

// acebot_nodes.c protos
int      ACEND_FindCost(short from, short to);
short      ACEND_FindCloseReachableNode(edict_t *self, int dist, short type);
short      ACEND_FindClosestReachableNode(edict_t *self, int range, short type);
void     ACEND_SetGoal(edict_t *self, short goal_node);
qboolean ACEND_FollowPath(edict_t *self);
//void     ACEND_GrapFired(edict_t *self);
//qboolean ACEND_CheckForLadder(edict_t *self);
void     ACEND_PathMap(edict_t *self);
void     ACEND_InitNodes(void);
void     ACEND_ShowNode(short node, int isTmpNode);
void     ACEND_DrawPath();
void     ACEND_ShowPath(edict_t *self, short goal_node);
short      ACEND_AddNode(edict_t *self, short type);
void     ACEND_UpdateNodeEdge(short from, short to, qboolean check);
void     ACEND_RemoveNodeEdge(edict_t *self, short from, short to);
//void     ACEND_ResolveAllPaths();
void     ACEND_SaveNodes();
void     ACEND_LoadNodes();
void	ACEND_JumpPadUpdate(edict_t *bot); //add hypov8
void	ACEND_DebugNodesLocal(void); //add hypov8
void ACEND_TeleporterUpdate(edict_t *bot); //add hypov8


// acebot_spawn.c protos
//void	 ACESP_SaveBots();
void	 ACESP_LoadBots();
// void     ACESP_HoldSpawn(edict_t *self);
//void     ACESP_PutClientInServer (edict_t *bot, qboolean respawn, int team);
void     ACESP_Respawn (edict_t *self);
//edict_t *ACESP_FindFreeClient (void);
void     ACESP_SetName(edict_t *bot, char *name, char *skin/*, char *team*/);
void     ACESP_SpawnBot (char *team, char *name, char *skin, char *userinfo);
void	ACESP_SpawnRandomBot(char *team, char *name, char *skin, char *userinfo); //add hypov8
void     ACESP_ReAddBots();
void     ACESP_RemoveBot(char *name);
void	 safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...);
void     safe_centerprintf (edict_t *ent, char *fmt, ...);
void     safe_bprintf (int printlevel, char *fmt, ...);
void     debug_printf (char *fmt, ...);
int ACESP_LoadRandomBotCFG(void);// load custom bot file

//hypo
//tace for pmove
//trace_t	PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
#endif