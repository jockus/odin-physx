#include "physx_lib.h"
#include <ctype.h>
#include "PxPhysicsAPI.h"

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.
#define PX_RELEASE(x)	if(x)	{ x->release(); x = NULL;	}

using namespace physx;

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = NULL;
PxPhysics* gPhysics	= NULL;
PxDefaultCpuDispatcher* gDispatcher = NULL;
PxMaterial* gMaterial = NULL;
PxPvd* gPvd = NULL;

PxRigidDynamic* createDynamic(PxScene* scene, const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity=PxVec3(0))
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	scene->addActor(*dynamic);
	return dynamic;
}

void init(bool initialize_pvd) {
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	if(initialize_pvd) {
		gPvd = PxCreatePvd(*gFoundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		gPvd->connect(*transport,PxPvdInstrumentationFlag::eALL);
	}

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);

	// Create some test shapes
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
}

void destroy() {
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	if(gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();
		gPvd = NULL;
		PX_RELEASE(transport);
	}
	PX_RELEASE(gFoundation);
}

Scene_Handle scene_create() {
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader	= PxDefaultSimulationFilterShader;
	
	// Enable contacts between kinematic/kinematic/static actors
	sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;

	PxScene* scene = gPhysics->createScene(sceneDesc);
	
	PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	return (Scene_Handle) scene;
}

void scene_release(Scene_Handle scene_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PX_RELEASE(scene);
}

void scene_simulate(Scene_Handle scene_handle, float dt) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->simulate(dt);
	scene->fetchResults(true);
}

void scene_set_gravity(Scene_Handle scene_handle, Vector3f32 gravity) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->setGravity(*(PxVec3*) &gravity);
}

void scene_add_actor(Scene_Handle scene_handle, Actor_Handle actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->addActor(*actor);
}

void scene_remove_actor(Scene_Handle scene_handle, Actor_Handle actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->removeActor(*actor);
}

Actor_Handle** scene_get_active_actors(Scene_Handle scene_handle, uint32_t* numActorsOut) {
	PxScene* scene = (PxScene*) scene_handle;
	return (Actor_Handle**) scene->getActiveActors(*numActorsOut);
}

Actor_Handle actor_create(bool kinematic) {
	PxRigidDynamic* actor = gPhysics->createRigidDynamic(PxTransform());
	if(kinematic) {
		actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
	}

	// TODO: Test geometry, remove once geometry add is in
	PxSphereGeometry geometry(1);
	PxShape* shape = gPhysics->createShape(geometry, *gMaterial, true);
	actor->attachShape(*shape);
	shape->release();

	return (Actor_Handle) actor;
}

void actor_release(Actor_Handle actor_handle) {
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	actor->release();
}

void* actor_get_user_data(Actor_Handle actor_handle) {
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	return actor->userData;
}

void actor_set_user_data(Actor_Handle actor_handle, void* user_data) {
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	actor->userData = user_data;
}

Vector3f32 actor_get_position(Actor_Handle actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return *(Vector3f32*) &actor->getGlobalPose().p;
}

void actor_set_position(Actor_Handle actor_handle, Vector3f32 position) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setGlobalPose(PxTransform(*(PxVec3*) &position));
}

Vector3f32 actor_get_velocity(Actor_Handle actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return *(Vector3f32*) &actor->getLinearVelocity();
}

void actor_set_velocity(Actor_Handle actor_handle, Vector3f32 velocity) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setLinearVelocity(*(PxVec3*) &velocity);
}
