package physx

import "core:math/linalg"
import "core:mem"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"};

Transform :: struct {
	q : linalg.Quaternionf32,
	p : linalg.Vector3f32,
}

Scene :: distinct rawptr;
Actor :: distinct rawptr;
Triangle_Mesh :: distinct rawptr;
Convex_Mesh :: distinct rawptr;
Controller_Manager :: distinct rawptr;
Controller :: distinct rawptr;

Buffer :: struct {
	data : rawptr,
	size : uint,
}

// TODO: Slice wrapper
Mesh_Description :: struct {
	vertices : ^linalg.Vector3f32,
	num_vertices : u32,
	vertex_stride : u32,
	indices : ^u32,
	num_triangles : u32,
	triangle_stride : u32,
};

@(default_calling_convention="c")
foreign physx {
	init :: proc(initialize_cooking, initialize_pvd : bool) ---;
	destroy :: proc() ---;

	scene_create :: proc() -> Scene ---;
	scene_release :: proc(scene : Scene) ---;
	scene_simulate :: proc(scene : Scene, dt : f32) ---;
	scene_set_gravity :: proc(scene : Scene, gravity : linalg.Vector3f32) ---;
	scene_add_actor :: proc(scene : Scene, actor : Actor) ---;
	scene_remove_actor :: proc(scene : Scene, actor : Actor) ---;
    @(link_name="scene_get_active_actors") _scene_get_active_actors :: proc(scene : Scene, num_active : ^u32) -> ^Actor ---;

	actor_create :: proc() -> Actor ---;
	actor_release :: proc(actor : Actor) ---;
	actor_get_user_data :: proc(actor : Actor) -> rawptr ---;
	actor_set_user_data :: proc(actor : Actor, user_data : rawptr) ---;
	actor_set_kinematic :: proc(actor : Actor, kinematic : bool) ---;
	actor_get_transform :: proc(actor : Actor) -> Transform ---;
	actor_set_transform :: proc(actor : Actor, transform : Transform) ---;
	actor_get_velocity :: proc(actor : Actor) -> linalg.Vector3f32 ---;
	actor_set_velocity :: proc(actor : Actor, linear_velocity : linalg.Vector3f32) ---;
	actor_add_shape_triangle_mesh :: proc(actor : Actor, triangle_mesh : Triangle_Mesh) ---;
	actor_add_shape_convex_mesh :: proc(actor : Actor, convex_mesh : Convex_Mesh) ---;

	cook_triangle_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	cook_convex_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	buffer_free :: proc(buffer : Buffer) ---;

	triangle_mesh_create :: proc(buffer : Buffer) -> Triangle_Mesh ---;
	triangle_mesh_release :: proc(triangle_mesh : Triangle_Mesh) ---;
	convex_mesh_create :: proc(buffer : Buffer) -> Convex_Mesh ---;
	convex_mesh_release :: proc(convex_mesh : Convex_Mesh) ---;

	controller_manager_create :: proc(scene : Scene) -> Controller_Manager ---;
	controller_manager_release :: proc(controller_manager : Controller_Manager) ---;
	controller_create :: proc(controller_manager : Controller_Manager) -> Controller ---;
	controller_release :: proc(controller : Controller) ---;
	controller_get_position :: proc(controller : Controller) -> linalg.Vector3f32 ---;
	controller_set_position :: proc(controller : Controller, position : linalg.Vector3f32) ---;
	controller_move :: proc(controller : Controller, displacement : linalg.Vector3f32, minimum_distance : f32, dt : f32) ---;
}

scene_get_active_actors :: proc(scene : Scene) -> []Actor {
	num_active : u32;
	actors := _scene_get_active_actors(scene, &num_active);
	return mem.slice_ptr(actors, cast(int) num_active);
}
