#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CELL_SIZE 30.0f
#define PIECE_SIZE 22.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define MOVE_INTERVAL 0.10
#define MIN_INTERVAL 0.05
#define MAX_INTERVAL 0.15
#define SPEED_STEP 0.01
#define SPEED_LEVELS 5
#define EFFECT_DURATION 0.30

int OBSTACLES_PER_TYPE;

struct Vec2 { float x, y; };
struct Color { float r, g, b; };
enum PieceType { SQUARE, TRIANGLE, INVERTED_TRIANGLE };
struct Piece { PieceType type; int column, row; Color color; };
struct Effect { int column, row; double time; Color color; };

vector<Piece> pieces;
vector<Effect> effects;
Piece player;
mt19937 generator(random_device{}());
int columns = 20, rows = 20, pathIndex = 0, speedLevel = 0;
bool moving = false;
double previousTime = 0.0, moveTimer = 0.0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint offsetLocation = -1, sizeLocation = -1;
GLint windowLocation = -1, colorLocation = -1, alphaLocation = -1;

string ReadFile(const char* name);
bool InitShader();
void InitBuffer();
vector<Vec2> Vertices(PieceType type);
int ReadBoardSize();
Vec2 CellCenter(int column, int row);
void InitScene();
void DrawPiece(PieceType type, float x, float y, float size, Color color, bool fill, float alpha = 1.0f);
void DrawLine(Vec2 a, Vec2 b);
void DrawScene();
void MovePlayer();
void AdvanceTime(double dt);
void Update(GLFWwindow* window);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main()
{
	if (!ReadBoardSize()) return 0;
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
		"1-11 Zigzag Board", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }
	glfwSwapInterval(1);
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glfwSetKeyCallback(window, KeyCallback);
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!InitShader()) { glfwTerminate(); return -1; }
	InitBuffer();
	InitScene();
	previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		Update(window);
		DrawScene();
		glfwSwapBuffers(window);
	}
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(programID);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

string ReadFile(const char* name)
{
	ifstream file(name);
	if (!file) { cerr << "ERROR: 파일을 열 수 없습니다: " << name << endl; return ""; }
	stringstream stream; stream << file.rdbuf(); return stream.str();
}

bool InitShader()
{
	string sources[2] = { ReadFile("vertex.glsl"), ReadFile("fragment.glsl") };
	if (sources[0].empty() || sources[1].empty()) return false;
	GLuint shaders[2] = { glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER) };
	for (int i = 0; i < 2; ++i) {
		const char* code = sources[i].c_str();
		glShaderSource(shaders[i], 1, &code, nullptr); glCompileShader(shaders[i]);
		GLint ok; glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
		if (!ok) { GLchar log[1024]; glGetShaderInfoLog(shaders[i], 1024, nullptr, log); cerr << log << endl; return false; }
	}
	programID = glCreateProgram();
	glAttachShader(programID, shaders[0]); glAttachShader(programID, shaders[1]);
	glLinkProgram(programID);
	GLint ok; glGetProgramiv(programID, GL_LINK_STATUS, &ok);
	if (!ok) { GLchar log[1024]; glGetProgramInfoLog(programID, 1024, nullptr, log); cerr << log << endl; return false; }
	glDeleteShader(shaders[0]); glDeleteShader(shaders[1]);
	offsetLocation = glGetUniformLocation(programID, "uOffset");
	sizeLocation = glGetUniformLocation(programID, "uSize");
	windowLocation = glGetUniformLocation(programID, "uWindowSize");
	colorLocation = glGetUniformLocation(programID, "uColor");
	alphaLocation = glGetUniformLocation(programID, "uAlpha");
	return alphaLocation >= 0 && offsetLocation >= 0 && sizeLocation >= 0 && windowLocation >= 0 && colorLocation >= 0;
}

void InitBuffer()
{
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);
	glEnableVertexAttribArray(0); glBindVertexArray(0);
}

vector<Vec2> Vertices(PieceType type)
{
	if (type == SQUARE) return { {-0.5f,-0.5f},{0.5f,-0.5f},{0.5f,0.5f},{-0.5f,0.5f} };
	if (type == TRIANGLE) return { {0,-0.5f},{0.5f,0.5f},{-0.5f,0.5f} };
	return { {-0.5f,-0.5f},{0.5f,-0.5f},{0,0.5f} };
}

int ReadBoardSize()
{
	string line;
	while (true) {
		cout << "보드 가로, 세로 입력 (각 10-30): ";
		if (!getline(cin, line)) return false;
		istringstream input(line); string extra;
		if (input >> columns >> rows && !(input >> extra) &&
			columns >= 10 && columns <= 30 && rows >= 10 && rows <= 30) return true;
		cout << "10~30 사이의 두 정수를 입력하시오, 예시: 20 20" << endl;
	}
}

Vec2 CellCenter(int column, int row)
{
	return { (WINDOW_WIDTH - columns * CELL_SIZE) / 2 + (column + 0.5f) * CELL_SIZE,
		(WINDOW_HEIGHT - rows * CELL_SIZE) / 2 + (row + 0.5f) * CELL_SIZE };
}

void InitScene()
{
	OBSTACLES_PER_TYPE = (columns + rows) / 2;
	pieces.clear(); effects.clear();
	player = { SQUARE,0,0,{1.0f,1.0f,1.0f} };
	pathIndex = 0; speedLevel = 0; moving = false;
	moveTimer = 0.0;
	vector<int> cells;
	for (int i = 1; i < columns * rows; ++i) cells.push_back(i);
	shuffle(cells.begin(), cells.end(), generator);
	uniform_real_distribution<float> color(COLOR_MIN, COLOR_MAX);
	for (int type = 0; type < 3; ++type) {
		for (int i = 0; i < OBSTACLES_PER_TYPE; ++i) {
			int cell = cells[type * OBSTACLES_PER_TYPE + i];
			pieces.push_back({ (PieceType)type,cell % columns,cell / columns,
				{color(generator),color(generator),color(generator)} });
		}
	}
}

void DrawPiece(PieceType type, float x, float y, float size, Color color, bool fill, float alpha)
{
	vector<Vec2> v = Vertices(type);
	glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(Vec2), v.data(), GL_DYNAMIC_DRAW);
	glUniform2f(offsetLocation, x, y); glUniform1f(sizeLocation, size);
	glUniform1f(alphaLocation, alpha);
	if (fill) {
		glUniform3f(colorLocation, color.r, color.g, color.b);
		glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)v.size());
	}
	Color outline = fill ? Color{0,0,0} : color;
	glUniform3f(colorLocation, outline.r, outline.g, outline.b);
	glLineWidth(1); glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)v.size());
}

void DrawLine(Vec2 a, Vec2 b)
{
	Vec2 v[2] = { a,b }; glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_DYNAMIC_DRAW);
	glUniform2f(offsetLocation, 0, 0); glUniform1f(sizeLocation, 1);
	glUniform1f(alphaLocation, 1); glUniform3f(colorLocation, 0.65f, 0.65f, 0.65f);
	glLineWidth(1); glDrawArrays(GL_LINES, 0, 2);
}

void DrawScene()
{
	glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT); glUseProgram(programID);
	glUniform2f(windowLocation, WINDOW_WIDTH, WINDOW_HEIGHT);
	glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	float left = (WINDOW_WIDTH - columns * CELL_SIZE) / 2;
	float top = (WINDOW_HEIGHT - rows * CELL_SIZE) / 2;
	for (int i = 0; i <= columns; ++i)
		DrawLine({left + i * CELL_SIZE,top}, {left + i * CELL_SIZE,top + rows * CELL_SIZE});
	for (int i = 0; i <= rows; ++i)
		DrawLine({left,top + i * CELL_SIZE}, {left + columns * CELL_SIZE,top + i * CELL_SIZE});
	for (const Piece& p : pieces) {
		Vec2 center = CellCenter(p.column, p.row);
		DrawPiece(p.type, center.x, center.y, PIECE_SIZE, p.color, true);
	}
	Vec2 center = CellCenter(player.column, player.row);
	DrawPiece(player.type, center.x, center.y, PIECE_SIZE, player.color, true);
	for (const Effect& effect : effects) {
		float progress = (float)(effect.time / EFFECT_DURATION);
		Vec2 position = CellCenter(effect.column, effect.row);
		DrawPiece(SQUARE, position.x, position.y, CELL_SIZE * (1 + progress),
			effect.color, false, 1 - progress);
	}
	glBindVertexArray(0);
}

void MovePlayer()
{
	if (pathIndex >= columns * rows - 1) { moving = false; return; }
	++pathIndex;
	player.row = pathIndex / columns;
	int step = pathIndex % columns;
	player.column = player.row % 2 == 0 ? step : columns - 1 - step;
	for (Piece& p : pieces) {
		if (p.column != player.column || p.row != player.row) continue;
		swap(player.type, p.type); swap(player.color, p.color);
		effects.push_back({player.column,player.row,0.0,player.color});
		break;
	}
	if (pathIndex == columns * rows - 1) moving = false;
}

void AdvanceTime(double dt)
{
	for (Effect& effect : effects) effect.time += dt;
	effects.erase(remove_if(effects.begin(), effects.end(),
		[](const Effect& effect) { return effect.time >= EFFECT_DURATION; }), effects.end());
}

void Update(GLFWwindow*)
{
	double now = glfwGetTime(), dt = now - previousTime; previousTime = now;
	double interval = clamp(MOVE_INTERVAL - speedLevel * SPEED_STEP, MIN_INTERVAL, MAX_INTERVAL);
	while (moving && moveTimer + dt >= interval) {
		double remaining = max(0.0, interval - moveTimer);
		AdvanceTime(remaining); dt -= remaining; moveTimer = 0.0;
		MovePlayer();
	}
	AdvanceTime(dt);
	if (moving) moveTimer += dt;
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS) return;
	if (key == GLFW_KEY_S && pathIndex < columns * rows - 1) {
		moving = !moving; moveTimer = 0.0; previousTime = glfwGetTime();
	}
	if (key == GLFW_KEY_R) { InitScene(); previousTime = glfwGetTime(); }
	if (key == GLFW_KEY_KP_ADD || (key == GLFW_KEY_EQUAL && (mods & GLFW_MOD_SHIFT))) {
		speedLevel = min(speedLevel + 1, SPEED_LEVELS); moveTimer = 0.0;
	}
	if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) {
		speedLevel = max(speedLevel - 1, -SPEED_LEVELS); moveTimer = 0.0;
	}
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
}
