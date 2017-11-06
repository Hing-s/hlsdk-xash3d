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
	void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void EXPORT BlowerCannonThink( void );
	void EXPORT BlowerCannonStart( void );
	void EXPORT BlowerCannonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	static TYPEDESCRIPTION m_SaveData[];

	int m_iWeapType;
	float m_flDelay;
	int m_iFireType;
	int m_iZOffSet;
};

LINK_ENTITY_TO_CLASS(env_blowercannon, CBlowerCannon);

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD(CBlowerCannon, m_iFireType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iWeapType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iZOffSet, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_flDelay, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseEntity )


void CBlowerCannon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "firetype"))
	{
		m_iFireType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = (float)atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaptype"))
	{
		m_iWeapType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "zoffset"))
	{
		m_iZOffSet = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CBlowerCannon::Spawn(void)
{
	Precache();
	UTIL_SetSize( pev, Vector(-16, -16, -16), Vector( 16, 16, 16 ) );
	pev->solid = SOLID_TRIGGER;
	SetUse( &CBlowerCannon::BlowerCannonUse );
}

void CBlowerCannon::BlowerCannonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CBlowerCannon::BlowerCannonStart );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBlowerCannon::Precache( void )
{
	UTIL_PrecacheOther( "shock_beam" );
	UTIL_PrecacheOther( "portal" );
	UTIL_PrecacheOther( "spore" );
}

void CBlowerCannon::BlowerCannonThink( void )
{
	CBaseEntity *pTarget = GetNextTarget();
	Vector angles, direction;

	if( pTarget && pTarget->IsAlive() )
	{
		direction = pTarget->pev->origin - pev->origin;
		direction.z = m_iZOffSet + pTarget->pev->origin.z - pev->origin.z;

		Vector angles = UTIL_VecToAngles( direction );
		UTIL_MakeVectors( angles );

		if( m_iWeapType == 1 )//spore rocket
		{
			CSpore *pSpore = CSpore::CreateSporeRocket( pev->origin, angles, this );
			pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1500;
		}
		else if( m_iWeapType == 2 )//spore grenade
		{
			CSpore *pSpore = CSpore::CreateSporeGrenade( pev->origin, angles, this, FALSE );
			pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 700;
		}
		else if( m_iWeapType == 3 )//shock beam
		{
			CBaseEntity *pShock = CBaseEntity::Create( "shock_beam", pev->origin, angles, this->edict() );
			pShock->pev->velocity = gpGlobals->v_forward * 2000;
		}
		else if( m_iWeapType == 4 )//displacer ball
		{
			CPortal::Shoot(pev, pev->origin, gpGlobals->v_forward * 600);
		}
	}
	if( m_iFireType == 2 )
		SetThink( NULL );

	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CBlowerCannon::BlowerCannonStart( void )
{
	SetThink(&CBlowerCannon::BlowerCannonThink);
	pev->nextthink = gpGlobals->time + m_flDelay;
}