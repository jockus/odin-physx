#include "physx_lib.h"
#include "PxPhysicsAPI.h"
#include <stdio.h>

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

using namespace physx;

PxDefaultAllocator gAllocator;
PxDefaultErrorCallback gErrorCallback;
PxFoundation* gFoundation = nullptr;
PxPhysics* gPhysics	= nullptr;
PxDefaultCpuDispatcher* gDispatcher = nullptr;
PxMaterial* gMaterial = nullptr;
PxPvd* gPvd = nullptr;
PxCooking* gCooking	= nullptr;

uint32_t nextPowerOfTwo(uint32_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}

// TODO: Custom allocators
class DefaultMemoryOutputStream : public PxOutputStream
{
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
			mCapacity = PxMax(nextPowerOfTwo(expectedSize), 4096u);

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

class DefaultMemoryInputData : public PxInputData
{
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



void init(bool initialize_cooking, bool initialize_pvd) {
	// TODO: Replace with custom allocator procs
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

	// TODO: Material support
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
}

void destroy() {
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

Scene scene_create() {
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher	= gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	
	// Enable contacts between kinematic/kinematic/static actors
	sceneDesc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
	sceneDesc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;

	// Enable getActiveActors
	sceneDesc.flags.set(PxSceneFlag::eENABLE_ACTIVE_ACTORS);
	// sceneDesc.flags.set(PxSceneFlag::eENABLE_CCD);

	PxScene* scene = gPhysics->createScene(sceneDesc);
	
	PxPvdSceneClient* pvdClient = scene->getScenePvdClient();
	if(pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	return (Scene) scene;
}

void scene_release(Scene scene_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->release();
}

void scene_simulate(Scene scene_handle, float dt) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->simulate(dt);
	scene->fetchResults(true);
}

void scene_set_gravity(Scene scene_handle, Vector3f32 gravity) {
	PxScene* scene = (PxScene*) scene_handle;
	scene->setGravity(*(PxVec3*) &gravity);
}

void scene_add_actor(Scene scene_handle, Actor actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->addActor(*actor);
}

void scene_remove_actor(Scene scene_handle, Actor actor_handle) {
	PxScene* scene = (PxScene*) scene_handle;
	PxRigidActor* actor = (PxRigidActor*) actor_handle;
	scene->removeActor(*actor);
}

Actor* scene_get_active_actors(Scene scene_handle, uint32_t* numActorsOut) {
	PxScene* scene = (PxScene*) scene_handle;
	return (Actor*) scene->getActiveActors(*numActorsOut);
}

Actor actor_create() {
	return (Actor) gPhysics->createRigidDynamic(PxTransform(PxZero, PxIdentity));
}

void actor_release(Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->release();
}

void* actor_get_user_data(Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return actor->userData;
}

void actor_set_user_data(Actor actor_handle, void* user_data) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->userData = user_data;
}

void actor_set_kinematic(Actor actor_handle, bool kinematic) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

Transform actor_get_transform(Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return *(Transform*) &actor->getGlobalPose();
}

void actor_set_transform(Actor actor_handle, Transform transform, bool teleport) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	if(!teleport && (actor->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
	{
		actor->setKinematicTarget(*(PxTransform*) &transform);
	}
	else
	{
		actor->setGlobalPose(*(PxTransform*) &transform);
	}
}

Vector3f32 actor_get_velocity(Actor actor_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	return *(Vector3f32*) &actor->getLinearVelocity();
}

void actor_set_velocity(Actor actor_handle, Vector3f32 velocity) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	actor->setLinearVelocity(*(PxVec3*) &velocity);
}

void actor_add_shape_triangle_mesh(Actor actor_handle, Triangle_Mesh triangle_mesh_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxTriangleMeshGeometry geometry;
	geometry.triangleMesh = (PxTriangleMesh*) triangle_mesh_handle;
	PxShape* shape = gPhysics->createShape(geometry, *gMaterial, true);
	actor->attachShape(*shape);
	shape->release();
}

void actor_add_shape_convex_mesh(Actor actor_handle, Convex_Mesh convex_mesh_handle) {
	PxRigidDynamic* actor = (PxRigidDynamic*) actor_handle;
	PxConvexMeshGeometry geometry;
	geometry.convexMesh = (PxConvexMesh*) convex_mesh_handle;
	PxShape* shape = gPhysics->createShape(geometry, *gMaterial, true);
	actor->attachShape(*shape);
	shape->release();
	PxRigidBodyExt::updateMassAndInertia(*actor, 1);
}

Buffer cook_triangle_mesh(Mesh_Description mesh_description)
{
	bool inserted = false;
	bool skipMeshCleanup = false; // true for fast
	bool skipEdgeData = false; // true for fast
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

	// Create BVH34 midphase
	params.midphaseDesc = PxMeshMidPhase::eBVH34;

	// we suppress the triangle mesh remap table computation to gain some speed, as we will not need it 
	// in this snippet
	params.suppressTriangleMeshRemapTable = true;

	// If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking. The input mesh must be valid. 
	// The following conditions are true for a valid triangle mesh :
	//  1. There are no duplicate vertices(within specified vertexWeldTolerance.See PxCookingParams::meshWeldTolerance)
	//  2. There are no large triangles(within specified PxTolerancesScale.)
	// It is recommended to run a separate validation check in debug/checked builds, see below.

	if (!skipMeshCleanup)
		params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
	else
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

	// If DISABLE_ACTIVE_EDGES_PREDOCOMPUTE is set, the cooking does not compute the active (convex) edges, and instead 
	// marks all edges as active. This makes cooking faster but can slow down contact generation. This flag may change 
	// the collision behavior, as all edges of the triangle mesh will now be considered active.
	if (!skipEdgeData)
		params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
	else
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;

	// Cooking mesh with less triangles per leaf produces larger meshes with better runtime performance
	// and worse cooking performance. Cooking time is better when more triangles per leaf are used.
	params.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = numTrisPerLeaf;

	gCooking->setParams(params);

	if(!gCooking->validateTriangleMesh(meshDesc)) {
		// TODO: error result
	}

	DefaultMemoryOutputStream outBuffer;
	gCooking->cookTriangleMesh(meshDesc, outBuffer);

	Buffer result;
	result.data = outBuffer.getData();
	result.size = outBuffer.getSize();
	return result;
}

Buffer cook_convex_mesh(Mesh_Description mesh_description)
{
	PxConvexMeshDesc desc;
	desc.points.data = mesh_description.vertices;
	desc.points.count = mesh_description.num_vertices;
	desc.points.stride = mesh_description.vertex_stride;
	// TODO: Options to pass in complete convex mesh
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	DefaultMemoryOutputStream outBuffer;
	gCooking->cookConvexMesh(desc, outBuffer);

	Buffer result;
	result.data = outBuffer.getData();
	result.size = outBuffer.getSize();
	return result;
}

void buffer_free(Buffer buffer) {
	PxAllocatorCallback &allocator = PxGetFoundation().getAllocatorCallback();
	if(buffer.data) {
		allocator.deallocate(buffer.data);
	}
}

Triangle_Mesh triangle_mesh_create(Buffer buffer) {
	DefaultMemoryInputData stream((PxU8*) buffer.data, buffer.size);
	return (Triangle_Mesh) gPhysics->createTriangleMesh(stream);
}
void triangle_mesh_release(Triangle_Mesh triangle_mesh_handle) {
	PxTriangleMesh* triangle_mesh = (PxTriangleMesh*) triangle_mesh_handle;
	triangle_mesh->release();
}

Convex_Mesh convex_mesh_create(Buffer buffer) {
	DefaultMemoryInputData stream((PxU8*) buffer.data, buffer.size);
	return (Convex_Mesh) gPhysics->createConvexMesh(stream);
}

void convex_mesh_release(Convex_Mesh convex_mesh_handle) {
	PxConvexMesh* convex_mesh = (PxConvexMesh*) convex_mesh_handle;
	convex_mesh->release();
}

Controller_Manager controller_manager_create(Scene scene_handle) {
	return (Controller_Manager) PxCreateControllerManager(*((PxScene*) scene_handle));
}

void controller_manager_release(Controller_Manager controller_manager_handle) {
	PxControllerManager* controller_manager = (PxControllerManager*) controller_manager_handle;
	controller_manager->release();
}

Controller controller_create(Controller_Manager controller_manager_handle) {
	// TODO: Config
	#define CONTACT_OFFSET			0.01f
	#define STEP_OFFSET				0.05f
	#define SLOPE_LIMIT				0.0f
	#define INVISIBLE_WALLS_HEIGHT	0.0f
	#define MAX_JUMP_HEIGHT			0.0f
	static const float gScaleFactor			= 1.5f;
	static const float gStandingSize		= 1.0f * gScaleFactor;
	static const float gCrouchingSize		= 0.25f * gScaleFactor;
	static const float gControllerRadius	= 0.3f * gScaleFactor;

	PxControllerManager* controller_manager = (PxControllerManager*) controller_manager_handle;
	PxCapsuleControllerDesc desc;
	desc.position				= PxExtendedVec3(0,0,0);
	desc.slopeLimit			= SLOPE_LIMIT;
	desc.contactOffset			= CONTACT_OFFSET;
	desc.stepOffset			= STEP_OFFSET;
	desc.invisibleWallHeight	= INVISIBLE_WALLS_HEIGHT;
	desc.maxJumpHeight			= MAX_JUMP_HEIGHT;
	desc.radius				= gControllerRadius;
	desc.height				= gStandingSize;
	desc.material = gMaterial;
	desc.upDirection = PxVec3(0,1,0);
	// desc.crouchHeight			= gCrouchingSize;
	// desc.reportCallback		= this;
	// desc.behaviorCallback		= this;
	PxController* controller = controller_manager->createController(desc); 
	return (Controller) controller;
}

void controller_release(Controller controller_handle) {
	PxController* controller = (PxController*) controller_handle;
	controller->release();
}

Vector3f32 controller_get_position(Controller controller_handle) {
	PxController* controller = (PxController*) controller_handle;
	Vector3f32 result;
	result.data[0] = controller->getPosition().x;
	result.data[1] = controller->getPosition().y;
	result.data[2] = controller->getPosition().z;
	return result;
}

void controller_set_position(Controller controller_handle, Vector3f32 position) {
	PxController* controller = (PxController*) controller_handle;
}

void controller_move(Controller controller_handle, Vector3f32 displacement, float minimum_distance, float dt) {
	PxController* controller = (PxController*) controller_handle;
	PxControllerFilters filters;
	PxVec3 disp = *(PxVec3*) &displacement;
	controller->move(disp, minimum_distance, dt, filters);
}
