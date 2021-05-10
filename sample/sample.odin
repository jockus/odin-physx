package sample
import "core:fmt"
import "core:runtime"
import "core:mem"
import px "../"

allocate :: proc "c" (allocator : ^px.Allocator, size : u64) -> rawptr {
	context = runtime.default_context();
	return mem.alloc(cast(int) size, 16);
}

deallocate :: proc "c" (allocator : ^px.Allocator, ptr : rawptr) {
	context = runtime.default_context();
	mem.free(ptr);
}

main :: proc() {
	using px;
	allocator : Allocator;
	allocator.allocate_16_byte_aligned = allocate;
	allocator.deallocate = deallocate;

	init(allocator, false, false);

	material := material_create(0.5, 0.5, 0.5);

	scene := scene_create();

	actor := actor_create();
	actor_add_shape_sphere(actor, 1, material, 0, 0, false);
	scene_add_actor(scene, actor);

	for step : int = 0; step < 10; step += 1 {
		t := actor_get_transform(actor);
		fmt.printf("Pos : %.2f %.2f %.2f\n", t.p.x, t.p.y, t.p.z);
		scene_simulate(scene, 0.1, nil, 0);
	}

	actor_release(actor);
	scene_release(scene);

	destroy();
}
