package physx

import "core:math/linalg"
import "core:mem"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"};

Transform :: struct {
	q : linalg.Quaternionf32,
	p : linalg.Vector3f32,
}

Scene :: distinct rawptr;
Material :: distinct rawptr;
Actor :: distinct rawptr;
Triangle_Mesh :: distinct rawptr;
Convex_Mesh :: distinct rawptr;
Controller_Manager :: distinct rawptr;
Controller :: distinct rawptr;

Allocator :: struct {
	allocate_16_byte_aligned : #type proc "c" (allocator : ^Allocator, size : u64) -> rawptr,
	deallocate : #type proc "c" (allocator : ^Allocator, ptr : rawptr),
	user_data : rawptr,
};

Buffer :: struct {
	data : rawptr,
	size : u64,
}

Mesh_Description :: struct {
	vertices : ^linalg.Vector3f32,
	num_vertices : u32,
	vertex_stride : u32,
	indices : ^u32,
	num_triangles : u32,
	triangle_stride : u32,
};

Contact :: struct {
	actor0 : Actor,
	actor1 : Actor,
	pos : linalg.Vector3f32,
	normal : linalg.Vector3f32,
	impulse : linalg.Vector3f32,
};

Trigger_State :: enum i32 {
	eNOTIFY_TOUCH_FOUND,
	eNOTIFY_TOUCH_LOST,
};

Trigger :: struct {
	trigger : Actor,
	other_actor : Actor,
	state : Trigger_State,
};

Query_Hit :: struct {
	hit : bool,
	pos : linalg.Vector3f32,
	normal : linalg.Vector3f32,
};


Controller_Settings :: struct {
	slope_limit_deg : f32,
	height : f32,
	radius : f32,
	up : linalg.Vector3f32,
	shape_layer_index : i32,
	mask_index : i32,
	material : Material,
};

@(default_calling_convention="c")
foreign physx {
	init :: proc(allocator : Allocator, initialize_cooking : bool, initialize_pvd : bool) ---;
	destroy :: proc() ---;

	scene_create :: proc() -> Scene ---;
	scene_release :: proc(scene : Scene) ---;
	scene_simulate :: proc(scene : Scene, dt : f32, scratch_memory_16_byte_aligned : rawptr = nil, scratch_size : u64 = 0) ---;
	scene_set_gravity :: proc(scene : Scene, gravity : linalg.Vector3f32) ---;
	scene_add_actor :: proc(scene : Scene, actor : Actor) ---;
	scene_remove_actor :: proc(scene : Scene, actor : Actor) ---;
    @(link_name="scene_get_active_actors") _scene_get_active_actors :: proc(scene : Scene, num_active : ^u32) -> ^Actor ---;
	@(link_name="scene_get_contacts") _scene_get_contacts :: proc(scene : Scene, numContacts : ^u32) -> ^Contact ---;
	@(link_name="scene_get_triggers") _scene_get_triggers :: proc(scene : Scene, numTriggers : ^u32) -> ^Trigger ---;
	scene_set_collision_mask :: proc(scene : Scene, mask_index : i32, layer_mask : u64) ---;
	scene_raycast :: proc(scene : Scene, origin : linalg.Vector3f32, direction : linalg.Vector3f32, distance : f32, mask_index : i32) -> Query_Hit ---;

	material_create :: proc(static_friction : f32, dynamic_friction : f32, restitution : f32) -> Material ---;
	material_release ::proc(material : Material) ---;

	actor_create :: proc() -> Actor ---;
	actor_release :: proc(actor : Actor) ---;
	actor_get_user_data :: proc(actor : Actor) -> rawptr ---;
	actor_set_user_data :: proc(actor : Actor, user_data : rawptr) ---;
	actor_set_kinematic :: proc(actor : Actor, kinematic : bool) ---;
	actor_get_transform :: proc(actor : Actor) -> Transform ---;
	actor_set_transform :: proc(actor : Actor, transform : Transform, teleport := false) ---;
	actor_get_velocity :: proc(actor : Actor) -> linalg.Vector3f32 ---;
	actor_set_velocity :: proc(actor : Actor, linear_velocity : linalg.Vector3f32) ---;
	actor_add_shape_box :: proc(actor : Actor, half_extents : linalg.Vector3f32, material : Material, shape_layer_index : i32, mask_index : i32, trigger : bool) ---;
	actor_add_shape_sphere :: proc(actor : Actor, radius : f32, material : Material, shape_layer_index : i32, mask_index : i32, trigger : bool) ---;
	actor_add_shape_triangle_mesh :: proc(actor : Actor, triangle_mesh : Triangle_Mesh, material : Material, shape_layer_index : i32, mask_index : i32) ---;
	actor_add_shape_convex_mesh :: proc(actor : Actor, convex_mesh : Convex_Mesh, material : Material, shape_layer_index : i32, mask_index : i32) ---;

	cook_triangle_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	cook_convex_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---;
	buffer_free :: proc(buffer : Buffer) ---;

	triangle_mesh_create :: proc(buffer : Buffer) -> Triangle_Mesh ---;
	triangle_mesh_release :: proc(triangle_mesh : Triangle_Mesh) ---;
	convex_mesh_create :: proc(buffer : Buffer) -> Convex_Mesh ---;
	convex_mesh_release :: proc(convex_mesh : Convex_Mesh) ---;

	controller_create :: proc(scene : Scene, controller_settings : Controller_Settings) -> Controller ---;
	controller_release :: proc(controller : Controller) ---;
	controller_get_position :: proc(controller : Controller) -> linalg.Vector3f32 ---;
	controller_set_position :: proc(controller : Controller, position : linalg.Vector3f32) ---;
	controller_move :: proc(controller : Controller, displacement : linalg.Vector3f32, dt : f32, mask_index : i32) ---;
}

scene_get_active_actors :: proc(scene : Scene) -> []Actor {
	num : u32;
	result := _scene_get_active_actors(scene, &num);
	return mem.slice_ptr(result, cast(int) num);
}

scene_get_contacts :: proc(scene : Scene) -> []Contact {
	num: u32;
	result := _scene_get_contacts(scene, &num);
	return mem.slice_ptr(result, cast(int) num);
}

scene_get_triggers :: proc(scene : Scene) -> []Trigger {
	num: u32;
	result := _scene_get_triggers(scene, &num);
	return mem.slice_ptr(result, cast(int) num);
}
