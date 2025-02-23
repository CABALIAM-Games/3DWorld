// 3D World - Dynamic Particle class definition
// by Frank Gennari
// 7/17/06

#include "3DWorld.h"
#include "dynamic_particle.h"
#include "mesh.h"
#include "physics_objects.h"
#include "shaders.h"
#include "lightmap.h"


bool     const ADD_DP_COBJS   = 0;
unsigned const NUM_COLL_STEPS = 4;
float    const TERMINAL_VEL   = 100.0;
float    const MAX_D_HEIGHT   = 0.1;


dynamic_particle_system d_part_sys;
dpart_params_t dp_params;


extern bool begin_motion, enable_dpart_shadows;
extern int window_width, iticks, animate2, display_mode, frame_counter;
extern float zbottom, ztop, fticks, base_gravity, TIMESTEP, XY_SCENE_SIZE;
extern obj_type object_types[];
extern vector<light_source_trig> light_sources_d;


// ************ dynamic_particle ************


dynamic_particle::dynamic_particle() : sphere_t(all_zeros, rand_uniform(dp_params.rmin, dp_params.rmax)), moves(1), lighted(1),
	collides(1), chdir(0), gravity(0), shadows_setup(0), tid(-1), cid(-1), intensity(rand_uniform(dp_params.imin, dp_params.imax)*XY_SCENE_SIZE),
	bwidth(1.0), velocity(signed_rand_vector(rand_uniform(dp_params.vmin, dp_params.vmax)))
{
	colorRGBA const colors[] = {WHITE, RED, GREEN, BLUE, YELLOW};
	color = colors[rand() % (sizeof(colors)/sizeof(colorRGBA))];
	gen_pos();
}


void dynamic_particle::gen_pos() {
	
	do {
		rand_xy_point(rand_uniform(zbottom, (MAX_D_HEIGHT + max(ztop, czmax))), pos, 0);
		UNROLL_3X(pos[i_] *= dp_params.sdist[pos[i_] >= 0.0][i_];)
	} while (point_inside_voxel_terrain(pos));
}


void dynamic_particle::draw() const { // lights, color, texture, shadowed

	// Note: currently, we only support emissive, untextured particles
	// if we need to support lighting and textures it can be added later by using a different shader
	assert(lighted && tid < 0);
	color.set_for_cur_shader();
	int const ndiv(min(N_SPHERE_DIV, max(3, int(3.0f*sqrt(radius*window_width/distance_to_camera(pos))))));
	draw_sphere_vbo(pos, radius, ndiv, (tid >= 0)); // point if far away?
}


// multiple steps?
void dynamic_particle::apply_physics(float stepsize, int index) { // begin_motion, move, random dir change, collision (mesh and cobjs), forces applied to?

	if (!begin_motion || !animate2) return;

	while (1) {
		if (!is_over_mesh(pos) || pos.z > (MAX_D_HEIGHT + max(ztop, czmax)) || pos.z < zbottom) {
			gen_pos(); // keep within simulation area
			continue;
		}
		int const xpos(get_xpos(pos.x)), ypos(get_ypos(pos.y));
		
		if (point_outside_mesh(xpos, ypos)) { // what about water/ice? stuck in cobj?
			gen_pos();
			continue;
		}
		if (!is_mesh_disabled(xpos, ypos)) {
			float const zval(interpolate_mesh_zval(pos.x, pos.y, radius, 0, 0));

			if ((pos.z - radius) < zval) { // bounce off the surface of the mesh
				pos.z = zval + radius;
				vector3d bounce_v;
				calc_reflection_angle(velocity, bounce_v, surface_normals[ypos][xpos]);
				velocity = bounce_v;
			}
		}
		break;
	}
	if (moves) {
		float const timestep(TIMESTEP*fticks*stepsize);

		if (gravity) {
			float const vz(-min(TERMINAL_VEL, -(velocity.z - base_gravity*GRAVITY*timestep)));
			if (vz < velocity.z) velocity.z = vz;
		}
		if (chdir && (rand() % (100*int(NUM_COLL_STEPS))) < iticks) {
			float const vmag(velocity.mag());
			velocity = signed_rand_vector_norm()*vmag; // same magnitude
		}
		pos += velocity*timestep;
	}
	if (collides) { // hack - check this for correctness
		dwobject obj(DYNAM_PART, pos, velocity, 1, 10000.0); // make a DYNAM_PART object for collision detection
		object_types[DYNAM_PART].radius = radius;
		//obj.multistep_coll(last_pos, index, NUM_COLL_STEPS);
		obj.check_vert_collision(index, 0, 0); // ignoring return value
		pos = obj.pos;
		float const vmag(obj.velocity.mag());
		if (vmag > TOLERANCE) {velocity = obj.velocity*(velocity.mag()/vmag);} // same magnitude
	}
}


void dynamic_particle::add_light(cube_map_shadow_manager &smgr, int index) { // dynamic lights, non-const due to caching of light_id

	if (!lighted) return;

	if (enable_dpart_shadows) {
		if (!shadows_setup) {
			cube_map_lix_t lix(smgr.add_obj(index, 1));
			lix.add_cube_face_lights(pos, intensity, color, 1.01*radius);
			shadows_setup = 1;
		}
		else {smgr.sync_light_pos(index, pos);}
	}
	else {add_dynamic_light(intensity, pos, color, velocity, bwidth);} // beam in direction of velocity
}

void dynamic_particle::add_cobj_shadows() const { // cobjs, dynamic objects
	add_shadow_obj(pos, radius, -1);
}

void dynamic_particle::add_cobj() {
	if (ADD_DP_COBJS) {cid = add_coll_sphere(pos, radius, cobj_params(0.7, color, 0, 1));}
}

void dynamic_particle::remove_cobj() {
	if (ADD_DP_COBJS) {remove_coll_object(cid);}
	cid = -1;
}


// ************ dynamic_particle_system ************


void dynamic_particle_system::create_particles(unsigned num, bool only_if_empty) {

	if (only_if_empty && size() > 0) return;
	clear();
	particles.reserve(num);
	for (unsigned i = 0; i < num; ++i) {add_particle(dynamic_particle());}
}


void dynamic_particle_system::draw() const {

	shader_t s;
	s.begin_color_only_shader();
	begin_sphere_draw(0);
	for (unsigned i = 0; i < size(); ++i) {particles[i].draw();}
	end_sphere_draw();
	s.end_shader();
}


void dynamic_particle_system::apply_physics(float stepsize) {
	
	for (unsigned i = 0; i < size(); ++i) {
		particles[i].remove_cobj();
		for (unsigned s = 0; s < NUM_COLL_STEPS; ++s) {particles[i].apply_physics(stepsize/NUM_COLL_STEPS, i);}
		particles[i].add_cobj();
	}
}


void dynamic_particle_system::add_lights() { // non-const due to caching of light_id
	for (unsigned i = 0; i < size(); ++i) {particles[i].add_light(*this, i);}
}

void dynamic_particle_system::remove_lights() {
	for (unsigned i = 0; i < size(); ++i) {remove_obj_light(i);}
}

void dynamic_particle_system::add_cobj_shadows() const {
	for (unsigned i = 0; i < size(); ++i) {particles[i].add_cobj_shadows();}
}


void dynamic_particle_system::build_lookup_matrix() {

	bins.clear();
	bins.resize(XY_MULT_SIZE);

	for (unsigned i = 0; i < size(); ++i) {
		point const &pos(particles[i].get_pos());
		int const xpos(get_xpos(pos.x)), ypos(get_ypos(pos.y));
		if (!point_outside_mesh(xpos, ypos)) bins[xpos + MESH_X_SIZE*ypos].push_back(i);
	}
}

