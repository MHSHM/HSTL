#include "Array.h"
#include "File.h"
#include "Log.h"
#include "Str.h"
#include "Hash_Set.h"
#include "Result.h"
#include "Memory.h"

#include <cmath>
#include <cstdint>
#include <chrono>

using namespace hstl;

struct Vec3 {
	float x, y, z;

	bool operator==(const Vec3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}
};

struct Vec3Hash {
	size_t operator()(const Vec3& v) const {
		size_t h1 = std::hash<float>{}(v.x);
		size_t h2 = std::hash<float>{}(v.y);
		size_t h3 = std::hash<float>{}(v.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

struct Face {
	int v1, v2, v3;
};

struct Mesh {
	Array<Vec3> vertices;
	Array<Face> faces;
	Str name;

	Mesh(Allocator* alloc)
		: vertices(alloc), faces(alloc), name(alloc) {
	}

	Mesh(Mesh&&) = default;
	Mesh& operator=(Mesh&&) = default;
};

struct Pixel {
	uint8_t b, g, r;
};

class Framebuffer {
public:
	int width, height;
	Array<Pixel> pixels;

	Framebuffer(int w, int h, Allocator* alloc)
		: width(w), height(h), pixels(alloc)
	{
		pixels.resize(w * h);
		clear();
	}

	void clear() {
		memset(pixels.begin(), 0, pixels.count() * sizeof(Pixel));
	}

	void set_pixel(int x, int y, Pixel color) {
		if (x >= 0 && x < width && y >= 0 && y < height) {
			pixels[y * width + x] = color;
		}
	}
};

Result<Mesh> load_obj(const char* filepath, Allocator* alloc) {
	auto file_res = File::open(filepath, File::FILE_OPEN_MODE::READ);
	if (!file_res) return Err("File not found");

	File& file = file_res.get_value();

	auto content_res = file.read_all(alloc);
	if (!content_res) return Err("Read failed");

	Array<uint8_t>& data = content_res.get_value();
	Str_View content((const char*)data.buffer(), data.count());

	Array<Str_View> lines = content.split('\n', alloc);

	Mesh mesh(alloc);
	mesh.name = Str(filepath, alloc);

	Hash_Set<Vec3, Vec3Hash> unique_vertices(alloc);

	log_info("Parsing '{}'...", filepath);

	for (size_t i = 0; i < lines.count(); ++i) {
		Str_View line = lines[i];
		if (line.count() == 0 || line[0] == '#') continue;

		Array<Str_View> parts = line.split(' ', alloc);

		Array<Str_View> tokens(alloc);
		for (size_t t = 0; t < parts.count(); ++t) {
			if (parts[t].count() > 0) tokens.push(parts[t]);
		}

		if (tokens.count() == 0) continue;

		if (tokens[0] == "v" && tokens.count() >= 4) {
			char b[3][32];
			for (int k = 0; k < 3; k++) {
				size_t len = tokens[k + 1].count() < 31 ? tokens[k + 1].count() : 31;
				memcpy(b[k], tokens[k + 1].data(), len);
				b[k][len] = '\0';
			}

			Vec3 v = { (float)atof(b[0]), (float)atof(b[1]), (float)atof(b[2]) };
			mesh.vertices.push(v);
			unique_vertices.insert(v);
		}
		else if (tokens[0] == "f" && tokens.count() >= 4) {
			char b[3][32];
			for (int k = 0; k < 3; k++) {
				Str_View t = tokens[k + 1];
				size_t len = 0;
				while (len < t.count() && t[len] != '/' && len < 31) len++;
				memcpy(b[k], t.data(), len);
				b[k][len] = '\0';
			}
			mesh.faces.push({ atoi(b[0]) - 1, atoi(b[1]) - 1, atoi(b[2]) - 1 });
		}
	}

	log_info("Loaded Mesh Stats:");
	log_info(" - Total Vertices: {}", mesh.vertices.count());
	log_info(" - Unique Vertices: {} (Calculated via Hash_Set)", unique_vertices.count());
	log_info(" - Total Faces:     {}", mesh.faces.count());

	return mesh;
}

void draw_line(Framebuffer& fb, int x0, int y0, int x1, int y1, Pixel color) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;
	while (true) {
		fb.set_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

struct Point2D { int x, y; bool valid; };

Point2D project(const Vec3& v, int width, int height) {
	float fov = 1.0f, dist = 4.0f;
	float theta = 0.5f;
	float rx = v.x * cos(theta) - v.z * sin(theta);
	float rz = v.x * sin(theta) + v.z * cos(theta) + dist;
	float ry = v.y;

	if (rz <= 0.1f) return { 0,0, false };

	int sx = (int)(((rx * fov / rz) + 0.5f) * width);
	int sy = (int)(((-ry * fov / rz) + 0.5f) * height);
	return { sx, sy, true };
}

void render_mesh(Framebuffer& fb, const Mesh* mesh) {
	if (!mesh) return;
	Pixel color = { 0, 255, 0 };
	for (size_t i = 0; i < mesh->faces.count(); ++i) {
		Face f = mesh->faces[i];
		Point2D p1 = project(mesh->vertices[f.v1], fb.width, fb.height);
		Point2D p2 = project(mesh->vertices[f.v2], fb.width, fb.height);
		Point2D p3 = project(mesh->vertices[f.v3], fb.width, fb.height);

		if (p1.valid && p2.valid) draw_line(fb, p1.x, p1.y, p2.x, p2.y, color);
		if (p2.valid && p3.valid) draw_line(fb, p2.x, p2.y, p3.x, p3.y, color);
		if (p3.valid && p1.valid) draw_line(fb, p3.x, p3.y, p1.x, p1.y, color);
	}
}

void save_bmp(const char* filename, Framebuffer& fb) {
#pragma pack(push, 1)
	struct { uint16_t t = 0x4D42; uint32_t s = 0; uint16_t r1 = 0; uint16_t r2 = 0; uint32_t o = 54; } fh;
	struct { uint32_t s = 40; int32_t w = 0, h = 0; uint16_t p = 1, b = 24; uint32_t c = 0, is = 0, x = 0, y = 0, cl = 0, cli = 0; } ih;
#pragma pack(pop)

	int pad = (4 - (fb.width * 3) % 4) % 4;
	fh.s = 54 + (fb.width * 3 + pad) * fb.height;
	ih.w = fb.width; ih.h = fb.height; ih.is = fh.s - 54;

	auto res = File::open(filename, File::FILE_OPEN_MODE::WRITE_OVERWRITE);
	if (!res) return;
	res.get_value().write(&fh, sizeof(fh));
	res.get_value().write(&ih, sizeof(ih));

	uint8_t zeros[3] = { 0 };
	for (int y = 0; y < fb.height; ++y) {
		for (int x = 0; x < fb.width; ++x) res.get_value().write(&fb.pixels[y * fb.width + x], 3);
		res.get_value().write(zeros, pad);
	}
}

int main() {
	Linear_Allocator main_allocator(1024 * 1024 * 128);

	auto start = std::chrono::high_resolution_clock::now();

	Framebuffer fb(1920, 1080, &main_allocator);

	auto mesh_res = load_obj("bunny.obj", &main_allocator);

	if (!mesh_res) {
		log_error("Load Failed: {}", mesh_res.get_err().get_message());
		return -1;
	}

	Mesh mesh = std::move(mesh_res.get_value());

	render_mesh(fb, &mesh);
	auto end = std::chrono::high_resolution_clock::now();

	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	log_info("Render time: {} ms", ms);

	save_bmp("wireframe.bmp", fb);

	return 0;
}