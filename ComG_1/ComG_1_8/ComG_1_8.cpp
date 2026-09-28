// 링커 -> 입력 -> 추가 종속성: opengl32.lib; glew32.lib; glfw3.lib
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>

using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define SHAPE_MIN_SIZE 50.0f
#define SHAPE_MAX_SIZE 150.0f

struct Color {
	float r, g, b;
};

struct Shape {
	bool visible = false;
	float x = 0.0f, y = 0.0f;
	float size = 0.0f;
	float animationTime = 0.0f;
	Color color = {};
};

Shape shapes[4];
bool filled = true;
double previousTime = 0.0;
random_device rd;
mt19937 generator(rd());

GLuint shaderProgramID = 0;
GLuint vertexShader = 0;
GLuint fragmentShader = 0;
GLuint vao = 0;
GLint offsetLocation = -1;
GLint sizeLocation = -1;
GLint colorLocation = -1;
GLint timeLocation = -1;

string filetobuf(const char* fileName);
bool CompileShader(GLuint shader, const string& source, const char* shaderName);
bool InitShader();
void InitBuffer();
void UpdateScene(GLFWwindow* window);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

int main()
{
	if (!glfwInit()) {
		cerr << "ERROR: GLFW 초기화 실패" << endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(
		WINDOW_WIDTH, WINDOW_HEIGHT, "1-8 Modern OpenGL Triangles", nullptr, nullptr);
	if (!window) {
		cerr << "ERROR: 윈도우 생성 실패" << endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		cerr << "ERROR: GLEW 초기화 실패" << endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSwapInterval(1);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	if (!InitShader()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}
	InitBuffer();
	previousTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		UpdateScene(window);
		DrawScene();
		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(shaderProgramID);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

string filetobuf(const char* fileName)
{
	ifstream file(fileName);
	if (!file.is_open()) {
		cerr << "ERROR: 셰이더 파일을 열 수 없습니다: " << fileName << endl;
		return "";
	}
	stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool CompileShader(GLuint shader, const string& source, const char* shaderName)
{
	const char* code = source.c_str();
	glShaderSource(shader, 1, &code, nullptr);
	glCompileShader(shader);

	GLint result;
	GLchar errorLog[1024];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderInfoLog(shader, 1024, nullptr, errorLog);
		cerr << "ERROR: " << shaderName << " 컴파일 실패\n" << errorLog << endl;
		return false;
	}
	return true;
}

bool InitShader()
{
	string vertexSource = filetobuf("vertex.glsl");
	string fragmentSource = filetobuf("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty())
		return false;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!CompileShader(vertexShader, vertexSource, "vertex shader") ||
		!CompileShader(fragmentShader, fragmentSource, "fragment shader")) {
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return false;
	}

	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	GLint result;
	GLchar errorLog[1024];
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderProgramID, 1024, nullptr, errorLog);
		cerr << "ERROR: shader program 링크 실패\n" << errorLog << endl;
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteProgram(shaderProgramID);
		shaderProgramID = 0;
		return false;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	offsetLocation = glGetUniformLocation(shaderProgramID, "uOffset");
	sizeLocation = glGetUniformLocation(shaderProgramID, "uSize");
	colorLocation = glGetUniformLocation(shaderProgramID, "uColor");
	timeLocation = glGetUniformLocation(shaderProgramID, "uAnimationTime");
	if (offsetLocation == -1 || sizeLocation == -1 || colorLocation == -1 || timeLocation == -1) {
		cerr << "ERROR: 도형 uniform을 찾을 수 없습니다." << endl;
		glDeleteProgram(shaderProgramID);
		shaderProgramID = 0;
		return false;
	}
	return true;
}

void InitBuffer()
{
	//--- 꼭짓점은 셰이더의 gl_VertexID로 생성하므로 VAO만 필요하다.
	glGenVertexArrays(1, &vao);
}

void UpdateScene(GLFWwindow* window)
{
	double currentTime = glfwGetTime();
	float deltaTime = static_cast<float>(currentTime - previousTime);
	previousTime = currentTime;
	deltaTime = min(deltaTime, 0.05f);

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
		return;

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if (mouseX < 0.0 || mouseX >= WINDOW_WIDTH || mouseY < 0.0 || mouseY >= WINDOW_HEIGHT)
		return;

	int index = (mouseX >= WINDOW_WIDTH / 2.0 ? 1 : 0) +
		(mouseY >= WINDOW_HEIGHT / 2.0 ? 2 : 0);
	Shape& shape = shapes[index];
	if (!shape.visible)
		return;

	//--- 활성 사분면의 시간만 누적하고 크기 계산은 셰이더에 맡긴다.
	shape.animationTime += deltaTime;
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);

	for (const Shape& shape : shapes) {
		if (!shape.visible)
			continue;

		glUniform2f(offsetLocation, shape.x, shape.y);
		glUniform1f(sizeLocation, shape.size);
		glUniform1f(timeLocation, shape.animationTime);
		if (filled) {
			glUniform3f(colorLocation, shape.color.r, shape.color.g, shape.color.b);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
		//--- 면 모드에서도 검은색 테두리를 함께 그린다.
		glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 3);
	}

	//--- 도형 위에 x축, y축을 그려 사분면 구분을 유지한다.
	glUniform2f(offsetLocation, 0.0f, 0.0f);
	glUniform1f(sizeLocation, 1.0f);
	glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);
	glDrawArrays(GL_LINES, 3, 4);
	glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_A) filled = true;
	else if (key == GLFW_KEY_B) filled = false;
	else if (key == GLFW_KEY_C) {
		for (Shape& shape : shapes)
			shape.visible = false;
	}
	else if (key == GLFW_KEY_Q)
		glfwSetWindowShouldClose(window, true);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if (mouseX < 0.0 || mouseX >= WINDOW_WIDTH || mouseY < 0.0 || mouseY >= WINDOW_HEIGHT)
		return;

	//--- 축 위의 클릭은 오른쪽 또는 아래쪽 사분면에 포함한다.
	int index = (mouseX >= WINDOW_WIDTH / 2.0 ? 1 : 0) +
		(mouseY >= WINDOW_HEIGHT / 2.0 ? 2 : 0);
	uniform_real_distribution<float> colorDistribution(COLOR_MIN, COLOR_MAX);
	uniform_real_distribution<float> sizeDistribution(SHAPE_MIN_SIZE, SHAPE_MAX_SIZE);
	Shape& shape = shapes[index];
	shape.visible = true;
	shape.x = static_cast<float>(mouseX);
	shape.y = static_cast<float>(mouseY);
	shape.size = sizeDistribution(generator);
	shape.animationTime = 0.0f;
	shape.color = {
		colorDistribution(generator),
		colorDistribution(generator),
		colorDistribution(generator)
	};
}