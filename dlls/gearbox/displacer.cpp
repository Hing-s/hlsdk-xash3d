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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS(info_displacer_xen_target, CPointEntity);
LINK_ENTITY_TO_CLASS(info_displacer_earth_target, CPointEntity);

int iPortalSprite = 0;
int iRingSprite = 0;

//=========================================================
// Displacement field
//=========================================================

LINK_ENTITY_TO_CLASS(portal, CPortal);

TYPEDESCRIPTION	CPortal::m_SaveData[] =
{
	DEFINE_FIELD(CPortal, m_maxFrame, FIELD_INTEGER),
	DEFINE_ARRAY(CPortal, m_pBeam, FIELD_CLASSPTR, MAX_PORTAL_BEAMS),
};

IMPLEMENT_SAVERESTORE(CPortal, CBaseEntity);

void CPortal::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("portal");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/exit1.spr");
	pev->frame = 0;
	pev->scale = 0.75;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	// Create beams.
	CreateBeams();
}

void CPortal::Animate(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}

	// Update beams.
	UpdateBeams();
}

void CPortal::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CPortal *pSpit = GetClassPtr((CPortal *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity *1.3;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CPortal::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CPortal::Shoot2(entvars_t *pevOwner, Vector vecStart, Vector vecAngles,Vector vecVelocity)
{
	CPortal *pSpit1 = GetClassPtr((CPortal *)NULL);
	pSpit1->Spawn();

	UTIL_SetOrigin(pSpit1->pev, vecStart);
	pSpit1->pev->angles = vecAngles;

	pSpit1->pev->velocity = vecVelocity;
	pSpit1->pev->owner = ENT(pevOwner);

	pSpit1->SetThink(&CPortal::Animate);
	pSpit1->pev->nextthink = gpGlobals->time + 0.1;
}

void CPortal::SelfCreate(entvars_t *pevOwner,Vector vecStart)
{
	CPortal *pSpit2 = GetClassPtr((CPortal *)NULL);
	pSpit2->Spawn();
	pSpit2->ClearBeams();
	UTIL_SetOrigin(pSpit2->pev, vecStart);

	pSpit2->pev->owner = ENT(pevOwner);
	pSpit2->Circle();
	pSpit2->SetTouch( NULL );
	pSpit2->SetThink(&CPortal::ExplodeThink);
	pSpit2->pev->nextthink = gpGlobals->time + 0.3;
}


void CPortal::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner || (ENT(pOther->pev) == VARS(pev->owner)->owner))
		return;

	TraceResult tr;
	Vector vecSpot;
	Vector vecSrc;
	edict_t *pEnt = NULL;
	pev->enemy = pOther->edict();
	CBaseEntity *pTarget = NULL;

	if (!pOther->pev->takedamage)
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/displacer_impact.wav", 1, ATTN_NORM, 0, 100);
    
	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
	{
		// Randomize the destination in multiplayer
		
		for( int i = RANDOM_LONG( 1, 5 ); i > 0; i-- )
			pEnt = FIND_ENTITY_BY_CLASSNAME(pEnt, "info_player_deathmatch");
		if( pEnt )
			pTarget = GetClassPtr((CBaseEntity *)VARS(pEnt));
	}

	if(pTarget && pOther->IsPlayer())
   	{
		Vector tmp = pTarget->pev->origin;
		UTIL_CleanSpawnPoint( tmp, 50 );

		EMIT_SOUND( pOther->edict(), CHAN_BODY, "weapons/displacer_self.wav", 1, ATTN_NORM );

		// make origin adjustments (origin in center, not at feet)
		tmp.z -= pOther->pev->mins.z +5;
		tmp.z++;

		pOther->pev->flags &= ~FL_ONGROUND;

		UTIL_SetOrigin(pOther->pev, tmp);

		pOther->pev->angles = pTarget->pev->angles;

		pOther->pev->v_angle = pTarget->pev->angles;

		pOther->pev->fixangle = TRUE;

		pOther->pev->velocity = pOther->pev->basevelocity = g_vecZero;
	}

	pev->movetype = MOVETYPE_NONE;

/*	if (pTarget)
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD( vecSrc.x );	// X
			WRITE_COORD( vecSrc.y );	// Y
			WRITE_COORD( vecSrc.z );	// Z
			WRITE_BYTE( 16 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 96 );		// b
			WRITE_BYTE( 10 );		// time * 10
			WRITE_BYTE( 10 );		// decay * 0.1
		MESSAGE_END( );
	}*/

	Circle();

	// Clear beams.
	ClearBeams();

	SetThink(&CPortal::ExplodeThink);
	pev->nextthink = gpGlobals->time + 0.3;
}
void CPortal::Circle( void )
{
	// portal circle
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 800); // reach damage radius over .2 seconds
		WRITE_SHORT(iRingSprite);
		WRITE_BYTE(0); // startframe
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(3); // life
		WRITE_BYTE(16);  // width
		WRITE_BYTE(0);   // noise
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255); // brightness
		WRITE_BYTE(0);		// speed
	MESSAGE_END();

}

void CPortal::ExplodeThink( void )
{
	pev->effects |= EF_NODRAW;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/displacer_teleport.wav", VOL_NORM, ATTN_NORM);

	if ( pRemoveEnt )
	{
		UTIL_Remove( pRemoveEnt );
	}

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;
		pev->owner = NULL;

	UTIL_Remove( this );

	::RadiusDamage( pev->origin, pev, pevOwner, 300, 400, CLASS_NONE, DMG_ENERGYBEAM );
}

void CPortal::CreateBeams()
{
	for (int i = 0; i < MAX_PORTAL_BEAMS; i++)
	{
		m_pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr", RANDOM_LONG(2, 3) * 10);
		m_pBeam[i]->PointEntInit( pev->origin, entindex() );
		m_pBeam[i]->SetColor(96, 128, 16);
		m_pBeam[i]->SetBrightness(RANDOM_LONG(24, 25) * 10);
		m_pBeam[i]->SetNoise(80);
	}
}

void CPortal::ClearBeams()
{
	for (int i = 0; i < MAX_PORTAL_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
}

void CPortal::UpdateBeams()
{
	TraceResult tr, tr1;
	int i, j;
	float flDist = 1.0;

	Vector vecSrc, vecTarget;
	vecSrc = pev->origin;

	for (i = 0; i < MAX_PORTAL_BEAMS; i++)
	{
		for (j = 0; j < 3; ++j)
		{
			vecTarget = gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1);

			UTIL_TraceLine(vecSrc, vecSrc + (vecTarget * 512), dont_ignore_monsters, ENT(pev), &tr1);
			if (flDist > tr1.flFraction)
			{
				tr = tr1;
				flDist = tr.flFraction;
			}
		}

		// Couldn't find anything close enough
		if (flDist == 1.0)
			continue;

		// Update the beams.
		m_pBeam[i]->SetStartPos(tr.vecEndPos);
		m_pBeam[i]->SetEndEntity( entindex() );
		m_pBeam[i]->RelinkBeam();
	}
}

#endif // !defined ( CLIENT_DLL )

enum displacer_e {
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER,
};


LINK_ENTITY_TO_CLASS(weapon_displacer, CDisplacer);

//=========================================================
// Purpose:
//=========================================================
int CDisplacer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = 0;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_DISPLACER;
	p->iWeight = DISPLACER_WEIGHT;

	return 1;
}

//=========================================================
// Purpose:
//=========================================================
int CDisplacer::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
			WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "buttons/button11.wav", 1, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Spawn()
{
	Precache();
	m_iId = WEAPON_DISPLACER;
	SET_MODEL(ENT(pev), "models/w_displacer.mdl");

	m_iDefaultAmmo = EGON_DEFAULT_GIVE*2;

	m_iFireState = FIRESTATE_NONE;
	m_iFireMode = FIREMODE_NONE;

#ifndef CLIENT_DLL
	if( !g_pGameRules->IsMultiplayer() || g_pGameRules->IsCoOp() )
	{
		edict_t* pEnt = NULL;
		pEnt = FIND_ENTITY_BY_CLASSNAME(pEnt, "info_displacer_earth_target");

		if( pEnt )
		{
			m_hTargetEarth = GetClassPtr((CBaseEntity *)VARS(pEnt));
		}
		else
		{
			ALERT(
				at_console,
				"ERROR: Couldn't find entity with classname %s\n",
				"info_displacer_earth_target");
		}

		pEnt = NULL;
		pEnt = FIND_ENTITY_BY_CLASSNAME(pEnt, "info_displacer_xen_target");

		if( pEnt )
		{
			m_hTargetXen = GetClassPtr((CBaseEntity *)VARS(pEnt));
		}
		else
		{
			ALERT(
				at_console,
				"ERROR: Couldn't find entity with classname %s\n",
				"info_displacer_xen_target");
		}
	}
#endif

	FallInit();// get ready to fall down.
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Precache(void)
{
	PRECACHE_MODEL("models/v_displacer.mdl");
	PRECACHE_MODEL("models/w_displacer.mdl");
	PRECACHE_MODEL("models/p_displacer.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_impact.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_spin2.wav");
	PRECACHE_SOUND("weapons/displacer_start.wav");
	PRECACHE_SOUND("weapons/displacer_teleport.wav");
	PRECACHE_SOUND("weapons/displacer_teleport_player.wav");

	PRECACHE_SOUND("buttons/button11.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

#ifndef CLIENT_DLL
	iPortalSprite = PRECACHE_MODEL("sprites/exit1.spr");
	iRingSprite = PRECACHE_MODEL("sprites/disp_ring.spr");
	m_iBeam = PRECACHE_MODEL("sprites/plasma.spr");
#endif
	PRECACHE_MODEL("sprites/plasma.spr");
	UTIL_PrecacheOther("portal");
	
	m_usDisplacer = PRECACHE_EVENT(1, "events/displacer.sc");
}

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::Deploy()
{
	return DefaultDeploy("models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "displacer", UseDecrement());
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	ClearSpin();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(DISPLACER_HOLSTER);
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::SecondaryAttack(void)
{
	if (m_fFireOnEmpty || (!HasAmmo() || !CanFireDisplacer2()))
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iFireState = FIRESTATE_SPINUP;
	m_iFireMode = FIREMODE_BACKWARD;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::PrimaryAttack()
{
	if ( m_fFireOnEmpty || (!HasAmmo() || !CanFireDisplacer()))
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	m_iFireState = FIRESTATE_SPINUP;
	m_iFireMode = FIREMODE_FORWARD;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1f;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iFireState != FIRESTATE_NONE)
	{
		switch (m_iFireState)
		{
		case FIRESTATE_SPINUP:
		{
			// Launch spinup sequence.
			SpinUp( m_iFireMode );
		}
		break;

		case FIRESTATE_FIRE:
		{
			// Fire using the selected mode.
			Fire(m_iFireMode == FIREMODE_FORWARD ? TRUE : FALSE);
		}
		break;
		}

		// Do not play other idle sequences while performing spin sequence.
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = DISPLACER_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}
	else
	{
		iAnim = DISPLACER_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	SendWeaponAnim(iAnim, UseDecrement());
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::ClearSpin( void )
{

	switch (m_iFireMode)
	{
	case FIREMODE_FORWARD:
		STOP_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_spin.wav");
		//STOP_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_spin2.wav");
		break;
	case FIREMODE_BACKWARD:
		STOP_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_spin2.wav");
		break;
	}

	m_iFireState = FIRESTATE_NONE;
	m_iFireMode = FIREMODE_NONE;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::SpinUp(int iFireMode)
{
	CreateEffects();
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usDisplacer,
		0.0,
		(float *)&g_vecZero,
		(float *)&g_vecZero,
		0.0,
		0.0,
		DISPLACER_SPINUP,
		iFireMode,
		0,
		0);

	m_iFireState = FIRESTATE_FIRE;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
}
//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Fire(BOOL fIsPrimary)
{
	if (fIsPrimary)
	{
		// Use the firemode 1, which launches a portal forward.
	
		Displace(  );
		UseAmmo(EGON_DEFAULT_GIVE);
	}
	else
	{
		Teleport(  );
	}

	ClearSpin();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.7f;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.7f;
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;

	// Decrement weapon ammunition.
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Displace( void )
{
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usDisplacer,
		0.0,
		(float *)&g_vecZero,
		(float *)&g_vecZero,
		0.0,
		0.0,
		DISPLACER_FIRE,
		FIREMODE_FORWARD,
		0,//&&55&
		0);

#ifndef CLIENT_DLL
	Vector vecSrc;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc = vecSrc + gpGlobals->v_forward	* 22;
	vecSrc = vecSrc + gpGlobals->v_right	* 8;
	vecSrc = vecSrc + gpGlobals->v_up		* -12;

	CPortal::Shoot(m_pPlayer->pev, vecSrc, gpGlobals->v_forward * 500);
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Teleport( void )
{
	ASSERT(m_hTargetEarth != NULL && m_hTargetXen);
#ifndef CLIENT_DLL
	edict_t *pEnt = NULL;
	CBaseEntity *pTarget = NULL;

	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
	{
		// Randomize the destination in multiplayer
		for( int i = RANDOM_LONG( 1, 5 ); i > 0; i-- )
			pEnt = FIND_ENTITY_BY_CLASSNAME(pEnt, "info_player_deathmatch");
		if( pEnt )
			pTarget = GetClassPtr((CBaseEntity *)VARS(pEnt));
	}
	else
		pTarget = (!m_pPlayer->m_fInXen) ? m_hTargetXen : m_hTargetEarth;
		Vector tmp = pTarget->pev->origin;

	if(pTarget && (tmp != Vector(0,0,0)))
	{	
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;

		UseAmmo(EGON_DEFAULT_GIVE*3);

		UTIL_CleanSpawnPoint( tmp, 50 );

		EMIT_SOUND( edict(), CHAN_BODY, "weapons/displacer_self.wav", 1, ATTN_NORM );
	 	CPortal::SelfCreate(m_pPlayer->pev, m_pPlayer->pev->origin);
		SendWeaponAnim( DISPLACER_IDLE1 );

		// make origin adjustments (origin in center, not at feet)
		tmp.z -= m_pPlayer->pev->mins.z +5;
		tmp.z++;


		m_pPlayer->pev->flags &= ~FL_ONGROUND;

		UTIL_SetOrigin(m_pPlayer->pev, tmp);

		m_pPlayer->pev->angles = pTarget->pev->angles;

		m_pPlayer->pev->v_angle = pTarget->pev->angles;

		m_pPlayer->pev->fixangle = TRUE;
		m_pPlayer->pev->velocity = m_pPlayer->pev->basevelocity = g_vecZero;

		m_pPlayer->m_fInXen = !m_pPlayer->m_fInXen;
		if( !g_pGameRules->IsMultiplayer())
		{
			if (m_pPlayer->m_fInXen)
				m_pPlayer->pev->gravity = 0.5;
			else
				m_pPlayer->pev->gravity = 1.0;
		}
	}
	else
	{
		EMIT_SOUND( edict(), CHAN_BODY, "buttons/button10.wav", 1, ATTN_NORM );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 3;
	}
#endif
		int flags;
#if defined( CLIENT_WEAPONS )
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		// Used to play teleport sound.
		PLAYBACK_EVENT_FULL(
			flags,
			m_pPlayer->edict(),
			m_usDisplacer,
			0.0,
			(float *)&g_vecZero,
			(float *)&g_vecZero,
			0.0,
			0.0,
			0.0,
			FIREMODE_BACKWARD,
			0,
			0);
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::CreateEffects(void)
{
	m_pBeam = CBeam::BeamCreate("sprites/plasma.spr", 60);
	//m_pBeam->PointEntInit(vecEndPos, m_pPlayer->entindex());
	m_pBeam->SetStartAttachment(2);
	m_pBeam->SetEndAttachment(3);
	m_pBeam->SetColor(96, 128, 16);
	m_pBeam->SetBrightness(64);
	m_pBeam->pev->framerate = 20;
}


//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::HasAmmo(void)
{
	if (m_pPlayer->ammo_uranium <= 0)
		return FALSE;

	return TRUE;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::UseAmmo(int count)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count)
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
	else
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::CanFireDisplacer() const
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= EGON_DEFAULT_GIVE;
}

BOOL CDisplacer::CanFireDisplacer2() const
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= EGON_DEFAULT_GIVE*3;
}
