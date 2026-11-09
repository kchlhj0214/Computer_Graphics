#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glu.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
using namespace std;

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define AXIS_LENGTH 1.0f
#define CONE_RADIUS 0.0225f
#define CONE_HEIGHT (CONE_RADIUS * 2.0f)
#define CONE_SLICES 24
#define CONE_STACKS 16
#define PERSPECTIVE_FOVY 27.0f
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 30.0f
#define CAMERA_X 3.0f
#define CAMERA_Y 2.4f
#define CAMERA_Z 4.0f
#define CAMERA_MOVE_SPEED 1.0f
#define CAMERA_RADIUS_MIN 1.50f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define OBJECT_SIZE 0.65f
#define OBJECT_ROTATE_X 0.0f
#define OBJECT_ROTATE_Y 0.0f
#define OBJECT_RED 0.10f
#define OBJECT_GREEN 0.80f
#define OBJECT_BLUE 0.15f
#define ANIMATION_FPS 120
#define OBJECT_ROTATE_SPEED 60.0f
#define LIGHT_ORBIT_SPEED 45.0f
#define LIGHT_MOVE_SPEED 0.50f
#define LIGHT_RADIUS_INITIAL 1.15f
#define LIGHT_RADIUS_MIN 0.70f
#define LIGHT_RADIUS_MAX 2.20f
#define LIGHT_ANGLE_INITIAL 0.0f
#define LIGHT_HEIGHT 0.0f
#define LIGHT_MARKER_SIZE 0.09f
#define LIGHT_ORBIT_SEGMENTS 180
#define DIFFUSE_STRENGTH 0.85f
#define SPECULAR_STRENGTH 0.45f
#define SHININESS 32.0f

struct Vertex { glm::vec3 position, normal; };
enum ObjectType { CUBE, PYRAMID };
ObjectType objectType = CUBE;
bool lightEnabled = true, objectSpinning = false, dragging = false;
int lightOrbitDirection = 0;
float objectAngle = 0, lightAngle = LIGHT_ANGLE_INITIAL, lightRadius = LIGHT_RADIUS_INITIAL;
double animationAccumulator = 0, previousMouseX = 0, previousMouseY = 0;
glm::vec3 cameraTarget(0), cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z);
float cameraRadius = glm::length(cameraPosition), cameraYaw = 0, cameraPitch = 0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation;
GLint lightingLocation, lightLocation, eyeLocation;
int cubeFirst = 0, pyramidFirst = 0, axesFirst = 0, orbitFirst = 0;
GLUquadric* qobj = nullptr;

string ReadFile(const char* name);
bool InitShader();
void InitBuffer();
void DrawScene(int width, int height);
void UpdateAnimation(float elapsed, int distanceDirection);
glm::vec3 LightPosition();
void KeyCallback(GLFWwindow*, int key, int, int action, int mods);
void MouseCallback(GLFWwindow*, int button, int action, int);
void CursorCallback(GLFWwindow*, double x, double y);
void ScrollCallback(GLFWwindow*, double, double yoffset);

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-26 Object Lighting", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	InitBuffer();
	qobj = gluNewQuadric();
	if (!qobj) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	gluQuadricDrawStyle(qobj, GLU_FILL); gluQuadricNormals(qobj, GLU_NONE);
	gluQuadricOrientation(qobj, GLU_OUTSIDE);
	glEnable(GL_DEPTH_TEST); glfwSwapInterval(1);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	cout << "N: cube/pyramid | M: light on/off | Y: spin toggle | r/R: light orbit +/- toggle\n"
		<< "z/Z held: light closer/farther | drag: orbit | wheel: zoom | arrows: camera XZ | Q: quit" << endl;
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
		bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		int distanceDirection = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS ? (shift ? 1 : -1) : 0;
		UpdateAnimation(dt, distanceDirection);
		int width, height; glfwGetFramebufferSize(window, &width, &height);
		DrawScene(width, height); glfwSwapBuffers(window);
	}
	gluDeleteQuadric(qobj); glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

void InitBuffer()
{
	vector<Vertex> vertices;
	auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
		// 원점을 내부에 가진 닫힌 도형이므로 면 중심 방향과 비교해 노멀을 바깥쪽으로 맞춥니다.
		if (glm::dot(normal, a + b + c) < 0) { auto temp = b; b = c; c = temp; normal = -normal; }
		vertices.push_back({ a,normal }); vertices.push_back({ b,normal }); vertices.push_back({ c,normal });
	};
	glm::vec3 corners[] = { {-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
		{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f} };
	int indices[] = { 0,2,1,0,3,2,4,5,6,4,6,7,0,4,7,0,7,3,1,2,6,1,6,5,3,7,6,3,6,2,0,1,5,0,5,4 };
	for (int i = 0; i < 36; i += 3) triangle(corners[indices[i]], corners[indices[i + 1]], corners[indices[i + 2]]);
	pyramidFirst = (int)vertices.size();
	glm::vec3 base[] = { {-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f},{-.5f,-.5f,.5f} };
	for (int i = 0; i < 4; ++i) triangle(base[i], base[(i + 1) % 4], glm::vec3(0, .5f, 0));
	triangle(base[0], base[1], base[2]); triangle(base[0], base[2], base[3]);
	axesFirst = (int)vertices.size();
	for (int i = 0; i < 3; ++i) {
		glm::vec3 a(0), b(0); a[i] = -AXIS_LENGTH; b[i] = AXIS_LENGTH;
		vertices.push_back({ a,glm::vec3(0) }); vertices.push_back({ b,glm::vec3(0) });
	}
	orbitFirst = (int)vertices.size();
	for (int i = 0; i < LIGHT_ORBIT_SEGMENTS; ++i) {
		float angle = glm::two_pi<float>() * i / LIGHT_ORBIT_SEGMENTS;
		vertices.push_back({ {glm::sin(angle),0,glm::cos(angle)},glm::vec3(0) });
	}
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void UpdateAnimation(float elapsed, int distanceDirection)
{
	animationAccumulator += elapsed;
	const float step = 1.0f / ANIMATION_FPS;
	while (animationAccumulator >= step) {
		animationAccumulator -= step;
		if (objectSpinning) objectAngle = glm::mod(objectAngle + OBJECT_ROTATE_SPEED * step, 360.0f);
		lightAngle = glm::mod(lightAngle + lightOrbitDirection * LIGHT_ORBIT_SPEED * step, 360.0f);
		// 회전한 육면체의 외접구보다 큰 반지름을 유지하여 광원이 내부로 들어가지 않게 합니다.
		float minimum = glm::max(LIGHT_RADIUS_MIN, OBJECT_SIZE * glm::sqrt(3.0f) * .5f + LIGHT_MARKER_SIZE);
		lightRadius = glm::clamp(lightRadius + distanceDirection * LIGHT_MOVE_SPEED * step, minimum, LIGHT_RADIUS_MAX);
	}
}

glm::vec3 LightPosition()
{
	float angle = glm::radians(lightAngle);
	return glm::vec3(lightRadius * glm::sin(angle), LIGHT_HEIGHT, lightRadius * glm::cos(angle));
}

void DrawScene(int width, int height)
{
	if (width <= 0 || height <= 0) return;
	glViewport(0, 0, width, height); glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(programID);
	glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));
	glm::mat4 projection = glm::perspective(glm::radians(PERSPECTIVE_FOVY), (float)width / height, PERSPECTIVE_NEAR, PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	glm::vec3 light = LightPosition();
	glUniform3fv(lightLocation, 1, glm::value_ptr(light)); glUniform3fv(eyeLocation, 1, glm::value_ptr(cameraPosition));
	glm::mat4 model = glm::rotate(glm::mat4(1), glm::radians(objectAngle), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(OBJECT_ROTATE_Y), glm::vec3(0, 1, 0));
	model = glm::rotate(model, glm::radians(OBJECT_ROTATE_X), glm::vec3(1, 0, 0));
	model = glm::scale(model, glm::vec3(OBJECT_SIZE));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(colorLocation, OBJECT_RED, OBJECT_GREEN, OBJECT_BLUE); glUniform1i(lightingLocation, lightEnabled);
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, objectType == CUBE ? cubeFirst : pyramidFirst, objectType == CUBE ? 36 : 18);
	// 궤도, 좌표축, 광원 표시는 조명 영향을 받지 않고 고유 색상으로 표시합니다.
	glUniform1i(lightingLocation, false);
	model = glm::translate(glm::mat4(1), glm::vec3(0, LIGHT_HEIGHT, 0));
	model = glm::scale(model, glm::vec3(lightRadius));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform3f(colorLocation, .12f, .45f, .16f); glLineWidth(1); glDrawArrays(GL_LINE_LOOP, orbitFirst, LIGHT_ORBIT_SEGMENTS);
	glm::vec3 colors[] = { {.85f,.12f,.12f},{.12f,.65f,.20f},{.15f,.30f,.90f} };
	model = glm::mat4(1);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	for (int i = 0; i < 3; ++i) { glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i])); glDrawArrays(GL_LINES, axesFirst + i * 2, 2); }
	model = glm::scale(glm::translate(glm::mat4(1), light), glm::vec3(LIGHT_MARKER_SIZE));
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	// 흰 배경에서도 광원 위치를 구분할 수 있도록 흰 육면체에 회색 테두리를 표시합니다.
	glEnable(GL_POLYGON_OFFSET_FILL); glPolygonOffset(1, 1);
	glUniform3f(colorLocation, 1, 1, 1); glDrawArrays(GL_TRIANGLES, cubeFirst, 36);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glUniform3f(colorLocation, .35f, .35f, .35f); glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); glDrawArrays(GL_TRIANGLES, cubeFirst, 36); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glBindVertexArray(0);
	for (int i = 0; i < 3; ++i) {
		glm::vec3 end(0); end[i] = AXIS_LENGTH;
		model = glm::translate(glm::mat4(1), end);
		if (i == 0) model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		if (i == 1) model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		model = glm::translate(model, glm::vec3(0, 0, -CONE_HEIGHT * .5f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model)); glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
		gluCylinder(qobj, CONE_RADIUS, 0, CONE_HEIGHT, CONE_SLICES, CONE_STACKS);
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model)); gluDisk(qobj, 0, CONE_RADIUS, CONE_SLICES, 1);
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_N) objectType = objectType == CUBE ? PYRAMID : CUBE;
	if (key == GLFW_KEY_M) lightEnabled = !lightEnabled;
	if (key == GLFW_KEY_Y) objectSpinning = !objectSpinning;
	if (key == GLFW_KEY_R) {
		int direction = (mods & GLFW_MOD_SHIFT) ? -1 : 1;
		lightOrbitDirection = lightOrbitDirection == direction ? 0 : direction;
	}
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
		// 원근 orbit 원본과 동일하게 버튼을 놓으면 기본 방향으로 복귀합니다.
		cameraPosition = cameraTarget + glm::normalize(glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z)) * cameraRadius;
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
	glUniform1f(glGetUniformLocation(programID, "diffuseStrength"), DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "specularStrength"), SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "shininess"), SHININESS);
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0
		&& lightingLocation >= 0 && lightLocation >= 0 && eyeLocation >= 0;
}
