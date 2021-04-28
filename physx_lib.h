#pragma once

#include <stdint.h>

// Same layout as PxVec3 and linalg.Vector3f32
struct Vector3f32 {
	float x;	
	float y;	
	float z;	
};

typedef void* Scene_Handle;
typedef void* Actor_Handle;

extern "C" {

	void init(bool initialize_pvd);
	void destroy();

	// Scene
	Scene_Handle scene_create();
	void scene_release(Scene_Handle scene_handle);
	void scene_simulate(Scene_Handle scene_handle, float dt);
	void scene_set_gravity(Scene_Handle scene_handle, Vector3f32 gravity);
	void scene_add_actor(Scene_Handle scene_handle, Actor_Handle actor_handle);
	void scene_remove_actor(Scene_Handle scene_handle, Actor_Handle actor_handle);
	Actor_Handle** scene_get_active_actors(Scene_Handle scene_handle, uint32_t* numActorsOut);

	// Shape

	// Actor
	Actor_Handle actor_create(void* userData);
	void actor_release(Actor_Handle actor_handle);

	void* actor_get_user_data(Actor_Handle actor_handle);
	void actor_set_user_data(Actor_Handle actor_handle, void* user_data);
	
	Vector3f32 actor_get_position(Actor_Handle actor_handle);
	void actor_set_position(Actor_Handle actor_handle, Vector3f32 position);
	Vector3f32 actor_get_velocity(Actor_Handle actor_handle);
	void actor_set_velocity(Actor_Handle actor_handle, Vector3f32 velocity);
}


