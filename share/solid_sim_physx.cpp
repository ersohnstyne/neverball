/*
 * Copyright (C) 2024 Microsoft / Neverball authors
 *
 * NEVERBALL is  free software; you can redistribute  it and/or modify
 * it under the  terms of the GNU General  Public License as published
 * by the Free  Software Foundation; either version 2  of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT  ANY  WARRANTY;  without   even  the  implied  warranty  of
 * MERCHANTABILITY or  FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
 * General Public License for more details.
 */

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#endif

#include <math.h>

extern "C"
{
#include "vec3.h"
#include "common.h"
#include "log.h"

#include "solid_vary.h"
#include "solid_sim.h"
#include "solid_all.h"
}

/*---------------------------------------------------------------------------*/

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1
#if _WIN64
#pragma comment(lib, "PhysX_64.lib")
#ifdef _DEBUG
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#endif
#else
#pragma comment(lib, "PhysX_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "PhysXPvdSDK_static_32.lib")
#endif
#endif
#endif

/*---------------------------------------------------------------------------*/

#if _WIN32 && _MSC_VER && ENABLE_NVIDIA_PHYSX==1

/* The new powerful Nvidia PhysX version comes into Neverball. */

physx::PxDefaultAllocator      vary_alloc_callback;
physx::PxDefaultErrorCallback  vary_error_callback;
physx::PxDefaultCpuDispatcher *vary_dispatcher      = 0;
physx::PxTolerancesScale       vary_tolerance_scale;

physx::PxFoundation *vary_foundation = 0;
physx::PxPhysics    *vary_physics    = 0;

#ifdef _DEBUG
physx::PxPvd            *vary_pvd           = 0;
physx::PxPvdTransport   *vary_pvd_transport = 0;
physx::PxPvdSceneClient *vary_pvd_client    = 0;
#endif

physx::PxScene    *vary_scene = 0;

physx::PxMaterial *vary_wborder_mtrl = 0;
physx::PxMaterial *vary_player_mtrl  = 0;
physx::PxMaterial *vary_sqwood_mtrl  = 0;
physx::PxMaterial *vary_sqmetal_mtrl = 0;

physx::PxShape       *wborder_shape = 0;
physx::PxRigidStatic *wborder_body  = 0;

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/*
 * Accumulate and convert simulation time to integer milliseconds.
 */

static void ms_init_physx(float *accum)
{
    *accum = 0.0f;
}

static int ms_step_physx(float *accum, float dt)
{
    int ms = 0;

    *accum += dt;

    while (*accum >= 0.001f)
    {
        *accum -= 0.001f;
        ms += 1;
    }

    return ms;
}

static int ms_peek_physx(float *accum, float dt)
{
    float at = *accum;

    return ms_step_physx(&at, dt);
}

/*---------------------------------------------------------------------------*/

extern "C" float sol_step_physx(struct s_vary *vary, cmd_fn cmd_func,
                                const float *g, float dt, int ui)
{
    float P[3], V[3], v[3], r[3], a[3], d, nt, b = 0.0f, tt = dt;
    int c;

    if (ui < vary->uc)
    {
        struct v_ball *up = vary->uv + ui;

        //v_cpy(a, up->v);
        //v_cpy(v, up->v);
        //v_cpy(up->v, g);

        vary_scene->setGravity(physx::PxVec3(g[0], g[1], g[2]));
    }

    vary_scene->simulate(physx::PxReal(1.0f / 90.0f));
    vary_scene->fetchResults();

    return b;
}

/*---------------------------------------------------------------------------*/

static void sol_release_sim_physx()
{
    log_printf("PhysX: Now quitting...\n");

    if (wborder_body)
    {
        vary_scene->removeActor(*wborder_body);
        wborder_body->detachShape(*wborder_shape);
        wborder_body->release();
        wborder_body = 0;
    }

    if (wborder_shape)
    {
        wborder_shape->release();
        wborder_shape = 0;
    }

    if (vary_sqmetal_mtrl)
    {
        vary_sqmetal_mtrl->release();
        vary_sqmetal_mtrl = 0;
    }

    if (vary_sqwood_mtrl)
    {
        vary_sqwood_mtrl->release();
        vary_sqwood_mtrl = 0;
    }

    if (vary_player_mtrl)
    {
        vary_player_mtrl->release();
        vary_player_mtrl = 0;
    }

    if (vary_wborder_mtrl)
    {
        vary_wborder_mtrl->release();
        vary_wborder_mtrl = 0;
    }

    if (vary_scene)
    {
        vary_scene->release();
        vary_scene = 0;
    }

    if (vary_dispatcher)
    {
        vary_dispatcher->release();
        vary_dispatcher = 0;
    }

#ifdef _DEBUG
    if (vary_pvd)
    {
        if (vary_pvd->isConnected())
            vary_pvd->disconnect();

        vary_pvd->release();
        vary_pvd = 0;
    }
#endif

    if (vary_foundation)
    {
        vary_foundation->release();
        vary_foundation = 0;
    }

    log_printf("PhysX: Done freed resources!\n");
}

extern "C" void sol_init_sim_physx(struct s_vary *vary)
{
    log_printf("PhysX: Initializes now...\n");

    ms_init_physx(&vary->ms_accum);

    vary_foundation = PxCreateFoundation(PX_PHYSICS_VERSION,
                                         vary_alloc_callback,
                                         vary_error_callback);
    if (!vary_foundation)
    {
        log_errorf("PhysX: Could not create foundation!\n");

        return;
    }

#ifdef _DEBUG
    vary_pvd           = PxCreatePvd(*vary_foundation);
    vary_pvd_transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1",
                                                                  5425, 500);

    vary_pvd->connect(*vary_pvd_transport,
                      physx::PxPvdInstrumentationFlag::eALL);
#endif

    vary_tolerance_scale.length = 100;
    vary_tolerance_scale.speed = 981;
    vary_physics = PxCreatePhysics(PX_PHYSICS_VERSION,
                                   *vary_foundation,
                                   vary_tolerance_scale,
                                   true);
    if (!vary_physics)
    {
        log_errorf("PhysX: Could not create physics!\n");

        sol_release_sim_physx();
        return;
    }

    vary_dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
    if (!vary_dispatcher)
    {
        log_errorf("PhysX: Could not create CPU dispatcher!\n");

        sol_release_sim_physx();
        return;
    }

    physx::PxSceneDesc sceneDesc(vary_physics->getTolerancesScale());
    sceneDesc.gravity       = physx::PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = vary_dispatcher;
    sceneDesc.filterShader  = physx::PxDefaultSimulationFilterShader;

    vary_scene = vary_physics->createScene(sceneDesc);
    if (!vary_scene)
    {
        log_errorf("PhysX: Could not create scene!\n");

        sol_release_sim_physx();
        return;
    }

#ifdef _DEBUG
    vary_pvd_client = vary_scene->getScenePvdClient();
    if (vary_pvd_client)
    {
        vary_pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS,
                                         true);
        vary_pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS,
                                         true);
        vary_pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES,
                                         true);
    }
#endif

    physx::PxTransform default_tm(physx::PxVec3(0));

    // Mtrl setups
    vary_wborder_mtrl = vary_physics->createMaterial(0.0f, 0.0f, 0.0f);
    vary_player_mtrl  = vary_physics->createMaterial(0.5f, 0.5f, 0.85f);
    vary_sqwood_mtrl  = vary_physics->createMaterial(0.4f, 0.3f, 0.6f);
    vary_sqmetal_mtrl = vary_physics->createMaterial(0.4f, 0.7f, 0.6f);

    // Finish these setups (world border)
    wborder_shape = vary_physics->createShape(physx::PxBoxGeometry(1023,
                                                                   1,
                                                                   1023),
                                              *vary_wborder_mtrl);
    physx::PxTransform wborder_tm(physx::PxVec3(0, -1023, 0));
    wborder_body = vary_physics->createRigidStatic(default_tm.transform(wborder_tm));
    wborder_body->attachShape(*wborder_shape);
    vary_scene->addActor(*wborder_body);

    log_printf("PhysX: All done!\n");

    vary->sim_uses_px = 1;
}

extern "C" void sol_quit_sim_physx(struct s_vary *vary)
{
    if (vary->sim_uses_px)
        sol_release_sim_physx();

    vary->sim_uses_px = 0;
}

#endif

/*---------------------------------------------------------------------------*/
