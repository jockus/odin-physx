#include "physx_lib.h"
#include "PxPhysicsAPI.h"
#include "foundation/PxMath.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <assert.h>

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

using namespace physx;

PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics	= nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxPvd* gPvd = nullptr;
PxCooking* gCooking	= nullptr;

Px_Vector3f32 to_vec(PxVec3 v) {
	return *(Px_Vector3f32*) &v;
}

PxVec3 to_px(Px_Vector3f32 v) {
	return *(PxVec3*) &v;
}

class Allocator_Callback : public PxAllocatorCallback
{
public:
	Allocator_Callback() {}

	Allocator_Callback(Px_Allocator allocator)
		: allocator(allocator) {
	}
	virtual ~Allocator_Callback()
	{
	}

	virtual void* allocate(size_t size, const char* type_name, const char* filename, int line) override {
		return allocator.allocate_16_byte_aligned(&allocator, size, filename, line);
	}

	virtual void deallocate(void* ptr) {
		allocator.deallocate(&allocator, ptr);
	}

	Px_Allocator allocator;
};
Allocator_Callback gAllocator;


uint32_t next_power_of_two(uint32_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

class DefaultMemoryOutputStream : public PxOutputStream {
public:
	DefaultMemoryOutputStream(PxAllocatorCallback &allocator = PxGetFoundation().getAllocatorCallback()) 
		: mAllocator(allocator)
		, mData(0)
		, mSize(0)
		, mCapacity(0) {
	}

	virtual ~DefaultMemoryOutputStream() {
		// Note, does not free
	}

	virtual PxU32 write(const void* src, PxU32 size) {
		PxU32 expectedSize = mSize + size;
		if(expectedSize > mCapacity)
		{
			mCapacity = PxMax(next_power_of_two(expectedSize), 4096u);

			PxU8* newData = reinterpret_cast<PxU8*>(mAllocator.allocate(mCapacity,"PxDefaultMemoryOutputStream",__FILE__,__LINE__));
			PX_ASSERT(newData!=NULL);

			PxMemCopy(newData, mData, mSize);
			if(mData)
				mAllocator.deallocate(mData);

			mData = newData;
		}
		PxMemCopy(mData+mSize, src, size);
		mSize += size;
		return size;
	}

	virtual	PxU32 getSize()	const {
		return mSize;
	}
	virtual	PxU8* getData()	const {
		return mData;
	}

private:
	DefaultMemoryOutputStream(const DefaultMemoryOutputStream&);
	DefaultMemoryOutputStream& operator=(const DefaultMemoryOutputStream&);

	PxAllocatorCallback& mAllocator;
	PxU8* mData;
	PxU32 mSize;
	PxU32 mCapacity;
};

class DefaultMemoryInputData : public PxInputData {
public:
	DefaultMemoryInputData(PxU8* data, PxU32 length) 
		: mSize(length)
		  , mData(data)
		  , mPos(0) {
			  mData = data;
			  mSize = length;
		  }


	virtual PxU32 read(void* dest, PxU32 count) {
		PxU32 length = PxMin<PxU32>(count, mSize-mPos);
		PxMemCopy(dest, mData+mPos, length);
		mPos += length;
		return length;
	}

	virtual PxU32 getLength() const {
		return mSize;
	}

	virtual void seek(PxU32 offset) {
		mPos = PxMin<PxU32>(mSize, offset);
	}

	virtual PxU32 tell() const {
		return mPos;
	}

private:
	PxU32		mSize;
	const PxU8*	mData;
	PxU32		mPos;
};

#define NUM_GROUPS 64
typedef uint64_t Collision_Masks[NUM_GROUPS];
Collision_Masks collision_masks;

PxFilterFlags CollisionFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// TODO: for per scene masks rather than global store in constantBlock
	// collision_masks = *(Collision_Masks const&) constantBlock;

	if(((1 << filterData0.word0) & collision_masks[filterData1.word1]) != 0 ||
		((1 << filterData1.word0) & collision_masks[filterData0.word1]) != 0) {

		if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		}
		else {
			pairFlags = PxPairFlag::eCONTACT_DEFAULT;
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
			pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}
	}
	else {
		return PxFilterFlag::eSUPPRESS;
	}

    return PxFilterFlag::eDEFAULT;
}

class SimulationEventCallback : public PxSimulationEventCallback {
public:

	// TODO: Init setting
	#define MAX_NOTIFY 256
	Px_Contact touch[MAX_NOTIFY];
	int numTouch = 0;

	Px_Trigger triggers[MAX_NOTIFY];
	int numTrigger = 0;

	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	void onWake(PxActor** actors, PxU32 count) {}
	void onSleep(PxActor** actors, PxU32 count) {}
	void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
	{	
		if(pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1) )
		{
			return;
		}
		if(numTouch >= MAX_NOTIFY)
		{
			printf("Too many contacts, discarding the rest\n");
			return;
		}

		PxVec3 pos(PxZero);
		PxVec3 normal(PxZero);
		PxVec3 impulse(PxZero);
		float separation = 0.0f;
		for(PxU32 i=0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];

			if(cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND || 
				cp.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
			{
				PxContactPairPoint ContactPointBuffer[16];
				int32_t NumContactPoints = cp.extractContacts(ContactPointBuffer, 16);
				// Use first contact point as collision posiion
				if(NumContactPoints > 0)
				{
					pos = ContactPointBuffer[0].position;
					normal = ContactPointBuffer[0].normal;
					separation = ContactPointBuffer[0].separation;
				}
				// Accumulate all contact impulses
				for(int32_t PointIdx=0; PointIdx < NumContactPoints; PointIdx++)
				{
					const PxContactPairPoint& Point = ContactPointBuffer[PointIdx];

					const PxVec3 NormalImpulse = Point.impulse.dot(Point.normal) * Point.normal; // project impulse along normal
					impulse += NormalImpulse;
				}	
			}
		}
		Px_Contact& contact = touch[numTouch++];
		contact.actor0 = pairHeader.actors[0];
		contact.actor1 = pairHeader.actors[1];
		contact.pos = to_vec(pos);
		contact.normal = to_vec(normal);
		contact.impulse = to_vec(impulse);
	}

	void onTrigger(PxTriggerPair* pairs, PxU32 count) override
	{
		for(PxU32 i=0; i < count; i++)
		{
			if(numTrigger >= MAX_NOTIFY)
			{
				printf("Too many trigger events, discarding the rest\n");
				return;
			}
			// ignore pairs when shapes have been deleted
			if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
				continue;

			Px_Trigger_State state = eNOTIFY_TOUCH_FOUND;
			if(pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				state = eNOTIFY_TOUCH_FOUND;
			}
			if(pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST) {
				state = eNOTIFY_TOUCH_LOST;
			}
			fflush(stdout);

			Px_Trigger& trigger = triggers[numTrigger++];
			trigger.trigger = pairs[i].triggerActor;
			trigger.other_actor = pairs[i].otherActor;
			trigger.state = state;
		}
	}
};

void px_init(Px_Allocator allocator, bool initialize_cooking, bool initialize_pvd) {
	gAllocator = Allocator_Callback(allocator);
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);

	if(initialize_pvd) {
		gPvd = PxCreatePvd(*gFoundation);
		PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
		gPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);
	}

	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), false, gPvd);
	PxInitExtensions(*gPhysics, gPvd);

	if(initialize_cooking) {
		gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, PxCookingParams(PxTolerancesScale()));
	}

	gDispatcher = PxDefaultCpuDispatcherCreate(4);

	for(int i = 0; i < NUM_GROUPS; ++i) {
		// Collide against all groups by Default
		collision_masks[i] = UINT64_MAX;
	}
}

void px_destroy() {
	PxCloseExtensions();
	gDispatcher->release();
	if(gCooking) {
		gCooking->release();
	}
	gPhysics->release();
	if(gPvd)
	{
		PxPvdTransport* transport = gPvd->getTransport();
		gPvd->release();
		gPvd = nullptr;
		if(transport) {
			transport->release();
		}
	}
	gFoundation->release();
}

Px_Scene px_scene_create() {
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader = CollisionFilterShader;
	sceneDesc.simulationEventCallback = new SimulationEventCallback();
	
	// Enable contacts between kinematic/kinematic/static actors
	sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;

	// Enable getActiveActors
	sceneDesc.flags.set(PxSceneFlag::eENABLE_ACTIVE_ACTORS);

	PxScene* scene = gPhysics->createScene(sceneDesc);
	scene->userData = PxCreateControllerManager(*scene);
	
	PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	return (Px_Scene) scene;
}

void px_scene_release(Px_Scene scene_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	((PxControllerManager*) scene->userData)->release();
	if(scene->getSimulationEventCallback()) {
		delete scene->getSimulationEventCallback();
		scene->setSimulationEventCallback(nullptr);
	}
	scene->release();
}

void px_scene_simulate(Px_Scene scene_handle, float dt, void* scratch_memory_16_byte_aligned, size_t scratch_size) {
	PxScene* scene = (PxScene*) scene_handle;

	SimulationEventCallback* callback = (SimulationEventCallback*) scene->getSimulationEventCallback();
	callback->numTouch = 0;
	callback->numTrigger = 0;

	scene->simulate(dt, nullptr, scratch_memory_16_byte_aligned, scratch_size);
	scene->fetchResults(true);
}

void px_scene_set_gravity(Px_Scene scene_handle, Px_Vector3f32 gravity) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->setGravity(*(PxVec3*) &gravity);
}

void px_scene_add_actor(Px_Scene scene_handle, Px_Actor actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->addActor(*actor);
}

void px_scene_remove_actor(Px_Scene scene_handle, Px_Actor actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->removeActor(*actor);
}

Px_Actor* px_scene_get_active_actors(Px_Scene scene_handle, uint32_t* num_actors) {
	PxScene* scene = (PxScene*) scene_handle;
	return (Px_Actor*) scene->getActiveActors(*num_actors);
}

Px_Contact* px_scene_get_contacts(Px_Scene scene_handle, uint32_t* num_contacts) {
	PxScene* scene = (PxScene*) scene_handle;
	SimulationEventCallback* callback = (SimulationEventCallback*) scene->getSimulationEventCallback();
	*num_contacts = (uint32_t) callback->numTouch;
	return callback->touch;
}

Px_Trigger* px_scene_get_triggers(Px_Scene scene_handle, uint32_t* num_triggers) {
	PxScene* scene = (PxScene*) scene_handle;
	SimulationEventCallback* callback = (SimulationEventCallback*) scene->getSimulationEventCallback();
	*num_triggers = (uint32_t) callback->numTrigger;
	return callback->triggers;
}

void px_scene_set_collision_mask(Px_Scene scene_handle, uint32_t mask_index, uint64_t mask) {
	if(mask_index >= 0 && mask_index < NUM_GROUPS) {
		collision_masks[mask_index] = mask;
	}
}

Px_Query_Hit px_scene_raycast(Px_Scene scene_handle, Px_Vector3f32 origin, Px_Vector3f32 direction, float distance, uint32_t mask_index) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRaycastBuffer raycast_buffer;
	PxQueryFilterData query_filter_data;
	query_filter_data.data.word0 = collision_masks[mask_index];
	scene->raycast(to_px(origin), to_px(direction), distance, raycast_buffer, PxHitFlags(PxHitFlag::eDEFAULT), query_filter_data);
	Px_Query_Hit result;
	result.valid = raycast_buffer.hasBlock;
	result.pos = result.valid ? to_vec(raycast_buffer.block.position) : Px_Vector3f32{0,0,0};
	result.normal = result.valid ? to_vec(raycast_buffer.block.normal) : Px_Vector3f32{0,0,0};
	return result;
}

Px_Material px_material_create(float static_friction, float dynamic_friction, float restitution) {
	return (Px_Material) gPhysics->createMaterial(static_friction, dynamic_friction, restitution);
}

void px_material_release(Px_Material material_handle) {
	PxMaterial* material = (PxMaterial*) material_handle;
	material->release();
}

Px_Actor px_actor_create() {
	return (Px_Actor) gPhysics->createRigidDynamic(PxTransform(PxZero, PxIdentity));
}

void px_actor_release(Px_Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->release();
}

void* px_actor_get_user_data(Px_Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return actor->userData;
}

void px_actor_set_user_data(Px_Actor actor_handle, void* user_data) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->userData = user_data;
}

void px_actor_set_kinematic(Px_Actor actor_handle, bool kinematic) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

Px_Transform px_actor_get_transform(Px_Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return *(Px_Transform*) &actor->getGlobalPose();
}

void px_actor_set_transform(Px_Actor actor_handle, Px_Transform transform, bool teleport) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	if(!teleport && actor->getScene() && (actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
	{
		actor->setKinematicTarget(*(PxTransform*) &transform);
	}
	else
	{
		actor->setGlobalPose(*(PxTransform*) &transform);
	}
}

Px_Vector3f32 px_actor_get_velocity(Px_Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return to_vec(actor->getLinearVelocity());
}

void px_actor_set_velocity(Px_Actor actor_handle, Px_Vector3f32 velocity) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setLinearVelocity(to_px(velocity));
}

void px_actor_add_shape_box(Px_Actor actor_handle, Px_Vector3f32 half_extents, Px_Material material_handle, uint32_t shape_layer_index, uint32_t mask_index, bool trigger) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxMaterial* material = (PxMaterial*) material_handle;
	PxBoxGeometry geometry;
	geometry.halfExtents = to_px(half_extents);
	PxShape* shape = gPhysics->createShape(geometry, *material, false);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !trigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, trigger);
	shape->setSimulationFilterData(PxFilterData(shape_layer_index, mask_index, 0, 0));
	shape->setQueryFilterData(PxFilterData(1 << shape_layer_index, 0, 0, 0));
	actor->attachShape(*shape);
	shape->release();
	PxRigidBodyExt::updateMassAndInertia(*actor, 1);
}

void px_actor_add_shape_sphere(Px_Actor actor_handle, float radius, Px_Material material_handle, uint32_t shape_layer_index, uint32_t mask_index, bool trigger) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxMaterial* material = (PxMaterial*) material_handle;
	PxSphereGeometry geometry;
	geometry.radius = radius;
	PxShape* shape = gPhysics->createShape(geometry, *material, false);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !trigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, trigger);
	shape->setSimulationFilterData(PxFilterData(shape_layer_index, mask_index, 0, 0));
	shape->setQueryFilterData(PxFilterData(1 << shape_layer_index, 0, 0, 0));
	actor->attachShape(*shape);
	shape->release();
	PxRigidBodyExt::updateMassAndInertia(*actor, 1);
}

void px_actor_add_shape_triangle_mesh(Px_Actor actor_handle, Px_Triangle_Mesh triangle_mesh_handle, Px_Material material_handle, uint32_t shape_layer_index, uint32_t mask_index) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxMaterial* material = (PxMaterial*) material_handle;
	PxTriangleMeshGeometry geometry;
	geometry.triangleMesh = (PxTriangleMesh*) triangle_mesh_handle;
	PxShape* shape = gPhysics->createShape(geometry, *material, false);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
	shape->setSimulationFilterData(PxFilterData(shape_layer_index, mask_index, 0, 0));
	shape->setQueryFilterData(PxFilterData(1 << shape_layer_index, 0, 0, 0));
	actor->attachShape(*shape);
	shape->release();
	// Note - no mass/inertia update. Trimeshes can't be used for simulation
}

void px_actor_add_shape_convex_mesh(Px_Actor actor_handle, Px_Convex_Mesh convex_mesh_handle, Px_Material material_handle, uint32_t shape_layer_index, uint32_t mask_index) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxMaterial* material = (PxMaterial*) material_handle;
	PxConvexMeshGeometry geometry;
	geometry.convexMesh = (PxConvexMesh*) convex_mesh_handle;
	PxShape* shape = gPhysics->createShape(geometry, *material, false);
	shape->setSimulationFilterData(PxFilterData(shape_layer_index, mask_index, 0, 0));
	shape->setQueryFilterData(PxFilterData(1 << shape_layer_index, 0, 0, 0));
	actor->attachShape(*shape);
	shape->release();
	PxRigidBodyExt::updateMassAndInertia(*actor, 1);
}

Px_Buffer px_cook_triangle_mesh(Px_Mesh_Description mesh_description)
{
	uint32_t numTrisPerLeaf = 4; // 15 for fast

	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = mesh_description.num_vertices;
	meshDesc.points.data = mesh_description.vertices;
	meshDesc.points.stride = mesh_description.vertex_stride;
	meshDesc.triangles.count = mesh_description.num_triangles;
	meshDesc.triangles.data = mesh_description.indices;
	meshDesc.triangles.stride = mesh_description.triangle_stride;
	
	// TODO: option
	// meshDesc.flags.set(PxMeshFlag::eFLIPNORMALS);

	PxCookingParams params = gCooking->getParams();
	params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = numTrisPerLeaf;

	gCooking->setParams(params);

	DefaultMemoryOutputStream outBuffer;
	gCooking->cookTriangleMesh(meshDesc, outBuffer);

	Px_Buffer result;
	result.data = outBuffer.getData();
	result.size = outBuffer.getSize();
	return result;
}

Px_Buffer px_cook_convex_mesh(Px_Mesh_Description mesh_description)
{
	PxConvexMeshDesc desc;
	desc.points.data = mesh_description.vertices;
	desc.points.count = mesh_description.num_vertices;
	desc.points.stride = mesh_description.vertex_stride;
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	DefaultMemoryOutputStream outBuffer;
	gCooking->cookConvexMesh(desc, outBuffer);

	Px_Buffer result;
	result.data = outBuffer.getData();
	result.size = outBuffer.getSize();
	return result;
}

void px_buffer_free(Px_Buffer buffer) {
	PxAllocatorCallback &allocator = PxGetFoundation().getAllocatorCallback();
	if(buffer.data) {
		allocator.deallocate(buffer.data);
	}
}

Px_Triangle_Mesh px_triangle_mesh_create(Px_Buffer buffer) {
	DefaultMemoryInputData stream((PxU8*) buffer.data, buffer.size);
	return (Px_Triangle_Mesh) gPhysics->createTriangleMesh(stream);
}
void px_triangle_mesh_release(Px_Triangle_Mesh triangle_mesh_handle) {
	PxTriangleMesh* triangle_mesh = (PxTriangleMesh*) triangle_mesh_handle;
	triangle_mesh->release();
}

Px_Convex_Mesh px_convex_mesh_create(Px_Buffer buffer) {
	DefaultMemoryInputData stream((PxU8*) buffer.data, buffer.size);
	return (Px_Convex_Mesh) gPhysics->createConvexMesh(stream);
}

void px_convex_mesh_release(Px_Convex_Mesh convex_mesh_handle) {
	PxConvexMesh* convex_mesh = (PxConvexMesh*) convex_mesh_handle;
	convex_mesh->release();
}

Px_Controller px_controller_create(Px_Scene scene_handle, Px_Controller_Settings settings) {
	PxScene* scene = (PxScene*) scene_handle;
	PxControllerManager* controller_manager = (PxControllerManager*) scene->userData;
	PxCapsuleControllerDesc desc;
	desc.position = PxExtendedVec3(0,0,0);
	desc.slopeLimit = cosf(settings.slope_limit_deg * M_PI / 180.0f);
	desc.contactOffset = 0.01f;
	desc.stepOffset = 0.05f;
	desc.invisibleWallHeight = 0.0f;
	desc.maxJumpHeight = 0.0f;
	desc.radius = settings.radius;
	desc.height = settings.height;
	desc.material = (PxMaterial*) settings.material;
	desc.upDirection = to_px(settings.up);
	PxController* controller = controller_manager->createController(desc); 
	PxShape* shape = nullptr;
	assert(controller->getActor()->getNbShapes() == 1);
	controller->getActor()->getShapes(&shape, 1);
	shape->setSimulationFilterData(PxFilterData(settings.shape_layer_index, settings.mask_index, 0, 13));
	shape->setQueryFilterData(PxFilterData(1 << settings.shape_layer_index, 0, 0, 0));
	return (Px_Controller) controller;
}

void px_controller_release(Px_Controller controller_handle) {
	PxController* controller = (PxController*) controller_handle;
	controller->release();
}

Px_Vector3f32 px_controller_get_position(Px_Controller controller_handle) {
	PxController* controller = (PxController*) controller_handle;
	Px_Vector3f32 result;
	result.x = controller->getPosition().x;
	result.y = controller->getPosition().y;
	result.z = controller->getPosition().z;
	return result;
}

void px_controller_set_position(Px_Controller controller_handle, Px_Vector3f32 position) {
	PxController* controller = (PxController*) controller_handle;
}

void px_controller_move(Px_Controller controller_handle, Px_Vector3f32 displacement, float dt, uint32_t mask_index) {
	PxController* controller = (PxController*) controller_handle;
	PxFilterData filter_data(collision_masks[mask_index], 0, 0, 0);
	PxControllerFilters filters;
	filters.mFilterData = &filter_data;
	PxVec3 disp = to_px(displacement);
	controller->move(disp, 0.001f, dt, filters);
}
