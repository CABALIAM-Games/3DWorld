// 3D World - FILE Utility Functions
// by Frank Gennari
// 3/6/12
#pragma once

#include <fstream>
#include "3DWorld.h"

inline bool is_EOF(int v) {return (v == EOF || v == '\0');}
inline bool is_end_of_string(int v) {return (v == '#' || isspace(v) || is_EOF(v));}
bool read_block_comment(FILE *fp);

inline bool read_int  (FILE *fp, int      &val) {return (fscanf(fp, "%i", &val) == 1);}
inline bool read_uint (FILE *fp, unsigned &val) {return (fscanf(fp, "%u", &val) == 1);}
inline bool read_nonzero_uint(FILE *fp, unsigned &val) {return (fscanf(fp, "%u", &val) == 1 && val > 0);}
inline bool read_float(FILE *fp, float    &val) {return (fscanf(fp, "%f", &val) == 1);}
inline bool read_zero_one_float(FILE *fp, float &val) {return (read_float(fp, val) && val >= 0.0 && val <= 1.0);}
inline bool read_str  (FILE *fp, char     *val) {return (fscanf(fp, "%255s", val) == 1);}

inline bool check_file_exists(std::string const &fn) {return std::ifstream(fn).good();}

inline unsigned read_binary_uint(FILE *fp) {
	unsigned v(0);
	unsigned const v_read(fread(&v, sizeof(unsigned), 1, fp));
	assert(v_read == 1); // add error checking?
	return v;
}

inline void write_binary_uint(FILE *fp, unsigned v) {
	unsigned v_write(fwrite(&v, sizeof(unsigned), 1, fp));
	assert(v_write == 1); // add error checking?
}

inline bool read_vector(FILE *fp, vector3d &v) { // or point
	return (fscanf(fp, "%f%f%f", &v.x, &v.y, &v.z) == 3);
}

inline bool read_color(FILE *fp, colorRGBA &c) {
	c.A = 1.0; // default
	return (fscanf(fp, "%f%f%f%f", &c.R, &c.G, &c.B, &c.A) >= 3); // alpha is optional
}

inline bool read_bool (FILE *fp, bool     &val) {
	int tmp;
	if (fscanf(fp, "%i", &tmp) != 1) return 0;
	val = (tmp != 0);
	return 1;
}

inline bool read_string(FILE *fp, std::string &str) {
	char s[MAX_CHARS] = {0};
	if (!read_str(fp, s)) return 0;
	str = s;
	return 1;
}

inline int read_cube(FILE *fp, cube_t &c, bool z_is_optional=0) { // x1 x2 y1 y2 [z1 z2]
	int const num_read(fscanf(fp, "%f%f%f%f%f%f", &c.d[0][0], &c.d[0][1], &c.d[1][0], &c.d[1][1], &c.d[2][0], &c.d[2][1]));
	if (z_is_optional && num_read == 4) {c.d[2][0] = c.d[2][1] = 0.0; return 2;} // zvals only
	return (num_read == 6);
}


inline bool read_type_t(FILE *fp, int       &val) {return read_int   (fp, val);}
inline bool read_type_t(FILE *fp, unsigned  &val) {return read_uint  (fp, val);}
inline bool read_type_t(FILE *fp, float     &val) {return read_float (fp, val);}
inline bool read_type_t(FILE *fp, char      *val) {return read_str   (fp, val);}
inline bool read_type_t(FILE *fp, std::string &val) {return read_string(fp, val);}
inline bool read_type_t(FILE *fp, vector3d  &val) {return read_vector(fp, val);}
inline bool read_type_t(FILE *fp, colorRGBA &val) {return read_color (fp, val);}
inline bool read_type_t(FILE *fp,     bool  &val) {return read_bool  (fp, val);}

bool read_float_reset_pos_on_fail(FILE *fp, float &v);
bool read_int_reset_pos_on_fail(FILE *fp, int &v);
std::string read_quoted_string(FILE *fp, unsigned &line_num);

struct geom_xform_t;
unsigned read_cube(FILE *fp, geom_xform_t const &xf, cube_t &c);

