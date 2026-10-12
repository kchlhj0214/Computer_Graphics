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
#include <map>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define AXIS_LENGTH 1.0f
#define CONE_RADIUS 0.0225f
#define CONE_HEIGHT (CONE_RADIUS * 2.0f)
#define CONE_SLICES 24
#define CONE_STACKS 16
#define PERSPECTIVE_FOVY 27.360788f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / (float)WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 30.0f
#define ZOOM_STEP 0.05f
#define CAMERA_RADIUS_MIN 1.50f
#define CAMERA_RADIUS_MAX 20.0f
#define CAMERA_X 3.0f
#define CAMERA_Y 2.4f
#define CAMERA_Z 4.0f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define OBJECT_ROTATE_X 0.0f
#define OBJECT_ROTATE_Y 0.0f
#define OBJECT_SIZE 0.45f
#define OBJECT_DISTANCE 0.65f
#define OBJECT_SCALE_MIN 0.20f
#define OBJECT_SCALE_MAX 2.0f
#define ELEPHANT_HEIGHT_RATIO 1.5f
#define MESH_SLICES 32
#define MESH_STACKS 20
#define ANIMATION_FPS 60
#define ROTATE_PER_FRAME 2.0f
#define MOVE_PER_FRAME 0.01f
#define SCALE_PER_FRAME 0.01f
#define SWAP_SECONDS 3.0f
#define DETOUR_HEIGHT 0.35f
#define REVOLUTION_SECONDS 6.0f
#define SPIN_TILT 23.44f
#define SPINS_PER_ORBIT 4.0f
#define CYCLE_SCALE_MAX 1.5f
#define CYCLE_SCALE_MIN 0.5f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define COLOR_GAP 0.10f
#define SKY_RED 0.40f
#define SKY_GREEN 0.75f
#define SKY_BLUE 0.95f
#define TEXT_PIXEL_SIZE 2.0f
#define TEXT_MARGIN 18.0f

enum Shape { CUBE, SPHERE, CONE, PYRAMID, CYLINDER, HEAD, ELEPHANT, SHAPE_COUNT };
enum Animation { IDLE, THROUGH_ORIGIN, DETOUR, REVOLUTION };
struct Vertex { glm::vec3 position, color, normal; };
struct Mesh {
	GLuint vao = 0, vbo = 0;
	int count = 0;
	glm::vec3 center = glm::vec3(0);
	float unitScale = 1.0f;
};
struct Object {
	Shape shape = CUBE;
	glm::vec3 position = glm::vec3(0);
	glm::mat4 rotation = glm::mat4(1.0f);
	float scale = 1.0f;
};
const char* shapeNames[] = { "Cube", "Sphere", "Cone", "Pyramid", "Cylinder", "Head", "Elephant" };
Mesh meshes[SHAPE_COUNT];
Object objects[2], startObjects[2];
Animation animation = IDLE;
int selected = 3, activeKey = 0;
bool negative = false, wireframe = false, shading = false, dragging = false;
double accumulatedTime = 0.0;
float animationProgress = 0.0f;
glm::vec3 cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z);
float cameraRadius = glm::length(cameraPosition), cameraYaw = 0, cameraPitch = 0;
double previousMouseX = 0, previousMouseY = 0;
mt19937 randomEngine(random_device{}());
GLuint programID = 0, vao = 0, vbo = 0, textVao = 0, textVbo = 0;
int textVertexCount = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation, vertexColorLocation, shadingLocation;
GLUquadric* qobj = nullptr;

string ReadFile(const char* name);
bool InitShader();
bool InitBuffer();
void UploadMesh(Mesh& mesh, const vector<Vertex>& vertices);
vector<Vertex> MakeShape(Shape shape);
bool LoadObj(const char* path, Mesh& mesh, bool elephant);
void ResetScene();
void ChangeShapes(bool models);
void UpdateText();
void DrawScene();
void UpdateAnimation(double elapsed);
void ApplyCommand(int key, bool reverse);
void SetCyclePose(float progress);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// 축의 원뿔은 GLU로 만들고 모든 정점 변환은 셰이더에서 처리한다.
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-17 Modeling Transformations", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader() || !InitBuffer()) {
		glfwDestroyWindow(window); glfwTerminate(); return -1;
	}
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window, KeyCallback); glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback); glfwSetScrollCallback(window, ScrollCallback);
	ResetScene();
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double now = glfwGetTime();
		if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) activeKey = 0;
		UpdateAnimation(glm::min(now - previousTime, 0.1)); previousTime = now;
		glfwPollEvents();
		int width, height; glfwGetFramebufferSize(window, &width, &height); glViewport(0, 0, width, height);
		DrawScene(); glfwSwapBuffers(window);
	}
	for (Mesh& mesh : meshes) { glDeleteBuffers(1, &mesh.vbo); glDeleteVertexArrays(1, &mesh.vao); }
	gluDeleteQuadric(qobj); glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &textVbo); glDeleteVertexArrays(1, &textVao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

string ReadFile(const char* name)
{
	ifstream file(name);
	if (!file) { cerr << "Cannot open file: " << name << endl; return ""; }
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
		if (!ok) { char log[2048]; glGetShaderInfoLog(shaders[i], 2048, nullptr, log); cerr << log << endl; return false; }
	}
	programID = glCreateProgram();
	for (GLuint shader : shaders) glAttachShader(programID, shader);
	glLinkProgram(programID); GLint ok; glGetProgramiv(programID, GL_LINK_STATUS, &ok);
	if (!ok) { char log[2048]; glGetProgramInfoLog(programID, 2048, nullptr, log); cerr << log << endl; return false; }
	for (GLuint shader : shaders) glDeleteShader(shader);
	modelLocation = glGetUniformLocation(programID, "modelTransform");
	viewLocation = glGetUniformLocation(programID, "viewTransform");
	projectionLocation = glGetUniformLocation(programID, "projectionTransform");
	colorLocation = glGetUniformLocation(programID, "uColor");
	vertexColorLocation = glGetUniformLocation(programID, "useVertexColor");
	shadingLocation = glGetUniformLocation(programID, "useShading");
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0 && vertexColorLocation >= 0 && shadingLocation >= 0;
}

float RandomFloat(float low, float high) { return uniform_real_distribution<float>(low, high)(randomEngine); }

void UploadMesh(Mesh& mesh, const vector<Vertex>& vertices)
{
	if (!mesh.vao) { glGenVertexArrays(1, &mesh.vao); glGenBuffers(1, &mesh.vbo); }
	mesh.count = (int)vertices.size();
	glBindVertexArray(mesh.vao); glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	for (int i = 0; i < 3; ++i) glEnableVertexAttribArray(i);
	glBindVertexArray(0);
}

vector<Vertex> MakeShape(Shape shape)
{
	vector<Vertex> vertices;
	glm::vec3 palette[8];
	// 기본 도형은 성분 간격을 확보한 색상표를 사용하며, 곡면에서는 색상표를 반복한다.
	for (int channel = 0; channel < 3; ++channel) {
		float values[8], gap = COLOR_GAP + 0.00001f;
		float offset = RandomFloat(0, COLOR_MAX - COLOR_MIN - 7 * gap);
		for (int i = 0; i < 8; ++i) values[i] = COLOR_MIN + offset + i * gap;
		shuffle(values, values + 8, randomEngine);
		for (int i = 0; i < 8; ++i) palette[i][channel] = values[i];
	}
	auto color = [&](glm::vec3 p) { return palette[(p.x > 0 ? 1 : 0) + (p.y > 0 ? 2 : 0) + (p.z > 0 ? 4 : 0)]; };
	auto triangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 na, glm::vec3 nb, glm::vec3 nc) {
		if (glm::length(glm::cross(b-a,c-a)) < 0.000001f) return;
		vertices.push_back({a,color(a),na}); vertices.push_back({b,color(b),nb}); vertices.push_back({c,color(c),nc});
	};
	if (shape == CUBE || shape == PYRAMID) {
		glm::vec3 p[] = {{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f},
			{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f}};
		int cube[] = {0,1,2,0,2,3,1,5,6,1,6,2,5,4,7,5,7,6,4,0,3,4,3,7,3,2,6,3,6,7,4,5,1,4,1,0};
		int pyramid[] = {0,1,2,1,3,2,3,4,2,4,0,2,0,4,3,0,3,1};
		if (shape == PYRAMID) { p[2] = glm::vec3(0,.5f,0); p[3] = glm::vec3(.5f,-.5f,-.5f); }
		int* indices = shape == CUBE ? cube : pyramid;
		for (int i = 0; i < (shape == CUBE ? 36 : 18); i += 3) {
			glm::vec3 a=p[indices[i]], b=p[indices[i+1]], c=p[indices[i+2]];
			glm::vec3 n=glm::normalize(glm::cross(b-a,c-a)); triangle(a,b,c,n,n,n);
		}
	}
	else if (shape == SPHERE) {
		auto point = [](float u, float v) { return glm::vec3(glm::sin(v)*glm::cos(u),glm::cos(v),glm::sin(v)*glm::sin(u)) * .5f; };
		for (int j=0;j<MESH_STACKS;++j) for (int i=0;i<MESH_SLICES;++i) {
			float u=glm::two_pi<float>()*i/MESH_SLICES, un=glm::two_pi<float>()*(i+1)/MESH_SLICES;
			float v=glm::pi<float>()*j/MESH_STACKS, vn=glm::pi<float>()*(j+1)/MESH_STACKS;
			glm::vec3 a=point(u,v), b=point(un,v), c=point(un,vn), d=point(u,vn);
			triangle(a,b,c,a*2.0f,b*2.0f,c*2.0f); triangle(a,c,d,a*2.0f,c*2.0f,d*2.0f);
		}
	}
	else {
		for (int i=0;i<MESH_SLICES;++i) {
			float a=glm::two_pi<float>()*i/MESH_SLICES, b=glm::two_pi<float>()*(i+1)/MESH_SLICES;
			glm::vec3 p(.5f*glm::cos(a),-.5f,.5f*glm::sin(a)), q(.5f*glm::cos(b),-.5f,.5f*glm::sin(b));
			glm::vec3 na=glm::normalize(glm::vec3(glm::cos(a),shape==CONE ? .5f : 0,glm::sin(a)));
			glm::vec3 nb=glm::normalize(glm::vec3(glm::cos(b),shape==CONE ? .5f : 0,glm::sin(b)));
			glm::vec3 down(0,-1,0), up(0,1,0);
			triangle(glm::vec3(0,-.5f,0),p,q,down,down,down);
			if (shape == CONE) triangle(p,glm::vec3(0,.5f,0),q,na,glm::normalize(na+nb),nb);
			else {
				glm::vec3 pt=p+up, qt=q+up;
				triangle(p,pt,qt,na,na,nb); triangle(p,qt,q,na,nb,nb);
				triangle(glm::vec3(0,.5f,0),qt,pt,up,up,up);
			}
		}
	}
	return vertices;
}

bool LoadObj(const char* path, Mesh& mesh, bool elephant)
{
	ifstream file(path);
	if (!file) { cerr << "Cannot open model: " << path << endl; return false; }
	vector<glm::vec3> positions, normals, colors;
	vector<Vertex> vertices;
	string line;
	// 정점/텍스처/법선 인덱스는 서로 독립적이므로 각각 읽는다.
	try {
		while (getline(file,line)) {
			istringstream stream(line); string tag; stream >> tag;
			if (tag == "v" || tag == "vn") {
				glm::vec3 value; if (!(stream >> value.x >> value.y >> value.z)) return false;
				if (tag == "v") { positions.push_back(value); colors.push_back(glm::vec3(RandomFloat(COLOR_MIN,COLOR_MAX),RandomFloat(COLOR_MIN,COLOR_MAX),RandomFloat(COLOR_MIN,COLOR_MAX))); }
				else normals.push_back(value);
			}
			else if (tag == "f") {
				vector<int> pi, ni; string token;
				while (stream >> token) {
					if (token[0] == '#') break;
					size_t slash=token.find('/'), last=token.find_last_of('/');
					int p=stoi(token.substr(0,slash)); p=p>0 ? p-1 : (int)positions.size()+p;
					int n=-1;
					if (slash!=string::npos && last!=slash && last+1<token.size()) {
						n=stoi(token.substr(last+1)); n=n>0 ? n-1 : (int)normals.size()+n;
						if (n<0 || n>=(int)normals.size()) return false;
					}
					if (p<0 || p>=(int)positions.size()) return false;
					pi.push_back(p); ni.push_back(n);
				}
				for (int i=1;i+1<(int)pi.size();++i) {
					glm::vec3 n=glm::cross(positions[pi[i]]-positions[pi[0]],positions[pi[i+1]]-positions[pi[0]]);
					if (glm::length(n)<0.000001f) continue;
					n=glm::normalize(n);
					for (int j : {0,i,i+1}) vertices.push_back({positions[pi[j]],colors[pi[j]],ni[j]>=0 && glm::length(normals[ni[j]])>0 ? glm::normalize(normals[ni[j]]) : n});
				}
			}
		}
	} catch (const exception& error) { cerr << "Invalid OBJ: " << path << ": " << error.what() << endl; return false; }
	if (vertices.empty()) return false;
	glm::vec3 low=vertices[0].position, high=low;
	for (const Vertex& vertex : vertices) { low=glm::min(low,vertex.position); high=glm::max(high,vertex.position); }
	mesh.center=(low+high)*.5f; glm::vec3 extent=high-low;
	float length=elephant ? extent.y : glm::max(extent.x,glm::max(extent.y,extent.z));
	if (length<=0.000001f) return false;
	mesh.unitScale=(elephant ? ELEPHANT_HEIGHT_RATIO : 1.0f)/length;
	UploadMesh(mesh,vertices); return true;
}

bool InitBuffer()
{
	float axes[] = {-AXIS_LENGTH,0,0,AXIS_LENGTH,0,0,0,-AXIS_LENGTH,0,0,AXIS_LENGTH,0,0,0,-AXIS_LENGTH,0,0,AXIS_LENGTH};
	glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER,vbo);
	glBufferData(GL_ARRAY_BUFFER,sizeof(axes),axes,GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr); glEnableVertexAttribArray(0);
	glGenVertexArrays(1,&textVao); glGenBuffers(1,&textVbo); glBindVertexArray(textVao); glBindBuffer(GL_ARRAY_BUFFER,textVbo);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr); glEnableVertexAttribArray(0); glBindVertexArray(0);
	qobj=gluNewQuadric(); if (!qobj) return false;
	gluQuadricDrawStyle(qobj,GLU_FILL); gluQuadricNormals(qobj,GLU_NONE); gluQuadricOrientation(qobj,GLU_OUTSIDE);
	return LoadObj("head.obj",meshes[HEAD],false) && LoadObj("elephant.obj",meshes[ELEPHANT],true);
}

void ChangeShapes(bool models)
{
	if (models) { objects[0].shape=HEAD; objects[1].shape=ELEPHANT; }
	else {
		int first,second;
		do {
			first=uniform_int_distribution<int>(0,4)(randomEngine);
			second=uniform_int_distribution<int>(0,3)(randomEngine); if (second>=first) ++second;
		} while (first==objects[0].shape || second==objects[1].shape);
		objects[0].shape=(Shape)first; objects[1].shape=(Shape)second;
	}
	UpdateText();
}

void ResetScene()
{
	animation=IDLE; animationProgress=0; accumulatedTime=0; activeKey=0; negative=false;
	selected=3; wireframe=false; shading=false; dragging=false;
	cameraPosition=glm::vec3(CAMERA_X,CAMERA_Y,CAMERA_Z); cameraRadius=glm::length(cameraPosition); cameraYaw=cameraPitch=0;
	for (int i=0;i<5;++i) UploadMesh(meshes[i],MakeShape((Shape)i));
	// 초기화할 때 OBJ의 색상도 다시 뽑되, 같은 위치의 정점은 같은 색상을 사용한다.
	for (int type=HEAD;type<=ELEPHANT;++type) {
		Mesh& mesh=meshes[type];
		vector<Vertex> vertices(mesh.count);
		glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);
		glGetBufferSubData(GL_ARRAY_BUFFER,0,vertices.size()*sizeof(Vertex),vertices.data());
		map<vector<float>,glm::vec3> colors;
		for (Vertex& vertex : vertices) {
			vector<float> key={vertex.position.x,vertex.position.y,vertex.position.z};
			auto found=colors.find(key);
			if (found==colors.end()) found=colors.emplace(key,glm::vec3(RandomFloat(COLOR_MIN,COLOR_MAX),RandomFloat(COLOR_MIN,COLOR_MAX),RandomFloat(COLOR_MIN,COLOR_MAX))).first;
			vertex.color=found->second;
		}
		UploadMesh(mesh,vertices);
	}
	// 기본 좌표축은 고정하고 초기 기울기는 객체의 자세에만 적용한다.
	glm::mat4 tilt=glm::rotate(glm::mat4(1),glm::radians(OBJECT_ROTATE_Y),glm::vec3(0,1,0));
	tilt=glm::rotate(tilt,glm::radians(OBJECT_ROTATE_X),glm::vec3(1,0,0));
	for (int i=0;i<2;++i) {
		objects[i]=Object(); objects[i].shape=(Shape)-1;
		objects[i].position=glm::vec3(i==0 ? -OBJECT_DISTANCE : OBJECT_DISTANCE,0,0);
		objects[i].rotation=tilt;
	}
	ChangeShapes(false);
}

void ApplyCommand(int key, bool reverse)
{
	float direction=reverse ? -1.0f : 1.0f;
	for (int i=0;i<2;++i) {
		if (selected!=3 && selected!=i+1) continue;
		Object& object=objects[i];
		if (key==GLFW_KEY_X || key==GLFW_KEY_Y) {
			glm::vec3 axis=key==GLFW_KEY_X ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
			object.rotation=glm::rotate(object.rotation,glm::radians(ROTATE_PER_FRAME*direction),axis);
		}
		else if (key==GLFW_KEY_R) {
			// 공전은 중심 위치만 변경하므로 객체의 자전 자세에는 영향을 주지 않는다.
			object.position=glm::vec3(glm::rotate(glm::mat4(1),glm::radians(ROTATE_PER_FRAME*direction),glm::vec3(0,1,0))*glm::vec4(object.position,1));
		}
		else if (key==GLFW_KEY_A || key==GLFW_KEY_B) {
			float factor=glm::pow(1.0f+SCALE_PER_FRAME,direction);
			factor=glm::clamp(object.scale*factor,OBJECT_SCALE_MIN,OBJECT_SCALE_MAX)/object.scale;
			if (key==GLFW_KEY_B) {
				float radius=glm::length(glm::vec2(object.position.x,object.position.z));
				if (radius>0) factor=glm::min(factor,AXIS_LENGTH/radius);
				if (glm::abs(object.position.y)>0) factor=glm::min(factor,AXIS_LENGTH/glm::abs(object.position.y));
				object.position*=factor;
			}
			object.scale*=factor;
		}
		else if (key==GLFW_KEY_D) {
			float limit=glm::sqrt(glm::max(0.0f,AXIS_LENGTH*AXIS_LENGTH-object.position.z*object.position.z));
			object.position.x=glm::clamp(object.position.x-MOVE_PER_FRAME*direction,-limit,limit);
		}
		else if (key==GLFW_KEY_E) object.position.y=glm::clamp(object.position.y+MOVE_PER_FRAME*direction,-AXIS_LENGTH,AXIS_LENGTH);
	}
}

void SetCyclePose(float progress)
{
	for (int i=0;i<2;++i) {
		objects[i]=startObjects[i];
		glm::vec3 from=startObjects[i].position,to=startObjects[1-i].position;
		if (animation==THROUGH_ORIGIN) objects[i].position=progress<=.5f ? glm::mix(from,glm::vec3(0),progress*2) : glm::mix(glm::vec3(0),to,progress*2-1);
		else if (animation==DETOUR) {
			float level=i==0 ? glm::min(AXIS_LENGTH,glm::max(from.y,to.y)+DETOUR_HEIGHT) : glm::max(-AXIS_LENGTH,glm::min(from.y,to.y)-DETOUR_HEIGHT);
			glm::vec3 first(from.x,level,from.z),second(to.x,level,to.z);
			if (progress<1.0f/3) objects[i].position=glm::mix(from,first,progress*3);
			else if (progress<2.0f/3) objects[i].position=glm::mix(first,second,progress*3-1);
			else objects[i].position=glm::mix(second,to,progress*3-2);
		}
		else if (animation==REVOLUTION) {
			float angle=glm::two_pi<float>()*progress;
			objects[i].position=glm::vec3(glm::rotate(glm::mat4(1),angle,glm::vec3(0,1,0))*glm::vec4(from,1));
			glm::vec3 spinAxis(glm::sin(glm::radians(SPIN_TILT)),glm::cos(glm::radians(SPIN_TILT)),0);
			objects[i].rotation=glm::rotate(glm::mat4(1),angle*SPINS_PER_ORBIT,spinAxis)*startObjects[i].rotation;
			float wave=glm::sin(glm::pi<float>()*progress); wave*=wave;
			float factor=glm::mix(1.0f,i==0 ? CYCLE_SCALE_MAX : CYCLE_SCALE_MIN,wave);
			objects[i].scale=glm::clamp(startObjects[i].scale*factor,OBJECT_SCALE_MIN,OBJECT_SCALE_MAX);
		}
		if (progress>=1) { objects[i]=startObjects[i]; if (animation!=REVOLUTION) objects[i].position=to; }
	}
}

void UpdateAnimation(double elapsed)
{
	accumulatedTime+=elapsed;
	while (accumulatedTime>=1.0/ANIMATION_FPS) {
		accumulatedTime-=1.0/ANIMATION_FPS;
		if (animation!=IDLE) {
			float duration=animation==REVOLUTION ? REVOLUTION_SECONDS : SWAP_SECONDS;
			int segments=animation==DETOUR ? 3 : 2;
			float endpoint=((int)(animationProgress*segments)+1)/(float)segments;
			animationProgress=glm::min(endpoint,animationProgress+1.0f/(ANIMATION_FPS*duration));
			SetCyclePose(animationProgress);
			if (animationProgress>=1) { animation=IDLE; animationProgress=0; activeKey=0; }
		}
		else if (activeKey) ApplyCommand(activeKey,negative);
	}
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action==GLFW_RELEASE && key==activeKey) activeKey=0;
	if (key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT) negative=(mods&GLFW_MOD_SHIFT)!=0;
	if (action!=GLFW_PRESS) return;
	if (key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(window,true); return; }
	if (key==GLFW_KEY_S) { ResetScene(); return; }
	if (key==GLFW_KEY_W) { wireframe=!wireframe; return; }
	if (key==GLFW_KEY_Z) { shading=!shading; return; }
	if (animation!=IDLE) return;
	if (key>=GLFW_KEY_1 && key<=GLFW_KEY_3) { selected=key-GLFW_KEY_0; activeKey=0; UpdateText(); return; }
	if (key==GLFW_KEY_C) { activeKey=0; ChangeShapes((mods&GLFW_MOD_SHIFT)!=0); return; }
	if (key==GLFW_KEY_T || key==GLFW_KEY_U || key==GLFW_KEY_V) {
		if (activeKey) return;
		animation=key==GLFW_KEY_T ? THROUGH_ORIGIN : (key==GLFW_KEY_U ? DETOUR : REVOLUTION);
		for (int i=0;i<2;++i) startObjects[i]=objects[i];
		animationProgress=0; accumulatedTime=0; return;
	}
	if (!activeKey && (key==GLFW_KEY_X || key==GLFW_KEY_Y || key==GLFW_KEY_R || key==GLFW_KEY_A || key==GLFW_KEY_B || key==GLFW_KEY_D || key==GLFW_KEY_E)) {
		activeKey=key; negative=(mods&GLFW_MOD_SHIFT)!=0;
	}
}

void DrawScene()
{
	glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); glUseProgram(programID);
	glEnable(GL_DEPTH_TEST); glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glUniform1i(vertexColorLocation,GL_FALSE); glUniform1i(shadingLocation,GL_FALSE);
	glm::mat4 view=glm::lookAt(cameraPosition,glm::vec3(0),glm::vec3(0,1,0));
	glm::mat4 projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),PERSPECTIVE_ASPECT,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view)); glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
	glm::mat4 axes(1.0f); glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(axes));
	glm::vec3 colors[]={{.85f,.12f,.12f},{.12f,.65f,.20f},{.15f,.30f,.90f}};
	glm::vec3 ends[]={{AXIS_LENGTH,0,0},{0,AXIS_LENGTH,0},{0,0,AXIS_LENGTH}};
	glBindVertexArray(vao); glLineWidth(1);
	for (int i=0;i<3;++i) { glUniform3fv(colorLocation,1,glm::value_ptr(colors[i])); glDrawArrays(GL_LINES,i*2,2); }
	glBindVertexArray(0);
	for (int i=0;i<3;++i) {
		glm::mat4 model=glm::translate(axes,ends[i]);
		if (i==0) model=glm::rotate(model,glm::radians(90.0f),glm::vec3(0,1,0));
		else if (i==1) model=glm::rotate(model,glm::radians(-90.0f),glm::vec3(1,0,0));
		model=glm::translate(model,glm::vec3(0,0,-CONE_HEIGHT*.5f));
		glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model)); glUniform3fv(colorLocation,1,glm::value_ptr(colors[i]));
		gluCylinder(qobj,CONE_RADIUS,0,CONE_HEIGHT,CONE_SLICES,CONE_STACKS);
		model=glm::rotate(model,glm::radians(180.0f),glm::vec3(1,0,0));
		glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model)); gluDisk(qobj,0,CONE_RADIUS,CONE_SLICES,1);
	}
	glUniform1i(vertexColorLocation,GL_TRUE); glUniform1i(shadingLocation,shading);
	glUniform3f(colorLocation,SKY_RED,SKY_GREEN,SKY_BLUE);
	glPolygonMode(GL_FRONT_AND_BACK,wireframe ? GL_LINE : GL_FILL);
	for (const Object& object : objects) {
		const Mesh& mesh=meshes[object.shape];
		// 객체의 기울기와 자전은 중심 위치에 영향을 주지 않으며 이동은 고정 좌표축을 따른다.
		glm::mat4 model=glm::translate(glm::mat4(1.0f),object.position)*object.rotation;
		model=glm::scale(model,glm::vec3(OBJECT_SIZE*object.scale*mesh.unitScale)); model=glm::translate(model,-mesh.center);
		glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
		glBindVertexArray(mesh.vao); glDrawArrays(GL_TRIANGLES,0,mesh.count);
	}
	// 화면 안내는 깊이와 원근투영에 영향을 받지 않도록 마지막에 그린다.
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL); glDisable(GL_DEPTH_TEST);
	glUniform1i(shadingLocation,GL_FALSE); glUniform1i(vertexColorLocation,GL_FALSE); glUniform3f(colorLocation,.08f,.08f,.08f);
	glm::mat4 identity(1), textProjection=glm::ortho(0.0f,(float)WINDOW_WIDTH,(float)WINDOW_HEIGHT,0.0f,-1.0f,1.0f);
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(identity)); glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(identity));
	glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(textProjection));
	glBindVertexArray(textVao); glDrawArrays(GL_TRIANGLES,0,textVertexCount); glBindVertexArray(0); glEnable(GL_DEPTH_TEST);
}

void UpdateText()
{
	// 필요한 영문 글자를 작은 사각형으로 구성하여 별도 글꼴 라이브러리 없이 표시한다.
	struct Glyph { char letter; unsigned char rows[7]; };
	static const Glyph glyphs[]={
		{'L',{16,16,16,16,16,16,31}},{'R',{30,17,17,30,20,18,17}},
		{'S',{15,16,16,14,1,1,30}},{'C',{14,17,16,16,16,17,14}},
		{'P',{30,17,17,30,16,16,16}},{'H',{17,17,17,31,17,17,17}},
		{'E',{31,16,16,30,16,16,31}},{'B',{30,17,17,30,17,17,30}},
		{'a',{0,0,14,1,15,17,15}},{'b',{16,16,30,17,17,17,30}},
		{'c',{0,0,14,16,16,17,14}},{'d',{1,1,15,17,17,17,15}},
		{'e',{0,0,14,17,31,16,14}},{'f',{6,8,8,30,8,8,8}},
		{'g',{0,0,15,17,15,1,14}},{'h',{16,16,30,17,17,17,17}},
		{'i',{4,0,12,4,4,4,14}},{'l',{12,4,4,4,4,4,14}},
		{'m',{0,0,26,21,21,17,17}},{'n',{0,0,30,17,17,17,17}},
		{'o',{0,0,14,17,17,17,14}},{'p',{0,0,30,17,30,16,16}},
		{'r',{0,0,22,25,16,16,16}},{'s',{0,0,15,16,14,1,30}},
		{'t',{8,8,30,8,8,9,6}},{'u',{0,0,17,17,17,19,13}},
		{'y',{0,0,17,17,15,1,14}},{':',{0,4,4,0,4,4,0}},
		{'/',{1,2,2,4,8,8,16}}
	};
	string text=string("Left : ")+shapeNames[objects[0].shape]+" / Right : "+shapeNames[objects[1].shape]+"\nSelected : "+(selected==1 ? "Left" : selected==2 ? "Right" : "Both");
	vector<glm::vec3> pixels;
	float x=TEXT_MARGIN,y=TEXT_MARGIN,size=TEXT_PIXEL_SIZE;
	for (char letter : text) {
		if (letter=='\n') { x=TEXT_MARGIN; y+=11*size; continue; }
		for (const Glyph& glyph : glyphs) if (glyph.letter==letter) {
			for (int row=0;row<7;++row) for (int col=0;col<5;++col) if (glyph.rows[row]&(1<<(4-col))) {
				float px=x+col*size,py=y+row*size;
				pixels.insert(pixels.end(),{{px,py,0},{px+size,py,0},{px+size,py+size,0},{px,py,0},{px+size,py+size,0},{px,py+size,0}});
			}
			break;
		}
		x+=6*size;
	}
	textVertexCount=(int)pixels.size(); glBindBuffer(GL_ARRAY_BUFFER,textVbo);
	glBufferData(GL_ARRAY_BUFFER,pixels.size()*sizeof(glm::vec3),pixels.data(),GL_DYNAMIC_DRAW);
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (button!=GLFW_MOUSE_BUTTON_LEFT) return;
	if (action==GLFW_PRESS) {
		dragging=true; glfwGetCursorPos(window,&previousMouseX,&previousMouseY);
		cameraYaw=glm::atan(cameraPosition.x,cameraPosition.z); cameraPitch=glm::asin(cameraPosition.y/cameraRadius);
	}
	else if (action==GLFW_RELEASE) { dragging=false; cameraPosition=glm::normalize(glm::vec3(CAMERA_X,CAMERA_Y,CAMERA_Z))*cameraRadius; }
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (!dragging) return;
	int width,height; glfwGetWindowSize(window,&width,&height); if (width<=0 || height<=0) return;
	cameraYaw-=glm::radians(ORBIT_DEGREES)*(float)(x-previousMouseX)/width;
	cameraPitch+=glm::radians(ORBIT_DEGREES)*(float)(y-previousMouseY)/height;
	previousMouseX=x; previousMouseY=y;
	cameraYaw=glm::mod(cameraYaw,glm::two_pi<float>());
	cameraPitch=glm::clamp(cameraPitch,-glm::radians(ORBIT_PITCH_LIMIT),glm::radians(ORBIT_PITCH_LIMIT));
	cameraPosition=cameraRadius*glm::vec3(glm::cos(cameraPitch)*glm::sin(cameraYaw),glm::sin(cameraPitch),glm::cos(cameraPitch)*glm::cos(cameraYaw));
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
	cameraRadius=glm::clamp(cameraRadius-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
	cameraPosition=glm::normalize(cameraPosition)*cameraRadius;
}
