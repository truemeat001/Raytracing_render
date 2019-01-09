// Raytracing_n.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "common.h"
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include "sphere.h"
#include "hitable_list.h"
#include "float.h"
#include "camera.h"
#include "stdlib.h"
#include "material.h"
#include "moving_sphere.h"
#include "aarect.h"
#include "triangle.h"
#include "box.h"
#include "constant_medium.h"
#include "bvh.h"
#include "pdf.h"
#include "model.h"
#include "brdf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define RaysBackgroundY

using namespace std;

const int thread_count = 4;
int **colors;
int finishedPixel = 0;
int curfinishedcount;
int threadCount = 1;
int threadfinishedcount = 0;
int nx = 500;
int ny = 500;
int ns = 100;
mutex g_lock;
ofstream outfile("..\\results\\181228_bunny_beckmann_gold_1.ppm", ios_base::out);

inline vec3 de_nan(const vec3& c) {
	vec3 temp = c;
	if (!(temp[0] == temp[0])) temp[0] = 0;
	if (!(temp[1] == temp[1])) temp[1] = 0;
	if (!(temp[2] == temp[2])) temp[2] = 0;
	return temp;
}

vec3 color(const ray& r, hitable *world, hitable *light_shape, int depth)
{
	hit_record hrec;
	if (world->hit(r, 0.001, numeric_limits<float>::max(), hrec))
	{
		scatter_record srec;
		vec3 emitted = hrec.mat_ptr->emitted(r, hrec, hrec.u, hrec.v, hrec.p);
		float pdf_val = 0;
		if (depth < 50 && hrec.mat_ptr->scatter(r, hrec, srec))
		{
			if (srec.is_specular)
				return srec.attenuation * color(srec.specular_ray, world, light_shape, depth + 1);
			else
			{
				ray scattered;
				hitable_pdf plight(light_shape, hrec.p);
				mixture_pdf p(&plight, srec.pdf_ptr);
				pdf_val = 0;
				while (pdf_val == 0)
				{
					scattered = ray(hrec.p, p.generate(r.direction()), r.time());
					pdf_val = p.value(r.direction(), scattered.direction());
				}

				delete srec.pdf_ptr;
				
				return emitted + srec.attenuation * hrec.mat_ptr->scattering_pdf(r, hrec, scattered) * color(scattered, world, light_shape, depth + 1) / pdf_val;
				//
			}
		}else
		{
			return emitted;
		}
	}
	else
	{
		return vec3(0, 0, 0);
	}
}

hitable *random_scene() {
	int n = 500;
	hitable **list = new hitable*[n + 1];
	texture *checker = new checker_texture(new constant_texture(vec3(0.2, 0.3, 0.1)), new constant_texture(vec3(0.9, 0.9, 0.9)));
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker));
	int i = 1;
	for (int a = -11; a < 11; a++)
	{
		for (int b = -11; b < 11; b++)
		{
			float choose_mat = drand48();
			vec3 center(a + 0.9*drand48(), 0.2, b + drand48());
			if ((center - vec3(4, 0.2, 0)).length() > 0.9)
			{
				if (choose_mat < 0.8) {
					//list[i++] = new sphere(center, 0.2, new lambertian(vec3(drand48()*drand48(), drand48()*drand48(), drand48()*drand48())));
					list[i++] = new moving_sphere(center, center + vec3(0, 0.5 * drand48(), 0), 0.0, 1.0, 0.2, new lambertian(new constant_texture(vec3(0.5, 0.5, 0.5))));
				}
				else if (choose_mat < 0.95)
				{
					list[i++] = new sphere(center, 0.2,
						new metal(vec3(0.5*(1 + drand48()), 0.5*(1 + drand48()), 0.5*(1 + drand48())), 0.5*drand48()));
				}
				else {
					list[i++] = new sphere(center, 0.2, new dielectric(1.5));
				}
			}
		}
	}
	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
	list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4, 0.2, 0.1))));
	list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

	return new hitable_list(list, i);
}

hitable *two_spheres() {
	texture *checker = new checker_texture(new constant_texture(vec3(0.2, 0.3, 0.1)), new constant_texture(vec3(0.9, 0.9, 0.9)));
	int n = 50;
	hitable ** list = new hitable*[n + 1];
	list[0] = new sphere(vec3(0, -10, 0), 10, new lambertian(checker));
	list[1] = new sphere(vec3(0, 10, 0), 10, new lambertian(checker));

	return new hitable_list(list, 2);
}

hitable *two_perlin_spheres() {
	texture* pertext = new noise_texture(1);
	hitable **list = new hitable*[2];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, new lambertian(pertext));
	return new hitable_list(list, 2);
}

hitable *earth_shpere() {
	int nx, ny, nn;
	unsigned char* tex_data = stbi_load(".\\Contents\\earthmap.jpg", &nx, &ny, &nn, 0);
	material* mat = new diffuse_light(new image_texture(tex_data, nx, ny));
	hitable **list = new hitable*[2];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(new constant_texture(vec3(0.9, 0.9, 0.9))));
	list[1] = new sphere(vec3(0, 2, 0), 2, mat);
	return new hitable_list(list, 2);
}

hitable *simple_light() {
	texture *pertext = new noise_texture(4);
	hitable **list = new hitable*[3];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, new lambertian(pertext));
	//list[2] = new sphere(vec3(0, 7, 0), 2, new diffuse_light(new constant_texture(vec3(4, 4, 4))));
	list[2] = new xy_rect(3, 5, 1, 3, -2, new diffuse_light(new constant_texture(vec3(4, 4, 4))));
	return new hitable_list(list, 3);
}

void cornell_box(hitable **scene, camera **cam, hitable **hlist, float aspect) {

	vec3 lookfrom(278, 278, -800);
	vec3 lookat(278, 278, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.0;
	float vfov = 40.0;
	*cam = new camera(lookfrom, lookat, vec3(0, 1, 0), vfov, aspect, aperture, dist_to_focus, 0.0, 1.0);

	int i = 0;
	brdf* aluminum_brdf = new brdf();
	hitable **list = new hitable*[12];
	material *red = new lambertian(new constant_texture(vec3(0.65, 0.05, 0.05)));
	material *red_200 = new lambertian(new constant_texture(vec3(0.93, 0.6, 0.6)));
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	material *green = new lambertian(new constant_texture(vec3(0.12, 0.45, 0.15)));
	material *blue = new lambertian(new constant_texture(vec3(0.05, 0.12, 0.55)));
	material *blue_200 = new lambertian(new constant_texture(vec3(0.56, 0.79, 0.97)));
	material *light = new diffuse_light(new constant_texture(vec3(15, 15, 15)));
	material *glass = new dielectric(1.3);
	//material *aluminum = new metal(vec3(0.8, 0.85, 0.88), 0.0, "..\\contents\\brdfs\\aluminium.binary");
	//material *aluminimu = new metal(vec3(0.8, 0.85, 0.88), 0.0);
	material *silver = new brdfmaterial("..\\contents\\brdfs\\silver-metallic-paint.binary", vec3(0.8, 0.85, 0.88));
	material *gold_24 = new metal(vec3(0.852, 0.695, 0.449), 0.0);
	material *gold = new metal(vec3(0.945, 0.75, 0.336), 0.0);
	float roughx = 0.9;
	float roughy = 0.1;

	material *beckmann_silver = new beckmann(new constant_texture(vec3(0.8, 0.85, 0.88)), roughx, roughy);
	material *beckmann_white = new beckmann(new constant_texture(vec3(.9, .9, .9)), roughx, roughy);
	material *beckmann_blue = new beckmann(new constant_texture(vec3(0.05, 0.12, 0.55)), roughx, roughy);
	material *beckmann_brow = new beckmann(new constant_texture(vec3(0.426, 0.3, 0.254)), roughx, roughy);
	material *beckmann_gold = new beckmann(new constant_texture(vec3(0.945, 0.75, 0.336)), roughx, roughy);

	material* orennayar_white_0 = new orennayar(new constant_texture(vec3(0.73)), 0);
	material* orennayar_blue_200_0 = new orennayar(new constant_texture(vec3(0.56, 0.79, 0.97)), 0);
	material* orennayar_red_200_0 = new orennayar(new constant_texture(vec3(0.93, 0.6, 0.6)), 0);
	material* orennayar_white_10 = new orennayar(new constant_texture(vec3(0.73)), 10);
	material* orennayar_white_20 = new orennayar(new constant_texture(vec3(0.73)), 20);


	list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, orennayar_blue_200_0));		// right
	list[i++] = new yz_rect(0, 555, 0, 555, 0, orennayar_red_200_0);							// left
	list[i++] = new flip_normals(new xz_rect(203, 353, 217, 343, 545, light));	// light
	list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, orennayar_white_0));		// top
	list[i++] = new xz_rect(0, 555, 0, 555, 0, orennayar_white_0);							// bottom
	list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, orennayar_white_0));		// back
	hitable *b1 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), beckmann_gold), -18), vec3(130, 0, 65));
	hitable *b2 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), beckmann_silver), 15), vec3(265, 0, 295));
	//list[i++] = b1;
	//list[i++] = b2;
	vec3 v0 = vec3(100, 100, 200);
	vec3 v1 = vec3(300, 100, 250);
	vec3 v2 = vec3(135, 200, 200);
	vec3 v3 = vec3(135, 50, 200);
	/*list[i++] = new triangle(v0, v1, v2, green);
	list[i++] = new triangle(v0, v3, v1, green);
	list[i++] = new triangle(v0, v2, v3, green);
	list[i++] = new triangle(v1, v2, v3, green);*/
	model *bunny = new model("..\\contents\\models\\bunny.ply", false, true, beckmann_gold, vec3(2000, 2000, 2000));
	//model *dragon = new model("..\\contents\\models\\dragon.ply", false, false, beckmann_gold, vec3(2000, 2000, 2000));
	hitable* b = new translate(new rotate_y(new bvh_node(bunny->genhitablemodel(), bunny->gettrianglecount(), 0, 1), 180), vec3(250, 0, 400));
	list[i++] = b;
	//list[i++] = new constant_medium(b, 0.2, new constant_texture(vec3(0.3, 0.8, 0.2)));

	//list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 300, 165), aluminum), 15), vec3(265, 0, 295));
	//list[i++] = new sphere(vec3(130, 80, 135), 80, glass);
	*scene = new hitable_list(list, i);
	

	hitable* light_shape = new flip_normals(new xz_rect(213, 343, 227, 333, 554, 0));
	hitable* glass_shape = new sphere(vec3(130, 80, 135), 80, 0);;
	//model *bunny_l = new model("..\\contents\\models\\bunny.ply", true, 0, vec3(1000, 1000, 1000));  // bvh_node 的value_pdf 和random是不正确的
	//hitable* b_l = new translate(new rotate_y(bunny->genhitablemodel(), 180), vec3(165, -34, 200));
	hitable** a = new hitable*[2];
	a[0] = light_shape;// new flip_normals(new xz_rect(213, 343, 227, 333, 554, 0));;
	//a[1] = glass_shape;
	//a[1] = b_l;
	*hlist = new hitable_list(a, 1);
}

void final(hitable **scene, camera **cam, float aspect) {
	int nb = 20;
	hitable **list = new hitable*[30];
	hitable **boxlist = new hitable*[10000];
	hitable **boxlist2 = new hitable*[10000];
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	material *ground = new lambertian(new constant_texture(vec3(0.48, 0.83, 0.53)));
	int b = 0;
	for (int i = 0; i < nb; i++) {
		for (int j = 0; j < nb; j++) {
			float w = 100;
			float x0 = -1000 + i * w;
			float z0 = -1000 + j * w;
			float y0 = 0;
			float x1 = x0 + w;
			float y1 = 100 * (drand48() + 0.01);
			float z1 = z0 + w;
			boxlist[b++] = new box(vec3(x0, y0, z0), vec3(x1, y1, z1), ground);
		}
	}
	int l = 0;
	list[l++] = new bvh_node(boxlist, b, 0, 1);
	material *light = new diffuse_light(new constant_texture(vec3(7, 7, 7)));
	list[l++] = new flip_normals(new xz_rect(123, 423, 147, 412, 554, light));
	vec3 center(400, 400, 200);
	list[l++] = new moving_sphere(center, center + vec3(30, 0, 0), 0, 1, 50, new lambertian(new constant_texture(vec3(0.7, 0.3, 0.1))));
	list[l++] = new sphere(vec3(260, 150, 45), 50, new dielectric(1.5));
	list[l++] = new sphere(vec3(0, 150, 145), 50, new metal(vec3(0.8, 0.8, 0.9), 1.0));// , 10.0));
	hitable *boundary = new sphere(vec3(360, 150, 145), 70, new dielectric(1.5));
	list[l++] = boundary;
	list[l++] = new constant_medium(boundary, 0.2, new constant_texture(vec3(0.2, 0.4, 0.9)));
	boundary = new sphere(vec3(0, 0, 0), 5000, new dielectric(1.5));
	list[l++] = new constant_medium(boundary, 0.0001, new constant_texture(vec3(1.0, 1.0, 1.0)));
	int nx, ny, nn;
	unsigned char *tex_data = stbi_load(".\\Contents\\earthmap.jpg", &nx, &ny, &nn, 0);
	material *emat = new lambertian(new image_texture(tex_data, nx, ny));
	list[l++] = new sphere(vec3(400, 200, 400), 100, emat);
	texture *pertext = new noise_texture(0.1);
	list[l++] = new sphere(vec3(220, 280, 300), 80, new lambertian(pertext));
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		boxlist2[j] = new sphere(vec3(165 * drand48(), 165 * drand48(), 165 * drand48()), 10, white);
	}
	list[l++] = new translate(new rotate_y(new bvh_node(boxlist2, ns, 0.0, 1.0), 15), vec3(-100, 270, 395));

	*scene = new hitable_list(list, l);
	vec3 lookfrom(478, 278, -600);
	vec3 lookat(278, 278, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.0;
	float vfov = 40.0;
	*cam = new camera(lookfrom, lookat, vec3(0, 1, 0), vfov, aspect, aperture, dist_to_focus, 0.0, 1.0);
}

hitable* final1()
{
	int nb = 20;
	hitable **list = new hitable*[10000];
	hitable **boxlist = new hitable*[10000];
	hitable **boxlist2 = new hitable*[10000];
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	int l = 0;
	material *light = new diffuse_light(new constant_texture(vec3(7, 7, 7)));
	list[l++] = new xz_rect(123, 423, 147, 412, 554, light);
	vec3 center(400, 400, 200);
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		//list[l++] = new translate(new sphere(vec3(165 * drand48(), 165 * drand48(), 165 * drand48()), 10, white), vec3(-100, 270, 395));
		boxlist2[j] = new sphere(vec3(165 * drand48(), 165 * drand48(), 165 * drand48()), 10, white);
	}
	list[l++] = new translate(new rotate_y(new bvh_node(boxlist2, ns, 0.0, 1.0), 15), vec3(-100, 270, 395));
	return new hitable_list(list, l);
}

bool isfinished = false;

int getpixel()
{
	return finishedPixel++;
}

void renderthread(hitable* world, hitable* hlist, camera *cam)
{
	while (finishedPixel < nx*ny)
	{
		g_lock.lock();
		int curpixel = finishedPixel++;
		if ((finishedPixel - 1) % ny == 0)
		{
			std::cout << "\r............" << (float)curpixel / (float)(nx*ny) * 100 << "%" << std::flush;
		}
		g_lock.unlock();
		vec3 col(0, 0, 0);
		int i = curpixel % ny;
		int j = ny - 1 - (curpixel / nx);
		for (int s = 0; s < ns; s++)
		{
			float u = float(i + drand48()) / float(nx);
			float v = float(j + drand48()) / float(ny);
			ray r = cam->get_ray(u, v);
			//vec3 p = r.point_at_parameter(2.0);
			col += de_nan(color(r, world, hlist, 0));
		}
		col /= float(ns);

		col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
		/*if (col.length() > 1)
			col.make_unit_vector();*/
		int ir = int(255.99 * col[0]);
		int ig = int(255.99 * col[1]);
		int ib = int(255.99 * col[2]);

		ir = ir > 255 ? 255 : ir;
		ig = ig > 255 ? 255 : ig;
		ib = ib > 255 ? 255 : ib;
		ir = ir < 0 ? 0 : ir;
		ig = ig < 0 ? 0 : ig;
		ib = ib < 0 ? 0 : ib;
		int index = curpixel;
		colors[index] = new int[3];
		colors[index][0] = ir;
		colors[index][1] = ig;
		colors[index][2] = ib;
	}
	g_lock.lock();
	if (!isfinished)
	{
		isfinished = true;
		for (int i = 0; i < nx*ny; i++)
		{
			outfile << colors[i][0] << " " << colors[i][1] << " " << colors[i][2] << "\n";
		}
	}
	g_lock.unlock();
}

int main()
{
#ifdef RaysBackgroundY
	//ofstream outfile("..\\results\\final.ppm", ios_base::out);
	outfile << "P3\n" << nx << " " << ny << "\n255\n";

	std::cout << "P3\n" << nx << " " << ny << "\n255\n";

	hitable *world;
	camera *cam;
	hitable *hlist;

	cornell_box(&world, &cam, &hlist, float(nx) / float(ny));
	//final(&world, &cam, float(nx) / float(ny));


	

	//final(&world, &cam, float(nx) / float(ny));
	int total = nx * ny;

	std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	int startmscout = ms.count();
	colors = new int*[ny*nx];
	
	thread t[thread_count];
	for (int i = 0; i < thread_count; i++)
	{
		t[i] = thread(renderthread, world, hlist, cam);
	}
	curfinishedcount = 0;
	for (int i = 0; i < thread_count; i++)
	{
		t[i].join();
	}
	
	std::cout << endl;
	ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	int endmscount = ms.count();
	std::cout << endmscount - startmscout<< "ms" << endl;

#endif // RaysBackgroundY

	_CrtDumpMemoryLeaks();
	return 0;
}

//inline float pdf(const vec3& p)
//{
//	return 1 / (4 * M_PI);
//}
//
//int main()
//{
//	int N = 1000000;
//	float sum = 0;
//	for (int i = 0; i < N; i++)
//	{
//		vec3 d = random_on_unit_sphere();
//		float cosine_squared = d.z() * d.z();
//		sum += cosine_squared / pdf(d);
//	}
//
//	std::cout << "I = " << sum / N << "\n";
//}