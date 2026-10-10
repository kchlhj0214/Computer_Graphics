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
#define OBJECT_SIZE 0.65f
#define OBJECT_ROTATE_X 0.0f
#define OBJECT_ROTATE_Y 0.0f
#define PYRAMID_HEIGHT 1.0f
#define ANIMATION_FPS 60
#define ROTATE_PER_FRAME 2.0f
#define SCALE_PER_FRAME 0.015f
#define FRONT_OPEN_ANGLE 135.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define COLOR_GAP 0.10f

enum ObjectType { CUBE, PYRAMID };
enum Animation { IDLE, SPIN, TOP_SPIN, FRONT_OPEN, SIDE_SPIN, BACK_SCALE,
	EDGE_SPIN, SIDE_SWAP, ORBIT_Y, ORBIT_X, PYRAMID_ALL, PYRAMID_SEQUENCE,
	PYRAMID_SWAP, PYRAMID_APEX };
struct Vertex { glm::vec3 position, color; };
ObjectType objectType = CUBE;
Animation animation = IDLE;
bool hiddenSurface = true;
int faceCount = 6, selectedSide = 0;
int faceFirst[6] = {}, faceVertices[6] = {}, faceOwner[6] = {};
glm::mat4 faceTransforms[6];
// 옆면 위치: +Z, +X, -Z, -X 순서. 육면체 윗면/밑면: 4/5, 사각뿔 밑면: 4.
glm::vec3 sideNormals[] = { {0,0,1}, {1,0,0}, {0,0,-1}, {-1,0,0} };
float animationProgress = 0.0f;
double animationAccumulator = 0.0;
mt19937 randomEngine(random_device{}());
GLuint objectVao = 0, objectVbo = 0;
GLint vertexColorLocation = -1;

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
void InitBuffer();
void ResetObject();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void UpdateAnimation(double elapsed);
float AnimationDistance();
glm::mat4 FaceAnimation(int slot);
glm::mat4 RotateAt(glm::vec3 pivot, glm::vec3 axis, float degrees);
float PyramidFlatAngle();

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// GLU가 전달한 정점도 셰이더에서 변환한다.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-15 / 1-16 Face Animations", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetKeyCallback(window, KeyCallback);
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
	glGenVertexArrays(1, &objectVao); glGenBuffers(1, &objectVbo);
	InitBuffer(); ResetObject();
	qobj = gluNewQuadric();
	if (!qobj) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	gluQuadricDrawStyle(qobj, GLU_FILL);
	gluQuadricNormals(qobj, GLU_NONE);
	gluQuadricOrientation(qobj, GLU_OUTSIDE);
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double now = glfwGetTime();
		UpdateAnimation(glm::min(now - previousTime, 0.1)); previousTime = now;
		glfwPollEvents();
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
	if (hiddenSurface) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
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
		// 원뿔의 기본 방향인 +Z를 각 좌표축의 양의 방향에 맞춘다.
		if (i == 0) model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
		else if (i == 1) model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
		model = glm::translate(model, glm::vec3(0, 0, -CONE_HEIGHT * 0.5f));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
		gluCylinder(qobj, CONE_RADIUS, 0.0, CONE_HEIGHT, CONE_SLICES, CONE_STACKS);
		// 바깥쪽을 향하는 원판으로 원뿔의 밑면을 막는다.
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		gluDisk(qobj, 0.0, CONE_RADIUS, CONE_SLICES, 1);
	}
	// 면 변환은 객체 좌표계 기준이며, 전체 회전만 고정된 월드 y축을 사용한다.
	glm::mat4 objectModel(1.0f);
	if (animation == SPIN) objectModel = glm::rotate(objectModel, glm::radians(360.0f * animationProgress), glm::vec3(0,1,0));
	objectModel = glm::rotate(objectModel, glm::radians(OBJECT_ROTATE_Y), glm::vec3(0,1,0));
	objectModel = glm::rotate(objectModel, glm::radians(OBJECT_ROTATE_X), glm::vec3(1,0,0));
	objectModel = glm::scale(objectModel, glm::vec3(OBJECT_SIZE));
	glUniform1i(vertexColorLocation, GL_TRUE); glBindVertexArray(objectVao);
	for (int slot = 0; slot < faceCount; ++slot) {
		int owner = faceOwner[slot];
		model = objectModel * FaceAnimation(slot) * faceTransforms[owner];
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLES, faceFirst[owner], faceVertices[owner]);
	}
	glBindVertexArray(0);
}

void InitBuffer()
{
	glm::vec3 positions[] = {
		{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f},
		{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}
	};
	int cube[] = { 0,1,2,0,2,3, 1,5,6,1,6,2, 5,4,7,5,7,6,
		4,0,3,4,3,7, 3,2,6,3,6,7, 4,5,1,4,1,0 };
	int pyramid[] = { 0,1,2, 1,3,2, 3,4,2, 4,0,2, 0,4,3,0,3,1 };
	if (objectType == PYRAMID) {
		positions[2] = glm::vec3(0, PYRAMID_HEIGHT - .5f, 0);
		positions[3] = glm::vec3(.5f,-.5f,-.5f);
	}
	int count = objectType == CUBE ? 8 : 5;
	glm::vec3 colors[8];
	// 무한 재추첨을 피하면서 RGB 성분별로 COLOR_GAP 이상의 간격을 확보한다.
	for (int channel = 0; channel < 3; ++channel) {
		float values[8];
		float step = COLOR_GAP + 0.00001f;
		float slack = COLOR_MAX - COLOR_MIN - (count - 1) * step;
		float offset = uniform_real_distribution<float>(0.0f, slack)(randomEngine);
		for (int i = 0; i < count; ++i) values[i] = COLOR_MIN + offset + i * step;
		shuffle(values, values + count, randomEngine);
		for (int i = 0; i < count; ++i) colors[i][channel] = values[i];
	}
	faceCount = objectType == CUBE ? 6 : 5;
	int* indices = objectType == CUBE ? cube : pyramid;
	vector<Vertex> vertices;
	int index = 0;
	for (int face = 0; face < faceCount; ++face) {
		faceFirst[face] = index;
		faceVertices[face] = objectType == CUBE || face == 4 ? 6 : 3;
		for (int j = 0; j < faceVertices[face]; ++j, ++index)
			vertices.push_back({positions[indices[index]], colors[indices[index]]});
	}
	glBindVertexArray(objectVao); glBindBuffer(GL_ARRAY_BUFFER, objectVbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1); glBindVertexArray(0);
}

void ResetObject()
{
	animation = IDLE; animationProgress = 0; animationAccumulator = 0;
	hiddenSurface = true;
	for (int i = 0; i < 6; ++i) { faceOwner[i] = i; faceTransforms[i] = glm::mat4(1.0f); }
	dragging = false; cameraPosition = glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z);
}

glm::mat4 RotateAt(glm::vec3 pivot, glm::vec3 axis, float degrees)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), pivot);
	transform = glm::rotate(transform, glm::radians(degrees), axis);
	return glm::translate(transform, -pivot);
}

float PyramidFlatAngle()
{
	// 기울어진 옆면을 바깥쪽으로 펼쳐 밑면과 같은 평면에 놓는 각도이다.
	return 180.0f - glm::degrees(glm::atan(PYRAMID_HEIGHT / 0.5f));
}

glm::mat4 FaceAnimation(int slot)
{
	float turn = 360.0f * animationProgress;
	float open = 1.0f - glm::abs(2.0f * animationProgress - 1.0f);
	glm::vec3 x(1,0,0), y(0,1,0), z(0,0,1);
	glm::mat4 identity(1.0f);
	if (animation == TOP_SPIN && slot == 4) return RotateAt(glm::vec3(0,.5f,0), z, turn);
	// 기본 카메라에서 오른쪽에 보이는 +X 면을 위쪽 모서리 기준으로 바깥으로 연다.
	if (animation == FRONT_OPEN && slot == 1) return RotateAt(glm::vec3(.5f,.5f,0), z, FRONT_OPEN_ANGLE * open);
	if (animation == SIDE_SPIN && (slot == 0 || slot == 2)) return RotateAt(sideNormals[slot] * .5f, z, turn);
	if (animation == BACK_SCALE && slot == 2) {
		glm::vec3 center = sideNormals[slot] * .5f;
		glm::mat4 transform = glm::translate(identity, center);
		transform = glm::scale(transform, glm::vec3(1.0f - open));
		return glm::translate(transform, -center);
	}
	int nextSide = (selectedSide + 1) % 4;
	if (animation == EDGE_SPIN && (slot == selectedSide || slot == nextSide))
		return RotateAt((sideNormals[selectedSide] + sideNormals[nextSide]) * .5f, y, turn);
	if (animation == SIDE_SWAP) {
		if (slot == selectedSide) return RotateAt(glm::vec3(0), y, 90.0f * animationProgress);
		if (slot == nextSide) return RotateAt(glm::vec3(0), y, -90.0f * animationProgress);
	}
	if (animation == ORBIT_Y && slot < 4) return RotateAt(glm::vec3(0), y, turn);
	if (animation == ORBIT_X && slot != 1 && slot != 3) return RotateAt(glm::vec3(0), x, turn);
	if (animation == PYRAMID_SWAP) {
		glm::vec3 apex(0, PYRAMID_HEIGHT - .5f, 0);
		if (slot == selectedSide) return RotateAt(apex, y, 180.0f * animationProgress);
		if (slot == (selectedSide + 2) % 4) return RotateAt(apex, y, -180.0f * animationProgress);
	}
	if (slot < 4 && (animation == PYRAMID_ALL || animation == PYRAMID_SEQUENCE || animation == PYRAMID_APEX)) {
		glm::vec3 axis = glm::cross(y, sideNormals[slot]);
		glm::vec3 hinge = sideNormals[slot] * .5f + glm::vec3(0,-.5f,0);
		if (animation == PYRAMID_ALL) return RotateAt(hinge, axis, 2.0f * PyramidFlatAngle() * open);
		if (animation == PYRAMID_SEQUENCE) {
			float stage = animationProgress * 4.0f;
			if ((int)stage != slot) return identity;
			float phase = stage - slot;
			return RotateAt(hinge, axis, PyramidFlatAngle() * (1.0f - glm::abs(2.0f * phase - 1.0f)));
		}
		return RotateAt(glm::vec3(0, PYRAMID_HEIGHT - .5f, 0), axis, -(180.0f - PyramidFlatAngle()) * open);
	}
	return identity;
}

float AnimationDistance()
{
	if (animation == FRONT_OPEN) return FRONT_OPEN_ANGLE * 2.0f;
	if (animation == BACK_SCALE) return 2.0f;
	if (animation == SIDE_SWAP) return 90.0f;
	if (animation == PYRAMID_ALL) return PyramidFlatAngle() * 4.0f;
	if (animation == PYRAMID_SEQUENCE) return PyramidFlatAngle() * 8.0f;
	if (animation == PYRAMID_SWAP) return 180.0f;
	if (animation == PYRAMID_APEX) return (180.0f - PyramidFlatAngle()) * 2.0f;
	return 360.0f;
}

void UpdateAnimation(double elapsed)
{
	if (animation == IDLE) return;
	animationAccumulator += elapsed;
	while (animationAccumulator >= 1.0 / ANIMATION_FPS && animation != IDLE) {
		animationAccumulator -= 1.0 / ANIMATION_FPS;
		float step = animation == BACK_SCALE ? SCALE_PER_FRAME : ROTATE_PER_FRAME;
		// 회전각이 갱신 간격으로 나누어떨어지지 않아도 완전히 열리고 닫히는 지점을 거친다.
		int segments = animation == PYRAMID_SEQUENCE ? 8 :
			(animation == FRONT_OPEN || animation == BACK_SCALE || animation == PYRAMID_ALL || animation == PYRAMID_APEX ? 2 : 1);
		float endpoint = ((int)(animationProgress * segments) + 1) / (float)segments;
		animationProgress = glm::min(endpoint, animationProgress + step / AnimationDistance());
		if (animationProgress >= 1.0f) {
			if (animation == SIDE_SWAP || animation == PYRAMID_SWAP) {
				int other = (selectedSide + (animation == SIDE_SWAP ? 1 : 2)) % 4;
				int firstOwner = faceOwner[selectedSide], secondOwner = faceOwner[other];
				faceTransforms[firstOwner] = FaceAnimation(selectedSide) * faceTransforms[firstOwner];
				faceTransforms[secondOwner] = FaceAnimation(other) * faceTransforms[secondOwner];
				swap(faceOwner[selectedSide], faceOwner[other]);
			}
			animation = IDLE; animationProgress = 0; animationAccumulator = 0;
		}
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window, true); return; }
	if (key == GLFW_KEY_H) { hiddenSurface = !hiddenSurface; return; }
	if (key == GLFW_KEY_R) { ResetObject(); return; }
	if (animation != IDLE) return;
	if (key == GLFW_KEY_P) {
		objectType = objectType == CUBE ? PYRAMID : CUBE;
		InitBuffer(); ResetObject(); return;
	}
	if (key == GLFW_KEY_Y) animation = SPIN;
	else if (objectType == CUBE) {
		switch (key) {
		case GLFW_KEY_T: animation = TOP_SPIN; break;
		case GLFW_KEY_F: animation = FRONT_OPEN; break;
		case GLFW_KEY_S: animation = SIDE_SPIN; break;
		case GLFW_KEY_B: animation = BACK_SCALE; break;
		case GLFW_KEY_1: animation = EDGE_SPIN; break;
		case GLFW_KEY_2: animation = SIDE_SWAP; break;
		case GLFW_KEY_4: animation = ORBIT_Y; break;
		case GLFW_KEY_5: animation = ORBIT_X; break;
		}
	}
	else {
		switch (key) {
		case GLFW_KEY_6: animation = PYRAMID_ALL; break;
		case GLFW_KEY_7: animation = PYRAMID_SEQUENCE; break;
		case GLFW_KEY_8: animation = PYRAMID_SWAP; break;
		case GLFW_KEY_9: animation = PYRAMID_APEX; break;
		}
	}
	selectedSide = uniform_int_distribution<int>(0, 3)(randomEngine);
	animationProgress = 0; animationAccumulator = 0;
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
	// 마우스의 가로·세로 이동량으로 카메라의 좌우·상하 회전을 조절한다.
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
