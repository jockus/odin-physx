#pragma once

#include <stdbool.h>
#include <stdint.h>

// Same layout as PxVec3 and linalg.Vector3f32
typedef struct Px_Vector3f32 {
	float x;
	float y;
	float z;
} Px_Vector3f32;

// Same layout as PxQuat and linalg.Quaternionf32
typedef struct Px_Quaternionf32 {
	float x;
	float y;
	float z;
	float w;
} Px_Quaternionf32;

// Same layout as PxTransform
typedef struct Px_Transform {
	Px_Quaternionf32 q;
	Px_Vector3f32 p;
} Px_Transform;

typedef void* Px_Scene;
typedef void* Px_Material;
typedef void* Px_Actor;
typedef void* Px_Triangle_Mesh;
typedef void* Px_Convex_Mesh;
typedef void* Px_Controller;

typedef struct Px_Allocator {
	void* (*allocate_16_byte_aligned)(struct Px_Allocator* allocator, size_t size);
	void (*deallocate)(struct Px_Allocator* allocator, void* ptr);
	void* user_data;
} Px_Allocator;

typedef struct Px_Buffer {
	void* data;
	size_t size;
} Px_Buffer;

typedef struct Px_Mesh_Description {
	Px_Vector3f32* vertices;
	uint32_t num_vertices;
	uint32_t vertex_stride;
	uint32_t* indices;
	uint32_t num_triangles;
	uint32_t triangle_stride;
} Px_Mesh_Description;

typedef struct Px_Contact {
	Px_Actor actor0;
	Px_Actor actor1;
	Px_Vector3f32 pos;
	Px_Vector3f32 normal;
	Px_Vector3f32 impulse;
} Px_Contact;

typedef enum Px_Trigger_State {
	eNOTIFY_TOUCH_FOUND,
	eNOTIFY_TOUCH_LOST
} Px_Trigger_State;

typedef struct Px_Trigger {
	Px_Actor trigger;
	Px_Actor other_actor;
	Px_Trigger_State state;
} Px_Trigger;

typedef struct Px_Query_Hit {
	bool valid;
	Px_Vector3f32 pos;
	Px_Vector3f32 normal;
} Px_Query_Hit;

typedef struct Px_Controller_Settings {
	float slope_limit_deg;
	float height;
	float radius;
	Px_Vector3f32 up;
	uint32_t shape_layer_index;
	uint32_t mask_index;
	Px_Material material;
} Px_Controller_Settings;

#ifdef __cplusplus
extern "C" {
#endif
	void px_init(Px_Allocator allocator, bool initialize_cooking, bool initialize_pvd);
	void px_destroy();

	Px_Scene px_scene_create();
	void px_scene_release(Px_Scene scene);
	void px_scene_simulate(Px_Scene scene, float dt, void* scratch_memory_16_byte_aligned, size_t scratch_size);
	void px_scene_set_gravity(Px_Scene scene, Px_Vector3f32 gravity);
	void px_scene_add_actor(Px_Scene scene, Px_Actor actor);
	void px_scene_remove_actor(Px_Scene scene, Px_Actor actor);
	Px_Actor* px_scene_get_active_actors(Px_Scene scene, uint32_t* num_actors);
	Px_Contact* px_scene_get_contacts(Px_Scene scene, uint32_t* num_contacts);
	Px_Trigger* px_scene_get_triggers(Px_Scene scene, uint32_t* num_contacts);
	void px_scene_set_collision_mask(Px_Scene scene, uint32_t mask_index, uint64_t layer_mask);
	Px_Query_Hit px_scene_raycast(Px_Scene scene, Px_Vector3f32 origin, Px_Vector3f32 direction, float distance, uint32_t mask_index);

	Px_Material px_material_create(float static_friction, float dynamic_friction, float restitution);
	void px_material_release(Px_Material material);

	Px_Actor px_actor_create();
	void px_actor_release(Px_Actor actor);
	void* px_actor_get_user_data(Px_Actor actor);
	void px_actor_set_user_data(Px_Actor actor, void* user_data);
	void px_actor_set_kinematic(Px_Actor actor, bool kinematic);
	Px_Transform px_actor_get_transform(Px_Actor actor);
	void px_actor_set_transform(Px_Actor actor, Px_Transform transform, bool teleport);
	Px_Vector3f32 px_actor_get_velocity(Px_Actor actor);
	void px_actor_set_velocity(Px_Actor actor, Px_Vector3f32 velocity);

	void px_actor_add_shape_box(Px_Actor actor, Px_Vector3f32 half_extents, Px_Material material, uint32_t shape_layer_index, uint32_t mask_index, bool trigger);
	void px_actor_add_shape_sphere(Px_Actor actor, float radius, Px_Material material, uint32_t shape_layer_index, uint32_t mask_index, bool trigger);
	void px_actor_add_shape_triangle_mesh(Px_Actor actor, Px_Triangle_Mesh triangle_mesh, Px_Material material, uint32_t shape_layer_index, uint32_t mask_index);
	void px_actor_add_shape_convex_mesh(Px_Actor actor, Px_Convex_Mesh convex_mesh, Px_Material material, uint32_t shape_layer_index, uint32_t mask_index);

	Px_Buffer px_cook_triangle_mesh(Px_Mesh_Description mesh_description);
	Px_Buffer px_cook_convex_mesh(Px_Mesh_Description mesh_description);
	void px_buffer_free(Px_Buffer buffer);

	Px_Triangle_Mesh px_triangle_mesh_create(Px_Buffer buffer);
	void px_triangle_mesh_release(Px_Triangle_Mesh triangle_mesh);
	Px_Convex_Mesh px_convex_mesh_create(Px_Buffer buffer);
	void px_convex_mesh_release(Px_Convex_Mesh convex_mesh);

	Px_Controller px_controller_create(Px_Scene scene, Px_Controller_Settings settings);
	void px_controller_release(Px_Controller controller);
	Px_Vector3f32 px_controller_get_position(Px_Controller controller);
	void px_controller_set_position(Px_Controller controller, Px_Vector3f32 position);
	void px_controller_move(Px_Controller controller, Px_Vector3f32 displacement, float dt, uint32_t mask_index);

#ifdef __cplusplus
}
#endif
