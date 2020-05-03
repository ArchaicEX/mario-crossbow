// breakable_box.c.inc

struct ObjectHitbox sBreakableBoxSmallHitbox = {
    /* interactType:      */ INTERACT_GRABBABLE,
    /* downOffset:        */ 20,
    /* damageOrCoinValue: */ 0,
    /* health:            */ 1,
    /* numLootCoins:      */ 0,
    /* radius:            */ 8,
    /* height:            */ 16,
    /* hurtboxRadius:     */ 8,
    /* hurtboxHeight:     */ 16,
};

void bhv_breakable_box_small_init(void) {
    o->oGravity = 0;
    o->oFriction = 0.99f;
    o->oBuoyancy = 1.4f;
    cur_obj_scale(0.0256f);
    obj_set_hitbox(o, &sBreakableBoxSmallHitbox);
    o->oAnimState = 1;
    o->activeFlags |= 0x200;
	o->oHeldState = 2;
}

void small_breakable_box_spawn_dust(void) {
    struct Object *sp24 = spawn_object(o, MODEL_SMOKE, bhvSmoke);
    sp24->oPosX += (s32)(random_float() * 80.0f) - 40;
    sp24->oPosZ += (s32)(random_float() * 80.0f) - 40;
}

void small_breakable_box_act_move(void) {
	obj_move_xyz_using_fvel_and_yaw(o);
    obj_attack_collided_from_other_object(o);
}

void breakable_box_small_released_loop(void) {
    o->oBreakableBoxSmallFramesSinceReleased++;
    if (o->oBreakableBoxSmallFramesSinceReleased > 120) {
        o->activeFlags = 0;
    }
}

void breakable_box_small_idle_loop(void) {
    switch (o->oAction) {
        case 0:
            small_breakable_box_act_move();
            break;

        case 100:
            obj_lava_death();
            break;

        case 101:
            o->activeFlags = 0;
            break;
    }

    if (o->oBreakableBoxSmallReleased == 1)
        breakable_box_small_released_loop();
}

void breakable_box_small_get_dropped(void) {
    cur_obj_become_tangible();
    cur_obj_enable_rendering();
    cur_obj_get_dropped();
    o->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
    o->oHeldState = 0;
    o->oBreakableBoxSmallReleased = 1;
    o->oBreakableBoxSmallFramesSinceReleased = 0;
}

void breakable_box_small_get_thrown(void) {
    cur_obj_become_tangible();
    cur_obj_enable_rendering_2();
    cur_obj_enable_rendering();
    o->header.gfx.node.flags &= ~GRAPH_RENDER_INVISIBLE;
    o->oHeldState = 0;
    o->oFlags &= ~0x08;
    o->oForwardVel = coss(-o->oFaceAnglePitch) * 350.f;
    o->oVelY = sins(-o->oFaceAnglePitch) * 350.f;
    o->oBreakableBoxSmallReleased = 1;
    o->oBreakableBoxSmallFramesSinceReleased = 0;
    o->activeFlags &= ~0x200;
}

void bhv_breakable_box_small_loop(void) {
    switch (o->oHeldState) {
        case 0:
            breakable_box_small_idle_loop();
            break;

        case 1:
            cur_obj_disable_rendering();
            cur_obj_become_intangible();
            break;

        case 2:
            breakable_box_small_get_thrown();
            break;

        case 3:
            breakable_box_small_get_dropped();
            break;
    }

    o->oInteractStatus = 0;
}
