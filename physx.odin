package physx

import "core:math/linalg"
import "core:mem"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"};

Transform :: struct {
	q : linalg.Quaternionf32,
	p : linalg.Vector3f32,
}

Scene_Handle :: distinct rawptr;
Actor_Handle :: distinct rawptr;
Triangle_Mesh_Handle :: distinct rawptr;
Convex_Mesh_Handle :: distinct rawptr;

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

	scene_create :: proc() -> Scene_Handle ---;
	scene_release :: proc(scene_handle : Scene_Handle) ---;
	scene_simulate :: proc(scene_handle : Scene_Handle, dt : f32) ---;
	scene_set_gravity :: proc(scene_handle : Scene_Handle, gravity : linalg.Vector3f32) ---;
	scene_add_actor :: proc(scene_handle : Scene_Handle, actor_handle : Actor_Handle) ---;
	scene_remove_actor :: proc(scene_handle : Scene_Handle, actor_handle : Actor_Handle) ---;
    @(link_name="scene_get_active_actors") _scene_get_active_actors :: proc(scene_handle : Scene_Handle, num_active : ^u32) -> ^Actor_Handle ---;

	actor_create :: proc() -> Actor_Handle ---;
	actor_release :: proc(actor : Actor_Handle) ---;
	actor_get_user_data :: proc(actor : Actor_Handle) -> rawptr ---;
	actor_set_user_data :: proc(actor : Actor_Handle, user_data : rawptr) ---;
	actor_set_kinematic :: proc(actor : Actor_Handle, kinematic : bool) ---;
	actor_get_transform :: proc(actor : Actor_Handle) -> Transform ---;
	actor_set_transform :: proc(actor : Actor_Handle, transform : Transform) ---;
	actor_get_velocity :: proc(actor : Actor_Handle) -> linalg.Vector3f32 ---;
	actor_set_velocity :: proc(actor : Actor_Handle, linear_velocity : linalg.Vector3f32) ---;

	actor_add_shape_triangle_mesh :: proc(actor_handle : Actor_Handle, triangle_mesh_handle : Triangle_Mesh_Handle) ---;
	actor_add_shape_convex_mesh :: proc(actor_handle : Actor_Handle, convex_mesh_handle : Convex_Mesh_Handle) ---;

	cook_triangle_mesh_precise :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	cook_convex_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	buffer_free :: proc(buffer : Buffer) ---;

	triangle_mesh_create :: proc(buffer : Buffer) -> Triangle_Mesh_Handle ---;
	triangle_mesh_release :: proc(triangle_mesh_handle : Triangle_Mesh_Handle) ---;

	convex_mesh_create :: proc(buffer : Buffer) -> Convex_Mesh_Handle ---;
	convex_mesh_release :: proc(convex_mesh_handle : Convex_Mesh_Handle) ---;
}

scene_get_active_actors :: proc(scene_handle : Scene_Handle) -> []Actor_Handle {
	num_active : u32;
	actors := _scene_get_active_actors(scene_handle, &num_active);
	return mem.slice_ptr(actors, cast(int) num_active);
}
