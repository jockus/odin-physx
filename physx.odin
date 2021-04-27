package physx

import "core:math/linalg"

when ODIN_OS == "windows" do foreign import physx {"lib/physx_lib.lib"};

Body_Handle :: rawptr;

@(default_calling_convention="c")
foreign physx {
	init :: proc(initialize_pv : bool) ---;
	destroy :: proc() ---;
	step :: proc(dt : f32) ---;

	create_body :: proc(kinematic : bool) -> Body_Handle ---;
	get_position :: proc(body : Body_Handle) -> linalg.Vector3f32 ---;
	set_position :: proc(body : Body_Handle, position : linalg.Vector3f32) ---;
}

