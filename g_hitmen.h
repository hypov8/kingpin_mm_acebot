
////////////////////////////////////////////////////////////////////////////////
//
//	g_Hitmen.h
//
//	These are the routines which are called from outside the Hitmen code
//
//

extern void hm_CheckWeaponTimer( void );
extern void hm_Initialise( void );
extern void Hm_Setcurrentweapon(gclient_t* cl, qboolean bResetAmmo);
extern void Hm_Set_Timers(gclient_t* client);
extern void Hm_Check_Timers(edict_t* ent);
extern void Hm_DisplayMOTD(edict_t *ent);
extern void hm_KilledCheckHealthIncrease(edict_t* attacker);

