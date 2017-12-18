/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Black Ops - Male Assassin
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"hgrunt.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define MASSN_LIMP_HEALTH           20

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)

// Body groups.
#define HEAD_GROUP					1
#define GUN_GROUP					2

// Head values
#define HEAD_WHITE					0
#define HEAD_BLACK					1
#define HEAD_GOGGLES				2

// Gun values
#define GUN_MP5						0
#define GUN_SNIPERRIFLE				1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

//=========================================================
// Purpose:
//=========================================================
class CMassn : public CHGrunt
{
public:
    int  Classify(void);
    void HandleAnimEvent(MonsterEvent_t *pEvent);
    void Sniperrifle(void);
    void SetActivity(Activity NewActivity);
    void GibMonster();

    BOOL FOkToSpeak(void);

    void Spawn( void );
    void Precache( void );

    void DeathSound(void);
    void PainSound(void);
    void IdleSound(void);
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMassn);

//=========================================================
// Purpose:
//=========================================================
BOOL CMassn::FOkToSpeak(void)
{
    return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CMassn::Classify(void)
{
    return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// Purpose:
//=========================================================
void CMassn::IdleSound(void)
{
}

//=========================================================
// Shoot
//=========================================================
void CMassn::Sniperrifle(void)
{
    if (m_hEnemy == NULL)
    {
        return;
    }

    Vector vecShootOrigin = GetGunPosition();
    Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

    UTIL_MakeVectors(pev->angles);

    Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
    EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
    FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_PLAYER_357, 0); // shoot +-7.5 degrees

    pev->effects |= EF_MUZZLEFLASH;

    EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);

    Vector angDir = UTIL_VecToAngles(vecShootDir);
    SetBlending(0, angDir.x);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
    Vector	vecShootDir;
    Vector	vecShootOrigin;

    switch (pEvent->event)
    {
    case 2:
    {
        if(FBitSet(pev->weapons, MASSN_9MMAR))
        {
            EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
            m_cAmmoLoaded = m_cClipSize;
            ClearConditions( bits_COND_NO_AMMO_LOADED );
        }
        else
        {
            EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/sniper_bolt1.wav", 1, ATTN_NORM );
            m_cAmmoLoaded = 5;
            ClearConditions( bits_COND_NO_AMMO_LOADED );
        }
    }
    break;

    case MASSN_AE_DROP_GUN:
    {
            Vector	vecGunPos;
            Vector	vecGunAngles;

            GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
            SetBodygroup(GUN_GROUP, GUN_NONE);

		// now spawn a gun.
        if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
        {
            DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
        }
        else
        {
            DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
        }

        if (FBitSet(pev->weapons, MASSN_GRENADELAUNCHER))
        {
            DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
        }
    }
    break;


    case MASSN_AE_BURST1:
    {
        if (FBitSet(pev->weapons, MASSN_9MMAR))
        {
            Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
            if (RANDOM_LONG(0, 1))
            {
                EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
            }
            else
            {
                EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
            }
        }
        else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
        {
			Sniperrifle();
        }

        CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
    }
    break;

    case MASSN_AE_KICK:
    {
        CBaseEntity *pHurt = Kick();

        if (pHurt)
        {
                        // SOUND HERE!
            UTIL_MakeVectors(pev->angles);
            pHurt->pev->punchangle.x = 15;
            pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
            pHurt->TakeDamage(pev, pev, gSkillData.massnDmgKick, DMG_CLUB);
        }
    }
    break;

    case MASSN_AE_CAUGHT_ENEMY:
        break;

    default:
        CHGrunt::HandleAnimEvent(pEvent);
            break;
    }
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
    Precache();

    SET_MODEL(ENT(pev), "models/massn.mdl");
    UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

    pev->solid = SOLID_SLIDEBOX;
    pev->movetype = MOVETYPE_STEP;
    m_bloodColor = BLOOD_COLOR_RED;
    pev->effects = 0;
    pev->health = gSkillData.massnHealth;
    m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
    m_MonsterState = MONSTERSTATE_NONE;
    m_flNextGrenadeCheck = gpGlobals->time + 1;
    m_flNextPainTime = gpGlobals->time;
    m_iSentence = -1;

    m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

    m_fEnemyEluded = FALSE;
    m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

    m_HackedGunPos = Vector(0, 0, 55);

    if (pev->weapons == 0)
    {
        // initialize to original values
        pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
        // pev->weapons = HGRUNT_SHOTGUN;
        // pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
    }

    if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
    {
        SetBodygroup(GUN_GROUP, GUN_SNIPERRIFLE);
        m_flFieldOfView = 0.5;
        m_cClipSize = 5;
    }
    else
    {
        m_cClipSize = MASSN_CLIP_SIZE;
    }
    m_cAmmoLoaded = m_cClipSize;

    if (RANDOM_LONG(0, 99) < 80)
        pev->skin = 0;	// light skin
    else
        pev->skin = 1;	// dark skin

    CTalkMonster::g_talkWaitTime = 0;

    m_flNextAttack = gpGlobals->time;

    MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
    PRECACHE_MODEL("models/massn.mdl");

    PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
    PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

    PRECACHE_SOUND("hgrunt/gr_reload1.wav");

    PRECACHE_SOUND("weapons/glauncher.wav");

    PRECACHE_SOUND("weapons/sniper_bolt1.wav");

    PRECACHE_SOUND("weapons/sniper_fire.wav");

    PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

    // get voice pitch
    if (RANDOM_LONG(0, 1))
        m_voicePitch = 109 + RANDOM_LONG(0, 7);
    else
        m_voicePitch = 100;

    m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound(void)
{
}

//=========================================================
// DeathSound 
//=========================================================
void CMassn::DeathSound(void)
{
}

void CMassn::GibMonster( void )
{
    Vector vecGunPos;
    Vector vecGunAngles;

    if( GetBodygroup( 2 ) != 2 )
    {
        // throw a gun if the grunt has one
        GetAttachment( 0, vecGunPos, vecGunAngles );

        CBaseEntity *pGun;

        if( FBitSet( pev->weapons, MASSN_SNIPERRIFLE ) )
        {
            pGun = DropItem( "weapon_sniperrifle", vecGunPos, vecGunAngles );
        }
        else
        {
            pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
        }

        if( pGun )
        {
            pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
            pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
        }

        if( FBitSet( pev->weapons, MASSN_GRENADELAUNCHER ) )
        {
            pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
            if ( pGun )
            {
                pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
                pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
            }
        }
    }

    CBaseMonster::GibMonster();
}


//=========================================================
// SetActivity
//=========================================================
void CMassn::SetActivity( Activity NewActivity )
{
    int iSequence = ACTIVITY_NOT_AVAILABLE;

    switch( NewActivity )
    {
    case ACT_RANGE_ATTACK1:
        // grunt is either shooting standing or shooting crouched
        if( FBitSet( pev->weapons, MASSN_9MMAR ) )
        {
            if( m_fStanding )
            {
                // get aimable sequence
                iSequence = LookupSequence( "standing_mp5" );
            }
            else
            {
                // get crouching shoot
                iSequence = LookupSequence( "crouching_mp5" );
            }
        }
        else
        {
            if( m_fStanding )
            {
                // get aimable sequence
                iSequence = LookupSequence( "standing_shotgun" );
            }
            else
            {
                // get crouching shoot
                iSequence = LookupSequence( "crouching_shotgun" );
            }
        }
        break;
    case ACT_RANGE_ATTACK2:
        // grunt is going to a secondary long range attack. This may be a thrown
        // grenade or fired grenade, we must determine which and pick proper sequence
        if( pev->weapons & MASSN_HANDGRENADE )
        {
            // get toss anim
            iSequence = LookupSequence( "throwgrenade" );
        }
        else
        {
            // get launch anim
            iSequence = LookupSequence( "launchgrenade" );
        }
        break;
    case ACT_RUN:
        if( pev->health <= MASSN_LIMP_HEALTH )
        {
            // limp!
            iSequence = LookupActivity( ACT_RUN_HURT );
        }
        else
        {
            iSequence = LookupActivity( NewActivity );
        }
        break;
    case ACT_WALK:
        if( pev->health <= MASSN_LIMP_HEALTH )
        {
            // limp!
            iSequence = LookupActivity( ACT_WALK_HURT );
        }
        else
        {
            iSequence = LookupActivity( NewActivity );
        }
        break;
    case ACT_IDLE:
        if ( m_MonsterState == MONSTERSTATE_COMBAT )
        {
            NewActivity = ACT_IDLE_ANGRY;
        }
        iSequence = LookupActivity( NewActivity );
        break;
    default:
        iSequence = LookupActivity( NewActivity );
        break;
    }

    m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

    // Set to the desired anim, or default anim if the desired is not present
    if( iSequence > ACTIVITY_NOT_AVAILABLE )
    {
        if( pev->sequence != iSequence || !m_fSequenceLoops )
        {
            pev->frame = 0;
        }

        if(FBitSet(pev->weapons, MASSN_SNIPERRIFLE) && NewActivity == ACT_RANGE_ATTACK1)
        {
            if(gpGlobals->time - m_flNextAttack >= 2.25)
            {
                pev->sequence = iSequence;	// Set to the reset anim (if it's there)
                ResetSequenceInfo();
                SetYawSpeed();
                m_flNextAttack = gpGlobals->time;
            }
        }
        else
        {
            pev->sequence = iSequence;	// Set to the reset anim (if it's there)
            ResetSequenceInfo();
            SetYawSpeed();
        }
    }
    else
    {
        // Not available try to get default anim
        ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
        pev->sequence = 0;	// Set to the reset anim (if it's there)
    }
}


//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CAssassinRepel : public CHGruntRepel
{
public:
    void Precache(void);
    void EXPORT RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel);

void CAssassinRepel::Precache(void)
{
    UTIL_PrecacheOther("monster_male_assassin");
    m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CAssassinRepel::RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
    TraceResult tr;
    UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
	return NULL;
	*/

    CBaseEntity *pEntity = Create("monster_male_assassin", pev->origin, pev->angles);
    CBaseMonster *pGrunt = pEntity->MyMonsterPointer();
    pGrunt->pev->movetype = MOVETYPE_FLY;
    pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
    pGrunt->SetActivity(ACT_GLIDE);
	// UNDONE: position?
    pGrunt->m_vecLastPosition = tr.vecEndPos;

    CBeam *pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
    pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
    pBeam->SetFlags(BEAM_FSOLID);
    pBeam->SetColor(255, 255, 255);
    pBeam->SetThink(&CBeam::SUB_Remove);
    pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

    UTIL_Remove(this);
}
