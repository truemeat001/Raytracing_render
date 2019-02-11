#pragma once
#ifndef SPHERE_H
#define SPHERE_H
#include "onb.h"
#include "hitable.h"

inline vec3 random_to_sphere(float radius, float distance_squared) {
	float r1 = drand48();
	float r2 = drand48();
	float z = 1 + r2 * (sqrt(1 - radius * radius / distance_squared) - 1);
	float phi = 2 * M_PI*r1;
	float x = cos(phi)*sqrt(1 - z * z);
	float y = sin(phi)*sqrt(1 - z * z);
	return vec3(x, y, z);
}

class sphere : public hitable
{
public:
	sphere();
	sphere(vec3 cen, float r, material *m) : center(cen), radius(r), mat_ptr(m) {}
	virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec, bool is_medium = false) const;
	virtual bool bounding_box(float t0, float t1, aabb& box) const;
	virtual float pdf_value(const vec3& o, const vec3& v) const;
	virtual vec3 random(const vec3& o) const;
	vec3 center;
	float radius;
	material *mat_ptr;
};

bool sphere::bounding_box(float t0, float t1, aabb& box)const {
	box = aabb(center - vec3(radius, radius, radius), center + vec3(radius, radius, radius));
	return true;
}

bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec, bool is_medium) const {
	vec3 oc = r.origin() - center;
	float a = dot(r.direction(), r.direction());
	float b = dot(oc, r.direction());
	float c = dot(oc, oc) - radius * radius;
	float discriminant = b * b - a * c;
	if (discriminant > 0)
	{
		float temp = (-b - sqrt(discriminant)) / a;
		if (temp < t_max && temp > t_min)
		{
			rec.t = temp;
			rec.p = r.point_at_parameter(rec.t);
			get_sphere_uv((rec.p - center)/radius, rec.u, rec.v);
			rec.normal = (rec.p - center) / radius;
			rec.mat_ptr = mat_ptr;
			return true;
		}
		temp = (-b + sqrt(discriminant)) / a;
		if (temp < t_max && temp > t_min)
		{
			rec.t = temp;
			rec.p = r.point_at_parameter(rec.t);
			get_sphere_uv((rec.p - center) / radius, rec.u, rec.v);
			rec.normal = (rec.p - center) / radius;
			rec.mat_ptr = mat_ptr;
			return true;
		}
	}
	return false;
}


float sphere::pdf_value(const vec3& o, const vec3& v) const {
	hit_record rec;
	if (this->hit(ray(o, v), 0.001, FLT_MAX, rec)) {
		float cos_theta_max = sqrt(1 - radius * radius / (center - o).squared_length());
		float solid_angle = 2 * M_PI*(1 - cos_theta_max);
		return 1 / solid_angle;
	}
	else
		return 0;
}

vec3 sphere::random(const vec3& o) const {
	vec3 direction = center - o;
	float distance_squared = direction.squared_length();
	onb uvw;
	uvw.build_from_w(direction);
	return uvw.local(random_to_sphere(radius, distance_squared));
}
#endif // SPHERE_H
