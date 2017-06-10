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
//  acebot_movement.c - This file contains all of the 
//                      movement routines for the ACE bot
//           
///////////////////////////////////////////////////////////////////////

#include "..\g_local.h"
#include "acebot.h"

vec3_t ACE_look_out; //hypov8 global var

static qboolean ACEMV_CheckLavaAndSky(edict_t *self);
#define CHECKSKYDOWNDIST 3072

///////////////////////////////////////////////////////////////////////
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime. 
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_CanMove(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset,start,end;
	vec3_t angles;
	trace_t tr;

	// Now check to see if move will move us off an edgemap team_fast_cash
	VectorCopy(self->s.angles,angles);
	
	if(direction == MOVE_LEFT)
		angles[1] += 90;
	else if(direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == MOVE_BACK)
		angles[1] -=180;

	// Set up the vectors
	AngleVectors (angles, forward, right, NULL);
	
	VectorSet(offset, 36, 0, 24);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
		
	VectorSet(offset, 36, 0, -400);
	G_ProjectSource (self->s.origin, offset, forward, right, end);
	
	tr = gi.trace(start, NULL, NULL, end, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
	
	if(tr.fraction > 0.3 && tr.fraction != 1 || tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//if(debug_mode)
		//	debug_printf("%s: move blocked\n",self->client->pers.netname); //hypov8 disabled debug
		return false;	
	}
	
	return true; // yup, can move
}

//hypov8 added to check flat ground/edge
static qboolean ACEMV_CanMove_Simple(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset, end, down;
	vec3_t angles;
	trace_t tr;
	vec3_t minx = { -4, -4, 0 }; //was 24
	vec3_t maxx = { 4, 4, 0 }; //was 24
	vec3_t minWall = { -16, -16, 0 }; // fix for steps
	vec3_t maxWall = { 16, 16, 48 }; 

	// Now check to see if move will move us off an edgemap team_fast_cash
	VectorCopy(self->s.angles, angles); //MOVE_FORWARD

	if (direction == MOVE_LEFT)
		angles[1] += 90;
	else if (direction == MOVE_RIGHT)
		angles[1] -= 90;
	else if (direction == MOVE_BACK)
		angles[1] -= 180;

	// Set up the vectors
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 24, 0, 0); //hypo low value. incase steps
	G_ProjectSource(self->s.origin, offset, forward, right, end);

	tr = gi.trace(self->s.origin, minWall, maxWall, end, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);

	if (tr.fraction != 1 ) //wall hit
	{
		//if(debug_mode)
		//	debug_printf("%s: move blocked\n",self->client->pers.netname); //hypov8 disabled debug
		return false;
	}
	else //check for falling off edge
	{
		VectorSet(offset, 48, 0, 0); //hypo increase?
		G_ProjectSource(self->s.origin, offset, forward, right, end);

		VectorCopy(end, down);
		down[2] -= CHECKSKYDOWNDIST;
		tr = gi.trace(end, minx, maxx, down, self, MASK_BOT_DROP_SKY);
			///VectorCopy(start, down);

		if (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
			return false;
		if ((tr.surface->flags & SURF_SKY) && !tr.startsolid)
			return false;
	
	}


	return true; // yup, can move
}

///////////////////////////////////////////////////////////////////////
// Handle special cases of crouch/jump
//
// If the move is resolved here, this function returns
// true.
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_SpecialMove(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dir,forward,right,start,end,offset;
	vec3_t top, tmpStart, tmpEnd;
	trace_t tr, tr_monster; 
	
	// Get current direction
	//VectorCopy(self->client->ps.viewangles,dir);
	//dir[YAW] = self->s.angles[YAW];
	//hypov8 calculate straight ahead
	VectorSet(dir, 0, self->s.angles[YAW], 0);

	AngleVectors (dir, forward, right, NULL);

	VectorSet(offset, 18, 0, 0);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	offset[0] += 18;
	G_ProjectSource(self->s.origin, offset, forward, right, end);
	
	/////////////////////////////////
	VectorCopy(self->s.origin, start); //hypov8 scan from player centre? not outside player BBox

	VectorCopy(start, tmpStart);
	VectorCopy(end, tmpEnd);

	// trace it
	start[2] += 18; // so they are not jumping all the time
	end[2] += 18;
	tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE );
	tr_monster = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE_MON);

#if 1
	//hypo check if a player is stoping bot
	if ((tr_monster.contents & CONTENTS_MONSTER 
		|| strcmp(tr_monster.ent->classname, "player") == 0 
		|| strcmp(tr_monster.ent->classname, "bot") == 0)
		&& tr.fraction == 1
		&& tr_monster.fraction != 1
		&& self->acebot.dodge_time < level.framenum )
		{
			int i;
			vec3_t forward2, right2;
			vec3_t offset2, end2;
			vec3_t angles2;
			trace_t tr_sides;

			
			for (i = 0; i<2; i++)
			{
				VectorCopy(self->s.angles, angles2);
				if (i == 0 && ACEMV_CanMove_Simple(self, MOVE_LEFT))
					angles2[1] += 90;//MOVE_LEFT
				else
					angles2[1] -= 90; //MOVE_RIGHT

				AngleVectors(angles2, forward2, right2, NULL);
				VectorSet(offset2, 36, 0, 0);
				G_ProjectSource(self->s.origin, offset, forward2, right2, end2);

				tr_sides = gi.trace(self->s.origin, NULL, NULL, end2, self, MASK_BOT_SOLID_FENCE_MON);

				if (tr_sides.fraction == 1)
				{
					if (i == 0)
					{
						self->acebot.dodge_time = level.framenum+2;
						self->s.angles[1] += 45;
						ucmd->forwardmove = BOT_FORWARD_VEL;
						ucmd->sidemove = -BOT_SIDE_VEL;
						return true;
					}
					else
					{
						self->acebot.dodge_time = level.framenum +2;
						self->s.angles[1] -= 45;
						ucmd->forwardmove = BOT_FORWARD_VEL;
						ucmd->sidemove = BOT_SIDE_VEL;
						return true;
					}

				}
			}

		}
#endif


	//hypov8 if infront is not totaly solid. try to move to there, check jump then crouch
	if(/*tr.allsolid*/ tr.fraction != 1.0)
	{	
		//////////////////////////
		// Check for jump       //
		//////////////////////////

		// Check for jump up a step 60 units
		start[2] = tmpStart[2] + 60;//hypov8 max height we can jump is 60
		end[2] = tmpEnd[2] + 60;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 60;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
		// Check for jump up a step 40 units
		start[2] = tmpStart[2] + 40;
		end[2] = tmpEnd[2] + 40;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 42;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
		// Check for jump up a step 32 units
		start[2] = tmpStart[2] + 32;
		end[2] = tmpEnd[2] + 32;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 34;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}

		// Check for jump up a step 24 units
		start[2] = tmpStart[2] + 24;
		end[2] = tmpEnd[2] + 24;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 26;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
		
		
		
		///////////////////////// 
		// Check for crouching // hypov8 crouch. kp 72-48 height.
		/////////////////////////
		VectorCopy(self->maxs,top);
		top[2] = 24; // crouching height (DUCKING_MAX_Z = 24)

		start[2] = tmpStart[2]; 
		end[2] = tmpEnd[2];
		tr = gi.trace(start, self->mins, top, end, self, MASK_BOT_SOLID_FENCE);
		
		// Crouch
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = -400; 
			return true;
		}

		// Check for crouching up a step 8 units
		start[2] = tmpStart[2] + 8;
		end[2] = tmpEnd[2] + 8;
		tr = gi.trace(start, self->mins, top, end, self, MASK_BOT_SOLID_FENCE);

		// Crouch
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = -400;
			return true;
		}
		// Check for crouching up a step 16 units
		start[2] = tmpStart[2] + 16;// Check for crouching up a step 16 units
		end[2] = tmpEnd[2] + 16;
		tr = gi.trace(start, self->mins, top, end, self, MASK_BOT_SOLID_FENCE);

		// Crouch
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = -400;
			return true;
		}
		
		//////////////////////////
		// Check for jump       //
		//////////////////////////
		start[2] = tmpStart[2] + 60;//hypov8 max height we can jump is 60
		end[2] = tmpEnd[2] + 60;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		// Check for jump up a step 60 units
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{	
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 60;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
		// Check for jump up a step 40 units
		start[2] = tmpStart[2] + 40;
		end[2] = tmpEnd[2] + 40;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 42;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}
		// Check for jump up a step 32 units
		start[2] = tmpStart[2] + 32;
		end[2] = tmpEnd[2] + 32;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);

		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 34;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}

		// Check for jump up a step 24 units
		start[2] = tmpStart[2] + 24;
		end[2] = tmpEnd[2] + 24;
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_BOT_SOLID_FENCE);
		if (/*!tr.allsolid*/ tr.fraction == 1.0)
		{
			float dist;
			vec3_t	forward;
			ucmd->forwardmove = BOT_FORWARD_VEL;
			ucmd->upmove = BOT_JUMP_VEL; //hypo was 400

			dist = 26;
			AngleVectors(self->s.angles, forward, NULL, NULL);
			VectorScale(forward, dist, self->velocity);
			self->velocity[2] = dist;

			if (self->groundentity)
				self->groundentity = NULL;

			self->acebot.is_crate = true;
			self->acebot.crate_time = level.framenum + 5;

			return true;
		}


	}
	
	return false; // We did not resolve a move here
}

///////////////////////////////////////////////////////////////////////
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
// 
// Modified from the original id ChangeYaw code...
///////////////////////////////////////////////////////////////////////
static void ACEMV_ChangeBotAngle(edict_t *ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vec3_t  ideal_angle;

	// Normalize the move angle first
	VectorNormalize(ent->acebot.move_vector);

	current_yaw = anglemod(ent->s.angles[YAW]);
	current_pitch = anglemod(ent->s.angles[PITCH]);

	vectoangles(ent->acebot.move_vector, ideal_angle);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);

	// Yaw
	if (current_yaw != ideal_yaw)
	{
		move = ideal_yaw - current_yaw;
		speed = ent->yaw_speed;
		if (ideal_yaw > current_yaw)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[YAW] = anglemod(current_yaw + move);
	}

	// Pitch
	if (current_pitch != ideal_pitch)
	{
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed;
		if (ideal_pitch > current_pitch)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}
		ent->s.angles[PITCH] = anglemod(current_pitch + move);
	}
}


///////////////////////////////////////////////////////////////////////
// Checks for obstructions in front of bot
//
// This is a function I created origianlly for ACE that
// tries to help steer the bot around obstructions.
//
// If the move is resolved here, this function returns true.
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_CheckEyes(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  forward, right;
	vec3_t  leftstart, rightstart, focalpoint;
	vec3_t  upstart, upend;
	vec3_t  dir, offset;
	vec3_t	minx = { -12, -12, -22 };	//set new bb size, allow some errors from not walking totaly straight
	vec3_t	maxx = { 12, 12, 48 };
	qboolean isPlayerInfront = 0;

	trace_t traceRight, traceLeft, traceUp, traceFront, tr_monster; // for eyesight

	//set crouch if needed
	maxx[2] =self->maxs[2];

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles, dir);
	AngleVectors(dir, forward, right, NULL);

	// Let them move to targets by walls
	if (!self->movetarget)
		VectorSet(offset, 200, 0, 4); // focalpoint 
	else
		VectorSet(offset, 36, 0, 4); // focalpoint 

	G_ProjectSource(self->s.origin, offset, forward, right, focalpoint);

	// Check from self to focalpoint
	// Ladder code
	VectorSet(offset, 36, 0, 0); // set as high as possible
	G_ProjectSource(self->s.origin, offset, forward, right, upend);
	traceFront = gi.trace(self->s.origin, minx, maxx, upend, self, MASK_BOT_SOLID_FENCE);
	tr_monster = gi.trace(self->s.origin, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE_MON);

	if (traceFront.contents & CONTENTS_LADDER) // using detail brush here cuz sometimes it does not pick up ladders...??
	{
		ucmd->upmove = BOT_JUMP_VEL;
		//ucmd->forwardmove = 30; //ToDo: "0" has caused bot to die? kpdm4. to far away
		ucmd->upmove = 400; //add hypov8
		//self-> movetarget->s.origin, self->s.origin, ACE_look_out);

		self->acebot.ladder_time = level.framenum + 1;
		self->acebot.isOnLadder = true; //hypo stop bot attacking on ladders
		return true;
	}

#if 1 //hypo check if a player is stoping bot
	if ((tr_monster.contents & CONTENTS_MONSTER
		|| strcmp(tr_monster.ent->classname, "player") == 0
		|| strcmp(tr_monster.ent->classname, "bot") == 0)
		&& traceFront.fraction == 1
		&& tr_monster.fraction != 1
		/*&& self->acebot.dodge_time < level.framenum*/)
	{
		if (tr_monster.ent->client && tr_monster.ent->client->ps.pmove.pm_type == PM_NORMAL) //hypo make sure there not in noclip/dead etc
		{
			int i;
			vec3_t forward2, right2;
			vec3_t offset2, end2;
			vec3_t angles2;
			trace_t tr_sides;

			for (i = 0; i < 2; i++)
			{
				VectorCopy(self->s.angles, angles2);
				if (i == 0 && ACEMV_CanMove_Simple(self, MOVE_LEFT))
					angles2[1] += 90;//MOVE_LEFT
				else
					angles2[1] -= 90; //MOVE_RIGHT

				AngleVectors(angles2, forward2, right2, NULL);
				VectorSet(offset2, 36, 0, 0);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);

				tr_sides = gi.trace(self->s.origin, NULL, NULL, end2, self, MASK_BOT_SOLID_FENCE_MON);

				if (tr_sides.fraction == 1)
				{
					if (i == 0)
					{
						if (self->acebot.dodge_time < level.framenum)
						{
							self->acebot.dodge_time = level.framenum + 2;
							self->s.angles[1] += 45;
							ucmd->sidemove = -BOT_SIDE_VEL;
						}
						ucmd->forwardmove = BOT_FORWARD_VEL;
						return true;
					}
					else
					{
						if (self->acebot.dodge_time < level.framenum)
						{
							self->acebot.dodge_time = level.framenum + 2;
							self->s.angles[1] -= 45;
							ucmd->sidemove = BOT_SIDE_VEL;
						}
						ucmd->forwardmove = BOT_FORWARD_VEL;
						return true;
					}

				}
			}
		}
	}
#endif

	// If this check fails we need to continue on with more detailed checks
	if (traceFront.fraction == 1)
	{
		 if (ACEMV_CheckLavaAndSky(self) == 2) //add hypov8
			 return true;// standing on sky. dont move, die!!!
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return true;
	}
#if 0
	//hypo bot wandering, no path, trace small step, return can move
	{
		vec3_t  stepstart, stepend;
		VectorSet(offset, 0, 0, 18);
		G_ProjectSource(self->s.origin, offset, forward, right, stepstart);
		VectorSet(offset, 8, 0, 18);
		G_ProjectSource(self->s.origin, offset, forward, right, stepend);

		traceFront = gi.trace(stepstart, self->mins, self->maxs, stepend, self, MASK_BOT_SOLID_FENCE);
		if (traceFront.fraction == 1 && !traceFront.allsolid && !traceFront.startsolid)
		{
			vec3_t minTmp, maxTmp;
			VectorSet(minTmp, -12, -12, -24);
			VectorSet(maxTmp, 12, 12, 48);
			//make sure we are clear for further and up
			VectorSet(offset, 36, 0, 52);
			G_ProjectSource(self->s.origin, offset, forward, right, stepend);
			traceFront = gi.trace(stepstart, minTmp, maxTmp, stepend, self, MASK_BOT_SOLID_FENCE);
			if (traceFront.fraction == 1 && !traceFront.allsolid && !traceFront.startsolid)
			{
				//hypov8 ToDo: moving slow up some stairs
				ucmd->forwardmove = BOT_FORWARD_VEL;
				return true;
			}
		}
	}
#endif

	VectorSet(offset, 0, 18, 4);
	G_ProjectSource(self->s.origin, offset, forward, right, leftstart);

	offset[1] -= 36; // want to make sure this is correct
	//VectorSet(offset, 0, -18, 4);
	G_ProjectSource(self->s.origin, offset, forward, right, rightstart);

	traceRight = gi.trace(rightstart, NULL, NULL, focalpoint, self, MASK_BOT_SOLID_FENCE);
	traceLeft =  gi.trace(leftstart , NULL, NULL, focalpoint, self, MASK_BOT_SOLID_FENCE);

	// Wall checking code, this will degenerate progressivly so the least cost 
	// check will be done first.

	// If open space move ok
	//hypov8 if slowed down, check and turn as needed
	//keep moveing forward if a door
	if ((traceRight.fraction != 1 || traceLeft.fraction != 1 || isPlayerInfront)
		&&( strcmp(traceLeft.ent->classname, "func_door") != 0
		&& strcmp(traceLeft.ent->classname, "func_door_rotating") != 0) 
		&& !self->acebot.is_crate)
	{

		// Special uppoint logic to check for slopes/stairs/jumping etc.
		VectorSet(offset, 0, 0, 24); //hypov8 was  (, 0, 18, 24)
		G_ProjectSource(self->s.origin, offset, forward, right, upstart);

		VectorSet(offset, 0, 0, 48); // scan for height above head
		G_ProjectSource(self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE); //hypo min/max was null

		VectorSet(offset, 32, 0, 48 * traceUp.fraction /*- 5*/); // set as high as possible //hypov8 find roof??
		G_ProjectSource(self->s.origin, offset, forward, right, upend);
		traceUp = gi.trace(upstart, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE); //hypo min/max was null

		// If the upper trace is not open, we need to turn.
		if (traceUp.fraction != 1 /*|| (traceUp.allsolid || traceUp.startsolid)*/)
		{
			//hypov8 check time last uturned
			if (self->acebot.uTurnTime < level.framenum)
				self->acebot.uTurnCount = 0;

			if (traceRight.fraction > traceLeft.fraction)
			{
				self->s.angles[YAW] += (1.0 - traceLeft.fraction) * 45.0;

				//hypov8 add to uturn count
				if (self->acebot.uTurnTime > level.framenum)
					self->acebot.uTurnCount += 1;
				self->acebot.uTurnTime = level.framenum + 2;
			}
			else
			{
				self->s.angles[YAW] += -(1.0 - traceRight.fraction) * 40.0; //hypov8 was 45. stop bot walking in circles

				//hypov8 add to uturn count
				if (self->acebot.uTurnTime > level.framenum)
					self->acebot.uTurnCount += 1;
				self->acebot.uTurnTime = level.framenum + 2;
			}

			//U-turn when we keep spinning in circles
			if (self->acebot.uTurnCount > 20)
			{
				if (debug_mode && !debug_mode_origin_ents) //add hypo stop console nag when localnode is on )
					debug_printf(" *BOT STUCK* %s U-turn\n", self->client->pers.netname);
				self->s.angles[YAW] += 180.0;
				self->acebot.uTurnTime = 0;
			}
			ACEMV_CheckLavaAndSky(self); //add hypov8 //todo: check this
			ucmd->forwardmove = BOT_FORWARD_VEL;
			return true;
		}
	}	
	if (ACEMV_CheckLavaAndSky(self)== 1)
		return true; //add hypov8

	return false;
}

//hypov8
//func to stop bot looking up/down when not needed
//#define ACE_Look_Straight(target,player,out) (out[0]=target[0],out[1]=target[1],out[2]=player[2])
/*
void ACE_Look_Straight(vec3_t player, vec3_t target, vec3_t out)
{
	vec3_t out;
	out[0] = target[0];
	out[1] = target[1];
	out[2] = player[2]; //copy player height
}
*/
///////////////////////////////////////////////////////////////////////
// Set bot to move to it's movetarget. (following node path)
///////////////////////////////////////////////////////////////////////
static qboolean ACEMV_MoveToGoal(edict_t *self, usercmd_t *ucmd)
{
	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	if(strcmp(self->movetarget->classname,"rocket")==0)
	{
#if 0
		ACE_Look_Straight(self->movetarget->s.origin, self->s.origin, ACE_look_out);
		VectorSubtract(/*self->movetarget->s.origin*/ACE_look_out, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
#else
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);

#endif
		//if(debug_mode)
			//debug_printf("%s: Oh crap a rocket!\n",self->client->pers.netname);
		
		// strafe left/right
		if (rand() % 1 && ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT))
			ucmd->sidemove = -BOT_SIDE_VEL;
		else if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT))
			ucmd->sidemove = BOT_SIDE_VEL;
		return true;

	}
// acebot //hypo change grenade to move back. disable walk forward to?
	else if (strcmp(self->movetarget->classname, "grenade") == 0)
	{
#if 0
		ACE_Look_Straight(self->movetarget->s.origin, self->s.origin, ACE_look_out);
		VectorSubtract(/*self->movetarget->s.origin*/ACE_look_out, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);
#else
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		ACEMV_ChangeBotAngle(self);

#endif
		//if (debug_mode)
		//	debug_printf("%s: Oh crap a rocket!\n", self->client->pers.netname);

		// strafe left/right
		if (rand() % 1 && ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
		{
			ucmd->forwardmove = -BOT_FORWARD_VEL;
			ucmd->sidemove = 0;
		}
		else if (ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove(self, MOVE_BACK)
			&& ACEMV_CanMove_Simple(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_BACK))
		{
			ucmd->sidemove = BOT_SIDE_VEL;
			ucmd->forwardmove = -BOT_FORWARD_VEL;
		}
		else if (ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT))
			ucmd->sidemove = -BOT_SIDE_VEL;
		return true;

	}
	else
	{
		//hypov8 make bot crough if close to cash
		if (self->movetarget)
		{
			if (strcmp(self->movetarget->classname, "item_cashroll")==0)
			{
				vec3_t v;
				float distToTarget;

				VectorSubtract(self->s.origin, self->movetarget->s.origin, v);
				distToTarget = VectorLength(v);
				if (distToTarget <= 32)
					ucmd->upmove = -400;
			}
		}

		// Set bot's movement direction
#if 0
		ACE_Look_Straight(self->movetarget->s.origin, self->s.origin, ACE_look_out);
		VectorSubtract(/*self->movetarget->s.origin*/ACE_look_out, self->s.origin, self->acebot.move_vector);
#else
		self->client->ps.viewangles[2] = 0;
		VectorSubtract(self->movetarget->s.origin, self->s.origin, self->acebot.move_vector);
		//ACEMV_ChangeBotAngle(self);

#endif
		ACEMV_ChangeBotAngle(self);
		ucmd->forwardmove = BOT_FORWARD_VEL;
		return false;
	}
}

///////////////////////////////////////////////////////////////////////
// Main movement code. (following node path)
///////////////////////////////////////////////////////////////////////
void ACEMV_Move(edict_t *self, usercmd_t *ucmd)
{
	vec3_t dist;
	int current_node_type=-1;
	int next_node_type=-1;
	int i;
	qboolean isExplosive = 0;


	//hypov8 jumping upto crate timmer
	if (self->acebot.is_crate)
	{
		if (level.framenum > self->acebot.crate_time)
			self->acebot.is_crate = false;
	}
		
	// Get current and next node back from nav code.
	if(!ACEND_FollowPath(self))
	{
		self->acebot.state = BOTSTATE_WANDER;
		self->acebot.wander_timeout = level.time + 1.0;
		return;
	}

	current_node_type = nodes[self->acebot.current_node].type;
	next_node_type = nodes[self->acebot.next_node].type;
		
	///////////////////////////
	// Move To Goal
	///////////////////////////
	if (self->movetarget)
	{
		isExplosive = ACEMV_MoveToGoal(self, ucmd);
	}

/*	////////////////////////////////////////////////////////
	// Grapple
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_GRAPPLE)
	{
		ACEMV_ChangeBotAngle(self);
		ACEIT_ChangeWeapon(self,FindItem("grapple"));	
		ucmd->buttons = BUTTON_ATTACK;
		return;
	}
	// Reset the grapple if hangin on a graple node
	if(current_node_type == BOTNODE_GRAPPLE)
	{
		CTFPlayerResetGrapple(self);
		return;
	}*/
	
	////////////////////////////////////////////////////////
	// Platforms
	///////////////////////////////////////////////////////
	if(current_node_type != BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// check to see if lift is down?
		for(i=0;i<num_items;i++)
			if (item_table[i].node == self->acebot.next_node)
				if(item_table[i].ent->moveinfo.state != STATE_BOTTOM)
				    return; // Wait for elevator
	}
	if(current_node_type == BOTNODE_PLATFORM && next_node_type == BOTNODE_PLATFORM)
	{
		// Move to the center
		self->acebot.move_vector[2] = 0; // kill z movement	
		if (VectorLength(self->acebot.move_vector) > 10)
			ucmd->forwardmove = (BOT_FORWARD_VEL / 2); // walk to center
				
		ACEMV_ChangeBotAngle(self);
		
		return; // No move, riding elevator
	}

	////////////////////////////////////////////////////////
	// Jumpto Nodes
	///////////////////////////////////////////////////////
	if(next_node_type == BOTNODE_JUMP || 
		(current_node_type == BOTNODE_JUMP && next_node_type != BOTNODE_ITEM && nodes[self->acebot.next_node].origin[2] > self->s.origin[2]))
	{
		// Set up a jump move
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = BOT_JUMP_VEL;

		ACEMV_ChangeBotAngle(self);

		VectorCopy(self->acebot.move_vector, dist);
		VectorScale(dist,440,self->velocity);

		return;
	}
	
	////////////////////////////////////////////////////////
	// Ladder Nodes
	///////////////////////////////////////////////////////
	if (next_node_type == BOTNODE_LADDER && nodes[self->acebot.next_node].origin[2] > self->s.origin[2])
	{
		// Otherwise move as fast as we can
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = 400; ///add hypov8
		//self->velocity[2] = 400;
		
		ACEMV_ChangeBotAngle(self);

		 //hypo stop bot attacking on ladders
		self->acebot.ladder_time = level.framenum + 1;
		self->acebot.isOnLadder = true;
		return;

	}
	// If getting off the ladder
	if(current_node_type == BOTNODE_LADDER && next_node_type != BOTNODE_LADDER &&
		nodes[self->acebot.next_node].origin[2] > self->s.origin[2])
	{
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = 50;
		self->velocity[2] = 50;
		ACEMV_ChangeBotAngle(self);

		//hypo stop bot attacking on ladders //todo check if its to loog once off, should we allow attack
		self->acebot.ladder_time = level.framenum + 1;
		self->acebot.isOnLadder = true;
		return;
	}

	////////////////////////////////////////////////////////
	// Water Nodes
	///////////////////////////////////////////////////////
	if(current_node_type == BOTNODE_WATER)
	{
		// We need to be pointed up/down
		ACEMV_ChangeBotAngle(self);

		// If the next node is not in the water, then move up to get out.
		if (next_node_type != BOTNODE_WATER && !(gi.pointcontents(nodes[self->acebot.next_node].origin) & MASK_WATER)) // Exit water
			ucmd->upmove = BOT_JUMP_VEL;
		
		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
		return;

	}
	
	// Falling off ledge? //hypov8 or on ladder
	if (!self->groundentity && !self->acebot.isMovingUpPushed)
	{
		//hypov8 double check
		// Check from self to focalpoint
		// Ladder code
		vec3_t  forward, right;
		//vec3_t  leftstart, rightstart, focalpoint;
		vec3_t  upend;
		vec3_t  dir, offset;
		trace_t traceFront; // for eyesight
		//vec3_t ladderViewMin = { -16, -16, 24 };
		//vec3_t ladderViewMax = { 16, 16, 28 }; //thin ladder cods?

		// Get current angle and set up "eyes"
		VectorCopy(self->s.angles, dir);
		//dir z =0?
		AngleVectors(dir, forward, right, NULL);

		VectorSet(offset, 4, 0, 0); // set as high as possible
		G_ProjectSource(self->s.origin, offset, forward, right, upend);
		traceFront = gi.trace(self->s.origin, self->mins, self->maxs, upend, self, MASK_BOT_SOLID_FENCE);

		if (traceFront.contents & CONTENTS_LADDER) // using detail brush here cuz sometimes it does not pick up ladders...??
		{
			self->acebot.move_vector[2] = 0;
			ACEMV_ChangeBotAngle(self); 

			ucmd->upmove = BOT_JUMP_VEL;
			ucmd->forwardmove = BOT_FORWARD_VEL *0.75;

			self->acebot.ladder_time = level.framenum + 1;
			self->acebot.isOnLadder = true; //hypo stop bot attacking on ladders
			return;
		}
		else
		{
			ACEMV_ChangeBotAngle(self);
			self->velocity[0] = self->acebot.move_vector[0] * 360;
			self->velocity[1] = self->acebot.move_vector[1] * 360;
			return;
		}
		
	}
	//hypov8 add. if node just above us, jump. fix missplaced nodes
	if (/*current_node_type*/ next_node_type == BOTNODE_MOVE)
	{
		//if (self->goalentity)	
		if ((nodes[self->acebot.next_node].origin[2] - self->s.origin[2]) >= 15 &&
			(nodes[self->acebot.next_node].origin[2] - self->s.origin[2]) <= 48)
		{
			vec3_t v;
			float distToTarget;
			VectorSubtract(self->s.origin, nodes[self->acebot.next_node].origin, v);
			distToTarget = VectorLength(v);
			if (distToTarget <= 64)
			{
				ucmd->upmove = BOT_JUMP_VEL;
			}
		}

	}
		
	// Check to see if stuck, and if so try to free us
	// Also handles crouching
	if (VectorLength(self->velocity) < 37 && !self->acebot.isMovingUpPushed)
	{
		// Keep a random factor just in case....
		if(random() > 0.1 && ACEMV_SpecialMove(self, ucmd))
			return;
		
		self->s.angles[YAW] += random() * 180 - 90; 

		ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}

	// Otherwise move as fast as we can
	if (!isExplosive)
		ucmd->forwardmove = BOT_FORWARD_VEL;


	if (self->acebot.isMovingUpPushed && next_node_type == BOTNODE_MOVE)
	{
		vec3_t angles;
		//self->client->ps.viewangles[2] = 0;
		//VectorSubtract(nodes[self->acebot.next_node].origin, self->s.origin, self->acebot.move_vector);


		VectorSubtract(nodes[self->acebot.next_node].origin, self->s.origin, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);


	}


	ACEMV_ChangeBotAngle(self);
	
}


///////////////////////////////////////////////////////////////////////
// Wandering code (based on old ACE movement code) 
///////////////////////////////////////////////////////////////////////
void ACEMV_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;
	qboolean isExplosive = 0;
	// Do not move
	if (self->acebot.next_move_time > level.time)
		return; //todo: func plate timmer. check if stuck underneath, allow move

	//hypov8 jumping upto crate timmer
	if (self->acebot.is_crate)
	{
		if (level.framenum > self->acebot.crate_time)
			self->acebot.is_crate = false;
	}

	self->s.angles[PITCH] = 0; //hypov8 reset pitch

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
		if(self->groundentity->moveinfo.state == STATE_UP ||
		   self->groundentity->moveinfo.state == STATE_DOWN) // only move when platform not
		{
			self->velocity[0] = 0;
			self->velocity[1] = 0;
			self->velocity[2] = 0;
			self->acebot.next_move_time = level.time + 0.5;
			return;
		}
	
	
	// Is there a target to move to
	if (self->movetarget)
		isExplosive = ACEMV_MoveToGoal(self, ucmd);
		
	////////////////////////////////
	// Swimming?
	////////////////////////////////
	VectorCopy(self->s.origin,temp); //hypov8 swimming, water depth???
	temp[2]+=24;

	if(gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if(self->client->next_drown_time > 0)
		{
			ucmd->upmove = 1;	
			self->s.angles[PITCH] = -45; 
		}
		else
		{
			if (self->goalentity )
			{
				int index;
				float weight;
				index = ACEIT_ClassnameToIndex(self->goalentity->classname, self->style); //hypov8 add safe styles
				weight = ACEIT_ItemNeed(self, index, 0.0, self->goalentity->spawnflags);

				if (weight > 0.0f)
				{
					vec3_t angles;
					// Set direction
					VectorSubtract(self->goalentity->s.origin, self->s.origin, self->acebot.move_vector);
					vectoangles(self->acebot.move_vector, angles);
					VectorCopy(angles, self->s.angles);
					ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
				}
				//self->movetarget->s.origin;
				else
					ucmd->upmove = BOT_JUMP_VEL;
			}
			else
			{
				ucmd->upmove = BOT_JUMP_VEL;
				self->s.angles[PITCH] = -10;
			}
		}

		ucmd->forwardmove = BOT_FORWARD_VEL *0.75;
	}
	else
		self->client->next_drown_time = 0; // probably shound not be messing with this, but
	
	////////////////////////////////
	// Lava?
	////////////////////////////////
	temp[2]-=48;	//hypov8 was 48
	if(gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		//	safe_bprintf(PRINT_MEDIUM,"lava jump\n");
		self->s.angles[YAW] += random() * 360 - 180; 
		ucmd->forwardmove = BOT_FORWARD_VEL;
		ucmd->upmove = BOT_JUMP_VEL;
		return;
	}

	if (!self->acebot.isMovingUpPushed)
		if(ACEMV_CheckEyes(self,ucmd))
			return;


#if 0 // add hypo dont fall to death or lava
	{
		vec3_t dir, forward, right, offset,start, down;
		trace_t trace; // for eyesight
		vec3_t minx = { 0, 0, -24 };
		vec3_t maxx = { 0, 0, -24 };

		// Get current angle and set up "eyes"
		VectorCopy(self->s.angles, dir);
		AngleVectors(dir, forward, right, NULL);
		VectorSet(offset, 64, 0, 0); // focalpoint 
		G_ProjectSource(self->s.origin, offset, forward, right, start);
		VectorCopy(forward, down);
		down[2] -= CHECKSKYDOWNDIST;

		trace = gi.trace(start, self->mins, self->maxs, down, self, CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_SOLID);
		if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
		{
			self->s.angles[YAW] += 90;
			return;
		}
		if ((trace.surface->flags & SURF_SKY))
		{
			self->s.angles[YAW] += 90;
			return;
		}
	}

#endif
	// Check for special movement if we have a normal move (have to test)
	if (VectorLength(self->velocity) < 37 && !self->acebot.isMovingUpPushed) //hypov8 jump pad
	{
		//vec3_t start;
		if(random() > 0.1 && ACEMV_SpecialMove(self,ucmd))
			return;

		self->s.angles[YAW] += random() * 180 - 90; 

		if(!M_CheckBottom(self) || !self->groundentity) // if there is ground continue otherwise wait for next move
			ucmd->forwardmove = BOT_FORWARD_VEL;
		
		return;
	}

	//look in direction of movement
	if (self->acebot.isMovingUpPushed)
	{
		vec3_t  tmp2, angles;

		VectorCopy(self->acebot.oldOrigin, tmp2);
		tmp2[2] = self->s.origin[2];

		VectorSubtract(self->s.origin, tmp2, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles, self->s.angles);
	}

	if (!isExplosive)
		ucmd->forwardmove = BOT_FORWARD_VEL;

}

static int ACEMV_CheckLavaAndSky(edict_t *self)
{
	vec3_t dir, forward, right, offset, start,wall, down;
	trace_t trace; // for eyesight
	vec3_t minx = { 0, 0, 0 };
	vec3_t maxx = { 0, 0, 0 };
	vec3_t minx2 = { -12, -12, 0 }; //hypo 12, player not allways looking straight+steps
	vec3_t maxx2 = { 12, 12, 48 };

	//make sure we are not jumping. or allready falling :)
	if (!self->groundentity && self->velocity[2]< -120 )
		return false; //return didnot resolve

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles, dir);
	dir[2] = 0.0f;
	AngleVectors(dir, forward, right, NULL);
	VectorSet(offset, 24, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, wall);


	trace = gi.trace(self->s.origin, minx2, maxx2, wall, self, MASK_BOT_SOLID_FENCE);
	if (trace.fraction != 1)
		return false; // must b hitting wall, return didnot resolve


	VectorSet(offset, 58, 0, 0); // focalpoint 
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(start, down);
	down[2] -= CHECKSKYDOWNDIST;
								
	trace = gi.trace(start, minx, maxx, down, self, MASK_BOT_DROP_SKY); //hypov8 todo water? & test alpha
	if (trace.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}
	else if ((trace.surface->flags & SURF_SKY) && !trace.startsolid)
	{
		if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			self->s.angles[YAW] += 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			self->s.angles[YAW] -= 90;
			return true; //return resolved move
		}
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK)){ //add hypo fail if all sky
			self->s.angles[YAW] -= 180;
			return true; //return resolved move
		}
		return 2; //cant move
	}

	return false; //cant move
}


//hypov8 random player taunts
void ACEMV_BotTaunt(edict_t *self, edict_t *enemy)
{
	int randomTaunt;

	if (self->gender == GENDER_MALE)
	{
		randomTaunt = rand() % 9;
		switch (randomTaunt)
		{
		case 0:		Voice_Random(self, enemy, player_profanity_level2, NUM_PLAYER_PROFANITY_LEVEL2);break;
		case 1:		Voice_Random(self, enemy, kingpin_random, NUM_KINGPIN_RANDOM);					break;
		case 2:		Voice_Random(self, enemy, leroy_random, NUM_LEROY_RANDOM);						break;
		case 3:		Voice_Random(self, enemy, mj_random, NUM_MJ_RANDOM);							break;
		case 4:		Voice_Random(self, enemy, momo_random, NUM_MOMO_RANDOM);						break;
		case 5:		Voice_Random(self, enemy, lamont_random, NUM_LAMONT_RANDOM);					break;
		case 6:		Voice_Random(self, enemy, jesus_random, NUM_JESUS_RANDOM);						break;
		case 7:		Voice_Random(self, enemy, tyrone_random, NUM_TYRONE_RANDOM);					break;
		case 8:		Voice_Random(self, enemy, willy_random, NUM_WILLY_RANDOM);						break;
		case 9:		Voice_Random(self, enemy, moker_random, NUM_MOKER_RANDOM);						break;
		case 10:	
		default:	Voice_Random(self, enemy, heilman_random, NUM_HEILMAN_RANDOM);					break;
			//Voice_Random(self, enemy, player_profanity_level2, NUM_PLAYER_PROFANITY_LEVEL2);
		}
	}
	else if (self->gender == GENDER_FEMALE)
	{
		randomTaunt = rand() % 5;
		switch (randomTaunt)
		{
		case 0: 	Voice_Random(self, enemy, f_profanity_level2, F_NUM_PROFANITY_LEVEL2);	break;
		case 1:		Voice_Random(self, enemy, bambi_random, F_NUM_BAMBI_RANDOM);			break;
		case 2:		Voice_Random(self, enemy, yolanda_random, F_NUM_YOLANDA_RANDOM);		break;
		case 3:		Voice_Random(self, enemy, mona_random, F_NUM_MONA_RANDOM);				break;
		case 4:		Voice_Random(self, enemy, lola_random, F_NUM_LOLA_RANDOM);				break;
		case 5:		Voice_Random(self, enemy, blunt_random, F_NUM_BLUNT_RANDOM);			break;
		case 6:	
		default:	Voice_Random(self, enemy, beth_random, F_NUM_BETH_RANDOM);				break;
			//Voice_Random(self, enemy, f_profanity_level2, F_NUM_PROFANITY_LEVEL2);
		}
	}
}



///////////////////////////////////////////////////////////////////////
// Attack movement routine
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
///////////////////////////////////////////////////////////////////////
void ACEMV_Attack (edict_t *self, usercmd_t *ucmd)
{
	float c;
	vec3_t  target, angles;
	qboolean moveResolved = false;
	//qboolean boozooka, grenad;

	vec3_t player_origin;

	float range, rand_xy, skillMove_xy;
	vec3_t v;

	qboolean boozooka = false;
	qboolean grenad = false;

	qboolean strafe = false;
	qboolean strafeDir;
	static const int frames = 5;


	//hypo make player strafe in 1 dir longer
	if (self->acebot.last_strafeTime >= level.framenum)
	{
		strafe = true;
		if (self->acebot.last_strafeDir == MOVE_LEFT)
			strafeDir = MOVE_LEFT;
		else if(self->acebot.last_strafeDir == MOVE_RIGHT)
			strafeDir = MOVE_RIGHT;
	}

	// Randomly choose a movement direction
	c = random();

#if 1 //ndef HYPODEBUG

	//dont move on jump pads
	if (!self->acebot.isMovingUpPushed)
	{
		//Com_Printf("strafeDir=%i, strafe=%i, rand=%d\n", self->acebot.last_strafeDir, strafe, c);

		if (((c < 0.500f && !strafe)|| (strafe && strafeDir == MOVE_LEFT)) 
			&& ACEMV_CanMove(self, MOVE_LEFT) && ACEMV_CanMove_Simple(self, MOVE_LEFT)){
			ucmd->sidemove -= BOT_SIDE_VEL;
			moveResolved = true;
			if (!strafe){
				self->acebot.last_strafeTime = level.framenum + frames;
				self->acebot.last_strafeDir = MOVE_LEFT;}
		}
		else if (((c >= 0.500f && !strafe) || (strafe && strafeDir == MOVE_RIGHT))
			&& ACEMV_CanMove(self, MOVE_RIGHT) && ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
			ucmd->sidemove += BOT_SIDE_VEL;
			moveResolved = true;
			if (!strafe){
				self->acebot.last_strafeTime = level.framenum + frames;
				self->acebot.last_strafeDir = MOVE_RIGHT;}
		}

		if (c < 0.3 && ACEMV_CanMove(self, MOVE_FORWARD) && ACEMV_CanMove_Simple(self, MOVE_FORWARD)){
			ucmd->forwardmove += BOT_FORWARD_VEL;
			moveResolved = true;
		}
		else if (c > 0.7 && ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK)){ //was forward??
			ucmd->forwardmove -= BOT_FORWARD_VEL;
			moveResolved = true;
		}
	}

	//hyopv8 stop bots momentum running off edges, with no move random resolves
	if (!moveResolved && !self->acebot.isMovingUpPushed && self->groundentity)
		if (c < 0.500f)
		{
			if (ACEMV_CanMove_Simple(self, MOVE_LEFT)){
				ucmd->sidemove -= BOT_SIDE_VEL;
				moveResolved = true;}
			else if (ACEMV_CanMove_Simple(self, MOVE_RIGHT)){
				ucmd->sidemove += BOT_SIDE_VEL;
				moveResolved = true;}
		}
		else
		{
			if (ACEMV_CanMove_Simple(self, MOVE_RIGHT))	{
				ucmd->sidemove += BOT_SIDE_VEL;
				moveResolved = true;}
			else if (ACEMV_CanMove_Simple(self, MOVE_LEFT))	{
				ucmd->sidemove -= BOT_SIDE_VEL;
				moveResolved = true;}
		}

	if (!moveResolved)
	{
		if (ACEMV_CanMove_Simple(self, MOVE_FORWARD))
			ucmd->forwardmove += BOT_FORWARD_VEL;
		else if (ACEMV_CanMove_Simple(self, MOVE_BACK))
			ucmd->forwardmove -= BOT_FORWARD_VEL;
	}
#endif


	//hypov8 taunt
	if (self->acebot.tauntTime < level.framenum)
	{
		ACEMV_BotTaunt(self, self->enemy);
		self->acebot.tauntTime = level.framenum + (c * 900);
	}
	
	// Set the attack 
	ucmd->buttons = BUTTON_ATTACK;
	
	// Location to Aim at
	VectorCopy(self->enemy->s.origin,target);

	// Get distance.
	VectorSubtract( target, self->s.origin,v);
	range = VectorLength(v);

//ATTACK RANDOM
#if 1 //ndef HYPODEBUG //hypo disable skill for testing
	rand_xy = (random() - 0.5);
	skillMove_xy = (4 * 60) - (sv_botskill->value * 60);

	if (self->onfiretime > 0)
		skillMove_xy += 40;

	if (self->enemy->acebot.hunted)
		skillMove_xy = 1;

	//modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
	target[0] += rand_xy * skillMove_xy;
	target[1] += rand_xy * skillMove_xy;
#endif
//end ATTACK RANDOM


	//hypov8 special movement for crouch enemy and using GL/RL
	if (self->client->pers.weapon)
	{
		if ((Q_stricmp(self->client->pers.weapon->classname, "weapon_bazooka") == 0))
		{
			if (!self->acebot.aimHead)
			{
				boozooka = true;
				target[2] -= 48; //hypov8 move bots aim at feet (72 kp)
			}

			if (range < 80)
			{
				if (ACEMV_CanMove(self, MOVE_BACK) && ACEMV_CanMove_Simple(self, MOVE_BACK))
					ucmd->forwardmove -= BOT_FORWARD_VEL;
				ucmd->buttons = 0;
			}

		}
		else if ((Q_stricmp(self->client->pers.weapon->classname, "weapon_grenadelauncher") == 0))
		{
			float range;
			vec3_t v;
			VectorSubtract(self->enemy->s.origin, self->s.origin, v);
			range = VectorLength(v);
			if (range >=300) 
				target[2] += 100;
			grenad = true;
		}
		else if (Q_stricmp(self->client->pers.weapon->classname, "weapon_crowbar") == 0
			||Q_stricmp(self->client->pers.weapon->classname, "weapon_blackjack") == 0)
		{
			vec3_t forward2, right2;
			vec3_t offset2, start2, end2;
			vec3_t angles2;
			trace_t tr2;
			//float jumpUp;

			//if (range < 64)
			ucmd->forwardmove = BOT_FORWARD_VEL;

			//jumpUp = self->enemy->s.origin[2] - self->s.origin[2];
			//if (jumpUp > 18 /*&& jumpUp < 60*/)
			//{
			// Now check to see if move will move us off an edgemap team_fast_cash
			VectorCopy(self->s.angles, angles2);

			// Set up the vectors
			AngleVectors(angles2, forward2, right2, NULL);

			//check ground for obsticle
			VectorSet(offset2, 0, 0, 4);
			G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
			VectorSet(offset2, 16, 0, 4);
			G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
			tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
			if (tr2.fraction != 1.0)
			{
				VectorSet(offset2, 0, 0, 60);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
				VectorSet(offset2, 16, 0, 60);
				G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
				tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
				if (tr2.fraction == 1.0 /*&& tr.fraction != 1 || tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)*/)
				{
					ucmd->upmove = BOT_JUMP_VEL; //hypo was 400
				}
				else
				{
					VectorSet(offset2, 0, 0, 36);
					G_ProjectSource(self->s.origin, offset2, forward2, right2, start2);
					VectorSet(offset2, 16, 0, 36);
					G_ProjectSource(self->s.origin, offset2, forward2, right2, end2);
					tr2 = gi.trace(start2, self->mins, self->maxs, end2, self, /*MASK_OPAQUE*/ MASK_BOT_SOLID_FENCE);
					if (tr2.fraction == 1.0 /*&& tr.fraction != 1 || tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)*/)
					{
						ucmd->upmove = BOT_JUMP_VEL; //hypo was 400
					}
				}
			}
		}
	}

	if ((self->enemy->client->ps.pmove.pm_flags & PMF_DUCKED) && !boozooka && !grenad)
		target[2] -= 28; //hypov8 move bots aim down todo: check fence???

VectorCopy(target, self->acebot.aimPlayerOrigin);


	// Set inital Aim direction
	VectorSubtract(target, self->s.origin, self->acebot.move_vector);
	vectoangles(self->acebot.move_vector, angles);
	VectorCopy(angles, self->s.angles);


	{
		float fwd;
		float side ;
		vec3_t dist;
		vec3_t forward3, right3;
		vec3_t offset3, end3;
		vec3_t angles3;

		if (!self->acebot.isMovingUpPushed)
		{
			fwd = (float)ucmd->forwardmove / 11;
			side = (float)ucmd->sidemove / 11; //10 fps

			VectorSet(offset3, fwd, side, 0);
			VectorCopy(self->s.angles, angles3);
			AngleVectors(angles3, forward3, right3, NULL);
			G_ProjectSource(self->s.origin, offset3, forward3, right3, end3);
		}
		else
		{
			// use momentum on jump pad
		VectorSubtract(self->s.origin, self->acebot.oldOrigin, dist);
		VectorMA(self->acebot.oldOrigin, 2.0f, dist, end3);

		}

		// Set Aim direction
		VectorSubtract(target, end3, self->acebot.move_vector);
		vectoangles(self->acebot.move_vector, angles);
		VectorCopy(angles,self->s.angles);
	}



#if 0

	// Set Aim direction
	VectorSubtract(target, self->s.origin, self->acebot.move_vector);
	vectoangles(self->acebot.move_vector, angles);
	VectorCopy(angles,self->s.angles);
#endif



	//hypov8 move player up if it has a target in water
	VectorCopy(self->s.origin, player_origin);
	player_origin[2] += 24;
	if (gi.pointcontents(player_origin) & MASK_WATER)
	{
		ucmd->upmove = BOT_JUMP_VEL;
	}
	//else
		//self->client->next_drown_time = 0; // probably shound not be messing with this, but


}
