
/**
 * Behavior for bhvGoomba and bhvGoombaTripletSpawner,
 * Goombas can either be spawned individually, or spawned by a triplet spawner.
 * The triplet spawner comes before its spawned goombas in processing order.
 */

/**
 * Hitbox for goomba.
 */
static struct ObjectHitbox sGoombaHitbox = {
    /* interactType:      */ INTERACT_BOUNCE_TOP,
    /* downOffset:        */ 0,
    /* damageOrCoinValue: */ 1,
    /* health:            */ 0,
    /* numLootCoins:      */ 1,
    /* radius:            */ 72,
    /* height:            */ 50,
    /* hurtboxRadius:     */ 45,
    /* hurtboxHeight:     */ 40,
};

/**
 * Properties that vary based on goomba size.
 */
struct GoombaProperties {
    f32 scale;
    u32 deathSound;
    s16 drawDistance;
    s8 damage;
};

/**
 * Properties for regular, huge, and tiny goombas.
 */
static struct GoombaProperties sGoombaProperties[] = {
    { 1.5f, SOUND_OBJ_ENEMY_DEATH_HIGH, 4000, 1 },
    { 3.5f, SOUND_OBJ_ENEMY_DEATH_LOW, 4000, 2 },
    { 0.5f, SOUND_OBJ_ENEMY_DEATH_HIGH, 1500, 0 },
};

static void *sPaths[] = { bob_seg7_metal_ball_path0, bob_seg7_metal_ball_path1, bob_seg7_trajectory_koopa };

/**
 * Attack handlers for goombas.
 */
static u8 sGoombaAttackHandlers[][6] = {
    // regular and tiny
    {
        /* ATTACK_PUNCH:                 */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_KICK_OR_TRIP:          */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_FROM_ABOVE:            */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_GROUND_POUND_OR_TWIRL: */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_FAST_ATTACK:           */ ATTACK_HANDLER_KNOCKBACK,
        /* ATTACK_FROM_BELOW:            */ ATTACK_HANDLER_KNOCKBACK,
    },
    // huge
    {
        /* ATTACK_PUNCH:                 */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_KICK_OR_TRIP:          */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_FROM_ABOVE:            */ ATTACK_HANDLER_SQUISHED,
        /* ATTACK_GROUND_POUND_OR_TWIRL: */ ATTACK_HANDLER_SQUISHED_WITH_BLUE_COIN,
        /* ATTACK_FAST_ATTACK:           */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
        /* ATTACK_FROM_BELOW:            */ ATTACK_HANDLER_SPECIAL_HUGE_GOOMBA_WEAKLY_ATTACKED,
    },
};

/**
 * Update function for goomba triplet spawner.
 */
void bhv_goomba_triplet_spawner_update(void) {
    UNUSED s32 unused1;
    s16 goombaFlag;
    UNUSED s16 unused2;
    s32 angle;
    s32 dAngle;
    s16 dx;
    s16 dz;

    // If mario is close enough and the goombas aren't currently loaded, then
    // spawn them
    if (o->oAction == GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED) {
        if (o->oDistanceToMario < 3000.0f) {
            // The spawner is capable of spawning more than 3 goombas, but this
            // is not used in the game
            dAngle =
                0x10000
                / (((o->oBehParams2ndByte & GOOMBA_TRIPLET_SPAWNER_BP_EXTRA_GOOMBAS_MASK) >> 2) + 3);

            for (angle = 0, goombaFlag = 1 << 8; angle < 0xFFFF; angle += dAngle, goombaFlag <<= 1) {
                // Only spawn goombas which haven't been killed yet
                if (!(o->oBehParams & goombaFlag)) {
                    dx = 500.0f * coss(angle);
                    dz = 500.0f * sins(angle);

                    spawn_object_relative((o->oBehParams2ndByte & GOOMBA_TRIPLET_SPAWNER_BP_SIZE_MASK)
                                              | (goombaFlag >> 6),
                                          dx, 0, dz, o, MODEL_GOOMBA, bhvGoomba);
                }
            }

            o->oAction += 1;
        }
    } else if (o->oDistanceToMario > 4000.0f) {
        // If mario is too far away, enter the unloaded action. The goombas
        // will detect this and unload themselves
        o->oAction = GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED;
    }
}

/**
 * Initialization function for goomba.
 */
void bhv_goomba_init(void) {
    o->oGoombaSize = o->oBehParams2ndByte & GOOMBA_BP_SIZE_MASK;

    o->oGoombaScale = sGoombaProperties[o->oGoombaSize].scale;
    o->oDeathSound = sGoombaProperties[o->oGoombaSize].deathSound;

    obj_set_hitbox(o, &sGoombaHitbox);

    o->oDrawingDistance = sGoombaProperties[o->oGoombaSize].drawDistance;
    o->oDamageOrCoinValue = sGoombaProperties[o->oGoombaSize].damage;

    o->oGravity = -8.0f / 3.0f * o->oGoombaScale;

	o->oPathedStartWaypoint = o->oPathedPrevWaypoint =
		segmented_to_virtual(sPaths[o->oBehParams2ndByte >> 4]);
	o->oFlags |= OBJ_FLAG_ACTIVE_FROM_AFAR;
}

/**
 * Enter the jump action and set initial y velocity.
 */
static void goomba_begin_jump(void) {
    cur_obj_play_sound_2(SOUND_OBJ_GOOMBA_ALERT);
    o->oAction = GOOMBA_ACT_JUMP;
    o->oForwardVel = 0.0f;
    o->oVelY = 50.0f / 3.0f * o->oGoombaScale;
}

/**
 * If spawned by a triplet spawner, mark the flag in the spawner to indicate that
 * this goomba died. This prevents it from spawning again when mario leaves and
 * comes back.
 */
static void mark_goomba_as_dead(void) {
    if (o->parentObj != o) {
        set_object_respawn_info_bits(o->parentObj,
                                     (o->oBehParams2ndByte & GOOMBA_BP_TRIPLET_FLAG_MASK) >> 2);

        o->parentObj->oBehParams =
            o->parentObj->oBehParams | (o->oBehParams2ndByte & GOOMBA_BP_TRIPLET_FLAG_MASK) << 6;
    }
}

/**
 * Walk around randomly occasionally jumping. If mario comes within range,
 * chase him.
 */
static void goomba_act_walk(void) {
	cur_obj_follow_path(0);
	cur_obj_rotate_yaw_toward(o->oPathedTargetYaw, 0x400);
	o->oForwardVel = 40.0f;
}

/**
 * This action occurs when either the goomba attacks mario normally, or mario
 * attacks a huge goomba with an attack that doesn't kill it.
 */
static void goomba_act_attacked_mario(void) {
    if (o->oGoombaSize == GOOMBA_SIZE_TINY) {
        mark_goomba_as_dead();
        o->oNumLootCoins = 0;
        obj_die_if_health_non_positive();
    } else {
        goomba_begin_jump();
    }
}

/**
 * Move until landing, and rotate toward target yaw.
 */
static void goomba_act_jump(void) {
    obj_resolve_object_collisions(NULL);

    if (o->oMoveFlags & OBJ_MOVE_MASK_ON_GROUND) {
        o->oAction = GOOMBA_ACT_WALK;
    } else {
    }
}

/**
 * Attack handler for when mario attacks a huge goomba with an attack that
 * doesn't kill it.
 * From the goomba's perspective, this is the same as the goomba attacking
 * mario.
 */
void huge_goomba_weakly_attacked(void) {
    o->oAction = GOOMBA_ACT_ATTACKED_MARIO;
}

/**
 * Update function for goomba.
 */
void bhv_goomba_update(void) {
    // PARTIAL_UPDATE

    f32 animSpeed;

    if (obj_update_standard_actions(o->oGoombaScale)) {
        // If this goomba has a spawner and mario moved away from the spawner,
        // unload
        if (o->parentObj != o) {
            if (o->parentObj->oAction == GOOMBA_TRIPLET_SPAWNER_ACT_UNLOADED) {
                obj_mark_for_deletion(o);
            }
        }

        cur_obj_scale(o->oGoombaScale);
        cur_obj_update_floor_and_walls();

        if ((animSpeed = o->oForwardVel / o->oGoombaScale * 0.4f) < 1.0f) {
            animSpeed = 1.0f;
        }
        cur_obj_init_animation_with_accel_and_sound(0, animSpeed);

        switch (o->oAction) {
            case GOOMBA_ACT_WALK:
                goomba_act_walk();
                break;
            case GOOMBA_ACT_ATTACKED_MARIO:
                goomba_act_attacked_mario();
                break;
            case GOOMBA_ACT_JUMP:
                goomba_act_jump();
                break;
        }

        //! @bug Weak attacks on huge goombas in a triplet mark them as dead even if they're not.
        // obj_handle_attacks returns the type of the attack, which is non-zero
        // even for Mario's weak attacks. Thus, if Mario weakly attacks a huge goomba
        // without harming it (e.g. by punching it), the goomba will be marked as dead
        // and will not respawn if Mario leaves and re-enters the spawner's radius
        // even though the goomba isn't actually dead.
        if (obj_handle_attacks(&sGoombaHitbox, GOOMBA_ACT_ATTACKED_MARIO,
                               sGoombaAttackHandlers[o->oGoombaSize & 1])) {
            mark_goomba_as_dead();
        }

        cur_obj_move_standard(-78);
    } else {
        o->oAnimState = TRUE;
    }
}
