// 링커 -> 입력 -> 추가 종속성: opengl32.lib glew32.lib glfw3.lib
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>

using namespace std;

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define MAX_SHAPES 50
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define POINT_MIN_SIZE 18.0f
#define POINT_MAX_SIZE 30.0f
#define LINE_MIN_LENGTH 100.0f
#define LINE_MAX_LENGTH 240.0f
#define LINE_PICK_TOLERANCE 8.0f
#define SHAPE_MIN_SIZE 80.0f
#define SHAPE_MAX_SIZE 180.0f
#define MOVE_SPEED 300.0f
#define BASIC_OUTLINE_WIDTH 1.0f
#define OUTLINE_WIDTH 3.0f

struct Vec2 {
	float x, y;
};

struct Color {
	float r, g, b;
};

enum ShapeType {
	POINT_SHAPE,
	LINE_SHAPE,
	TRIANGLE_SHAPE,
	RECTANGLE_SHAPE
};

struct Shape {
	ShapeType type;
	float x, y;
	vector<Vec2> vertices;
	Color color;
};

vector<Shape> shapes;
int selectedIndex = -1;
double previousTime = 0.0;
random_device rd;
mt19937 generator(rd());

GLuint shaderProgramID = 0;
GLuint vertexShader = 0;
GLuint fragmentShader = 0;
GLuint vao = 0;
GLuint vbo = 0;
GLint offsetLocation = -1;

string filetobuf(const char* fileName);
bool CompileShader(GLuint shader, const string& source, const char* shaderName);
bool InitShader();
void InitBuffer();
void AddShape(ShapeType type);
void ClampShape(Shape& shape);
void MoveShape(Shape& shape, float dx, float dy);
void UpdateScene(GLFWwindow* window);
bool PointInShape(const Shape& shape, float x, float y);
void AddVertex(vector<GLfloat>& data, float x, float y, const Color& color);
void AddTriangle(vector<GLfloat>& data, Vec2 a, Vec2 b, Vec2 c, const Color& color);
void AddFilledShape(vector<GLfloat>& data, const Shape& shape);
void AddOutlineSegment(vector<GLfloat>& data, Vec2 a, Vec2 b, float width);
void AddShapeOutline(vector<GLfloat>& data, const Shape& shape, float width);
void AddSelectedOutline(vector<GLfloat>& data, const Shape& shape);
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
		WINDOW_WIDTH, WINDOW_HEIGHT, "1-7 Modern OpenGL Shapes", nullptr, nullptr);
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
	shapes.reserve(MAX_SHAPES);
	previousTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		UpdateScene(window);
		DrawScene();
		glfwSwapBuffers(window);
	}

	glDeleteBuffers(1, &vbo);
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
	if (offsetLocation == -1) {
		cerr << "ERROR: uOffset uniform을 찾을 수 없습니다." << endl;
		glDeleteProgram(shaderProgramID);
		shaderProgramID = 0;
		return false;
	}
	return true;
}

void InitBuffer()
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
		6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

void AddShape(ShapeType type)
{
	if (shapes.size() >= MAX_SHAPES)
		return;

	uniform_real_distribution<float> colorDistribution(COLOR_MIN, COLOR_MAX);
	uniform_real_distribution<float> shapeSizeDistribution(SHAPE_MIN_SIZE, SHAPE_MAX_SIZE);
	uniform_real_distribution<float> pointSizeDistribution(POINT_MIN_SIZE, POINT_MAX_SIZE);
	uniform_real_distribution<float> lineLengthDistribution(LINE_MIN_LENGTH, LINE_MAX_LENGTH);
	uniform_real_distribution<float> angleDistribution(0.0f, 6.283185307f);

	Shape shape;
	shape.type = type;
	shape.color = {
		colorDistribution(generator),
		colorDistribution(generator),
		colorDistribution(generator)
	};

	if (type == POINT_SHAPE) {
		float half = pointSizeDistribution(generator) / 2.0f;
		shape.vertices = { {-half, -half}, {half, -half}, {half, half}, {-half, half} };
	}
	else if (type == LINE_SHAPE) {
		float halfLength = lineLengthDistribution(generator) / 2.0f;
		float angle = angleDistribution(generator);
		float directionX = cos(angle);
		float directionY = sin(angle);
		shape.vertices = {
			{-directionX * halfLength, -directionY * halfLength},
			{ directionX * halfLength,  directionY * halfLength}
		};
	}
	else if (type == TRIANGLE_SHAPE) {
		float size = shapeSizeDistribution(generator);
		float halfWidth = size / 2.0f;
		float halfHeight = size * 0.45f;
		shape.vertices = { {0.0f, -halfHeight}, {halfWidth, halfHeight}, {-halfWidth, halfHeight} };
	}
	else {
		float width = shapeSizeDistribution(generator);
		float height = shapeSizeDistribution(generator);
		shape.vertices = {
			{-width / 2.0f, -height / 2.0f}, {width / 2.0f, -height / 2.0f},
			{ width / 2.0f,  height / 2.0f}, {-width / 2.0f,  height / 2.0f}
		};
	}

	float minX = shape.vertices[0].x;
	float maxX = shape.vertices[0].x;
	float minY = shape.vertices[0].y;
	float maxY = shape.vertices[0].y;
	for (const Vec2& vertex : shape.vertices) {
		minX = min(minX, vertex.x);
		maxX = max(maxX, vertex.x);
		minY = min(minY, vertex.y);
		maxY = max(maxY, vertex.y);
	}

	uniform_real_distribution<float> xDistribution(-minX, WINDOW_WIDTH - maxX);
	uniform_real_distribution<float> yDistribution(-minY, WINDOW_HEIGHT - maxY);
	shape.x = xDistribution(generator);
	shape.y = yDistribution(generator);
	shapes.push_back(shape);
}

void ClampShape(Shape& shape)
{
	float minX = shape.x + shape.vertices[0].x;
	float maxX = minX;
	float minY = shape.y + shape.vertices[0].y;
	float maxY = minY;
	for (const Vec2& vertex : shape.vertices) {
		minX = min(minX, shape.x + vertex.x);
		maxX = max(maxX, shape.x + vertex.x);
		minY = min(minY, shape.y + vertex.y);
		maxY = max(maxY, shape.y + vertex.y);
	}

	if (minX < 0.0f) shape.x -= minX;
	if (maxX > WINDOW_WIDTH) shape.x -= maxX - WINDOW_WIDTH;
	if (minY < 0.0f) shape.y -= minY;
	if (maxY > WINDOW_HEIGHT) shape.y -= maxY - WINDOW_HEIGHT;
}

void MoveShape(Shape& shape, float dx, float dy)
{
	shape.x += dx;
	shape.y += dy;
	ClampShape(shape);
}

void UpdateScene(GLFWwindow* window)
{
	double currentTime = glfwGetTime();
	float deltaTime = static_cast<float>(currentTime - previousTime);
	previousTime = currentTime;
	deltaTime = min(deltaTime, 0.05f);

	float selectedX = 0.0f;
	float selectedY = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) selectedX -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) selectedX += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) selectedY -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) selectedY += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) { selectedX -= 1.0f; selectedY -= 1.0f; }
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) { selectedX += 1.0f; selectedY -= 1.0f; }
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) { selectedX -= 1.0f; selectedY += 1.0f; }
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) { selectedX += 1.0f; selectedY += 1.0f; }

	float selectedLength = sqrt(selectedX * selectedX + selectedY * selectedY);
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>(shapes.size()) && selectedLength > 0.0f) {
		MoveShape(shapes[selectedIndex], selectedX / selectedLength * MOVE_SPEED * deltaTime,
			selectedY / selectedLength * MOVE_SPEED * deltaTime);
	}

	float allX = 0.0f;
	float allY = 0.0f;
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) allX -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) allX += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) allY -= 1.0f;
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) allY += 1.0f;

	float allLength = sqrt(allX * allX + allY * allY);
	if (allLength > 0.0f) {
		float dx = allX / allLength * MOVE_SPEED * deltaTime;
		float dy = allY / allLength * MOVE_SPEED * deltaTime;
		for (Shape& shape : shapes)
			MoveShape(shape, dx, dy);
	}
}

bool PointInShape(const Shape& shape, float x, float y)
{
	if (shape.type == LINE_SHAPE) {
		Vec2 a = { shape.x + shape.vertices[0].x, shape.y + shape.vertices[0].y };
		Vec2 b = { shape.x + shape.vertices[1].x, shape.y + shape.vertices[1].y };
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		float lengthSquared = dx * dx + dy * dy;
		float t = ((x - a.x) * dx + (y - a.y) * dy) / lengthSquared;
		t = clamp(t, 0.0f, 1.0f);
		float closestX = a.x + t * dx;
		float closestY = a.y + t * dy;
		float distanceX = x - closestX;
		float distanceY = y - closestY;
		return distanceX * distanceX + distanceY * distanceY <=
			LINE_PICK_TOLERANCE * LINE_PICK_TOLERANCE;
	}

	bool hasPositive = false;
	bool hasNegative = false;
	for (size_t i = 0; i < shape.vertices.size(); ++i) {
		Vec2 a = { shape.x + shape.vertices[i].x, shape.y + shape.vertices[i].y };
		Vec2 b = { shape.x + shape.vertices[(i + 1) % shape.vertices.size()].x,
			shape.y + shape.vertices[(i + 1) % shape.vertices.size()].y };
		float cross = (b.x - a.x) * (y - a.y) - (b.y - a.y) * (x - a.x);
		if (cross > 0.0f) hasPositive = true;
		if (cross < 0.0f) hasNegative = true;
		if (hasPositive && hasNegative) return false;
	}
	return true;
}

void AddVertex(vector<GLfloat>& data, float x, float y, const Color& color)
{
	//--- 중심 기준 상대 픽셀 좌표만 NDC 크기로 변환한다.
	//--- 화면상의 중심 위치는 버텍스 셰이더의 uOffset이 적용한다.
	data.push_back(x / WINDOW_WIDTH * 2.0f);
	data.push_back(-y / WINDOW_HEIGHT * 2.0f);
	data.push_back(0.0f);
	data.push_back(color.r);
	data.push_back(color.g);
	data.push_back(color.b);
}

void AddTriangle(vector<GLfloat>& data, Vec2 a, Vec2 b, Vec2 c, const Color& color)
{
	AddVertex(data, a.x, a.y, color);
	AddVertex(data, b.x, b.y, color);
	AddVertex(data, c.x, c.y, color);
}

void AddFilledShape(vector<GLfloat>& data, const Shape& shape)
{
	//--- 사각형은 두 개의 삼각형으로 명시적으로 나누어 그린다.
	if (shape.type == RECTANGLE_SHAPE) {
		Vec2 p0 = shape.vertices[0];
		Vec2 p1 = shape.vertices[1];
		Vec2 p2 = shape.vertices[2];
		Vec2 p3 = shape.vertices[3];

		AddTriangle(data, p0, p1, p2, shape.color);
		AddTriangle(data, p0, p2, p3, shape.color);
		return;
	}

	Vec2 first = shape.vertices[0];
	for (size_t i = 1; i + 1 < shape.vertices.size(); ++i) {
		Vec2 second = shape.vertices[i];
		Vec2 third = shape.vertices[i + 1];
		AddTriangle(data, first, second, third, shape.color);
	}
}

void AddOutlineSegment(vector<GLfloat>& data, Vec2 a, Vec2 b, float width)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float length = sqrt(dx * dx + dy * dy);
	if (length <= 0.0f) return;

	float offsetX = -dy / length * width / 2.0f;
	float offsetY = dx / length * width / 2.0f;
	Vec2 p1 = { a.x + offsetX, a.y + offsetY };
	Vec2 p2 = { b.x + offsetX, b.y + offsetY };
	Vec2 p3 = { b.x - offsetX, b.y - offsetY };
	Vec2 p4 = { a.x - offsetX, a.y - offsetY };
	Color black = { 0.0f, 0.0f, 0.0f };
	AddTriangle(data, p1, p2, p3, black);
	AddTriangle(data, p1, p3, p4, black);
}

void AddShapeOutline(vector<GLfloat>& data, const Shape& shape, float width)
{
	for (size_t i = 0; i < shape.vertices.size(); ++i) {
		Vec2 a = shape.vertices[i];
		Vec2 b = shape.vertices[(i + 1) % shape.vertices.size()];
		AddOutlineSegment(data, a, b, width);
	}
}

void AddSelectedOutline(vector<GLfloat>& data, const Shape& shape)
{
	if (shape.type == LINE_SHAPE) {
		AddOutlineSegment(data, shape.vertices[0], shape.vertices[1], OUTLINE_WIDTH);
		return;
	}
	AddShapeOutline(data, shape, OUTLINE_WIDTH);
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);

	//--- 도형마다 서로 다른 중심 위치를 uOffset으로 전달한다.
	for (const Shape& shape : shapes) {
		vector<GLfloat> fillData;
		if (shape.type == LINE_SHAPE) {
			AddVertex(fillData, shape.vertices[0].x, shape.vertices[0].y, shape.color);
			AddVertex(fillData, shape.vertices[1].x, shape.vertices[1].y, shape.color);
		}
		else {
			AddFilledShape(fillData, shape);
			//--- 모든 면 도형의 외곽을 검은색 1픽셀 선으로 감싼다.
			AddShapeOutline(fillData, shape, BASIC_OUTLINE_WIDTH);
			//--- 사각형을 나누는 두 삼각형 사이의 대각선을 화면에 표시한다.
			if (shape.type == RECTANGLE_SHAPE)
				AddOutlineSegment(fillData, shape.vertices[0], shape.vertices[2],
					BASIC_OUTLINE_WIDTH);
		}
		float offsetX = shape.x / WINDOW_WIDTH * 2.0f - 1.0f;
		float offsetY = 1.0f - shape.y / WINDOW_HEIGHT * 2.0f;
		glUniform2f(offsetLocation, offsetX, offsetY);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, fillData.size() * sizeof(GLfloat),
			fillData.data(), GL_DYNAMIC_DRAW);
		if (shape.type == LINE_SHAPE) {
			glLineWidth(1.0f);
			glDrawArrays(GL_LINES, 0, 2);
		}
		else
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fillData.size() / 6));
	}

	//--- 선택된 도형의 검은 윤곽은 모든 도형을 그린 뒤 마지막에 출력한다.
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>(shapes.size())) {
		vector<GLfloat> outlineData;
		const Shape& selectedShape = shapes[selectedIndex];
		AddSelectedOutline(outlineData, selectedShape);
		float offsetX = selectedShape.x / WINDOW_WIDTH * 2.0f - 1.0f;
		float offsetY = 1.0f - selectedShape.y / WINDOW_HEIGHT * 2.0f;
		glUniform2f(offsetLocation, offsetX, offsetY);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, outlineData.size() * sizeof(GLfloat),
			outlineData.data(), GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(outlineData.size() / 6));

		//--- 선택된 선의 검은 강조선 위에 원래 색상의 1픽셀 선을 다시 그린다.
		if (selectedShape.type == LINE_SHAPE) {
			vector<GLfloat> lineData;
			AddVertex(lineData, selectedShape.vertices[0].x,
				selectedShape.vertices[0].y, selectedShape.color);
			AddVertex(lineData, selectedShape.vertices[1].x,
				selectedShape.vertices[1].y, selectedShape.color);
			glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(GLfloat),
				lineData.data(), GL_DYNAMIC_DRAW);
			glLineWidth(1.0f);
			glDrawArrays(GL_LINES, 0, 2);
		}
	}

	glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	//--- GLFW_REPEAT은 처리하지 않아 생성 키를 길게 눌러도 한 번만 생성된다.
	if (action != GLFW_PRESS)
		return;

	if (key == GLFW_KEY_P) AddShape(POINT_SHAPE);
	else if (key == GLFW_KEY_E) AddShape(LINE_SHAPE);
	else if (key == GLFW_KEY_T) AddShape(TRIANGLE_SHAPE);
	else if (key == GLFW_KEY_R) AddShape(RECTANGLE_SHAPE);
	else if (key == GLFW_KEY_C) {
		shapes.clear();
		selectedIndex = -1;
	}
	else if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q)
		glfwSetWindowShouldClose(window, true);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
		return;

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	selectedIndex = -1;

	//--- 생성 순서의 역순으로 검사하여 화면에서 가장 위의 도형을 선택한다.
	for (int i = static_cast<int>(shapes.size()) - 1; i >= 0; --i) {
		if (PointInShape(shapes[i], static_cast<float>(mouseX), static_cast<float>(mouseY))) {
			selectedIndex = i;
			break;
		}
	}
}
