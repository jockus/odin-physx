#pragma once


extern "C" {
	 struct Vec3 {
		float x;	
		float y;	
		float z;	
	};

	void init(bool initialize_pv);
	void destroy();
	void step(float dt);

	typedef void* Body_Handle;
	Body_Handle create_body();
	Vec3 get_position(Body_Handle body);
	void set_position(Body_Handle body, Vec3 position);
}


