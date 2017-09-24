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
#include "explode.h"
#include "shake.h"
#include "gamerules.h"
#include "effects.h"
#include "decals.h"
#include "soundent.h"
#include "customentity.h"

#define SHOCK_AIR_VELOCITY		20000


LINK_ENTITY_TO_CLASS( shock_beam, CShockBeam );

void CShockBeam :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}

CShockBeam *CShockBeam::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CShockBeam *pShock = GetClassPtr((CShockBeam *)NULL);
	pShock->Spawn();

	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->pev->velocity = vecVelocity;
	pShock->pev->angles = UTIL_VecToAngles( pShock->pev->velocity );
	pShock->pev->owner = ENT(pevOwner);
	pShock->pev->classname = MAKE_STRING("shock_beam");
}

void CShockBeam::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/shock_effect.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	BlastOn();
	Glow ();

	SetTouch( &CShockBeam::ShockTouch );

	if ( g_pGameRules->IsMultiplayer() )
		pev->dmg = 20;
	else
		pev->dmg = 10;


	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;
}


void CShockBeam::Precache( )
{
	PRECACHE_MODEL("models/shock_effect.mdl");

	PRECACHE_SOUND( "weapons/shock_fire.wav" );
	PRECACHE_SOUND( "weapons/shock_impact.wav" );

	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/glow02.spr" );
 	
}

int	CShockBeam :: Classify ( void )
{
	return	CLASS_NONE;
}

void CShockBeam::ShockTouch( CBaseEntity *pOther )
{
	if ( FClassnameIs( pOther->pev, "monster_maria" ) 
		|| FClassnameIs( pOther->pev, "monster_boris" ) )
		return;

	entvars_t	*pevOwner;

	pevOwner = VARS( pev->owner );

	BlastOff();

	if ( UTIL_PointContents(pev->origin) == CONTENT_SKY )
	{
		return;
	}
	if ( UTIL_PointContents(pev->origin) == CONTENT_WATER )
	{
		::RadiusDamage ( pev->origin, pev, pevOwner, 100, 150, CLASS_NONE, DMG_SHOCK );
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		if ( g_pGameRules->IsMultiplayer() )
			pOther->TraceAttack(pevOwner, 20, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM || DMG_ALWAYSGIB );
		else
			pOther->TraceAttack(pevOwner, 10,pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM || DMG_ALWAYSGIB);

		ApplyMultiDamage( pev, pevOwner );

		if ( pOther->pev->flags & FL_MONSTER )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor.x = 0; // R
			pOther->pev->rendercolor.y = 255; // G
			pOther->pev->rendercolor.z = 255; // B
			pOther->pev->renderamt = 1;
			pShockedEnt = pOther;
		}
		if ( pOther->IsPlayer() )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor.x = 0; // R
			pOther->pev->rendercolor.y = 255; // G
			pOther->pev->rendercolor.z = 255; // B
			pOther->pev->renderamt = 1;
			pShockedEnt = pOther;
		}

		SetThink(&CShockBeam::FadeShock);
		pev->nextthink = gpGlobals->time +1;

		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", VOL_NORM, ATTN_NORM);
	}
	UTIL_Sparks( pev->origin );
	ExplodeThink();
}

void CShockBeam::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;

	BlastOff();

	iScale = 10;

	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 6 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );


	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	TraceResult tr;

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
	UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));
}
void CShockBeam::BlastOff ( void )
{
		UTIL_Remove( this );

		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;

		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;

		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
}
void CShockBeam::FadeShock ( void )
{
	if ( pShockedEnt )
	{
		pShockedEnt->pev->renderfx = kRenderFxNone;
		pShockedEnt->pev->rendercolor.x = 0; // R
		pShockedEnt->pev->rendercolor.y = 0; // G
		pShockedEnt->pev->rendercolor.z = 0; // B
		pShockedEnt->pev->renderamt = 255;
	}
}
void CShockBeam :: Glow( void )
{
	Vector		posGun, angleGun;
	int m_iAttachment = 1;
	GetAttachment( m_iAttachment, posGun, angleGun );

	m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), m_iAttachment );
	m_pSprite->pev->scale = 0.25;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 125, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
}
void CShockBeam::BlastOn ( void )
{
	Vector posGun;
	Vector angleGun;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 60 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	if(m_pBeam)
	{
		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetBrightness( 180 );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetScrollRate( 10 );
		m_pBeam->SetNoise( 0 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->pev->spawnflags |= SF_SPRITE_TEMPORARY;
		m_pBeam->pev->flags |= FL_SKIPLOCALHOST;
		m_pBeam->SetColor( 0, 253, 253 );
	}
	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	GetAttachment( 1, posGun, angleGun );
	GetAttachment( 2, posGun, angleGun );

	if(m_pNoise)
	{
		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetBrightness( 180 );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetScrollRate( 30 );
		m_pNoise->SetNoise( 30 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 157 );
		m_pNoise->pev->spawnflags |= SF_SPRITE_TEMPORARY;
		m_pNoise->pev->flags |= FL_SKIPLOCALHOST;
		m_pNoise->pev->rendermode = kRenderTransTexture;
		m_pNoise->pev->renderamt = 180;
	}
	EXPORT void RelinkBeam();
}
