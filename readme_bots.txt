comp 1.52 + antilag + ace bots. + some comp 2.0 additions

extracts file to your kingpin folder
eg c:/program files/kingpin/
it will create a folder called compbots

add "exec bot_setup.cfg" to your custom server.cfg. or add the setting in the file manualy as needed
defaults are stated in this file.


===========
config bots
===========
there is a default config file thats loaded for every map located in compbots/bots/_default.cfg
if you require differnt bots per level. create a <mapname>.cfg with the required info
using the "sv addbot" or "sv removebot" will not update the .cfg files


sv addbot thug_bot					(add a bot named thug_bot. can omit the name, will be ThugBot_x)
sv removebot thug_bot					(or sv removebot all)
sv addbot thugBot "male_thug/009 031 031" dragon	(add a bot with specific team and skin)
sv_botskill						(2=default 4=hardest)
sv_bot_allow_add 					(0=Default allow players to vote for add/remove bots)		
sv_bot_allow_skill 					(0=default allow players to vote for skill settings)
sv_bot_max_players					(0=default when set, if a player enters the server. a bot wil be removed if total players+bots are above this value)
sv_bot_max 						(8=default totaly bots allowed to be voted into to server)

===========
<mapname>.cfg
===========
all bots must have <name> <"skin"> <team>
eg...

bot01 "female_chick/005 005 005" dragon




===========
server commands
===========
"sv_antilag"		default=1. server side prediction for hi ping clients
"sv_keeppistol" 	default=1. will keep pilstol in use if a weapon was previously picked up, then switched to pistol
"anti_spawncamp"	default=1. stop players spawn camping


===========
client commands
===========		
"cl_noAntiLag"		1 disables client predeiction on weps. 
"endmap" 		if player is admin, it will go straight to end match
"votemap" 		will allow a non admin to call a map vote

"votebotadd 1" 		will vote to add a bot to team "1" (dragons), can also use "d"
"votebotremove bot_1" 	will vote to remove bot called bot_1
"votebotskill"		will allow vote on changing bot skill level





===========
Developer
===========
bot nodes(paths) are created by defauly as a player moves around a map. they can be edited using these commands

"sv_botpath" 1 		(use player movement to add nodes and join path's.) on default
"sv_botjump" 1  	(stop players creating jump nodes for bots 0=disabled. bad when a player bunnyhops)
"sv savenodes"		(save node file before match has ended)

"sv botdebug" on  	(allow developer commands)

"addnode"    #		(add node type # to currunt playera xyz)
"removelink" # # 	(nodeFrom nodeTo. will remove a link between nodes)
"addlink"    # #	(nodeFrom nodeTo. will add a path between nodes)
"showpath"
"findnode" 		(shows closest node. location/origin)
"movenode" # X Y Z 	(node number then new XYZ. if no xyz specified, will use players current location)
"localnode"		(will make <=15 nodes apear that are surrounding you)

"nodeFinal" 		(marks file to never update rout table again.)



rebound key # for "addnode"
-----------------------
 =ITEM 		(not needed)
 =GRAPPLE 	(NA)
 =PLATFORM 	(not needed)
 =TELEPORTER 	(not needed)
 =DRAGON_SAFE	(not needed)
 =NIKKISAFE	(not needed)

5=WATER 	(not needed)
6=LADDER 	(place on top of ladders)
7=JUMP		(needs to be added where player will jump upto/land)
8=findnode	(locates closets node. then binds key9 to this node)
9=movenode 	(stored from using key 8. will move the note to current location **note** use key with caution)
0=MOVE		(the default node for a bots path)


note: when placing jump or ladder node. make sure they are at the destination(landing/top of ladder)



===========
GeoIP setup
===========
MM can display what country players connect from, using the MaxMind GeoIP
library and database.

On Windows, the "geoip.dll" library (included in the Monkey Mod ZIP) and "geoip.dat"
database should be placed in your kingpin directory.

The latest GeoLite Country database (geoip.dat) can be downloaded from here:
	http://dev.maxmind.com/geoip/legacy/geolite/

On Linux, the GeoIP library and database can be installed from the distro's
repositories.

If you wish to disable country lookups, you can simply remove the GeoIP library
and/or database.




===========
log
===========
bots now aim for feet when using the rocken luncher
aim lower for crouched players


v24
bot will aim at next target sooner once old target dies. speed is x4 of skill
added display of stats in dedicated console at endgame
stoped connecting nodes while in noclip
stoped bots shooting while on a ladder
made bots not connect paths when play does RJ etc. delete old rout files if they look up at a target and spin. 
routs should not connect through fences anymore
added geoip MH:
added admin command. "mapvote". will let you chose next map
botdebug. added command "localnode on". when debugging, nodes that are within your location will be shown. path lines will be disabled
botdebug. changed viewed paths to be shorter. less overflows
botdebug. bound keys 1-10. key8 gets closest node and sets the info to bound key9. for easy relocation
added some bad spawn locations to be moved. 8mile, 420dm1

v25
removed print "connected" for bots
"votemap" changed to now allow non admin to call a vote. all maps are now playable. even in main.
"endmap" admin command added. will end map early and allow all players to vote
"commands" only print available options. non admin list is shorter
added glass/alpha to find enemy calculations
bot now select weapon when picked up. it used to switch when only when attacking
introduced some random best weapons, 3 main groups. top weapon being tommy, hmg and rl.
stoped bot only shooting at closest player. if old target exists. it will keep shooting them
bots now look more to the side for items. instead of just in front
fixed a bug where ammo in weapon was not counted for selectable guns
bots with RL now try to move backwards when to close to an enemy. instead of selecting another weapon
made shotgun reload 3 bullets at once. without animations.
added MH: fix for bad spawn locations

**note you may need to reduce skill in this version**


v26
"nodefinal" debug command. mark .nod file to never be updated. good for distributing custom rout tables. regardless of "sv_botpath"
"sv_botjump" 0 off default. to disable the auto creation of nodes when a player jumps. bunnyhop will make maps a mess.
found missing code not commented in acebots. for jump nodes. code updates map with many node when bunny hoping, so i created sv_botjump default 0.
fixed shared armour issue. was not checking light/heavy properly
bots now die better. they would still continue there movement actions
trace for slopes was not calculated with bounding box. causing wandering bot to get stuck in crates etc.
trace for slope has also been shortened 
added some code to U-turn bot if its stuck making the same decision
fixed issue with bots accurecy. now they miss alot more depending on sv_botskill value.
bug. bots will be stuck and stil have "velocity". velocity is calculated from last frame
when a bot climbed the back of a ladder it would get stuck and die, now uses the 180 u-turn 
made bots try to route around players when trying to get to a goal
decrease bot accuracy if on fire upto 20 units.
lower bot skill's also decreses the angle a bot will detect an enamy, FOV, making them easyer.
votemap now shows map name your are voting for
players with 4 or more frags will be hunted down by bots
stop players auto switching to crow bar when picked up
new cvar sv_keeppistol. if a player selected the pistol after picking up a weapon. it wont switch

v27
added 1 more check on keeping pistol
random taunts for bots when attacking
changed random weps for bots. hmg will get a boost in percent as fav wep
added some check for shooting boozooka above small railings. will aim for head instead of legs
bots with crowbar will now chase you. not stand back like using pistol etc.
bot more accurate. fixed a bug that would calculate bots shot 0.1 sec late. if player strafed bots would miss
fixed votemap issue with more than 1 player
dm players that crashed/dissconected can now reconnect and resume where there score was for DM games
stoped view bob when in spec
bots now pickup droped weapons bassed on ammo count. not wep inventory anymore
when going to spec. you will keep your current position, instead of respawning.
maps with team spawns in dm will get used if every one of them have style set.
pause at player intermision for a few seconds. then allow free roam
added 100ms compensation to antilag.
antilag disabled for bots.
connecting bots now print "bot007 (BOT) connected. BestWep = "Heavy machinegun""
antispawn camp shows gun read while immortal, then shows green when player can be shot. updated times
decrease bot accuracy if on fire. increased to 40 units.
node links are only joined if player is within 92 units from a node. max node dist is 384

v28
fixed teleporter issue. caused by velocity not being reset and crashiing into floor
xx.cfg is now allways used. no longer using old local game style setup.
added votebotadd.  	"votebotadd 1" will add a bot to team "1" dragons
added votebotremove. 	"votebotremove thugbot_1" will remove bot to dragons
added votebotskill. 	"votebotskill 4" will make all bots maximum skill of 4
fixed alot of the bots walking of the edge on floating maps. map must have sky underneath
fix above. also includes a lava check
added more random strafing when attacking
found bot aim issue. pmove calculates movement then shoot dir, causing off target hits when strafing
	-fix is to predict where they will be and set view angle. causing slight off centre but still within target
	-this also effects clients but less noticable because of framerate. bot are 10fps

bots that have just spawned will look for a better weapon, if in range. even in skill 4. stops pistol spamming
"sv_bot_allow_add".   add cvar to allow voteing
"sv_bot_allow_skill". add cvar to allow voteing
"sv_bot_max_players". when set, if a player enters the server. a bot wil be removed if total players+bots are above this value
	-only one bot will be added/removed when a client leaves/enteres at a time. 
	-if this value is lower than the number of bots in <mapname>.cfg the value wil never be as low unless bots are removed manualy
	-will allow added bots above value untill a new player enters then one will be removed
	-does not check count if only 1 player in server. 
	-map change resets bots

"sv_bot_max"	 default 8. totaly bots allowed to be voted into to server
"kick_flamehack" default is now 1. can be set in server config file
"anti_spawncamp" default is now 1  can be set in server config file


**note** auto routing to higher places are disabled. noticed on jump pads not linking. need to manualy add the link
disabled because player may have used rocketjump to get there. N/A.. yet :)


v29
added menu for easy voteing
bot now times out by comparing origin not velocity





VectorCopy(self->s.origin, self->acebot.oldOrigin);


===========
todo
===========
change version number on nav files
prevent bots jumping down from great heights?
incorporated MH: map selector(stop overflow)
incorporated MH: MM 2.0
add different debug node icons for each type
spectate is not rembered in dm on map change?
end game bot comment
MAKE some rout tables
if wep pistol. serch longer for a gun. random..
reloading makes bot serch for weps? WEAPON_RELOADING code not working on bot
bots climbing when "next" to a ladder, like in thin air
bot slows down up some stairs/jumps
node pathes will be recalulated every time its visited. also getting infinate loop in rout tables.
rout tables have values below -1. have updated code for all refrences to be (short)int, fixed???
bots stll miss with skill4. but main issue fixed
remove old bot config. only allow .cfg files
add skill per player
endmap stats to sort...
func plate
bots attacking with pistol and a better wep right next to them. make bot pick it up (not recently spawned)
predict rl direction to shoot
shoot enamy if attacked, regardless of skill?
add a bot think just for shoot? last bot in will look like it missed bot target, because next bots think has not been run yet(repositioned)

**antilag is out 1/2 body length(100ms behind) when tested on lan, same as main game tho. DUNNO WHY???


check node heights items + player droped


===========
maps to test
===========
ladders===
toomuchblood
fragndie3 4
nycdm3_kp
fucked
sym //stairs

oher===
bigcube_v3
nwth
dm_cw
downhere
kp_biodm_v3

stdm5
kpdm11
sonik_e1m7
panzer_tly
dm_fis_b1
team_float

lava==
brutal
sinister 
kpq2dm4
team_crossfire
chrome

crash===
parkgarage
ddkp1

you read down this far!!!!
