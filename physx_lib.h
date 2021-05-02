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

typedef void* Scene;
typedef void* Actor;
typedef void* Triangle_Mesh;
typedef void* Convex_Mesh;
typedef void* Shape;
typedef void* Controller_Manager;
typedef void* Controller;

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

	Scene scene_create();
	void scene_release(Scene scene);
	void scene_simulate(Scene scene, float dt);
	void scene_set_gravity(Scene scene, Vector3f32 gravity);
	void scene_add_actor(Scene scene, Actor actor);
	void scene_remove_actor(Scene scene, Actor actor);
	Actor* scene_get_active_actors(Scene scene, uint32_t* numActorsOut);

	Actor actor_create();
	void actor_release(Actor actor);
	void* actor_get_user_data(Actor actor);
	void actor_set_user_data(Actor actor, void* user_data);
	void actor_set_kinematic(Actor actor, bool kinematic);
	Transform actor_get_transform(Actor actor);
	void actor_set_transform(Actor actor, Transform transform, bool teleport = false);
	Vector3f32 actor_get_velocity(Actor actor);
	void actor_set_velocity(Actor actor, Vector3f32 velocity);

	void actor_add_shape_triangle_mesh(Actor actor, Triangle_Mesh triangle_mesh);
	void actor_add_shape_convex_mesh(Actor actor, Convex_Mesh convex_mesh);

	// TODO: Provide allocator proc
	Buffer cook_triangle_mesh(Mesh_Description mesh_description);
	// TODO: Shouldn't reuse Mesh_Description here. Convex has polygons
	Buffer cook_convex_mesh(Mesh_Description mesh_description);
	void buffer_free(Buffer buffer);
	Triangle_Mesh triangle_mesh_create(Buffer buffer);
	void triangle_mesh_release(Triangle_Mesh triangle_mesh);
	Convex_Mesh convex_mesh_create(Buffer buffer);
	void convex_mesh_release(Convex_Mesh convex_mesh);

	Controller_Manager controller_manager_create(Scene scene);
	void controller_manager_release(Controller_Manager controller_manager);
	Controller controller_create(Controller_Manager controller_manager);
	void controller_release(Controller controller);
	Vector3f32 controller_get_position(Controller controller);
	void controller_set_position(Controller controller, Vector3f32 position);
	void controller_move(Controller controller, Vector3f32 displacement, float minimum_distance, float dt);
}


