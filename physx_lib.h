#pragma once

#include <stdint.h>

// Same layout as PxVec3 and linalg.Vector3f32
struct Vector3f32 { float data[3]; };

// Same layout as PxQuat and linalg.Quaternionf32
struct Quaternionf32 { float data[4]; };

// Same layout as PxTransform
struct Transform {
	Quaternionf32 q;
	Vector3f32 p;
};

typedef void* Scene_Handle;
typedef void* Actor_Handle;
typedef void* Triangle_Mesh_Handle;
typedef void* Convex_Mesh_Handle;
typedef void* Shape_Handle;

struct Buffer {
	void* data;
	size_t size;
};

struct Mesh_Description {
	Vector3f32* vertices;
	uint32_t num_vertices;
	uint32_t vertex_stride;
	uint32_t* indices;
	uint32_t num_triangles;
	uint32_t triangle_stride;
};

extern "C" {
	void init(bool initialize_cooking, bool initialize_pvd);
	void destroy();

	Scene_Handle scene_create();
	void scene_release(Scene_Handle scene_handle);
	void scene_simulate(Scene_Handle scene_handle, float dt);
	void scene_set_gravity(Scene_Handle scene_handle, Vector3f32 gravity);
	void scene_add_actor(Scene_Handle scene_handle, Actor_Handle actor_handle);
	void scene_remove_actor(Scene_Handle scene_handle, Actor_Handle actor_handle);
	Actor_Handle* scene_get_active_actors(Scene_Handle scene_handle, uint32_t* numActorsOut);

	Actor_Handle actor_create();
	void actor_release(Actor_Handle actor_handle);
	void* actor_get_user_data(Actor_Handle actor_handle);
	void actor_set_user_data(Actor_Handle actor_handle, void* user_data);
	void actor_set_kinematic(Actor_Handle actor_handle, bool kinematic);
	Transform actor_get_transform(Actor_Handle actor_handle);
	void actor_set_transform(Actor_Handle actor_handle, Transform transform, bool teleport = false);
	Vector3f32 actor_get_velocity(Actor_Handle actor_handle);
	void actor_set_velocity(Actor_Handle actor_handle, Vector3f32 velocity);

	void actor_add_shape_triangle_mesh(Actor_Handle actor_handle, Triangle_Mesh_Handle triangle_mesh_handle);
	void actor_add_shape_convex_mesh(Actor_Handle actor_handle, Convex_Mesh_Handle convex_mesh_handle);

	// TODO: Provide allocator proc
	Buffer cook_triangle_mesh_precise(Mesh_Description mesh_description);
	// TODO: Shouldn't reuse Mesh_Description here. Convex has polygons
	Buffer cook_convex_mesh(Mesh_Description mesh_description);
	void buffer_free(Buffer buffer);

	Triangle_Mesh_Handle triangle_mesh_create(Buffer buffer);
	void triangle_mesh_release(Triangle_Mesh_Handle triangle_mesh_handle);

	Convex_Mesh_Handle convex_mesh_create(Buffer buffer);
	void convex_mesh_release(Convex_Mesh_Handle convex_mesh_handle);
}


