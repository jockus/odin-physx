package physx

import "core:math/linalg"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"};

Scene_Handle :: rawptr;
Actor_Handle :: rawptr;

@(default_calling_convention="c")
foreign physx {
	init :: proc(initialize_pvd : bool) ---;
	destroy :: proc() ---;

	scene_create :: proc() -> Scene_Handle ---;
	scene_release :: proc(scene_handle : Scene_Handle) ---;
	scene_simulate :: proc(scene_handle : Scene_Handle, dt : f32) ---;
	scene_set_gravity :: proc(scene_handle : Scene_Handle, gravity : linalg.Vector3f32) ---;
	scene_add_actor :: proc(scene_handle : Scene_Handle, actor_handle : Actor_Handle) ---;
	scene_release_actor :: proc(scene_handle : Scene_Handle, actor_handle : Actor_Handle) ---;
	scene_get_active_actors :: proc(numActorsOut : ^u32) -> ^^Actor_Handle --;

	actor_create :: proc(scene_handle : Scene_Handle) -> Actor_Handle ---;
	actor_release :: proc(actor : Actor_Handle) ---;

	actor_get_user_data :: proc(actor : Actor_Handle) -> rawptr --;
	actor_set_user_data :: proc(actor : Actor_Handle, user_data : rawptr) --;

	actor_get_position :: proc(actor : Actor_Handle) -> linalg.Vector3f32 ---;
	actor_set_position :: proc(actor : Actor_Handle, position : linalg.Vector3f32) ---;
	actor_get_velocity :: proc(actor : Actor_Handle) -> linalg.Vector3f32 ---;
	actor_set_velocity :: proc(actor : Actor_Handle, linear_velocity : linalg.Vector3f32) ---;
}

