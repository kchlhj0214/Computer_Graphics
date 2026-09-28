// 링커 -> 입력 -> 추가 종속성: opengl32.lib; glew32.lib; glfw3.lib
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <algorithm>
#include <vector>
#include <cmath>

using namespace std;

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define SHAPE_MIN_SIZE 50.0f
#define SHAPE_MAX_SIZE 150.0f
#define SPEED_MIN 400.0f
#define SPEED_MAX 800.0f
#define ZIGZAG_VERTICAL_DISTANCE 100.0f
#define VERTICAL_ZIGZAG_X_RATIO 0.25f
#define PATH_PREVIEW_LENGTH 3000.0f
#define PI 3.14159265359f
#define ROTATE_SPEED (2.0f * PI)

struct Vec2 { float x, y; };
struct Color { float r, g, b; };
struct Shape {
	float x = 0.0f, y = 0.0f;
	float size = 100.0f, speed = 100.0f, angle = 0.0f;
	Color color = {};
	//--- dx/dy는 앞으로 추가할 경로의 방향이다.
	float dx = 1.0f, dy = 1.0f;
	bool vertical = false;
	vector<Vec2> path;
	int target = 1, pathDirection = 1;
};

vector<Shape> shapes;
int motion = 0;
double previousTime = 0.0;
mt19937 generator(random_device{}());
GLuint shaderProgramID = 0, vertexShader = 0, fragmentShader = 0;
GLuint vao = 0, vbo = 0;
GLint offsetLocation = -1, sizeLocation = -1, colorLocation = -1;
GLint angleLocation = -1, modeLocation = -1;

string filetobuf(const char* fileName);
bool CompileShader(GLuint shader, const string& source, const char* shaderName);
bool InitShader();
void InitBuffer();
void BuildPath(Shape& shape, bool reset);
void UpdateScene();
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
		WINDOW_WIDTH, WINDOW_HEIGHT, "1-9 Moving Triangles", nullptr, nullptr);
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
		UpdateScene();
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
	sizeLocation = glGetUniformLocation(shaderProgramID, "uSize");
	colorLocation = glGetUniformLocation(shaderProgramID, "uColor");
	angleLocation = glGetUniformLocation(shaderProgramID, "uAngle");
	modeLocation = glGetUniformLocation(shaderProgramID, "uPath");
	if (offsetLocation == -1 || sizeLocation == -1 || colorLocation == -1 || angleLocation == -1 || modeLocation == -1) {
		cerr << "ERROR: 도형 uniform을 찾을 수 없습니다." << endl;
		glDeleteProgram(shaderProgramID);
		shaderProgramID = 0;
		return false;
	}
	return true;
}

void InitBuffer()
{
	//--- 경로의 시작점/끝점만 전송한다. 선 두께와 삼각형 꼭짓점은 셰이더에서 생성.
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribDivisor(0, 1);
	glBindVertexArray(0);
}

void BuildPath(Shape& shape, bool reset)
{
	//--- 2번은 제자리 회전 중에도 꼭짓점이 창 밖으로 나가지 않도록 여유를 둔다.
	float halfX = motion == 2 ? shape.size * sqrt(1.25f) : shape.size / 2.0f;
	float halfY = motion == 2 ? halfX : shape.size;
	float right = WINDOW_WIDTH - halfX, bottom = WINDOW_HEIGHT - halfY;
	if (reset) {
		shape.x = clamp(shape.x, halfX + 1.0f, right - 1.0f);
		shape.y = clamp(shape.y, halfY + 1.0f, bottom - 1.0f);
		shape.angle = 0.0f;
		shape.vertical = false;
		shape.target = 1;
		shape.pathDirection = 1;
		shape.path = { {shape.x, shape.y} };
	}
	if (motion == 0) return;

	if (motion == 4) {
		//--- 아르키메데스 나선. 벽과 처음 만나는 지점까지만 미리 계산한다.
		Vec2 center = shape.path.front();
		float startAngle = atan2(shape.dy, shape.dx);
		float spin = shape.dx > 0.0f ? 1.0f : -1.0f;
		for (int i = 1; i <= 10000; ++i) {
			float theta = i * 0.025f;
			float radius = theta * 12.0f;
			Vec2 next = { center.x + radius * cos(startAngle + spin * theta),
				center.y + radius * sin(startAngle + spin * theta) };
			Vec2 last = shape.path.back();
			float t = 1.0f;
			if (next.x < halfX) t = min(t, (halfX - last.x) / (next.x - last.x));
			if (next.x > right) t = min(t, (right - last.x) / (next.x - last.x));
			if (next.y < halfY) t = min(t, (halfY - last.y) / (next.y - last.y));
			if (next.y > bottom) t = min(t, (bottom - last.y) / (next.y - last.y));
			shape.path.push_back({last.x + (next.x - last.x) * t,
				last.y + (next.y - last.y) * t});
			if (t < 1.0f) break;
		}
		return;
	}

	//--- 현재 이동 중인 구간을 제외해도 표시 길이만큼 미래 경로를 확보한다.
	float queuedLength = 0.0f;
	for (size_t i = 2; i < shape.path.size(); ++i) {
		float dx = shape.path[i].x - shape.path[i - 1].x;
		float dy = shape.path[i].y - shape.path[i - 1].y;
		queuedLength += sqrt(dx * dx + dy * dy);
	}
	while (queuedLength < max(1.0f, PATH_PREVIEW_LENGTH) || shape.path.size() < 3) {
		Vec2 p = shape.path.back();
		if (motion == 2) {
			if (!shape.vertical) {
				p.x = shape.dx > 0.0f ? right : halfX;
				shape.dx = -shape.dx;
			}
			else {
				if (p.y <= halfY + 0.001f) shape.dy = abs(shape.dy);
				if (p.y >= bottom - 0.001f) shape.dy = -abs(shape.dy);
				p.y = clamp(p.y + (shape.dy > 0.0f ? 1.0f : -1.0f) *
					ZIGZAG_VERTICAL_DISTANCE, halfY, bottom);
			}
			shape.vertical = !shape.vertical;
		}
		else {
			if (p.x <= halfX + 0.001f) shape.dx = abs(shape.dx);
			if (p.x >= right - 0.001f) shape.dx = -abs(shape.dx);
			if (p.y <= halfY + 0.001f) shape.dy = abs(shape.dy);
			if (p.y >= bottom - 0.001f) shape.dy = -abs(shape.dy);
			float vx = motion == 3 ? (shape.dx > 0.0f ? VERTICAL_ZIGZAG_X_RATIO : -VERTICAL_ZIGZAG_X_RATIO) : shape.dx;
			float vy = motion == 3 ? (shape.dy > 0.0f ? 1.0f : -1.0f) : shape.dy;
			float tx = ((vx > 0.0f ? right : halfX) - p.x) / vx;
			float ty = ((vy > 0.0f ? bottom : halfY) - p.y) / vy;
			float distance = min(tx, ty);
			p.x = clamp(p.x + vx * distance, halfX, right);
			p.y = clamp(p.y + vy * distance, halfY, bottom);
			if (tx <= distance + 0.001f) shape.dx = -shape.dx;
			if (ty <= distance + 0.001f) shape.dy = -shape.dy;
		}
		float segmentX = p.x - shape.path.back().x;
		float segmentY = p.y - shape.path.back().y;
		queuedLength += sqrt(segmentX * segmentX + segmentY * segmentY);
		shape.path.push_back(p);
	}
}

void UpdateScene()
{
	double now = glfwGetTime();
	float deltaTime = min(static_cast<float>(now - previousTime), 0.05f);
	previousTime = now;
	if (motion == 0) return;

	for (Shape& shape : shapes) {
		float remaining = deltaTime;
		while (remaining > 0.000001f && shape.path.size() > 1) {
			Vec2 destination = shape.path[shape.target];
			float dx = destination.x - shape.x, dy = destination.y - shape.y;
			float distance = sqrt(dx * dx + dy * dy);
			if (motion == 2 && distance > 0.001f) {
				float targetAngle = atan2(dx, -dy);
				float difference = remainder(targetAngle - shape.angle, 2.0f * PI);
				float turnTime = abs(difference) / ROTATE_SPEED;
				if (turnTime > remaining) {
					shape.angle += (difference > 0.0f ? 1.0f : -1.0f) * ROTATE_SPEED * remaining;
					break;
				}
				shape.angle = targetAngle;
				remaining -= turnTime;
			}
			float travelTime = distance / shape.speed;
			if (travelTime > remaining) {
				shape.x += dx / distance * shape.speed * remaining;
				shape.y += dy / distance * shape.speed * remaining;
				break;
			}
			shape.x = destination.x;
			shape.y = destination.y;
			remaining -= travelTime;
			if (motion == 4) {
				//--- 벽에서는 경로를 역순으로 따라 중심까지 돌아온다.
				if (shape.target == static_cast<int>(shape.path.size()) - 1) shape.pathDirection = -1;
				if (shape.target == 0) shape.pathDirection = 1;
				shape.target += shape.pathDirection;
				//--- 가장자리에서 생성되어 나선 길이가 0인 경우 반복을 방지한다.
				if (distance < 0.000001f && shape.path.size() == 2) break;
			}
			else {
				shape.path.erase(shape.path.begin());
				BuildPath(shape, false);
			}
		}
	}
}

void DrawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramID);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	//--- 앞으로 이동할 경로를 먼저 그린다. 3픽셀 선은 셰이더가 면으로 생성한다.
	vector<float> segments;
	for (const Shape& shape : shapes) {
		float remaining = max(0.0f, PATH_PREVIEW_LENGTH);
		Vec2 a = {shape.x, shape.y};
		int index = shape.target, direction = shape.pathDirection;
		//--- 나선도 현재 진행 방향으로 미리 표시한다. 왕복 한 주기까지만 그린다.
		size_t limit = motion == 4 ? shape.path.size() * 2 : shape.path.size();
		for (size_t step = 0; step < limit && remaining > 0.0f &&
			index >= 0 && index < static_cast<int>(shape.path.size()); ++step) {
			Vec2 b = shape.path[index];
			float dx = b.x - a.x, dy = b.y - a.y;
			float length = sqrt(dx * dx + dy * dy);
			if (length > 0.000001f) {
				float drawn = min(remaining, length);
				Vec2 end = {a.x + dx * drawn / length, a.y + dy * drawn / length};
				segments.insert(segments.end(), {a.x, a.y, end.x, end.y});
				remaining -= drawn;
			}
			a = b;
			if (motion == 4) {
				if (index == static_cast<int>(shape.path.size()) - 1) direction = -1;
				if (index == 0) direction = 1;
			}
			index += direction;
		}
	}
	//--- 삼각형만 그릴 때에도 활성화된 인스턴스 속성에 유효한 버퍼를 제공한다.
	if (segments.empty()) segments = {0.0f, 0.0f, 0.0f, 0.0f};
	glBufferData(GL_ARRAY_BUFFER, segments.size() * sizeof(float), segments.data(), GL_DYNAMIC_DRAW);
	glUniform1i(modeLocation, 1);
	glUniform3f(colorLocation, 1.0f, 0.0f, 0.0f);
	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(segments.size() / 4));

	glUniform1i(modeLocation, 0);
	for (const Shape& shape : shapes) {
		glUniform2f(offsetLocation, shape.x, shape.y);
		glUniform1f(sizeLocation, shape.size);
		glUniform1f(angleLocation, shape.angle);
		glUniform3f(colorLocation, shape.color.r, shape.color.g, shape.color.b);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glUniform3f(colorLocation, 0.0f, 0.0f, 0.0f);
		glDrawArrays(GL_LINE_LOOP, 0, 3);
	}
	glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action != GLFW_PRESS) return;
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4) {
		motion = key - GLFW_KEY_0;
		for (Shape& shape : shapes) BuildPath(shape, true);
	}
	else if (key == GLFW_KEY_C) shapes.clear();
	else if (key == GLFW_KEY_Q) glfwSetWindowShouldClose(window, true);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
{
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	if (x < 0.0 || x >= WINDOW_WIDTH || y < 0.0 || y >= WINDOW_HEIGHT) return;
	uniform_real_distribution<float> sizeDistribution(SHAPE_MIN_SIZE, SHAPE_MAX_SIZE);
	uniform_real_distribution<float> colorDistribution(0.1f, 0.9f);
	uniform_real_distribution<float> speedDistribution(SPEED_MIN, SPEED_MAX);
	uniform_real_distribution<float> directionDistribution(0.4f, 1.0f);
	Shape shape;
	shape.x = static_cast<float>(x);
	shape.y = static_cast<float>(y);
	shape.size = sizeDistribution(generator);
	shape.speed = speedDistribution(generator);
	shape.dx = directionDistribution(generator) * (generator() % 2 ? 1.0f : -1.0f);
	shape.dy = directionDistribution(generator) * (generator() % 2 ? 1.0f : -1.0f);
	shape.color = {colorDistribution(generator), colorDistribution(generator), colorDistribution(generator)};
	BuildPath(shape, true);
	shapes.push_back(shape);
}