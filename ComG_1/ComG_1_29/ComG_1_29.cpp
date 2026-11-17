#define NOMINMAX
#include <windows.h>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <random>
#include <stdexcept>
#include "snow_accumulation/SnowAccumulation.h"
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
using namespace std;

// 화면·카메라 설정 (26번과 같은 orbit 카메라)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 50.0f
#define CAMERA_X 9.0f
#define CAMERA_Y 7.5f
#define CAMERA_Z 12.0f
#define CAMERA_MOVE_SPEED 2.0f
#define CAMERA_RADIUS_MIN 2.0f
#define CAMERA_RADIUS_MAX 30.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define ANIMATION_FPS 120
#define FLOOR_SIZE 12.0f
#define PYRAMID_SIZE 1.4f
#define LIGHT_ORBIT_SPEED 30.0f
#define LIGHT_MOVE_SPEED 1.0f
#define LIGHT_RADIUS_INITIAL 5.0f
#define LIGHT_RADIUS_MIN 2.8f
#define LIGHT_RADIUS_MAX 8.0f
#define LIGHT_ANGLE_INITIAL 45.0f
#define LIGHT_HEIGHT 5.0f
#define SUN_SIZE 0.45f
#define LIGHT_INTENSITY_INITIAL 1.2f
#define LIGHT_INTENSITY_MIN 0.0f
#define LIGHT_INTENSITY_MAX 4.0f
#define LIGHT_INTENSITY_STEP 0.1f
#define DIFFUSE_STRENGTH 0.85f
#define SPECULAR_STRENGTH 0.35f
#define SHININESS 32.0f
#define FILL_LIGHT_X 2.0f
#define FILL_LIGHT_Y 4.0f
#define FILL_LIGHT_Z 2.0f
#define FILL_LIGHT_INTENSITY 0.08f
// 세 행성은 다른 반지름·높이·속도로 공전한다.
#define MERCURY_RADIUS 1.7f
#define VENUS_RADIUS 2.8f
#define EARTH_RADIUS 4.0f
#define MERCURY_SIZE 0.20f
#define VENUS_SIZE 0.32f
#define EARTH_SIZE 0.40f
#define MERCURY_SPEED 42.0f
#define VENUS_SPEED 28.0f
#define EARTH_SPEED 18.0f
#define PLANET_HEIGHT 1.0f
#define ORBIT_SEGMENTS 180
// S는 새 눈의 생성만 켜고 끈다. 이미 생성된 눈은 계속 떨어진다.
#define SNOW_COUNT 350
#define SNOW_TOP 7.0f
#define SNOW_SIZE_MIN 0.055f
#define SNOW_SIZE_MAX 0.14f
#define SNOW_FALL_MIN 0.45f
#define SNOW_FALL_MAX 1.1f
#define SNOW_DRIFT_SPEED 0.32f
#define SNOW_TURN_MIN 25.0f
#define SNOW_TURN_MAX 100.0f
#define SNOW_DIRECTION_SECONDS 2.0f
#define SNOW_SPAWN_PER_SECOND 30.0f
// 정상 생성량으로 눈이 도착했을 때 한 단계 쌓이는 기준 시간
#define SNOW_ACCUMULATE_STAGE_SECONDS 6.0f
// 빛 세기 1에서 한 단계가 녹는 시간. 3단계 전체는 이 시간의 3배이다.
#define SNOW_MELT_STAGE_SECONDS 20.0f
#define SNOW_MELT_REFERENCE_INTENSITY 1.0f
#define SNOW_MELT_DARK_MULTIPLIER 0.20f
#define SNOW_MELT_MAX_MULTIPLIER 4.0f
#define SNOW_COVER_SEGMENTS 32
#define SNOW_COVER_UPDATE_FPS 30
// 받침 도형 크기는 유지하고, 쌓이는 눈의 두께만 늘린다.
#define SNOW_COVER_THICKNESS_SCALE 3.0f

struct Vertex { glm::vec3 position, normal; glm::vec2 uv; };
struct Mesh { int first = 0, count = 0; GLuint texture = 0; };
struct Snow { glm::vec3 position, velocity, targetVelocity; float size, speed, angle, turn, timer; bool active = false; };
Mesh planetMeshes[3], sunMesh, snowMesh;
Snow flakes[SNOW_COUNT];
float snowAmount = 0, spawnAccumulator = 0;
float coverDisplayedAmount = -1, coverUpdateAccumulator = 0;
GLuint coverVao = 0, coverVbo = 0;
vector<Vertex> coverStages[4], coverVertices;
float planetAngles[3] = { 0, 120, 240 };
const float planetRadii[] = { MERCURY_RADIUS, VENUS_RADIUS, EARTH_RADIUS };
const float planetSizes[] = { MERCURY_SIZE, VENUS_SIZE, EARTH_SIZE };
const float planetSpeeds[] = { MERCURY_SPEED, VENUS_SPEED, EARTH_SPEED };
bool snowing = false, dragging = false;
int lightOrbitDirection = 0;
float lightAngle = LIGHT_ANGLE_INITIAL, lightRadius = LIGHT_RADIUS_INITIAL;
float lightIntensity = LIGHT_INTENSITY_INITIAL;
mt19937 randomEngine(random_device{}());
double animationAccumulator = 0, previousMouseX = 0, previousMouseY = 0;
glm::vec3 cameraTarget(0), cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z);
float cameraRadius = glm::length(cameraPosition), cameraYaw = 0, cameraPitch = 0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation;
GLint lightingLocation, lightLocation, eyeLocation;
int floorFirst = 0, pyramidFirst = 0, orbitFirst = 0;
GLint textureLocation, intensityLocation;

string ReadFile(const char* name);
bool InitShader();
void InitBuffer();
Mesh LoadModel(const char* name, vector<Vertex>& vertices);
GLuint LoadTexture(const string& filename);
float Random(float minimum, float maximum);
void ResetSnow(Snow& flake);
void InitSnowCover();
void DrawSnowCover(float elapsed);
float SnowSurfaceHeight(float x, float z);
void DrawMesh(const Mesh& mesh, const glm::mat4& model, glm::vec3 color, bool lighting);
void DrawScene(int width, int height);
void UpdateAnimation(float elapsed, int distanceDirection);
glm::vec3 LightPosition();
void KeyCallback(GLFWwindow*, int key, int, int action, int mods);
void MouseCallback(GLFWwindow*, int button, int action, int);
void CursorCallback(GLFWwindow*, double x, double y);
void ScrollCallback(GLFWwindow*, double, double yoffset);

int main()
{
	HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-29 Snow and Planets", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	try { InitBuffer(); InitSnowCover(); }
	catch (const exception& error) { cerr << error.what() << endl; glfwDestroyWindow(window); glfwTerminate(); return -1; }
	glEnable(GL_DEPTH_TEST); glfwSwapInterval(1);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	cout << "S: snow emission on/off | R: sun orbit | N/F held: sun closer/farther | +/-: light intensity\n"
		<< "drag: orbit | wheel: zoom | arrows: camera XZ | Q: quit" << endl;
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double now = glfwGetTime(); float dt = (float)glm::min(now - previousTime, .1); previousTime = now;
		glm::vec3 direction(0);
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) direction.z -= 1;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) direction.z += 1;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) direction.x -= 1;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) direction.x += 1;
		if (glm::length(direction) > 0) {
			glm::vec3 movement = glm::normalize(direction) * CAMERA_MOVE_SPEED * dt;
			cameraPosition += movement; cameraTarget += movement;
		}
		int distanceDirection = (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS ? 1 : 0)
			- (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS ? 1 : 0);
		UpdateAnimation(dt, distanceDirection);
		int width, height; glfwGetFramebufferSize(window, &width, &height);
		DrawScene(width, height); glfwSwapBuffers(window);
	}
	for (const Mesh& mesh : planetMeshes) glDeleteTextures(1, &mesh.texture);
	glDeleteTextures(1, &sunMesh.texture); glDeleteTextures(1, &snowMesh.texture);
	glDeleteBuffers(1, &coverVbo); glDeleteVertexArrays(1, &coverVao);
	glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); if (SUCCEEDED(comResult)) CoUninitialize(); return 0;
}

// 모델의 정점/UV/노멀을 기존 VBO에 이어서 넣는다. 준비된 OBJ는 재질이 하나씩이다.
Mesh LoadModel(const char* name, vector<Vertex>& vertices)
{
	Mesh mesh; mesh.first = (int)vertices.size();
	string stem(name), line;
	ifstream material(stem + ".mtl"), file(stem + ".obj");
	if (!material || !file) throw runtime_error("Cannot open model: " + stem);
	while (getline(material, line)) {
		istringstream input(line); string type, path; input >> type;
		if (type == "map_Kd") { input >> path; mesh.texture = LoadTexture(path); }
	}
	vector<glm::vec3> positions, normals; vector<glm::vec2> uvs;
	while (getline(file, line)) {
		istringstream input(line); string type; input >> type;
		if (type == "v") { glm::vec3 p; input >> p.x >> p.y >> p.z; positions.push_back(p); }
		else if (type == "vn") { glm::vec3 n; input >> n.x >> n.y >> n.z; normals.push_back(n); }
		else if (type == "vt") { glm::vec2 uv; input >> uv.x >> uv.y; uvs.push_back(uv); }
		else if (type == "f") {
			vector<Vertex> face; string token;
			while (input >> token) {
				size_t first = token.find('/'), last = token.rfind('/');
				int p = stoi(token.substr(0, first)), uv = stoi(token.substr(first + 1, last - first - 1)), n = stoi(token.substr(last + 1));
				face.push_back({ positions.at(p - 1), normals.at(n - 1), uvs.at(uv - 1) });
			}
			for (size_t i = 1; i + 1 < face.size(); ++i) {
				vertices.push_back(face[0]); vertices.push_back(face[i]); vertices.push_back(face[i + 1]);
			}
		}
	}
	mesh.count = (int)vertices.size() - mesh.first;
	if (!mesh.count || !mesh.texture) throw runtime_error("Missing mesh/texture: " + stem);
	return mesh;
}

// Windows 기본 이미지 디코더로 PNG/JPG를 읽는다. 별도 이미지 라이브러리 설치는 필요 없다.
GLuint LoadTexture(const string& filename)
{
	using Microsoft::WRL::ComPtr;
	ComPtr<IWICImagingFactory> factory; ComPtr<IWICBitmapDecoder> decoder;
	ComPtr<IWICBitmapFrameDecode> frame; ComPtr<IWICFormatConverter> converter;
	wstring path(filename.begin(), filename.end());
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))
		|| FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder))
		|| FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter))
		|| FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)))
		throw runtime_error("Cannot decode texture: " + filename);
	UINT width, height; converter->GetSize(&width, &height);
	vector<unsigned char> pixels((size_t)width * height * 4), flipped(pixels.size());
	if (FAILED(converter->CopyPixels(nullptr, width * 4, (UINT)pixels.size(), pixels.data())))
		throw runtime_error("Cannot read texture: " + filename);
	for (UINT y = 0; y < height; ++y)
		copy_n(pixels.data() + (size_t)y * width * 4, width * 4, flipped.data() + (size_t)(height - 1 - y) * width * 4);
	GLuint texture; glGenTextures(1, &texture); glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return texture;
}

void InitBuffer()
{
	vector<Vertex> vertices;
	auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
		vertices.push_back({ a,normal,{} }); vertices.push_back({ b,normal,{} }); vertices.push_back({ c,normal,{} });
	};
	triangle({-1,0,-1},{-1,0,1},{1,0,1}); triangle({-1,0,-1},{1,0,1},{1,0,-1});
	pyramidFirst = (int)vertices.size();
	glm::vec3 base[] = { {-.5f,0,-.5f},{.5f,0,-.5f},{.5f,0,.5f},{-.5f,0,.5f} };
	for (int i = 0; i < 4; ++i) triangle(base[(i + 1) % 4], base[i], glm::vec3(0, 1, 0));
	triangle(base[0],base[1],base[2]); triangle(base[0],base[2],base[3]);
	orbitFirst = (int)vertices.size();
	for (int i = 0; i < ORBIT_SEGMENTS; ++i) {
		float a = glm::two_pi<float>() * i / ORBIT_SEGMENTS;
		vertices.push_back({ {glm::sin(a),0,glm::cos(a)},{},{} });
	}
	planetMeshes[0] = LoadModel("mercury", vertices); planetMeshes[1] = LoadModel("venus", vertices);
	planetMeshes[2] = LoadModel("earth", vertices); sunMesh = LoadModel("sun", vertices); snowMesh = LoadModel("snowflake", vertices);
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position)); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal)); glEnableVertexAttribArray(1);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv)); glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

// 현재 사각뿔 높이와 눈 덮개의 최대 두께를 포함한 낙하 종료 높이이다.
float SnowSurfaceHeight(float x, float z)
{
	float base = snow::PyramidHeight(x,z,PYRAMID_SIZE,PYRAMID_SIZE,PYRAMID_SIZE);
	float depth = 0;
	if (snowAmount > 0) {
		int upper = (int)glm::ceil(snowAmount), lower = upper - 1;
		float low = lower == 0 ? 0 : snow::kStages[lower - 1].maximumDepth;
		depth = glm::mix(low,snow::kStages[upper - 1].maximumDepth,snowAmount - lower);
	}
	return base + depth * SNOW_COVER_THICKNESS_SCALE;
}

// 준비된 가이드의 Build로 현재 바닥과 사각뿔에 맞는 3단계 덮개를 생성한다.
// 샘플 OBJ(4x4, 피라미드 높이 1.6)를 늘리지 않으므로 실제 크기에 정확히 맞는다.
void InitSnowCover()
{
	for (int stage = 0; stage <= 3; ++stage) {
		for (int surface = 0; surface < 2; ++surface) {
			snow::Settings settings;
			settings.width = settings.depth = surface ? PYRAMID_SIZE : FLOOR_SIZE;
			settings.segments = SNOW_COVER_SEGMENTS;
			auto height = [surface](float x, float z) {
				return surface ? snow::PyramidHeight(x,z,PYRAMID_SIZE,PYRAMID_SIZE,PYRAMID_SIZE) : 0.0f;
			};
			// 높이를 먼저 나눈 표면에서 생성한 뒤 Y를 확대하면 눈 두께만 배율이 적용된다.
			settings.undersideInset /= SNOW_COVER_THICKNESS_SCALE;
			auto buildHeight = [&](float x, float z) { return height(x,z) / SNOW_COVER_THICKNESS_SCALE; };
			snow::Mesh mesh = snow::Build(settings,buildHeight,stage ? (float)stage : .00001f);
			for (auto index : mesh.indices) {
				const auto& v = mesh.vertices[index];
				glm::vec3 p(v.position.x,v.position.y * SNOW_COVER_THICKNESS_SCALE,v.position.z);
				glm::vec3 normal = glm::normalize(glm::vec3(v.normal.x,
					v.normal.y / SNOW_COVER_THICKNESS_SCALE,v.normal.z));
				if (stage == 0) {
					// 0단계도 정점 연결을 보존해 1단계와 보간할 수 있게 한다.
					float base = height(p.x,p.z);
					if (p.y >= base) p.y = base;
				}
				coverStages[stage].push_back({p,normal,{v.u,v.v}});
			}
		}
	}
	coverVertices.resize(coverStages[0].size());
	glGenVertexArrays(1,&coverVao); glGenBuffers(1,&coverVbo);
	glBindVertexArray(coverVao); glBindBuffer(GL_ARRAY_BUFFER,coverVbo);
	glBufferData(GL_ARRAY_BUFFER,coverVertices.size()*sizeof(Vertex),nullptr,GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position)); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal)); glEnableVertexAttribArray(1);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,uv)); glEnableVertexAttribArray(2);
	glBindVertexArray(0);
}

// 0~3의 연속 쌓임량으로 인접 단계의 정점·노멀을 보간한다. 세 겹을 동시에 그리지 않는다.
void DrawSnowCover(float elapsed)
{
	if (snowAmount <= .0001f) return;
	if (coverDisplayedAmount < 0 || (elapsed >= 1.0f/SNOW_COVER_UPDATE_FPS && coverDisplayedAmount != snowAmount)) {
		int lower = glm::min((int)snowAmount,2), upper = lower + 1;
		float t = snowAmount - lower;
		for (size_t i = 0; i < coverVertices.size(); ++i) {
			coverVertices[i].position = glm::mix(coverStages[lower][i].position,coverStages[upper][i].position,t);
			coverVertices[i].normal = glm::normalize(glm::mix(coverStages[lower][i].normal,coverStages[upper][i].normal,t));
			coverVertices[i].uv = coverStages[lower][i].uv;
		}
		glBindBuffer(GL_ARRAY_BUFFER,coverVbo);
		glBufferSubData(GL_ARRAY_BUFFER,0,coverVertices.size()*sizeof(Vertex),coverVertices.data());
		coverDisplayedAmount = snowAmount; coverUpdateAccumulator = 0;
	}
	glm::mat4 model(1);
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
	glUniform3f(colorLocation,.92f,.96f,1); glUniform1i(lightingLocation,true); glUniform1i(textureLocation,false);
	glBindVertexArray(coverVao); glDrawArrays(GL_TRIANGLES,0,(GLsizei)coverVertices.size());
	glBindVertexArray(vao);
}

float Random(float minimum, float maximum)
{
	return uniform_real_distribution<float>(minimum, maximum)(randomEngine);
}

void ResetSnow(Snow& flake)
{
	flake.active = true;
	flake.size = Random(SNOW_SIZE_MIN, SNOW_SIZE_MAX);
	flake.position = glm::vec3(Random(-FLOOR_SIZE*.5f,FLOOR_SIZE*.5f),
		SNOW_TOP, Random(-FLOOR_SIZE*.5f,FLOOR_SIZE*.5f));
	flake.velocity = flake.targetVelocity = glm::vec3(Random(-SNOW_DRIFT_SPEED,SNOW_DRIFT_SPEED),0,Random(-SNOW_DRIFT_SPEED,SNOW_DRIFT_SPEED));
	flake.speed = Random(SNOW_FALL_MIN,SNOW_FALL_MAX); flake.angle = Random(0,360);
	flake.turn = Random(SNOW_TURN_MIN,SNOW_TURN_MAX) * (Random(0,1) < .5f ? -1 : 1);
	flake.timer = Random(0,SNOW_DIRECTION_SECONDS);
}

void UpdateAnimation(float elapsed, int distanceDirection)
{
	animationAccumulator += elapsed;
	const float step = 1.0f / ANIMATION_FPS;
	while (animationAccumulator >= step) {
		animationAccumulator -= step;
		for (int i = 0; i < 3; ++i) planetAngles[i] = glm::mod(planetAngles[i] + planetSpeeds[i] * step, 360.0f);
		lightAngle = glm::mod(lightAngle + lightOrbitDirection * LIGHT_ORBIT_SPEED * step,360.0f);
		lightRadius = glm::clamp(lightRadius + distanceDirection * LIGHT_MOVE_SPEED * step,LIGHT_RADIUS_MIN,LIGHT_RADIUS_MAX);
		coverUpdateAccumulator += step;
		if (snowing) {
			spawnAccumulator += step * SNOW_SPAWN_PER_SECOND;
			while (spawnAccumulator >= 1) {
				spawnAccumulator -= 1;
				for (Snow& flake : flakes)
					if (!flake.active) { ResetSnow(flake); break; }
			}
		}
		else {
			// 태양을 꺼도 약하게 녹고, 기준 밝기 1에서는 정확히 기준 시간대로 녹는다.
			float brightness = glm::clamp(lightIntensity / SNOW_MELT_REFERENCE_INTENSITY, 0.0f, 1.0f);
			float multiplier = glm::mix(SNOW_MELT_DARK_MULTIPLIER, 1.0f, brightness);
			multiplier += glm::max(lightIntensity / SNOW_MELT_REFERENCE_INTENSITY - 1.0f, 0.0f);
			multiplier = glm::min(multiplier, SNOW_MELT_MAX_MULTIPLIER);
			snowAmount = glm::max(0.0f, snowAmount - step * multiplier / SNOW_MELT_STAGE_SECONDS);
		}
		for (Snow& flake : flakes) {
			if (!flake.active) continue;
			flake.timer -= step;
			if (flake.timer <= 0) {
				flake.targetVelocity = glm::vec3(Random(-SNOW_DRIFT_SPEED,SNOW_DRIFT_SPEED),0,Random(-SNOW_DRIFT_SPEED,SNOW_DRIFT_SPEED));
				flake.timer = Random(SNOW_DIRECTION_SECONDS*.5f,SNOW_DIRECTION_SECONDS*1.5f);
			}
			flake.velocity = glm::mix(flake.velocity,flake.targetVelocity,step);
			flake.position += (flake.velocity + glm::vec3(0,-flake.speed,0)) * step;
			flake.angle = glm::mod(flake.angle + flake.turn * step,360.0f);
			float half = FLOOR_SIZE * .5f;
			for (int axis : {0,2}) {
				if (flake.position[axis] > half) flake.position[axis] -= FLOOR_SIZE;
				if (flake.position[axis] < -half) flake.position[axis] += FLOOR_SIZE;
			}
			if (flake.position.y <= SnowSurfaceHeight(flake.position.x, flake.position.z) + flake.size*.5f) {
				flake.active = false;
				// 생성 정지 후 남아 떨어지는 눈은 사라지며, 새 쌓임은 진행하지 않는다.
				if (snowing) snowAmount = glm::min(3.0f, snowAmount
					+ 1.0f / (SNOW_SPAWN_PER_SECOND * SNOW_ACCUMULATE_STAGE_SECONDS));
			}
		}
	}
}

glm::vec3 LightPosition()
{
	float angle = glm::radians(lightAngle);
	// 수평 거리와 높이를 같은 비율로 바꿔 원점 방향의 직선으로 이동한다.
	float height = LIGHT_HEIGHT * lightRadius / LIGHT_RADIUS_INITIAL;
	return glm::vec3(lightRadius * glm::sin(angle), height, lightRadius * glm::cos(angle));
}

// 모델 좌표와 색상/텍스처를 지정하고 그리기는 셰이더에 맡긴다.
void DrawMesh(const Mesh& mesh, const glm::mat4& model, glm::vec3 color, bool lighting)
{
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
	glUniform3fv(colorLocation,1,glm::value_ptr(color)); glUniform1i(lightingLocation,lighting);
	glUniform1i(textureLocation,mesh.texture != 0); glBindTexture(GL_TEXTURE_2D,mesh.texture);
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,mesh.first,mesh.count);
}

void DrawScene(int width, int height)
{
	if (width <= 0 || height <= 0) return;
	glViewport(0,0,width,height); glClearColor(.025f,.035f,.06f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glUseProgram(programID);
	glm::mat4 view = glm::lookAt(cameraPosition,cameraTarget,glm::vec3(0,1,0));
	glm::mat4 projection = glm::perspective(glm::radians(PERSPECTIVE_FOVY),(float)width/height,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view)); glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
	glm::vec3 light = LightPosition(); glUniform3fv(lightLocation,1,glm::value_ptr(light));
	glUniform3fv(eyeLocation,1,glm::value_ptr(cameraPosition)); glUniform1f(intensityLocation,lightIntensity);
	DrawMesh({floorFirst,6,0},glm::scale(glm::mat4(1),glm::vec3(FLOOR_SIZE*.5f,1,FLOOR_SIZE*.5f)),{.32f,.35f,.40f},true);
	DrawMesh({pyramidFirst,18,0},glm::scale(glm::mat4(1),glm::vec3(PYRAMID_SIZE)),{.1f,.8f,.85f},true);
	DrawSnowCover(coverUpdateAccumulator);
	for (int i = 0; i < 3; ++i) {
		float angle = glm::radians(planetAngles[i]);
		glm::vec3 center(planetRadii[i]*glm::sin(angle),PLANET_HEIGHT+i*.25f,planetRadii[i]*glm::cos(angle));
		glm::mat4 model = glm::scale(glm::translate(glm::mat4(1),center),glm::vec3(planetSizes[i]));
		DrawMesh(planetMeshes[i],model,glm::vec3(1),true);
		// 공전 궤도는 고유 색으로 표시한다.
		model = glm::scale(glm::translate(glm::mat4(1),glm::vec3(0,center.y,0)),glm::vec3(planetRadii[i]));
		glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
		glUniform1i(lightingLocation,false); glUniform1i(textureLocation,false);
		glUniform3f(colorLocation,.16f,.20f,.27f); glDrawArrays(GL_LINE_LOOP,orbitFirst,ORBIT_SEGMENTS);
	}
	// 태양은 흰 점광원의 위치 표시이며, 표면 텍스처는 조명 계산 없이 출력한다.
	DrawMesh(sunMesh,glm::scale(glm::translate(glm::mat4(1),light),glm::vec3(SUN_SIZE)),glm::vec3(1),false);
	for (const Snow& flake : flakes) {
		if (!flake.active) continue;
		glm::mat4 model = glm::translate(glm::mat4(1),flake.position);
		model = glm::rotate(model,glm::radians(flake.angle),glm::vec3(0,1,0));
		DrawMesh(snowMesh,glm::scale(model,glm::vec3(flake.size)),glm::vec3(1),true);
	}
	glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
	if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
		lightIntensity = glm::clamp(lightIntensity + LIGHT_INTENSITY_STEP,LIGHT_INTENSITY_MIN,LIGHT_INTENSITY_MAX);
	if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
		lightIntensity = glm::clamp(lightIntensity - LIGHT_INTENSITY_STEP,LIGHT_INTENSITY_MIN,LIGHT_INTENSITY_MAX);
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window,true);
	if (key == GLFW_KEY_S) { snowing = !snowing; spawnAccumulator = 0; }
	if (key == GLFW_KEY_R) lightOrbitDirection = lightOrbitDirection ? 0 : 1;
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;
	if (action == GLFW_PRESS) {
		dragging = true; glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
		cameraYaw = glm::atan(cameraPosition.x - cameraTarget.x, cameraPosition.z - cameraTarget.z);
		cameraPitch = glm::asin((cameraPosition.y - cameraTarget.y) / cameraRadius);
	}
	else if (action == GLFW_RELEASE) {
		dragging = false;
		// 버튼을 놓아도 현재 카메라 위치와 시선을 유지한다.
	}
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
	cameraRadius = glm::clamp(cameraRadius - (float)yoffset * ZOOM_STEP, CAMERA_RADIUS_MIN, CAMERA_RADIUS_MAX);
	cameraPosition = cameraTarget + glm::normalize(cameraPosition - cameraTarget) * cameraRadius;
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (!dragging) return;
	int width, height; glfwGetWindowSize(window, &width, &height); if (width <= 0 || height <= 0) return;
	cameraYaw -= glm::radians(ORBIT_DEGREES) * (float)(x - previousMouseX) / width;
	cameraPitch += glm::radians(ORBIT_DEGREES) * (float)(y - previousMouseY) / height;
	previousMouseX = x; previousMouseY = y;
	cameraYaw = glm::mod(cameraYaw, glm::two_pi<float>());
	cameraPitch = glm::clamp(cameraPitch, -glm::radians(ORBIT_PITCH_LIMIT), glm::radians(ORBIT_PITCH_LIMIT));
	cameraPosition = cameraTarget + cameraRadius * glm::vec3(glm::cos(cameraPitch) * glm::sin(cameraYaw), glm::sin(cameraPitch), glm::cos(cameraPitch) * glm::cos(cameraYaw));
}

string ReadFile(const char* name)
{
	ifstream file(name); if (!file) { cerr << "Cannot open shader: " << name << endl; return ""; }
	stringstream stream; stream << file.rdbuf(); return stream.str();
}

bool InitShader()
{
	string sources[] = { ReadFile("vertex.glsl"),ReadFile("fragment.glsl") };
	if (sources[0].empty() || sources[1].empty()) return false;
	GLuint shaders[] = { glCreateShader(GL_VERTEX_SHADER),glCreateShader(GL_FRAGMENT_SHADER) };
	for (int i = 0; i < 2; ++i) {
		const char* code = sources[i].c_str(); glShaderSource(shaders[i], 1, &code, nullptr); glCompileShader(shaders[i]);
		GLint ok; glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
		if (!ok) { char log[2048]; glGetShaderInfoLog(shaders[i], 2048, nullptr, log); cerr << log << endl; return false; }
	}
	programID = glCreateProgram(); for (GLuint shader : shaders) glAttachShader(programID, shader);
	glLinkProgram(programID); GLint ok; glGetProgramiv(programID, GL_LINK_STATUS, &ok);
	for (GLuint shader : shaders) glDeleteShader(shader);
	if (!ok) { char log[2048]; glGetProgramInfoLog(programID, 2048, nullptr, log); cerr << log << endl; return false; }
	modelLocation = glGetUniformLocation(programID, "modelTransform");
	viewLocation = glGetUniformLocation(programID, "viewTransform");
	projectionLocation = glGetUniformLocation(programID, "projectionTransform");
	colorLocation = glGetUniformLocation(programID, "uColor");
	lightingLocation = glGetUniformLocation(programID, "useLighting");
	lightLocation = glGetUniformLocation(programID, "lightPosition");
	eyeLocation = glGetUniformLocation(programID, "eyePosition");
	glUseProgram(programID);
	textureLocation = glGetUniformLocation(programID, "useTexture");
	intensityLocation = glGetUniformLocation(programID, "lightIntensity");
	glUniform1i(glGetUniformLocation(programID, "surfaceTexture"),0);
	glUniform1f(glGetUniformLocation(programID, "diffuseStrength"), DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "specularStrength"), SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "shininess"), SHININESS);
	glUniform3f(glGetUniformLocation(programID,"fillLightPosition"),FILL_LIGHT_X,FILL_LIGHT_Y,FILL_LIGHT_Z);
	glUniform1f(glGetUniformLocation(programID,"fillLightIntensity"),FILL_LIGHT_INTENSITY);
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0
		&& lightingLocation >= 0 && lightLocation >= 0 && eyeLocation >= 0;
}
