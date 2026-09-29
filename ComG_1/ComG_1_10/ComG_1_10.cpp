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
#define BOARD_DIVIDER_X 1200.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define POSITION_TOLERANCE 15.0f
#define ROTATION_TOLERANCE 10.0f
#define ROTATION_STEP 1.0f
#define ROTATION_INTERVAL 0.02
#define PI 3.14159265359f
#define TRIANGLE_HEIGHT 86.60254038f
#define PENTAGON_RADIUS 85.06508084f

struct Vec2 { float x, y; };
struct Color { float r, g, b; };
enum PieceType { SQUARE, RECTANGLE, EQUILATERAL, HALF_TRIANGLE, PENTAGON };
struct TemplatePiece { PieceType type; float x, y, angle; };
struct Piece {
	PieceType type; float x, y, angle; Color color;
	bool active, locked; int slot;
};
struct Slot { PieceType type; float x, y, angle; int board, piece; };
struct Board { bool completed = false; };

vector<TemplatePiece> sourcePieces;
vector<Piece> pieces;
vector<Slot> slots;
vector<Board> boards(5);
mt19937 generator(random_device{}());
int dragging = -1;
double previousTime = 0.0, rotationTimer = 0.0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint offsetLocation = -1, angleLocation = -1;
GLint windowLocation = -1, colorLocation = -1;

string ReadFile(const char* name);
bool InitShader();
void InitBuffer();
vector<Vec2> Vertices(PieceType type);
Vec2 Rotate(Vec2 p, float degree);
float Normalize(float angle);
float AngleDifference(float a, float b);
float PieceAngleDifference(PieceType type, float a, float b);
void AddSlot(PieceType type, float x, float y, float angle, int board);
void InitScene();
bool Contains(PieceType type, float x, float y, float angle, float mx, float my);
void DrawPiece(PieceType type, float x, float y, float angle, Color color, bool fill);
void DrawLine(Vec2 a, Vec2 b);
void DrawScene();
void Update(GLFWwindow* window);
void StartDrag(float x, float y);
void FinishDrag();
void CheckComplete(int board);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
		"1-10 Shape Puzzle", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) { glfwTerminate(); return -1; }
	glfwSwapInterval(1);
	glfwSetMouseButtonCallback(window, MouseCallback);
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
	angleLocation = glGetUniformLocation(programID, "uAngle");
	windowLocation = glGetUniformLocation(programID, "uWindowSize");
	colorLocation = glGetUniformLocation(programID, "uColor");
	return offsetLocation >= 0 && angleLocation >= 0 && windowLocation >= 0 && colorLocation >= 0;
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
	if (type == SQUARE) return { {-50,-50},{50,-50},{50,50},{-50,50} };
	if (type == RECTANGLE) return { {-25,-50},{25,-50},{25,50},{-25,50} };
	if (type == EQUILATERAL) return { {0,-TRIANGLE_HEIGHT*2/3}, {50,TRIANGLE_HEIGHT/3}, {-50,TRIANGLE_HEIGHT/3} };
	if (type == HALF_TRIANGLE) return { {-25,-TRIANGLE_HEIGHT/2}, {25,-TRIANGLE_HEIGHT/2}, {-25,TRIANGLE_HEIGHT/2} };
	return { {-50,-68.8191f},{50,-68.8191f},{80.9017f,26.2866f},{0,PENTAGON_RADIUS},{-80.9017f,26.2866f} };
}

Vec2 Rotate(Vec2 p, float degree)
{
	float r = degree * PI / 180.0f, c = cos(r), s = sin(r);
	return { c*p.x-s*p.y, s*p.x+c*p.y };
}

float Normalize(float a) { a = fmod(a,360.0f); return a < 0 ? a+360.0f : a; }
float AngleDifference(float a, float b) { float d=fabs(Normalize(a)-Normalize(b)); return min(d,360.0f-d); }
float PieceAngleDifference(PieceType type, float a, float b)
{
	float period = 360.0f;
	if (type == SQUARE) period = 90.0f;
	else if (type == RECTANGLE) period = 180.0f;
	else if (type == EQUILATERAL) period = 120.0f;
	else if (type == PENTAGON) period = 72.0f;

	float difference = fmod(fabs(a - b), period);
	return min(difference, period - difference);
}
void AddSlot(PieceType type,float x,float y,float angle,int board) { slots.push_back({type,x,y,Normalize(angle),board,-1}); }

void InitScene()
{
	pieces.clear();
	slots.clear();
	for (Board& board : boards) board.completed = false;
	dragging = -1;
	rotationTimer = 0.0;
	sourcePieces = { {SQUARE,260,140,0},{RECTANGLE,260,350,0},
		{EQUILATERAL,260,560,0},{HALF_TRIANGLE,260,770,0},
		{PENTAGON,260,1010,180} };
	AddSlot(HALF_TRIANGLE,1400,60,0,0); AddSlot(HALF_TRIANGLE,1400,60,180,0);
	float d=TRIANGLE_HEIGHT*2/3;
	AddSlot(EQUILATERAL,1400,205-d,180,1); AddSlot(EQUILATERAL,1400+d,205,270,1);
	AddSlot(EQUILATERAL,1400,205+d,0,1); AddSlot(EQUILATERAL,1400-d,205,90,1);
	AddSlot(RECTANGLE,1375,350,0,2); AddSlot(RECTANGLE,1425,350,0,2);
	AddSlot(RECTANGLE,1375,450,0,2); AddSlot(RECTANGLE,1425,450,0,2);
	Vec2 center={1400,680}; AddSlot(PENTAGON,center.x,center.y,0,3);
	vector<Vec2> pentagon=Vertices(PENTAGON);
	for(int i=0;i<5;++i){
		Vec2 a=pentagon[i], b=pentagon[(i+1)%5];
		Vec2 m={(a.x+b.x)/2,(a.y+b.y)/2}; float len=sqrt(m.x*m.x+m.y*m.y);
		Vec2 out={m.x/len,m.y/len}; float angle=atan2(out.y,out.x)*180/PI+90;
		AddSlot(EQUILATERAL,center.x+m.x+out.x*TRIANGLE_HEIGHT/3,
			center.y+m.y+out.y*TRIANGLE_HEIGHT/3,angle,3);
	}
	Vec2 pivot={1385,1050}; float groupAngle=-45;
	vector<pair<PieceType,Vec2>> group={{PENTAGON,{0,0}},{SQUARE,{0,-118.8191f}},
		{EQUILATERAL,{0,-197.6866f}}};
	for(const auto& item:group){ Vec2 p=Rotate(item.second,groupAngle);
		AddSlot(item.first,pivot.x+p.x,pivot.y+p.y,groupAngle,4); }
}

bool Contains(PieceType type,float x,float y,float angle,float mx,float my)
{
	Vec2 p=Rotate({mx-x,my-y},-angle); vector<Vec2> v=Vertices(type);
	bool pos=false,neg=false;
	for(size_t i=0;i<v.size();++i){ Vec2 a=v[i],b=v[(i+1)%v.size()];
		float cross=(b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);
		if(cross>0)pos=true; if(cross<0)neg=true; if(pos&&neg)return false; }
	return true;
}

void DrawPiece(PieceType type,float x,float y,float angle,Color color,bool fill)
{
	vector<Vec2> v=Vertices(type); glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(Vec2),v.data(),GL_DYNAMIC_DRAW);
	glUniform2f(offsetLocation,x,y); glUniform1f(angleLocation,angle*PI/180);
	if(fill){ glUniform3f(colorLocation,color.r,color.g,color.b); glDrawArrays(GL_TRIANGLE_FAN,0,(GLsizei)v.size()); }
	glUniform3f(colorLocation,0,0,0); glLineWidth(1); glDrawArrays(GL_LINE_LOOP,0,(GLsizei)v.size());
}

void DrawLine(Vec2 a,Vec2 b)
{
	Vec2 v[2]={a,b}; glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_DYNAMIC_DRAW);
	glUniform2f(offsetLocation,0,0); glUniform1f(angleLocation,0); glUniform3f(colorLocation,0,0,0);
	glLineWidth(1); glDrawArrays(GL_LINES,0,2);
}

void DrawScene()
{
	glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT); glUseProgram(programID);
	glUniform2f(windowLocation,WINDOW_WIDTH,WINDOW_HEIGHT); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
	DrawLine({BOARD_DIVIDER_X,0},{BOARD_DIVIDER_X,WINDOW_HEIGHT});
	for(const TemplatePiece& p:sourcePieces)
		DrawPiece(p.type,p.x,p.y,p.angle,{0,0,0},false);
	for(const Slot& s:slots) DrawPiece(s.type,s.x,s.y,s.angle,{0,0,0},false);
	for(const Piece& p:pieces) if(p.active) DrawPiece(p.type,p.x,p.y,p.angle,p.color,true);
	glBindVertexArray(0);
}

void Update(GLFWwindow* window)
{
	double now=glfwGetTime(),dt=min(now-previousTime,0.05); previousTime=now;
	if(dragging<0)return; double x,y; glfwGetCursorPos(window,&x,&y);
	pieces[dragging].x=(float)x; pieces[dragging].y=(float)y;
	if(glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS){
		rotationTimer+=dt; while(rotationTimer>=ROTATION_INTERVAL){
			pieces[dragging].angle=Normalize(pieces[dragging].angle+ROTATION_STEP); rotationTimer-=ROTATION_INTERVAL; }
	}else rotationTimer=0;
}

void StartDrag(float x,float y)
{
	for(int i=(int)pieces.size()-1;i>=0;--i){ Piece& p=pieces[i];
		if(!p.active||p.locked||!Contains(p.type,p.x,p.y,p.angle,x,y))continue;
		if(p.slot>=0){slots[p.slot].piece=-1;p.slot=-1;} dragging=i; return; }
	for(int i=(int)sourcePieces.size()-1;i>=0;--i){const TemplatePiece& source=sourcePieces[i];
		if(!Contains(source.type,source.x,source.y,source.angle,x,y))continue;
		uniform_real_distribution<float> c(COLOR_MIN,COLOR_MAX);
		pieces.push_back({source.type,x,y,source.angle,
			{c(generator),c(generator),c(generator)},true,false,-1});
		dragging=(int)pieces.size()-1; return; }
}

void FinishDrag()
{
	if(dragging<0)return; Piece& p=pieces[dragging]; int match=-1; float best=POSITION_TOLERANCE+1;
	for(int i=0;i<(int)slots.size();++i){const Slot& s=slots[i];
		if(boards[s.board].completed||s.piece>=0||s.type!=p.type)continue;
		float dx=p.x-s.x,dy=p.y-s.y,distance=sqrt(dx*dx+dy*dy);
		if(distance<=POSITION_TOLERANCE&&
			PieceAngleDifference(p.type,p.angle,s.angle)<=ROTATION_TOLERANCE&&
			distance<best){match=i;best=distance;} }
	if(match<0)p.active=false;
	else{Slot& s=slots[match];p.x=s.x;p.y=s.y;p.angle=s.angle;p.slot=match;s.piece=dragging;CheckComplete(s.board);}
	dragging=-1;rotationTimer=0;
}

void CheckComplete(int board)
{
	for(const Slot& s:slots)if(s.board==board&&s.piece<0)return;
	boards[board].completed=true;
	for(const Slot& s:slots)if(s.board==board)pieces[s.piece].locked=true;
}

void MouseCallback(GLFWwindow* window,int button,int action,int)
{
	if(button!=GLFW_MOUSE_BUTTON_LEFT)return;double x,y;glfwGetCursorPos(window,&x,&y);
	if(action==GLFW_PRESS)StartDrag((float)x,(float)y);else if(action==GLFW_RELEASE)FinishDrag();
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int)
{
	if(action==GLFW_PRESS&&key==GLFW_KEY_R)InitScene();
	if(action==GLFW_PRESS&&(key==GLFW_KEY_Q||key==GLFW_KEY_ESCAPE))glfwSetWindowShouldClose(window,true);
}
