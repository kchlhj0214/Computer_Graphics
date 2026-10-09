#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glu.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <GL/glm/glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <cstddef>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define AXIS_LENGTH 1.0f
#define CONE_RADIUS 0.0225f
#define CONE_HEIGHT (CONE_RADIUS * 2.0f)
#define CONE_SLICES 24
#define CONE_STACKS 16
#define VIEW_RANGE 1.35f
#define CAMERA_X 3.0f
#define CAMERA_Y 2.4f
#define CAMERA_Z 4.0f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define ANIMATION_FPS 60
#define MOVE_PER_FRAME 0.01f
#define ROTATE_PER_FRAME 1.0f
#define OBJECT_SIZE 0.8f
#define OBJECT_ROTATE_X 30.0f
#define OBJECT_ROTATE_Y 30.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define COLOR_GAP 0.10f

enum ObjectType { NONE, CUBE, PYRAMID };
ObjectType objectType = NONE;
struct Vertex { glm::vec3 position, color; };
GLuint objectVao = 0, objectVbo = 0;
GLint vertexColorLocation = -1;
int objectVertexCount = 0, rotationKey = 0;
bool wireframe = false, hiddenSurface = true;
bool moveKeys[4] = {};
glm::mat4 objectTransform(1.0f);
mt19937 randomEngine(random_device{}());

glm::vec3 cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z);
float cameraRadius = glm::length(cameraPosition);
float cameraYaw = 0.0f, cameraPitch = 0.0f;
double previousMouseX = 0.0, previousMouseY = 0.0;
bool dragging = false;

GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation = -1, viewLocation = -1, projectionLocation = -1, colorLocation = -1;
GLUquadric* qobj = nullptr;

string ReadFile(const char* name);
bool InitShader();
void DrawScene();
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void ResetObject();
void InitObject(ObjectType type);
void UpdateAnimation(GLFWwindow* window);

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// GLU submits legacy vertices; the shader still transforms them.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-14 3D Objects", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetKeyCallback(window, KeyCallback);
	glGenVertexArrays(1, &objectVao); glGenBuffers(1, &objectVbo);
	float vertices[] = {
		-AXIS_LENGTH,0,0, AXIS_LENGTH,0,0,
		0,-AXIS_LENGTH,0, 0,AXIS_LENGTH,0,
		0,0,-AXIS_LENGTH, 0,0,AXIS_LENGTH
	};
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0); glBindVertexArray(0);
	qobj = gluNewQuadric();
	if (!qobj) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_NONE);
	gluQuadricOrientation(qobj, GLU_OUTSIDE);
	double previousTime = glfwGetTime(), accumulatedTime = 0.0;
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double now = glfwGetTime();
		accumulatedTime += glm::min(now - previousTime, 0.1); previousTime = now;
		while (accumulatedTime >= 1.0 / ANIMATION_FPS) {
			UpdateAnimation(window); accumulatedTime -= 1.0 / ANIMATION_FPS;
		}
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		int width, height; glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		DrawScene(); glfwSwapBuffers(window);
	}
	gluDeleteQuadric(qobj);
	glDeleteBuffers(1, &objectVbo); glDeleteVertexArrays(1, &objectVao);
	glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

string ReadFile(const char* name)
{
	ifstream file(name);
	if (!file) { cerr << "Cannot open shader: " << name << endl; return ""; }
	stringstream stream; stream << file.rdbuf(); return stream.str();
}

bool InitShader()
{
	string sources[2] = { ReadFile("vertex.glsl"), ReadFile("fragment.glsl") };
	if (sources[0].empty() || sources[1].empty()) return false;
	GLuint shaders[2] = { glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER) };
	for (int i = 0; i < 2; ++i) {
		const char* code = sources[i].c_str(); glShaderSource(shaders[i], 1, &code, nullptr);
		glCompileShader(shaders[i]); GLint ok; glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
		if (!ok) { char log[1024]; glGetShaderInfoLog(shaders[i], 1024, nullptr, log); cerr << log << endl; return false; }
	}
	programID = glCreateProgram();
	glAttachShader(programID, shaders[0]); glAttachShader(programID, shaders[1]); glLinkProgram(programID);
	GLint ok; glGetProgramiv(programID, GL_LINK_STATUS, &ok);
	if (!ok) { char log[1024]; glGetProgramInfoLog(programID, 1024, nullptr, log); cerr << log << endl; return false; }
	glDeleteShader(shaders[0]); glDeleteShader(shaders[1]);
	modelLocation = glGetUniformLocation(programID, "modelTransform");
	viewLocation = glGetUniformLocation(programID, "viewTransform");
	projectionLocation = glGetUniformLocation(programID, "projectionTransform");
	colorLocation = glGetUniformLocation(programID, "uColor");
	vertexColorLocation = glGetUniformLocation(programID, "useVertexColor");
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0 && vertexColorLocation >= 0;
}

void DrawScene()
{
	glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(programID);
	glEnable(GL_DEPTH_TEST); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform1i(vertexColorLocation, GL_FALSE);
	glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0f), glm::vec3(0, 1, 0));
	glm::mat4 projection = glm::ortho(-VIEW_RANGE, VIEW_RANGE, -VIEW_RANGE, VIEW_RANGE, 0.1f, 20.0f);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	glm::vec3 colors[3] = { {0.85f,0.12f,0.12f},{0.12f,0.65f,0.20f},{0.15f,0.30f,0.90f} };
	glm::vec3 ends[3] = { {AXIS_LENGTH,0,0},{0,AXIS_LENGTH,0},{0,0,AXIS_LENGTH} };
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(vao); glLineWidth(1);
	for (int i = 0; i < 3; ++i) {
		glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
		glDrawArrays(GL_LINES, i * 2, 2);
	}
	glBindVertexArray(0);
	for (int i = 0; i < 3; ++i) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, ends[i]);
		// GLU cones point along local +Z; align it with the positive axis.
		if (i == 0) model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		else if (i == 1) model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		model = glm::translate(model, glm::vec3(0, 0, -CONE_HEIGHT * 0.5f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
		gluCylinder(qobj, CONE_RADIUS, 0.0, CONE_HEIGHT, CONE_SLICES, CONE_STACKS);
		// Close the base with an outward-facing disk.
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		gluDisk(qobj, 0.0, CONE_RADIUS, CONE_SLICES, 1);
	}
	if (objectType != NONE) {
		if (hiddenSurface && !wireframe) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		glm::mat4 objectModel = glm::scale(objectTransform, glm::vec3(OBJECT_SIZE));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(objectModel));
		glUniform1i(vertexColorLocation, GL_TRUE);
		glBindVertexArray(objectVao); glDrawArrays(GL_TRIANGLES, 0, objectVertexCount);
		glBindVertexArray(0);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); glEnable(GL_DEPTH_TEST);
}

void ResetObject()
{
	objectTransform = glm::mat4(1.0f);
	objectTransform = glm::rotate(objectTransform, glm::radians(OBJECT_ROTATE_Y), glm::vec3(0, 1, 0));
	objectTransform = glm::rotate(objectTransform, glm::radians(OBJECT_ROTATE_X), glm::vec3(1, 0, 0));
	rotationKey = 0;
	for (bool& pressed : moveKeys) pressed = false;
}

void InitObject(ObjectType type)
{
	objectType = type; ResetObject();
	glm::vec3 positions[] = {
		{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f},
		{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}
	};
	int cube[] = {0,1,2,0,2,3, 5,4,7,5,7,6, 4,0,3,4,3,7,
		1,5,6,1,6,2, 3,2,6,3,6,7, 4,5,1,4,1,0};
	int pyramid[] = {0,1,2,1,3,2,3,4,2,4,0,2, 0,4,3,0,3,1};
	if (type == PYRAMID) {
		positions[2] = glm::vec3(0,.5f,0);
		positions[3] = glm::vec3(.5f,-.5f,-.5f);
	}
	int count = type == CUBE ? 8 : 5;
	glm::vec3 colors[8];
	// Separated candidates prevent rejection sampling from reaching a dead end.
	for (int channel = 0; channel < 3; ++channel) {
		float values[8];
		float slack = COLOR_MAX - COLOR_MIN - (count - 1) * COLOR_GAP;
		float offset = uniform_real_distribution<float>(0.0f, slack)(randomEngine);
		for (int i = 0; i < count; ++i) values[i] = COLOR_MIN + offset + i * COLOR_GAP;
		shuffle(values, values + count, randomEngine);
		for (int i = 0; i < count; ++i) colors[i][channel] = values[i];
	}
	int* indices = type == CUBE ? cube : pyramid;
	objectVertexCount = type == CUBE ? 36 : 18;
	vector<Vertex> vertices;
	for (int i = 0; i < objectVertexCount; ++i) vertices.push_back({positions[indices[i]], colors[indices[i]]});
	glBindVertexArray(objectVao); glBindBuffer(GL_ARRAY_BUFFER, objectVbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1); glBindVertexArray(0);
}

void KeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
	int arrows[] = {GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_UP, GLFW_KEY_DOWN};
	for (int i = 0; i < 4; ++i) if (key == arrows[i]) {
		if (action == GLFW_PRESS) moveKeys[i] = true;
		if (action == GLFW_RELEASE) moveKeys[i] = false;
	}
	if (action == GLFW_RELEASE && key == rotationKey) rotationKey = 0;
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_C) InitObject(CUBE);
	else if (key == GLFW_KEY_P) InitObject(PYRAMID);
	else if (key == GLFW_KEY_H) hiddenSurface = !hiddenSurface;
	else if (key == GLFW_KEY_W) wireframe = !(mods & GLFW_MOD_SHIFT);
	else if (key == GLFW_KEY_S) ResetObject();
	else if (key == GLFW_KEY_X || key == GLFW_KEY_Y) rotationKey = key;
}

void UpdateAnimation(GLFWwindow* window)
{
	if (objectType == NONE) return;
	if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
		rotationKey = 0; for (bool& pressed : moveKeys) pressed = false; return;
	}
	if (rotationKey && glfwGetKey(window, rotationKey) == GLFW_PRESS) {
		bool negative = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
		glm::vec3 axis = rotationKey == GLFW_KEY_X ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
		// Premultiply to rotate both the center and orientation about the fixed world axis.
		objectTransform = glm::rotate(glm::mat4(1.0f), glm::radians(negative ? -ROTATE_PER_FRAME : ROTATE_PER_FRAME), axis) * objectTransform;
	}
	glm::vec3 movement((moveKeys[1] - moveKeys[0]) * MOVE_PER_FRAME, (moveKeys[2] - moveKeys[3]) * MOVE_PER_FRAME, 0);
	objectTransform = glm::translate(glm::mat4(1.0f), movement) * objectTransform;
	for (int i = 0; i < 3; ++i) objectTransform[3][i] = glm::clamp(objectTransform[3][i], -AXIS_LENGTH, AXIS_LENGTH);
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;
	if (action == GLFW_PRESS) {
		dragging = true;
		glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
		cameraYaw = glm::atan(cameraPosition.x, cameraPosition.z);
		cameraPitch = glm::asin(cameraPosition.y / cameraRadius);
	}
	else if (action == GLFW_RELEASE) {
		dragging = false;
		cameraPosition = glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z);
	}
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (!dragging) return;
	int width, height; glfwGetWindowSize(window, &width, &height);
	if (width <= 0 || height <= 0) return;
	// Drag the scene: horizontal and vertical deltas control yaw and pitch.
	cameraYaw -= glm::radians(ORBIT_DEGREES) * (float)(x - previousMouseX) / width;
	cameraPitch += glm::radians(ORBIT_DEGREES) * (float)(y - previousMouseY) / height;
	previousMouseX = x; previousMouseY = y;
	cameraYaw = glm::mod(cameraYaw, glm::radians(360.0f));
	cameraPitch = glm::clamp(cameraPitch, -glm::radians(ORBIT_PITCH_LIMIT), glm::radians(ORBIT_PITCH_LIMIT));
	cameraPosition = cameraRadius * glm::vec3(
		glm::cos(cameraPitch) * glm::sin(cameraYaw),
		glm::sin(cameraPitch),
		glm::cos(cameraPitch) * glm::cos(cameraYaw));
}
