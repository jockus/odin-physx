#pragma once

#include <stdbool.h>
#include <stdint.h>

// Same layout as PxVec3 and linalg.Vector3f32
typedef struct Vector3f32 {
	float x;
	float y;
	float z;
} Vector3f32;

// Same layout as PxQuat and linalg.Quaternionf32
typedef struct Quaternionf32 {
	float x;
	float y;
	float z;
	float w;
} Quaternionf32;

// Same layout as PxTransform
typedef struct Transform {
	Quaternionf32 q;
	Vector3f32 p;
} Transform;

typedef void* Scene;
typedef void* Material;
typedef void* Actor;
typedef void* Triangle_Mesh;
typedef void* Convex_Mesh;
typedef void* Shape;
typedef void* Controller;

typedef struct Allocator {
	void* (*allocate_16_byte_aligned)(struct Allocator* allocator, size_t size);
	void (*deallocate)(struct Allocator* allocator, void* ptr);
	void* user_data;
} Allocator;

typedef struct Buffer {
	void* data;
	size_t size;
} Buffer;

typedef struct Mesh_Description {
	Vector3f32* vertices;
	uint32_t num_vertices;
	uint32_t vertex_stride;
	uint32_t* indices;
	uint32_t num_triangles;
	uint32_t triangle_stride;
} Mesh_Description;

typedef struct Contact {
	Actor actor0;
	Actor actor1;
	Vector3f32 pos;
	Vector3f32 normal;
	Vector3f32 impulse;
} Contact;

typedef enum Trigger_State {
	eNOTIFY_TOUCH_FOUND,
	eNOTIFY_TOUCH_LOST
} Trigger_State;

typedef struct Trigger {
	Actor trigger;
	Actor other_actor;
	Trigger_State state;
} Trigger;

typedef struct Query_Hit {
	bool valid;
	Vector3f32 pos;
	Vector3f32 normal;
} Query_Hit;

typedef struct Controller_Settings {
	float slope_limit_deg;
	float height;
	float radius;
	Vector3f32 up;
	uint32_t shape_layer_index;
	uint32_t mask_index;
	Material material;
} Controller_Settings;

#ifdef __cplusplus
extern "C" {
#endif
	void init(Allocator allocator, bool initialize_cooking, bool initialize_pvd);
	void destroy();

	Scene scene_create();
	void scene_release(Scene scene);
	void scene_simulate(Scene scene, float dt, void* scratch_memory_16_byte_aligned, size_t scratch_size);
	void scene_set_gravity(Scene scene, Vector3f32 gravity);
	void scene_add_actor(Scene scene, Actor actor);
	void scene_remove_actor(Scene scene, Actor actor);
	Actor* scene_get_active_actors(Scene scene, uint32_t* num_actors);
	Contact* scene_get_contacts(Scene scene, uint32_t* num_contacts);
	Trigger* scene_get_triggers(Scene scene, uint32_t* num_contacts);
	void scene_set_collision_mask(Scene scene, uint32_t mask_index, uint64_t layer_mask);
	Query_Hit scene_raycast(Scene scene, Vector3f32 origin, Vector3f32 direction, float distance, uint32_t mask_index);

	Material material_create(float static_friction, float dynamic_friction, float restitution);
	void material_release(Material material);

	Actor actor_create();
	void actor_release(Actor actor);
	void* actor_get_user_data(Actor actor);
	void actor_set_user_data(Actor actor, void* user_data);
	void actor_set_kinematic(Actor actor, bool kinematic);
	Transform actor_get_transform(Actor actor);
	void actor_set_transform(Actor actor, Transform transform, bool teleport);
	Vector3f32 actor_get_velocity(Actor actor);
	void actor_set_velocity(Actor actor, Vector3f32 velocity);

	void actor_add_shape_box(Actor actor, Vector3f32 half_extents, Material material, uint32_t shape_layer_index, uint32_t mask_index, bool trigger);
	void actor_add_shape_sphere(Actor actor, float radius, Material material, uint32_t shape_layer_index, uint32_t mask_index, bool trigger);
	void actor_add_shape_triangle_mesh(Actor actor, Triangle_Mesh triangle_mesh, Material material, uint32_t shape_layer_index, uint32_t mask_index);
	void actor_add_shape_convex_mesh(Actor actor, Convex_Mesh convex_mesh, Material material, uint32_t shape_layer_index, uint32_t mask_index);

	Buffer cook_triangle_mesh(Mesh_Description mesh_description);
	Buffer cook_convex_mesh(Mesh_Description mesh_description);
	void buffer_free(Buffer buffer);

	Triangle_Mesh triangle_mesh_create(Buffer buffer);
	void triangle_mesh_release(Triangle_Mesh triangle_mesh);
	Convex_Mesh convex_mesh_create(Buffer buffer);
	void convex_mesh_release(Convex_Mesh convex_mesh);

	Controller controller_create(Scene scene, Controller_Settings settings);
	void controller_release(Controller controller);
	Vector3f32 controller_get_position(Controller controller);
	void controller_set_position(Controller controller, Vector3f32 position);
	void controller_move(Controller controller, Vector3f32 displacement, float dt, uint32_t mask_index);

#ifdef __cplusplus
}
#endif
