//=================================
// Opposing Forces spore ammo plant 
// Copyright Demiurge
//=================================


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "explode.h"
#include "sporegrenade.h"

//=========================================================
// Opposing Forces Spore Ammo
//=========================================================
#define		SACK_GROUP			1
#define		SACK_EMPTY			0
#define		SACK_FULL			1


class CSporeAmmo : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT BornThink ( void );
	void EXPORT IdleThink ( void );
	void EXPORT AmmoTouch ( CBaseEntity *pOther );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_iExplode;
	float borntime;
	float m_flTimeSporeIdle;
};


typedef enum
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDOWN,
	SPOREAMMO_SNATCHDOWN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2,
} SPOREAMMO;

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo );

TYPEDESCRIPTION	CSporeAmmo::m_SaveData[] = 
{
	DEFINE_FIELD( CSporeAmmo, m_flTimeSporeIdle, FIELD_TIME ),
	DEFINE_FIELD( CSporeAmmo, borntime, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CSporeAmmo, CBaseEntity );

void CSporeAmmo :: Precache( void )
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/spore_ammo.wav");
	UTIL_PrecacheOther ( "spore" );
}
//=========================================================
// Spawn
//=========================================================
void CSporeAmmo :: Spawn( void )
{
	TraceResult tr;
	Precache( );
	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ));
	pev->takedamage = DAMAGE_YES;
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;
	borntime = 1;

	pev->sequence = SPOREAMMO_IDLE;
	pev->body = 0;

	Vector vecOrigin = pev->origin;
	vecOrigin.z += 16;
	UTIL_SetOrigin( pev, vecOrigin );

	pev->angles.x -= 90;// :3

	SetThink (&CSporeAmmo::BornThink);
	SetTouch (&CSporeAmmo::AmmoTouch);
	
	m_flTimeSporeIdle = gpGlobals->time + 22;
	pev->nextthink = gpGlobals->time + 0.1;
}
	
//=========================================================
// Override all damage
//=========================================================
int CSporeAmmo::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!borntime) // rigth '!borntime'  // blast in anytime 'borntime || !borntime'
	{
		CBaseEntity* attacker = GetClassPtr((CBaseEntity*)pevAttacker);

		Vector vecSrc = pev->origin + gpGlobals->v_forward * -20;
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( vecSrc.x );	// Send to PAS because of the sound
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
		MESSAGE_END();

			
		//ALERT( at_console, "angles %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );

		Vector vecAngles = pev->angles + gpGlobals->v_up;
		vecAngles.x = vecAngles.x - 90 + RANDOM_LONG( -20, 20 );
		vecAngles.y = vecAngles.x + 180 + RANDOM_LONG( -20, 20 );
		vecAngles.z += RANDOM_LONG( -20, 20 );

		//ALERT( at_console, "angles %f %f %f\n", angles.x, angles.y, angles.z );

		CSpore *pSpore = CSpore::CreateSporeGrenade( vecSrc, vecAngles, attacker );
		pSpore->pev->velocity = pSpore->pev->velocity + gpGlobals->v_forward * 1000;

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence		= SPOREAMMO_SNATCHDOWN;
		pev->body			= 0;
		borntime			= 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
	}
}

//=========================================================
// Thinking begin
//=========================================================
void CSporeAmmo :: BornThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	pev->sequence = 3;
	pev->framerate		= 1.0;
	pev->animtime		= gpGlobals->time + 0.1;
	pev->body = 1;
	borntime = 0;
	SetThink (&CSporeAmmo::IdleThink);
	
	m_flTimeSporeIdle = gpGlobals->time + 16;
}

void CSporeAmmo :: IdleThink ( void )
{

	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_flTimeSporeIdle > gpGlobals->time )
		return;

	if (borntime)
	{
		pev->sequence = SPOREAMMO_IDLE;

		m_flTimeSporeIdle = gpGlobals->time + 18;
		SetThink(&CSporeAmmo::BornThink);
		return;
	}
	else
	{
		pev->sequence = SPOREAMMO_IDLE1;
	}
}

void CSporeAmmo :: AmmoTouch ( CBaseEntity *pOther )
{
	Vector		vecSpot;
	TraceResult	tr;

	if ( pOther->pev->velocity == g_vecZero || !pOther->IsPlayer() )
		return;

	if (borntime)
		return;

	int bResult = (pOther->GiveAmmo( AMMO_SPORE_GIVE, "spore", SPORE_MAX_CARRY) != -1);
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);

		pev->framerate		= 1.0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence = SPOREAMMO_SNATCHDOWN;
		pev->body = 0;
		borntime = 1;
		m_flTimeSporeIdle = gpGlobals->time + 1;
		SetThink (&CSporeAmmo::IdleThink);
	}
}