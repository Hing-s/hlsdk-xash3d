/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== gearbox_triggers.cpp ========================================================

spawn and use functions for editor-placed triggers

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"
#include "gamerules.h"
#include "triggers.h"
#include "weapons.h"
#include "sporegrenade.h"
//=========================================================
// CTriggerXenReturn
//=========================================================

class CTriggerXenReturn : public CTriggerTeleport
{
public:
	void Spawn(void);
	void EXPORT TeleportTouch(CBaseEntity *pOther);
};


LINK_ENTITY_TO_CLASS(trigger_xen_return, CTriggerXenReturn);


void CTriggerXenReturn::Spawn(void)
{
	CTriggerTeleport::Spawn();

	SetTouch(&CTriggerXenReturn::TeleportTouch);
}

void CTriggerXenReturn::TeleportTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	// Only teleport monsters or clients
	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (!(pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS))
	{// no monsters allowed!
		if (FBitSet(pevToucher->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS))
	{// no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_CLASSNAME(pentTarget, "info_displacer_earth_target");
	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin(pevToucher, tmp);

	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;

	if (pOther->IsPlayer())
	{
		// Ensure the current player is marked as being
		// on earth.
		((CBasePlayer*)pOther)->m_fInXen = FALSE;

		// Reset gravity to default.
		pOther->pev->gravity = 1.0f;
	}

	// Play teleport sound.
	EMIT_SOUND(ENT(pOther->pev), CHAN_STATIC, "debris/beamstart7.wav", 1, ATTN_NORM );
}

//=========================================================
// CTriggerGenewormHit
//=========================================================

class CTriggerGenewormHit : public CTriggerMultiple
{
public:
};

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, CTriggerMultiple);

//=========================================================
// Trigger to disable a player
//=========================================================

class CPlayerFreeze : public CBaseDelay
{
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pPlayer;
	pPlayer = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex( 1 ));

	if (pPlayer && pPlayer->pev->flags & FL_CLIENT)
	{
		if (pPlayer->pev->flags & FL_FROZEN)
		{
			// unfreeze him
			((CBasePlayer *)pPlayer)->EnableControl(TRUE);
		}
		else
		{
			// freeze him
			((CBasePlayer *)pPlayer)->EnableControl(FALSE);
		}
	}
}

LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CPlayerFreeze );

class CBlowerCannon : public CBaseEntity
{
public:
	void Spawn( void );
	//void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void Fire( void );

	virtual Vector UpdateTargetPosition(CBaseEntity *pTarget)
	{
		return pTarget->BodyTarget(pev->origin);
	}

	void EXPORT Touch( CBaseEntity *pOther );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	int WeaponType;
	float Delay;
	int FireType;
	int test;
	Vector m_sightOrigin;

};
LINK_ENTITY_TO_CLASS(env_blowercannon, CBlowerCannon);

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD(CBlowerCannon, FireType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, test, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, WeaponType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, Delay, FIELD_FLOAT),
	DEFINE_FIELD(CBlowerCannon, m_sightOrigin, FIELD_VECTOR),
};
IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseEntity )


void CBlowerCannon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "firetype"))
	{
		FireType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		Delay = (float)atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaptype"))
	{
		WeaponType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CBlowerCannon::Spawn(void)
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING("env_blowercannon");
	pev->solid = SOLID_TRIGGER;
	test = 1;
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));

	if( FireType == 1)
		SetUse( &CBlowerCannon::ToggleUse );
	else if( FireType == 2)
		SetTouch( &CBlowerCannon::Touch );
}

void CBlowerCannon::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//ALERT(at_console, "In USE!\n");
	SetThink( &CBlowerCannon::Fire );
	pev->nextthink = gpGlobals->time + Delay;
}

void CBlowerCannon::Touch( CBaseEntity *pOther)
{
	//ALERT(at_console, "TOUCH!\n");
	SetThink( &CBlowerCannon::Fire );
	pev->nextthink = gpGlobals->time + Delay;
}

void CBlowerCannon::Fire( void )
{
	//ALERT(at_console, "thinking!\n");

	edict_t *pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING( pev->target ) );

	CBaseEntity *pInstance = CBaseEntity::Instance(pTarget);


	if( pInstance && pInstance->IsAlive() )
	{
		m_sightOrigin = UpdateTargetPosition( pInstance );

		Vector direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		Vector angles = UTIL_VecToAngles( direction );
		UTIL_MakeVectors( angles );

		if( WeaponType == 1 )//spore rocket
		{
			CSpore *pSpore = CSpore::CreateSporeRocket( pev->origin, angles, this );
			pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1500;
		}
		else if( WeaponType == 2 )//spore grenade
		{
			CSpore *pSpore = CSpore::CreateSporeGrenade( pev->origin, angles, this );
			pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1000;
		}
		else if( WeaponType == 3 )//shock beam
		{
			CBaseEntity *pShock = CBaseEntity::Create( "shock_beam", pev->origin, angles, this->edict() );
			pShock->pev->velocity = gpGlobals->v_forward * 2000;
		}
		else if( WeaponType == 4 )//displacer ball
		{
			CPortal::Shoot(this->pev, pev->origin, gpGlobals->v_forward * 600, angles);
		}
	}
	if( test )
		pev->nextthink = gpGlobals->time + 3;
	else
		pev->nextthink = gpGlobals->time + Delay;
	test = 0;
}