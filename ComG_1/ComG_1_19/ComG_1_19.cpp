#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
using namespace std;

// ===========================================================================
// 1. 설정값 - 창과 카메라
// ===========================================================================
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CAMERA_X 2.8f
#define CAMERA_Y 2.4f
#define CAMERA_Z -4.0f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 0.0f
#define CAMERA_TARGET_Z 0.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 50.0f
#define CAMERA_MOVE_SPEED 1.0f
#define CAMERA_ROTATE_SPEED 35.0f
#define CAMERA_RADIUS_MIN 0.30f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
// ===========================================================================
// 바닥 설정
// ===========================================================================
#define FLOOR_WIDTH 4.0f
#define FLOOR_DEPTH 4.0f
#define FLOOR_HEIGHT 0.04f
#define FLOOR_RED 0.82f
#define FLOOR_GREEN 0.88f
#define FLOOR_BLUE 0.94f
// ===========================================================================
// 탱크 부품 크기
// ===========================================================================
#define LOWER_WIDTH 0.26f
#define LOWER_HEIGHT 0.24f
#define LOWER_DEPTH 0.90f
#define LOWER_GAP 0.26f
#define MIDDLE_WIDTH 0.90f
#define MIDDLE_HEIGHT 0.12f
#define MIDDLE_DEPTH 0.62f
#define UPPER_WIDTH 0.28f
#define UPPER_HEIGHT 0.22f
#define UPPER_DEPTH 0.32f
#define UPPER_GAP 0.16f
#define BARREL_WIDTH 0.045f
#define BARREL_HEIGHT 0.045f
#define BARREL_LENGTH 0.40f
#define POLE_WIDTH 0.035f
#define POLE_HEIGHT 0.42f
#define POLE_DEPTH 0.035f
// ===========================================================================
// 애니메이션과 색상 설정
// ===========================================================================
#define TANK_MOVE_SPEED 0.65f
#define LOWER_SCALE_MIN 0.50f
#define LOWER_SCALE_MAX 1.50f
#define LOWER_CYCLE_SPEED 1.8f
#define MIDDLE_ROTATE_SPEED 90.0f
#define BARREL_ROTATE_SPEED 90.0f
#define POLE_ROTATE_SPEED 120.0f
#define MIDDLE_CYCLE_ANGLE 180.0f
#define BARREL_CYCLE_ANGLE 180.0f
#define POLE_CYCLE_ANGLE 360.0f
#define SWAP_DURATION 1.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f

// ===========================================================================
// 2. 현재 상태와 OpenGL 변수
// ===========================================================================
glm::vec3 tankPosition(0), cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z);
glm::vec3 cameraTarget(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
glm::vec3 colors[5], initialColors[5];
mt19937 randomEngine(random_device{}());
uniform_real_distribution<float> randomColor(COLOR_MIN, COLOR_MAX);
float lowerPhase = 0, middleAngle = 0, barrelAngle = 0, poleAngle = 0;
float middleRemaining = 0, barrelRemaining = 0, poleRemaining = 0;
float swapProgress = 0, upperSide = 1, barrelDirection = -1;
bool lowerChanging = false, swapping = false, dragging = false;
int cameraSpin = 0, cameraOrbit = 0;
double previousMouseX = 0, previousMouseY = 0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation;

// ===========================================================================
// 3. 함수 선언
// ===========================================================================
string ReadFile(const char* name);
bool InitShader();
void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color);
void DrawScene();
void UpdateScene(GLFWwindow* window, float dt);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// ===========================================================================
// 4. 초기화와 메인 반복문
// ===========================================================================
int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-19 Tank", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window, KeyCallback); glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback); glfwSetScrollCallback(window, ScrollCallback);
	glm::vec3 corners[] = { {-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
		{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f} };
	int indices[] = { 0,2,1,0,3,2, 4,5,6,4,6,7, 0,4,7,0,7,3,
		1,2,6,1,6,5, 3,7,6,3,6,2, 0,1,5,0,5,4 };
	vector<glm::vec3> vertices;
	for (int index : indices)vertices.push_back(corners[index]);
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr); glEnableVertexAttribArray(0); glBindVertexArray(0);
	for (int i = 0; i < 5; ++i)initialColors[i] = colors[i] = glm::vec3(randomColor(randomEngine), randomColor(randomEngine), randomColor(randomEngine));
	cout << "Arrows: tank | x/X, z/Z: camera | d: height | t: middle | l: swap | g: barrels | p: poles\n"
		<< "y/Y: camera spin | r/R: orbit origin | drag/wheel: camera | o: colors | c: reset | q: quit" << endl;
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents(); double now = glfwGetTime(); float dt = (float)glm::min(now - previousTime, 0.1); previousTime = now;
		UpdateScene(window, dt);
		int width, height; glfwGetFramebufferSize(window, &width, &height); glViewport(0, 0, width, height);
		DrawScene(); glfwSwapBuffers(window);
	}
	glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

// ===========================================================================
// 5. 이동과 애니메이션 처리
// ===========================================================================
void UpdateScene(GLFWwindow* window, float dt)
{
	glm::vec3 movement(0), cameraMovement(0);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)movement.x -= 1;
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)movement.x += 1;
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)movement.z -= 1;
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)movement.z += 1;
	if (glm::length(movement) > 0)tankPosition += glm::normalize(movement) * TANK_MOVE_SPEED * dt;
	// 회전하는 부품까지 포함한 범위로 바닥 경계를 제한합니다.
	float extent = glm::max(LOWER_WIDTH + LOWER_GAP * .5f, LOWER_DEPTH * .5f);
	extent = glm::max(extent, glm::length(glm::vec2(MIDDLE_WIDTH, MIDDLE_DEPTH)) * .5f);
	float upperReach = glm::max(glm::length(glm::vec2(UPPER_WIDTH, UPPER_DEPTH)) * .5f,
		glm::max(glm::length(glm::vec2(BARREL_LENGTH, BARREL_WIDTH * .5f)), glm::length(glm::vec2(POLE_HEIGHT, POLE_DEPTH * .5f))));
	extent = glm::max(extent, (UPPER_WIDTH + UPPER_GAP) * .5f + upperReach);
	float limitX = glm::max(0.0f, FLOOR_WIDTH * .5f - extent), limitZ = glm::max(0.0f, FLOOR_DEPTH * .5f - extent);
	tankPosition.x = glm::clamp(tankPosition.x, -limitX, limitX); tankPosition.z = glm::clamp(tankPosition.z, -limitZ, limitZ);
	// 소문자: 양의 방향, Shift + 문자: 음의 방향 (월드 좌표 기준)
	bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
	float moveDirection = shift ? -1.0f : 1.0f;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)cameraMovement.x += moveDirection;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)cameraMovement.z += moveDirection;
	if (glm::length(cameraMovement) > 0) {
		cameraMovement = glm::normalize(cameraMovement) * CAMERA_MOVE_SPEED * dt;
		cameraPosition += cameraMovement; cameraTarget += cameraMovement;
	}
	if (cameraOrbit) {
		glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians(cameraOrbit * CAMERA_ROTATE_SPEED * dt), glm::vec3(0, 1, 0));
		cameraPosition = glm::vec3(rotation * glm::vec4(cameraPosition, 1));
		cameraTarget = glm::vec3(rotation * glm::vec4(cameraTarget, 1));
	}
	if (cameraSpin) {
		glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians(cameraSpin * CAMERA_ROTATE_SPEED * dt), glm::vec3(0, 1, 0));
		cameraTarget = cameraPosition + glm::vec3(rotation * glm::vec4(cameraTarget - cameraPosition, 0));
	}
	if (lowerChanging)lowerPhase = glm::mod(lowerPhase + LOWER_CYCLE_SPEED * dt, glm::two_pi<float>());
	float step = glm::min(middleRemaining, MIDDLE_ROTATE_SPEED * dt);
	middleAngle = glm::mod(middleAngle + step, 360.0f); middleRemaining -= step;
	step = glm::min(barrelRemaining, BARREL_ROTATE_SPEED * dt);
	barrelAngle = glm::mod(barrelAngle + barrelDirection * step, 360.0f); barrelRemaining -= step;
	step = glm::min(poleRemaining, POLE_ROTATE_SPEED * dt);
	poleAngle = glm::mod(poleAngle + step, 360.0f); poleRemaining -= step;
	if (swapping) {
		swapProgress = glm::min(swapProgress + dt / SWAP_DURATION, 1.0f);
		if (swapProgress >= 1) { upperSide = -upperSide; swapProgress = 0; swapping = false; }
	}
}

// ===========================================================================
// 6. 도형 그리기
// ===========================================================================
void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color)
{
	glm::mat4 transform = glm::scale(model, size);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(transform));
	glUniform3fv(colorLocation, 1, glm::value_ptr(color));
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 0, 36); glBindVertexArray(0);
}

void DrawScene()
{
	glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glUseProgram(programID);
	glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));
	glm::mat4 projection = glm::perspective(glm::radians(PERSPECTIVE_FOVY), PERSPECTIVE_ASPECT, PERSPECTIVE_NEAR, PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view)); glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	DrawBox(glm::translate(glm::mat4(1), glm::vec3(0, -FLOOR_HEIGHT * .5f, 0)),
		glm::vec3(FLOOR_WIDTH, FLOOR_HEIGHT, FLOOR_DEPTH), glm::vec3(FLOOR_RED, FLOOR_GREEN, FLOOR_BLUE));
	float lowerHeight = LOWER_HEIGHT * glm::mix(LOWER_SCALE_MIN, LOWER_SCALE_MAX, (glm::sin(lowerPhase) + 1) * .5f);
	float middleBottom = lowerHeight;
	glm::mat4 base = glm::translate(glm::mat4(1), tankPosition);
	for (int i = 0; i < 2; ++i) {
		float side = i == 0 ? -1.0f : 1.0f;
		DrawBox(glm::translate(base, glm::vec3(side * (LOWER_WIDTH + LOWER_GAP) * .5f, lowerHeight * .5f, 0)),
			glm::vec3(LOWER_WIDTH, lowerHeight, LOWER_DEPTH), colors[0]);
	}
	glm::mat4 middle = glm::translate(base, glm::vec3(0, middleBottom + MIDDLE_HEIGHT * .5f, 0));
	middle = glm::rotate(middle, glm::radians(middleAngle), glm::vec3(0, 1, 0));
	DrawBox(middle, glm::vec3(MIDDLE_WIDTH, MIDDLE_HEIGHT, MIDDLE_DEPTH), colors[1]);
	for (int i = 0; i < 2; ++i) {
		float side = i == 0 ? -1.0f : 1.0f;
		float x = side * upperSide * (1 - 2 * swapProgress) * (UPPER_WIDTH + UPPER_GAP) * .5f;
		// 중앙 몸체의 회전과 높이를 상속합니다.
		glm::mat4 upper = glm::translate(middle, glm::vec3(x, (MIDDLE_HEIGHT + UPPER_HEIGHT) * .5f, 0));
		DrawBox(upper, glm::vec3(UPPER_WIDTH, UPPER_HEIGHT, UPPER_DEPTH), colors[2]);
		// 상부 몸체 내부를 기준점으로 삼아 회전 중에도 연결을 유지합니다.
		glm::mat4 barrel = glm::rotate(upper, glm::radians(side * barrelAngle), glm::vec3(0, 1, 0));
		barrel = glm::translate(barrel, glm::vec3(0, 0, -BARREL_LENGTH * .5f));
		DrawBox(barrel, glm::vec3(BARREL_WIDTH, BARREL_HEIGHT, BARREL_LENGTH), colors[3]);
		glm::mat4 pole = glm::rotate(upper, glm::radians(side * poleAngle), glm::vec3(1, 0, 0));
		pole = glm::translate(pole, glm::vec3(0, POLE_HEIGHT * .5f, 0));
		DrawBox(pole, glm::vec3(POLE_WIDTH, POLE_HEIGHT, POLE_DEPTH), colors[4]);
	}
}

// ===========================================================================
// 7. 키보드 입력
// ===========================================================================
void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS)return;
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_D)lowerChanging = !lowerChanging;
	if (key == GLFW_KEY_T && middleRemaining == 0)middleRemaining = MIDDLE_CYCLE_ANGLE;
	if (key == GLFW_KEY_G && barrelRemaining == 0) {
		// 현재 위치와 방향을 기준으로 항상 바깥쪽 180도 회전을 선택합니다.
		float currentSide = upperSide * (1 - 2 * swapProgress);
		barrelDirection = currentSide * glm::cos(glm::radians(barrelAngle)) >= 0 ? -1.0f : 1.0f;
		barrelRemaining = BARREL_CYCLE_ANGLE;
	}
	if (key == GLFW_KEY_P && poleRemaining == 0)poleRemaining = POLE_CYCLE_ANGLE;
	if (key == GLFW_KEY_L && !swapping) { swapping = true; swapProgress = 0; }
	int direction = (mods & GLFW_MOD_SHIFT) ? 1 : -1;
	if (key == GLFW_KEY_Y)cameraSpin = cameraSpin == direction ? 0 : direction;
	if (key == GLFW_KEY_R)cameraOrbit = cameraOrbit == direction ? 0 : direction;
	if (key == GLFW_KEY_O)
		for (glm::vec3& color : colors)color = glm::vec3(randomColor(randomEngine), randomColor(randomEngine), randomColor(randomEngine));
	if (key == GLFW_KEY_C) {
		tankPosition = glm::vec3(0); cameraPosition = glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z);
		cameraTarget = glm::vec3(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
		lowerPhase = middleAngle = barrelAngle = poleAngle = 0;
		middleRemaining = barrelRemaining = poleRemaining = swapProgress = 0; upperSide = 1; barrelDirection = -1;
		lowerChanging = swapping = dragging = false; cameraSpin = cameraOrbit = 0;
		for (int i = 0; i < 5; ++i)colors[i] = initialColors[i];
	}
}

// ===========================================================================
// 8. 마우스 오비트와 확대/축소
// ===========================================================================
void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT)return;
	dragging = action == GLFW_PRESS;
	if (dragging)glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (!dragging)return;
	int width, height; glfwGetWindowSize(window, &width, &height); if (width <= 0 || height <= 0)return;
	glm::vec3 offset = cameraPosition - cameraTarget; float radius = glm::length(offset);
	float yaw = glm::atan(offset.x, offset.z), pitch = glm::asin(glm::clamp(offset.y / radius, -1.0f, 1.0f));
	yaw -= glm::radians(ORBIT_DEGREES) * (float)(x - previousMouseX) / width;
	pitch += glm::radians(ORBIT_DEGREES) * (float)(y - previousMouseY) / height;
	previousMouseX = x; previousMouseY = y;
	pitch = glm::clamp(pitch, -glm::radians(ORBIT_PITCH_LIMIT), glm::radians(ORBIT_PITCH_LIMIT));
	cameraPosition = cameraTarget + radius * glm::vec3(glm::cos(pitch) * glm::sin(yaw), glm::sin(pitch), glm::cos(pitch) * glm::cos(yaw));
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
	glm::vec3 offset = cameraPosition - cameraTarget;
	float radius = glm::clamp(glm::length(offset) - (float)yoffset * ZOOM_STEP, CAMERA_RADIUS_MIN, CAMERA_RADIUS_MAX);
	cameraPosition = cameraTarget + glm::normalize(offset) * radius;
}

// ===========================================================================
// 9. 셰이더 파일 로딩과 프로그램 생성
// ===========================================================================
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
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0;
}





