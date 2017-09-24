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
#include "soundent.h"
#include "gamerules.h"
#include "sporegrenade.h"
#include "decals.h"
#include "explode.h"

LINK_ENTITY_TO_CLASS( spore, CSpore );
CSpore *CSpore::CreateSporeGrenade( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CSpore *pSpore = GetClassPtr( (CSpore *)NULL );
	UTIL_SetOrigin( pSpore->pev, vecOrigin );
	pSpore->pev->angles = vecAngles;
	pSpore->m_iPrimaryMode = FALSE;
	pSpore->pev->angles = vecAngles;
	pSpore->pev->owner = pOwner->edict();
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->Spawn();

	return pSpore;
}

//=========================================================
//=========================================================
CSpore *CSpore::CreateSporeRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CSpore *pSpore = GetClassPtr( (CSpore *)NULL );
	UTIL_SetOrigin( pSpore->pev, vecOrigin );
	pSpore->pev->angles = vecAngles;
	pSpore->m_iPrimaryMode = TRUE;
	pSpore->pev->angles = vecAngles;
	pSpore->pev->owner = pOwner->edict();
	pSpore->pev->classname = MAKE_STRING("spore");
	pSpore->Spawn();
 	pSpore->pev->angles = vecAngles;

	return pSpore;
}
//=========================================================
//=========================================================

void CSpore :: Spawn( void )
{
	Precache( );
	// motor
	if (m_iPrimaryMode)
		pev->movetype = MOVETYPE_FLY;
	else
		pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, -4), Vector(4, 4, 4));
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_MakeVectors( pev->angles );

	pev->gravity = 0.5;
	Glow ();


	if (m_iPrimaryMode)
	{
		SetThink( &CSpore::FlyThink );
		SetTouch( &CSpore::ExplodeThink );
		pev->velocity = gpGlobals->v_forward * 250;
	}
	else
	{
		SetThink( &CSpore::FlyThink );
		SetTouch( &CSpore::BounceThink );
	}

	pev->dmgtime = gpGlobals->time + 2;
	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmg = gSkillData.plrDmgSpore;
}
//=========================================================

void CSpore :: Precache( void )
{
	PRECACHE_MODEL("models/spore.mdl");
	m_iDrips = PRECACHE_MODEL("sprites/tinyspit.spr");
	m_iGlow = PRECACHE_MODEL("sprites/glow02.spr");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_01.spr");
	m_iExplodeC = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND ("weapons/splauncher_impact.wav");
	PRECACHE_SOUND ("weapons/spore_hit1.wav");//Bounce grenade
	PRECACHE_SOUND ("weapons/spore_hit2.wav");//Bounce grenade
	PRECACHE_SOUND ("weapons/spore_hit3.wav");//Bounce grenade
}
//=========================================================

void CSpore :: Glow( void )
{
	m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.75;
	m_pSprite->SetTransparency( kRenderTransAdd, 150, 158, 19, 155, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
}
//=========================================================

void CSpore :: FlyThink( void  )
{
	pev->nextthink = gpGlobals->time + 0.001;
 
	TraceResult tr;
 
	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x + RANDOM_LONG(-5,5));	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y + RANDOM_LONG(-5,5) );
		WRITE_COORD( pev->origin.z + RANDOM_LONG(-5,5));
		WRITE_COORD( tr.vecPlaneNormal.x );
		WRITE_COORD( tr.vecPlaneNormal.y );
		WRITE_COORD( tr.vecPlaneNormal.z );
		WRITE_SHORT( m_iDrips );
		WRITE_BYTE( 1  ); // count
		WRITE_BYTE( 18  ); // speed
		WRITE_BYTE( 1000 );
	MESSAGE_END();
if(!m_iPrimaryMode)
{
if( pev->dmgtime <= gpGlobals->time )
Explode();
}
}
//=========================================================

void CSpore::BounceThink(  CBaseEntity *pOther  )
{
	if ( pOther->pev->flags & FL_MONSTER || pOther->IsPlayer()  )
	{
		if ( !FClassnameIs( pOther->pev, "monster_maria" ) 
			&& !FClassnameIs( pOther->pev, "monster_boris" ) )
		Explode ();
	}

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove( this );
		return;
	}

#ifndef CLIENT_DLL
	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_GENERIC ); 
			ApplyMultiDamage( pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}
#endif

	Vector vecTestVelocity;

	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}
//=========================================================

void CSpore :: BounceSound( void )
{
	switch (RANDOM_LONG (0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit1.wav", 0.25, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit2.wav", 0.25, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit3.wav", 0.25, ATTN_NORM); break;
	}
}
//=========================================================

void CSpore::ExplodeThink(  CBaseEntity *pOther  )
{
	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove( this );
		return;
	}
Explode();
	if ( !FClassnameIs( pOther->pev, "monster_maria" ) 
		|| !FClassnameIs( pOther->pev, "monster_boris" ) )
		return;

	Explode ();
}
//=========================================================

void CSpore::Explode( void )
{
	SetTouch( NULL );
	SetThink( NULL );
	EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/splauncher_impact.wav", 1, ATTN_NORM);

	TraceResult tr;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			switch ( RANDOM_LONG( 0, 1 ) )
			{
				case 0:	
					WRITE_SHORT( m_iExplode );
					break;

				default:
				case 1:
					WRITE_SHORT( m_iExplodeC );
					break;
			}
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 155  ); // framerate
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE_SPRAY );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( tr.vecPlaneNormal.x );
			WRITE_COORD( tr.vecPlaneNormal.y );
			WRITE_COORD( tr.vecPlaneNormal.z );
			WRITE_SHORT( m_iDrips );
			WRITE_BYTE( 50  ); // count
			WRITE_BYTE( 30  ); // speed
			WRITE_BYTE( 640 );
		MESSAGE_END();


	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 12 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( 20 );		// time * 10
		WRITE_BYTE( 20 );		// decay * 0.1
	MESSAGE_END( );

    	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set


	::RadiusDamage ( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_PLAYER_BIOWEAPON, DMG_GENERIC );

	if (m_iPrimaryMode)
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_YBLOOD5 + RANDOM_LONG(0, 1));
	}

	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
	UTIL_Remove ( this );
	UTIL_Remove( m_pSprite );
	m_pSprite = NULL;
}