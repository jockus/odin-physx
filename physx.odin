package physx

import "core:math/linalg"
import "core:mem"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"}

Transform :: struct {
	q : linalg.Quaternionf32,
	p : linalg.Vector3f32,
}

Scene :: distinct rawptr
Material :: distinct rawptr
Actor :: distinct rawptr
Triangle_Mesh :: distinct rawptr
Convex_Mesh :: distinct rawptr
Controller :: distinct rawptr

Allocator :: struct {
	allocate_16_byte_aligned : #type proc "c" (allocator : ^Allocator, size : u64) -> rawptr,
	deallocate : #type proc "c" (allocator : ^Allocator, ptr : rawptr),
	user_data : rawptr,
}

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
}

Contact :: struct {
	actor0 : Actor,
	actor1 : Actor,
	pos : linalg.Vector3f32,
	normal : linalg.Vector3f32,
	impulse : linalg.Vector3f32,
}

Trigger_State :: enum i32 {
	eNOTIFY_TOUCH_FOUND,
	eNOTIFY_TOUCH_LOST,
}

Trigger :: struct {
	trigger : Actor,
	other_actor : Actor,
	state : Trigger_State,
}

Query_Hit :: struct {
	valid : bool,
	pos : linalg.Vector3f32,
	normal : linalg.Vector3f32,
}


Controller_Settings :: struct {
	slope_limit_deg : f32,
	height : f32,
	radius : f32,
	up : linalg.Vector3f32,
	shape_layer_index : i32,
	mask_index : i32,
	material : Material,
}

@(default_calling_convention="c")
foreign physx {

	@(link_name="px_init")
	init :: proc(allocator : Allocator, initialize_cooking : bool, initialize_pvd : bool) ---

	@(link_name="px_destroy")
	destroy :: proc() ---


	@(link_name="px_scene_create")
	scene_create :: proc() -> Scene ---

	@(link_name="px_scene_release")
	scene_release :: proc(scene : Scene) ---

	@(link_name="px_scene_simulate")
	scene_simulate :: proc(scene : Scene, dt : f32, scratch_memory_16_byte_aligned : rawptr = nil, scratch_size : u64 = 0) ---

	@(link_name="px_scene_set_gravity")
	scene_set_gravity :: proc(scene : Scene, gravity : linalg.Vector3f32) ---

	@(link_name="px_scene_add_actor")
	scene_add_actor :: proc(scene : Scene, actor : Actor) ---

	@(link_name="px_scene_remove_actor")
	scene_remove_actor :: proc(scene : Scene, actor : Actor) ---

	@(link_name="px_scene_get_active_actors")
	_scene_get_active_actors :: proc(scene : Scene, num_active : ^u32) -> ^Actor ---

	@(link_name="px_scene_get_contacts")
	_scene_get_contacts :: proc(scene : Scene, num_contacts : ^u32) -> ^Contact ---

	@(link_name="px_scene_get_triggers")
	_scene_get_triggers :: proc(scene : Scene, num_triggers : ^u32) -> ^Trigger ---


	@(link_name="px_scene_set_collision_mask")
	scene_set_collision_mask :: proc(scene : Scene, mask_index : i32, layer_mask : u64) ---

	@(link_name="px_scene_raycast")
	scene_raycast :: proc(scene : Scene, origin : linalg.Vector3f32, direction : linalg.Vector3f32, distance : f32, mask_index : i32) -> Query_Hit ---


	@(link_name="px_material_create")
	material_create :: proc(static_friction : f32, dynamic_friction : f32, restitution : f32) -> Material ---

	@(link_name="px_material_release")
	material_release ::proc(material : Material) ---


	@(link_name="px_actor_create")
	actor_create :: proc() -> Actor ---

	@(link_name="px_actor_release")
	actor_release :: proc(actor : Actor) ---

	@(link_name="px_actor_get_user_data")
	actor_get_user_data :: proc(actor : Actor) -> rawptr ---

	@(link_name="px_actor_set_user_data")
	actor_set_user_data :: proc(actor : Actor, user_data : rawptr) ---

	@(link_name="px_actor_set_kinematic")
	actor_set_kinematic :: proc(actor : Actor, kinematic : bool) ---

	@(link_name="px_actor_get_transform")
	actor_get_transform :: proc(actor : Actor) -> Transform ---

	@(link_name="px_actor_set_transform")
	actor_set_transform :: proc(actor : Actor, transform : Transform, teleport := false) ---

	@(link_name="px_actor_get_velocity")
	actor_get_velocity :: proc(actor : Actor) -> linalg.Vector3f32 ---

	@(link_name="px_actor_set_velocity")
	actor_set_velocity :: proc(actor : Actor, linear_velocity : linalg.Vector3f32) ---

	@(link_name="px_actor_add_shape_box")
	actor_add_shape_box :: proc(actor : Actor, half_extents : linalg.Vector3f32, material : Material, shape_layer_index : i32, mask_index : i32, trigger : bool) ---

	@(link_name="px_actor_add_shape_sphere")
	actor_add_shape_sphere :: proc(actor : Actor, radius : f32, material : Material, shape_layer_index : i32, mask_index : i32, trigger : bool) ---

	@(link_name="px_actor_add_shape_triangle_mesh")
	actor_add_shape_triangle_mesh :: proc(actor : Actor, triangle_mesh : Triangle_Mesh, material : Material, shape_layer_index : i32, mask_index : i32) ---

	@(link_name="px_actor_add_shape_convex_mesh")
	actor_add_shape_convex_mesh :: proc(actor : Actor, convex_mesh : Convex_Mesh, material : Material, shape_layer_index : i32, mask_index : i32) ---


	@(link_name="px_cook_triangle_mesh")
	cook_triangle_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---

	@(link_name="px_cook_convex_mesh")
	cook_convex_mesh :: proc(mesh_description : Mesh_Description) -> Buffer ---

	@(link_name="px_buffer_free")
	buffer_free :: proc(buffer : Buffer) ---


	@(link_name="px_triangle_mesh_create")
	triangle_mesh_create :: proc(buffer : Buffer) -> Triangle_Mesh ---

	@(link_name="px_triangle_mesh_release")
	triangle_mesh_release :: proc(triangle_mesh : Triangle_Mesh) ---

	@(link_name="px_convex_mesh_create")
	convex_mesh_create :: proc(buffer : Buffer) -> Convex_Mesh ---

	@(link_name="px_convex_mesh_release")
	convex_mesh_release :: proc(convex_mesh : Convex_Mesh) ---


	@(link_name="px_controller_create")
	controller_create :: proc(scene : Scene, controller_settings : Controller_Settings) -> Controller ---

	@(link_name="px_controller_release")
	controller_release :: proc(controller : Controller) ---

	@(link_name="px_controller_get_position")
	controller_get_position :: proc(controller : Controller) -> linalg.Vector3f32 ---

	@(link_name="px_controller_set_position")
	controller_set_position :: proc(controller : Controller, position : linalg.Vector3f32) ---

	@(link_name="px_controller_move")
	controller_move :: proc(controller : Controller, displacement : linalg.Vector3f32, dt : f32, mask_index : i32) ---
}

scene_get_active_actors :: proc(scene : Scene) -> []Actor {
	num : u32
	result := _scene_get_active_actors(scene, &num)
	return mem.slice_ptr(result, cast(int) num)
}

scene_get_contacts :: proc(scene : Scene) -> []Contact {
	num: u32
	result := _scene_get_contacts(scene, &num)
	return mem.slice_ptr(result, cast(int) num)
}

scene_get_triggers :: proc(scene : Scene) -> []Trigger {
	num: u32
	result := _scene_get_triggers(scene, &num)
	return mem.slice_ptr(result, cast(int) num)
}
