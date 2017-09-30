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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "customentity.h"
#include "grapple_tonguetip.h"
#include <string.h>

#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS(grapple_tonguetip, CGrappleTonguetip);

TYPEDESCRIPTION	CGrappleTonguetip::m_SaveData[] =
{
	DEFINE_FIELD(CGrappleTonguetip, m_pMyGrappler, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CGrappleTonguetip, CBaseEntity);

//=========================================================
// Purpose: Spawn
//=========================================================
void CGrappleTonguetip::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING("grapple_tonguetip");
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	pev->gravity = 0.01;

	SET_MODEL(ENT(pev), "models/v_bgrap_tonguetip.mdl");

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CGrappleTonguetip::TipTouch);
}


//=========================================================
// Purpose: CreateTip
//=========================================================
CGrappleTonguetip* CGrappleTonguetip::CreateTip(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrappleTonguetip* pTonguetip = GetClassPtr((CGrappleTonguetip *)NULL);
	pTonguetip->Spawn();

	UTIL_SetOrigin(pTonguetip->pev, vecStart);
	pTonguetip->pev->velocity = vecVelocity;
	pTonguetip->pev->owner = ENT(pevOwner);
	pTonguetip->m_pMyGrappler = GetClassPtr((CGrapple*)pevOwner);
	pTonguetip->m_pMyGrappler->checkTarg = 0;

	return pTonguetip;
}

//=========================================================
// Purpose: HitThink
//=========================================================
void CGrappleTonguetip::HitThink(void)
{
	//ALERT(at_console, "HitT.hink\n");
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Purpose: TipTouch
//=========================================================
void CGrappleTonguetip::TipTouch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner || (ENT(pOther->pev) == VARS(pev->owner)->owner) || (pOther == m_pMyGrappler ))
		return;
//	CBaseEntity *pTouchedEnt = CBaseEntity::Instance( pOther );

	TraceResult tr;
	float rgfl1[3];
	float rgfl2[3];
	const char *pTextureName;
	Vector vecSrc = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity * 10;

	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	vecSrc.CopyToArray(rgfl1);
	vecEnd.CopyToArray(rgfl2);

	if (pEntity)
		pTextureName = TRACE_TEXTURE(ENT(pEntity->pev), rgfl1, rgfl2);
	else
		pTextureName = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);


	int content = UTIL_PointContents(tr.vecEndPos);
	int hitFlags = pOther->pev->flags;
	m_pMyGrappler->m_iHitFlags	= hitFlags;
	//float m_IsPull;
	if(FClassnameIs( pOther->pev, "monster_barnacle"))
		m_pMyGrappler->checkTarg = 1;
	else if((hitFlags & (FL_CLIENT | FL_MONSTER)) || (FClassnameIs( pOther->pev, "ammo_spore")))
	{
		// Set player attached flag.
		if (pOther->IsPlayer())
			((CBasePlayer*)pOther)->m_afPhysicsFlags |= PFLAG_ATTACHED;

		pev->movetype = MOVETYPE_FOLLOW;
		pev->aiment = ENT(pOther->pev);
		pev->velocity = pev->velocity.Normalize();

		m_pMyGrappler->OnTongueTipHitEntity(pOther);
		m_pMyGrappler->m_fTipHit	= TRUE;
		m_pMyGrappler->checkTarg = 0;
	/*	UTIL_SetOrigin( pev, pOther->Center());
		pev->velocity = pOther->velocity;
		SetThink(&CGrappleTonguetip::HitThink);
		pev->nextthink = gpGlobals->time;*/
	}
	else if( pTextureName && m_pMyGrappler && (!strcmp (pTextureName, "xeno_grapple" )))
	{
		pev->velocity = Vector(0, 0, 0);
		pev->movetype = MOVETYPE_NONE;
		pev->gravity = 0.0f;
		m_pMyGrappler->m_fTipHit	= TRUE;
		hitFlags = 0;
		m_pMyGrappler->OnTongueTipHitSurface(tr.vecEndPos);
		m_pMyGrappler->checkTarg = 0;
	}
	else
	{
		m_pMyGrappler->m_fTipHit	= FALSE;
		hitFlags = 0;
		m_pMyGrappler->checkTarg = 1;
	}

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/bgrapple_impact.wav", 1, ATTN_NORM, 0, 100);

	SetTouch( NULL );
}

void CGrappleTonguetip::PreRemoval(void)
{
	if (pev->aiment != NULL)
	{
		CBaseEntity* pEnt = GetClassPtr((CBaseEntity*)VARS(pev->aiment));
		if (pEnt && pEnt->IsPlayer())
		{
			// Remove attached flag of the target entity.
			((CBasePlayer*)pEnt)->m_afPhysicsFlags &= ~PFLAG_ATTACHED;
		}
	}
CBaseEntity::PreRemoval();
}
#endif

LINK_ENTITY_TO_CLASS(weapon_grapple, CGrapple);

enum bgrap_e {
	BGRAP_BREATHE = 0,
	BGRAP_LONGIDLE,
	BGRAP_SHORTIDLE,
	BGRAP_COUGH,
	BGRAP_DOWN,
	BGRAP_UP,
	BGRAP_FIRE,
	BGRAP_FIREWAITING,
	BGRAP_FIREREACHED,
	BGRAP_FIRETRAVEL,
	BGRAP_FIRERELEASE,
};

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Spawn()
{

	SetThink( NULL );
	Precache();
	m_iId = WEAPON_GRAPPLE;
	SET_MODEL(ENT(pev), "models/w_bgrap.mdl");
	m_iClip = -1;

	m_pBeam			= NULL;
	m_pTongueTip	= NULL;
	m_fTipHit		= FALSE;
	m_fPlayPullSound = FALSE;

	m_iHitFlags		= 0;
	m_iFirestate	= FIRESTATE_NONE;

	m_flNextPullSoundTime = 0.0f;

	FallInit();// get ready to fall down.
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Precache(void)
{
	PRECACHE_MODEL("models/v_bgrap.mdl");
	PRECACHE_MODEL("models/v_bgrap_tonguetip.mdl");

	PRECACHE_MODEL("models/w_bgrap.mdl");
	PRECACHE_MODEL("models/p_bgrap.mdl");

	PRECACHE_SOUND("weapons/alienweap_draw.wav");

	PRECACHE_SOUND("weapons/bgrapple_cough.wav");
	PRECACHE_SOUND("weapons/bgrapple_fire.wav");
	PRECACHE_SOUND("weapons/bgrapple_impact.wav");
	PRECACHE_SOUND("weapons/bgrapple_pull.wav");
	PRECACHE_SOUND("weapons/bgrapple_release.wav");
	PRECACHE_SOUND("weapons/bgrapple_wait.wav");

	PRECACHE_MODEL("sprites/tongue.spr");
}

//=========================================================
// Purpose:
//=========================================================
int CGrapple::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_GRAPPLE;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_GRAPPLE;
	p->iWeight = GRAPPLE_WEIGHT;
	return 1;
}

//=========================================================
// Purpose:
//=========================================================
BOOL CGrapple::Deploy()
{
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.9f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9f;
	timer = gpGlobals->time;
	deadflag = 0;
	return DefaultDeploy("models/v_bgrap.mdl", "models/p_bgrap.mdl", BGRAP_UP, "bgrap");
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Holster(int skiplocal /* = 0 */)
{
	FireRelease();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(BGRAP_DOWN);
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::PrimaryAttack(void)
{
	if (m_iFirestate != FIRESTATE_NONE)
		return;

	m_iFirestate = FIRESTATE_FIRE;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

//=========================================================
// Purpose:	``
//=========================================================
void CGrapple::ItemPostFrame(void)
{
	if (!(m_pPlayer->pev->button & IN_ATTACK))
	{
		m_flLastFireTime = 0.0f;
	}


	if (m_iFirestate != FIRESTATE_NONE)
	{
		if (m_fTipHit)
		{
			Pull();
		}

		// Check if fire is still eligible.
		CheckFireEligibility();
		CheckTargets();
		// Check if the current tip is attached to a monster or a wall.
		// If the tongue tip move type is MOVETYPE_FOLLOW, then it
		// implies that we are targetting a monster, so it will kill
		// the monster when close enough.
		CheckTargetProximity();

		// Update the tip velocity and position.
		UpdateTongueTip();

		// Update the tongue beam.
		UpdateBeam();

		// Update the pull sound.
		UpdatePullSound();
	}

	if ((m_pPlayer->pev->button & IN_ATTACK) && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{
		if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]))
		{
			m_fFireOnEmpty = TRUE;
		}

#ifndef CLIENT_DLL
		m_pPlayer->TabulateAmmo();
#endif
		PrimaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK;
	}
	else if (!(m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)))
	{
		if (m_iFirestate != FIRESTATE_NONE)
		{
			FireRelease();
		}

#ifndef CLIENT_DLL
		// no fire buttons down

		m_fFireOnEmpty = FALSE;

		if (!IsUseable() && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
		{
			// weapon isn't useable, switch.
			if (!(iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && g_pGameRules->GetNextBestWeapon(m_pPlayer, this))
			{
				m_flNextPrimaryAttack = (UseDecrement() ? 0.0 : gpGlobals->time) + 0.3;
				return;
			}
		}
#endif

		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::WeaponIdle(void)
{
	if(deadflag == 1)
	{
		FireRelease();
		deadflag = 0;
		return;
	}

	ResetEmptySound();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iFirestate != FIRESTATE_NONE)
	{
		switch (m_iFirestate)
		{
		case FIRESTATE_FIRE:
			Fire();
			break;

		case FIRESTATE_FIRE2:
			Fire2();
			break;

		case FIRESTATE_WAIT:
			FireWait();
			break;

		case FIRESTATE_REACH:
			FireReach();
			break;

		case FIRESTATE_TRAVEL:
			FireTravel();
			break;

		case FIRESTATE_RELEASE:
			FireRelease();
			break;

		default:
			break;
		}
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.8f)
	{
		iAnim = BGRAP_BREATHE;
		m_flTimeWeaponIdle = 2.6f;
	}
	else if (flRand <= 0.7f)
	{
		iAnim = BGRAP_LONGIDLE;
		m_flTimeWeaponIdle = 10.0f;
	}
	else if (flRand <= 0.95)
	{
		iAnim = BGRAP_SHORTIDLE;
		m_flTimeWeaponIdle = 1.3f;
	}
	else
	{
		iAnim = BGRAP_COUGH;
		m_flTimeWeaponIdle = 4.6f;

		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_cough.wav", 1, ATTN_NORM);
	}

	SendWeaponAnim(iAnim, UseDecrement());
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Fire(void)
{
	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_fire.wav", 1, ATTN_NORM);

	SendWeaponAnim(BGRAP_FIRE, UseDecrement());
	m_iFirestate = FIRESTATE_FIRE2;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(15, 20);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.45f;
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Fire2(void)
{
	// Create the tongue tip.
	CreateTongueTip();

	// Start the pull sound.
	StartPullSound();

	m_iFirestate = FIRESTATE_WAIT;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(15, 20);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::FireWait(void)
{
	SendWeaponAnim(BGRAP_FIREWAITING, UseDecrement());

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(15, 20);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::FireReach(void)
{
	SendWeaponAnim(BGRAP_FIREREACHED, UseDecrement());
	m_iFirestate = FIRESTATE_TRAVEL;

	// Start pulling the owner toward tongue tip.
	StartPull();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(15, 20);
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.7;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::FireTravel(void)
{
	SendWeaponAnim(BGRAP_FIRETRAVEL, UseDecrement());

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + RANDOM_FLOAT(15, 20);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6f;
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::FireRelease(void)
{
	// Stop pull sound.
	// STOP_SOUND(ENT(pev), CHAN_VOICE, "weapons/bgrapple_pull.wav");

	// Play release sound.
	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_release.wav", 1, ATTN_NORM);

	SendWeaponAnim(BGRAP_FIRERELEASE, UseDecrement());
	m_iFirestate = FIRESTATE_NONE;
	// Stop the pulling.
	StopPull();

	// Reset pull sound.
	ResetPullSound();

	// Destroy the tongue tip.
	DestroyTongueTip();

	m_fTipHit = FALSE;
	m_iHitFlags = 0;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::OnTongueTipHitSurface(const Vector& vecTarget)
{
#ifndef CLIENT_DLL
	//ALERT(at_console, "Hit surface at: (%f,%f,%f).\n", vecTarget.x, vecTarget.y, vecTarget.z);
#endif
	FireReach();
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::OnTongueTipHitEntity(CBaseEntity* pEntity)
{
#ifndef CLIENT_DLL
	 //ALERT(at_console, "Hit entity with classname %s.\n", (char*)STRING(pEntity->pev->classname));
#endif
	FireReach();
}
//=========================================================
// Purpose:
//=========================================================
void CGrapple::StartPull(void)
{
#ifndef CLIENT_DLL
	Vector vecOrigin = m_pPlayer->pev->origin;

	vecOrigin.z++;
	UTIL_SetOrigin(m_pPlayer->pev, vecOrigin);

	m_pPlayer->pev->movetype = MOVETYPE_FLY;
	m_pPlayer->pev->gravity = 0.0f;

	m_pPlayer->pev->flags &= ~FL_ONGROUND;

	m_pPlayer->m_afPhysicsFlags |= PFLAG_LATCHING;
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::StopPull(void)
{
#ifndef CLIENT_DLL
	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->gravity = 1.0f;

	m_pPlayer->m_afPhysicsFlags &= ~PFLAG_LATCHING;
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::Pull( void )
{
#ifndef CLIENT_DLL
	Vector vecOwnerPos, vecTipPos, vecDirToTip;
	vecOwnerPos = m_pPlayer->pev->origin;
	vecTipPos = m_pTongueTip->Center();
	vecDirToTip = (vecTipPos - vecOwnerPos).Normalize();

	m_pPlayer->pev->velocity = vecDirToTip * 550;

	m_pPlayer->m_afPhysicsFlags |= PFLAG_LATCHING;
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::CreateTongueTip( void )
{
#ifndef CLIENT_DLL
	Vector vecSrc, vecAiming, vecVel;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc = vecSrc + gpGlobals->v_forward * 8;
	vecSrc = vecSrc + gpGlobals->v_right * 3;
	vecSrc = vecSrc + gpGlobals->v_up * -3;

	vecVel = gpGlobals->v_forward * 1800;

	//GetAttachment(0, vecSrc, vecAiming);

	m_pTongueTip = CGrappleTonguetip::CreateTip(pev, vecSrc, vecVel);
	CreateBeam(m_pTongueTip);
#endif

}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::DestroyTongueTip(void)
{
#ifndef CLIENT_DLL
	DestroyBeam();
	checkTarg = 0;
	if (m_pTongueTip)
	{
		UTIL_Remove(m_pTongueTip);
		m_pTongueTip = NULL;
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::UpdateTongueTip(void)
{
#ifndef CLIENT_DLL
	if (m_pTongueTip)
	{
		Vector tipVel = m_pTongueTip->pev->velocity;
		Vector ownerVel = VARS(pev->owner)->velocity;

		Vector newVel;

		newVel.x = tipVel.x + (ownerVel.x * 0.02f);
		newVel.y = tipVel.y + (ownerVel.y * 0.05f);
		newVel.z = tipVel.z + (ownerVel.z * 0.01f);

		m_pTongueTip->pev->velocity = newVel;
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::CreateBeam( CBaseEntity* pTongueTip )
{
#ifndef CLIENT_DLL
	m_pBeam = CBeam::BeamCreate("sprites/tongue.spr", 8);

	if (m_pBeam)
	{
		m_pBeam->PointsInit(pTongueTip->pev->origin, pev->origin);
		m_pBeam->SetFlags( BEAM_FSOLID );
		m_pBeam->SetBrightness( 100.0 );

		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::DestroyBeam(void)
{
#ifndef CLIENT_DLL
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CGrapple::UpdateBeam(void)
{
#ifndef CLIENT_DLL
	if (m_pBeam)
	{
		Vector vecSrc;
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		vecSrc = m_pPlayer->GetGunPosition();
		vecSrc = vecSrc + gpGlobals->v_forward * 16;
		vecSrc = vecSrc + gpGlobals->v_right * 6;
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		m_pBeam->SetStartPos(vecSrc);
		m_pBeam->SetEndPos(m_pTongueTip->pev->origin);
		m_pBeam->RelinkBeam();
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
BOOL CGrapple::CanAttack(float attack_time, float curtime, BOOL isPredicted)
{
#if defined( CLIENT_WEAPONS )
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? TRUE : FALSE;
	}
	else
	{
		return (attack_time <= 0.0) ? TRUE : FALSE;
	}
}


void CGrapple::StartPullSound(void)
{
	m_flNextPullSoundTime = UTIL_WeaponTimeBase();
	m_fPlayPullSound = TRUE;
}

void CGrapple::UpdatePullSound(void)
{
	if (!m_fPlayPullSound)
		return;

	if (m_flNextPullSoundTime <= UTIL_WeaponTimeBase())
	{
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_pull.wav", 1, ATTN_NORM);
		m_flNextPullSoundTime = UTIL_WeaponTimeBase() + 0.6f;
	}
}

void CGrapple::ResetPullSound(void)
{
	STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_pull.wav");
	m_flNextPullSoundTime = 0.0f;
	m_fPlayPullSound = FALSE;
}

void CGrapple::CheckTargets( void )
{//If the target isn't monster or xeno_grapple
	if( checkTarg == 1)
	{
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/bgrapple_impact.wav", 1, ATTN_NORM);
		DestroyTongueTip();
		Fire2();
	}
}

BOOL CGrapple::IsTongueColliding(const Vector& vecShootOrigin, const Vector& vecTipPos)
{
#ifndef CLIENT_DLL
	TraceResult tr;
	UTIL_TraceLine(vecShootOrigin, vecTipPos, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	if (tr.flFraction != 1.0)
	{
		if (m_pTongueTip->pev->movetype == MOVETYPE_FOLLOW)
		{
			if (tr.pHit && m_pTongueTip->pev->aiment && m_pTongueTip->pev->aiment == tr.pHit)
				return FALSE;
		}

		return TRUE;
	}

	return FALSE;
#else
	return TRUE;
#endif
}

void CGrapple::CheckFireEligibility(void)
{
#ifndef CLIENT_DLL
	// Do not check for this if the tongue is not valid.
	if (!m_pTongueTip)
		return;

	// Check if the tongue is through walls or if other things
	// such as entities are between the owner and the tip.
	if (IsTongueColliding(m_pPlayer->GetGunPosition(), m_pTongueTip->pev->origin))
	{
		// Stop firing!
		//FireRelease();
	}
#endif
}


BOOL CGrapple::CheckTargetProximity(void)
{
#ifndef CLIENT_DLL
	// Do not check for this if the tongue is not valid.
	if (!m_pTongueTip)
		return FALSE;

		// Trace a hull around to see if we hit something within 20 units.
		TraceResult tr;
		UTIL_TraceHull(
			pev->origin,
			pev->origin + VARS(pev->owner)->velocity.Normalize() * 20,
			dont_ignore_monsters,
			head_hull,
			edict(),
			&tr);

		CBaseEntity *pEnt = CBaseEntity::Instance( tr.pHit );
		// Check to see if we are close enough to our target.
		// In the case o a monster, attempt to get a pointer to
		// the entity, gib it, release grappler fire.
		edict_t* pHit = ENT(tr.pHit);
		//CBaseEntity* pEnt = GetClassPtr((CBaseEntity*)VARS(tr.pHit));
		
		if(m_pTongueTip->pev->aiment == tr.pHit && (pEnt->pev->flags & FL_MONSTER))
		{
			if(timer <= gpGlobals->time)
			{
				//ALERT(at_console, "Attack\n");
				UTIL_MakeVectors( m_pPlayer->pev->v_angle );
				float flDamage = gSkillData.plrDmgGrapple;

				if(g_pGameRules->IsMultiplayer())
					flDamage *= 2;

				ClearMultiDamage();
				   pEnt->TraceAttack( m_pPlayer->pev, flDamage, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_ALWAYSGIB );
				ApplyMultiDamage( pev, m_pPlayer->pev );
				timer = gpGlobals->time + 0.6;
				if(!pEnt->IsAlive() && pEnt->IsMonster())
					deadflag = 1;
			}
		}
#endif
	return FALSE;
}