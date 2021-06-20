#include "..\physx_lib.h"
#include <stdio.h>
#include <windows.h>

#pragma comment(lib, "../lib/physx_lib.lib")

void* allocate(Allocator* allocator, size_t size) {
	return _aligned_malloc(size, 16);
}

void deallocate(Allocator* allocator, void* ptr) {
	_aligned_free(ptr);
}

int main(int argc, char const* argv) {
	Allocator allocator;
	allocator.allocate_16_byte_aligned = allocate;
	allocator.deallocate = deallocate;

	init(allocator, false, false);

	Material material = material_create(0.5f, 0.5f, 0.5f);

	Scene scene = scene_create();

	Actor actor = actor_create();
	actor_add_shape_sphere(actor, 1, material, 0, 0, false);
	scene_add_actor(scene, actor);

	for(int step = 0; step < 10; ++step) {
		Transform t = actor_get_transform(actor);
		printf("Pos : %.2f %.2f %.2f\n", t.p.x, t.p.y, t.p.z);
		scene_simulate(scene, 0.1f, NULL, 0);
	}

	actor_release(actor);
	scene_release(scene);

	destroy();
	return 0;
}
