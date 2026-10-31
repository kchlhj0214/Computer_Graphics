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
#include <cstddef>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CAMERA_Z 6.0f
#define PERSPECTIVE_FOVY 38.0f
#define PERSPECTIVE_NEAR 0.1f
#define PERSPECTIVE_FAR 150.0f
#define BOX_HALF_SIZE 1.25f
#define BOX_MOVE_SPEED 1.0f
#define BOX_Z_MIN -3.0f
#define BOX_Z_MAX 1.0f
#define BOX_Y_ROTATE_SPEED 60.0f
#define DRAG_DEGREES 360.0f
#define MAX_ROLL_SPEED 720.0f
#define PHYSICS_FPS 480
#define BALL_MAX_COUNT 5
#define BALL_RADIUS 0.09f
#define BALL_BASE_SPEED 2.2f
#define BALL_MAX_SPEED 12.0f
#define BALL_GRAVITY 1.5f
#define BALL_SPEED_RETURN 0.75f
#define BALL_RESTITUTION 1.05f
#define BALL_Z_MIN -0.85f
#define BALL_Z_MAX 0.80f
#define BALL_DEPTH_SPEED 2.8f
#define SPHERE_SLICES 32
#define SPHERE_STACKS 20
#define CUBE_COUNT 3
#define CUBE_FAR_SIZE 0.38f
#define CUBE_NEAR_SIZE 0.18f
#define CUBE_FAR_Z -0.72f
#define CUBE_NEAR_Z 0.72f
#define CUBE_GRAVITY 3.0f
#define CUBE_RESTITUTION 0.18f
#define CUBE_AIR_DRAG 0.10f
#define DOOR_OPEN_SECONDS 0.16f
#define DOOR_WAIT_SECONDS 2.8f
#define DOOR_CLOSE_SECONDS 1.3f
#define DOOR_OPEN_ANGLE 90.0f
#define ESCAPE_SPEED 4.0f
#define ESCAPE_GRAVITY 2.0f
#define LIGHT_X 0.2f
#define LIGHT_Y 0.4f
#define LIGHT_Z 3.0f
#define DIFFUSE_STRENGTH 0.85f
#define SPECULAR_STRENGTH 0.20f
#define SHININESS 40.0f

struct Vertex { glm::vec3 position, normal; };
struct Mesh { GLuint vao=0,vbo=0; int count=0; };
struct Ball { glm::vec3 position,velocity; bool escaping=false; };
struct Cube { glm::vec3 position,velocity; float size,depth; };
enum DoorState { CLOSED, OPENING, WAITING, CLOSING };
Mesh wallMesh,cubeMesh,sphereMesh;
vector<Ball> balls;
Cube cubes[CUBE_COUNT];
DoorState doorState=CLOSED;
float doorTime=0,doorAngle=0,boxZ=0,yaw=0,roll=0,targetRoll=0;
bool dragging=false;
double previousMouseX=0,accumulator=0;
int movementKey=0;
bool reverseKey=false;
glm::mat4 boxTransform(1),previousBoxTransform(1);
glm::vec3 eye(0,0,CAMERA_Z);
GLuint programID=0;
GLint modelLocation,viewLocation,projectionLocation,colorLocation;
mt19937 randomEngine(random_device{}());

string ReadFile(const char* path);
bool InitShader();
void InitBuffer();
void ResetScene();
void SpawnBall();
void PhysicsStep(float dt);
void UpdateAnimation(double elapsed);
void DrawScene();
void KeyCallback(GLFWwindow* window,int key,int scancode,int action,int mods);
void MouseCallback(GLFWwindow* window,int button,int action,int mods);
void CursorCallback(GLFWwindow* window,double x,double y);

int main()
{
	if(!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);
	GLFWwindow* window=glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"1-21 Bouncing Room",nullptr,nullptr);
	if(!window){glfwTerminate();return -1;}
	glfwMakeContextCurrent(window);glewExperimental=GL_TRUE;
	if(glewInit()!=GLEW_OK || !InitShader()){glfwDestroyWindow(window);glfwTerminate();return -1;}
	InitBuffer();ResetScene();glfwSwapInterval(1);
	glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);
	glfwSetKeyCallback(window,KeyCallback);glfwSetMouseButtonCallback(window,MouseCallback);glfwSetCursorPosCallback(window,CursorCallback);
	double last=glfwGetTime();
	while(!glfwWindowShouldClose(window)){
		double now=glfwGetTime();
		if(!glfwGetWindowAttrib(window,GLFW_FOCUSED)){movementKey=0;dragging=false;targetRoll=roll;}
		UpdateAnimation(glm::min(now-last,0.1));last=now;glfwPollEvents();
		int width,height;glfwGetFramebufferSize(window,&width,&height);glViewport(0,0,width,height);
		DrawScene();glfwSwapBuffers(window);
	}
	for(Mesh* mesh:{&wallMesh,&cubeMesh,&sphereMesh}){glDeleteBuffers(1,&mesh->vbo);glDeleteVertexArrays(1,&mesh->vao);}
	glDeleteProgram(programID);glfwDestroyWindow(window);glfwTerminate();return 0;
}

string ReadFile(const char* path)
{
	ifstream file(path);if(!file){cerr<<"Cannot open shader: "<<path<<endl;return "";}
	stringstream stream;stream<<file.rdbuf();return stream.str();
}

bool InitShader()
{
	string sources[]={ReadFile("vertex.glsl"),ReadFile("fragment.glsl")};
	GLuint shaders[]={glCreateShader(GL_VERTEX_SHADER),glCreateShader(GL_FRAGMENT_SHADER)};
	for(int i=0;i<2;++i){
		const char* code=sources[i].c_str();glShaderSource(shaders[i],1,&code,nullptr);glCompileShader(shaders[i]);
		GLint ok;glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);
		if(!ok){char log[2048];glGetShaderInfoLog(shaders[i],2048,nullptr,log);cerr<<log<<endl;return false;}
	}
	programID=glCreateProgram();for(GLuint shader:shaders)glAttachShader(programID,shader);glLinkProgram(programID);
	GLint ok;glGetProgramiv(programID,GL_LINK_STATUS,&ok);
	if(!ok){char log[2048];glGetProgramInfoLog(programID,2048,nullptr,log);cerr<<log<<endl;return false;}
	for(GLuint shader:shaders)glDeleteShader(shader);
	modelLocation=glGetUniformLocation(programID,"modelTransform");viewLocation=glGetUniformLocation(programID,"viewTransform");
	projectionLocation=glGetUniformLocation(programID,"projectionTransform");colorLocation=glGetUniformLocation(programID,"uColor");
	glUseProgram(programID);
	glUniform3fv(glGetUniformLocation(programID,"eyePosition"),1,glm::value_ptr(eye));
	glUniform3f(glGetUniformLocation(programID,"lightPosition"),LIGHT_X,LIGHT_Y,LIGHT_Z);
	glUniform1f(glGetUniformLocation(programID,"diffuseStrength"),DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"specularStrength"),SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"shininess"),SHININESS);
	return modelLocation>=0 && viewLocation>=0 && projectionLocation>=0 && colorLocation>=0;
}

void UploadMesh(Mesh& mesh,const vector<Vertex>& vertices)
{
	mesh.count=(int)vertices.size();glGenVertexArrays(1,&mesh.vao);glGenBuffers(1,&mesh.vbo);
	glBindVertexArray(mesh.vao);glBindBuffer(GL_ARRAY_BUFFER,mesh.vbo);
	glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(Vertex),vertices.data(),GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal));
	glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glBindVertexArray(0);
}

void InitBuffer()
{
	vector<Vertex> vertices;
	auto quad=[&](glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 d,glm::vec3 normal){
		for(glm::vec3 p:{a,b,c,a,c,d})vertices.push_back({p,normal});
	};
	float h=BOX_HALF_SIZE;
	// 안쪽 면이 앞면이 되도록 벽의 정점을 반시계 방향으로 배치한다.
	quad({-h,-h,h},{-h,-h,-h},{-h,h,-h},{-h,h,h},{1,0,0});
	quad({h,-h,-h},{h,-h,h},{h,h,h},{h,h,-h},{-1,0,0});
	quad({-h,-h,-h},{-h,-h,h},{h,-h,h},{h,-h,-h},{0,1,0});
	quad({-h,h,h},{-h,h,-h},{h,h,-h},{h,h,h},{0,-1,0});
	quad({-h,-h,-h},{h,-h,-h},{h,h,-h},{-h,h,-h},{0,0,1});
	UploadMesh(wallMesh,vertices);vertices.clear();h=.5f;
	quad({-h,-h,h},{h,-h,h},{h,h,h},{-h,h,h},{0,0,1});
	quad({h,-h,-h},{-h,-h,-h},{-h,h,-h},{h,h,-h},{0,0,-1});
	quad({-h,-h,-h},{-h,-h,h},{-h,h,h},{-h,h,-h},{-1,0,0});
	quad({h,-h,h},{h,-h,-h},{h,h,-h},{h,h,h},{1,0,0});
	quad({-h,h,h},{h,h,h},{h,h,-h},{-h,h,-h},{0,1,0});
	quad({-h,-h,-h},{h,-h,-h},{h,-h,h},{-h,-h,h},{0,-1,0});
	UploadMesh(cubeMesh,vertices);vertices.clear();
	auto point=[](float u,float v){return glm::vec3(glm::sin(v)*glm::cos(u),glm::cos(v),glm::sin(v)*glm::sin(u));};
	for(int j=0;j<SPHERE_STACKS;++j)for(int i=0;i<SPHERE_SLICES;++i){
		float u=glm::two_pi<float>()*i/SPHERE_SLICES,un=glm::two_pi<float>()*(i+1)/SPHERE_SLICES;
		float v=glm::pi<float>()*j/SPHERE_STACKS,vn=glm::pi<float>()*(j+1)/SPHERE_STACKS;
		glm::vec3 a=point(u,v),b=point(un,v),c=point(un,vn),d=point(u,vn);
		if(j>0)for(glm::vec3 p:{a,b,c})vertices.push_back({p,p});
		if(j+1<SPHERE_STACKS)for(glm::vec3 p:{a,c,d})vertices.push_back({p,p});
	}
	UploadMesh(sphereMesh,vertices);
}

glm::mat4 RoomMatrix()
{
	glm::mat4 model=glm::translate(glm::mat4(1),glm::vec3(0,0,boxZ));
	model=glm::rotate(model,glm::radians(yaw),glm::vec3(0,1,0));
	return glm::rotate(model,glm::radians(roll),glm::vec3(0,0,1));
}

void ResetScene()
{
	balls.clear();doorState=CLOSED;doorTime=doorAngle=0;
	boxZ=yaw=roll=targetRoll=0;dragging=false;movementKey=0;reverseKey=false;accumulator=0;
	boxTransform=previousBoxTransform=RoomMatrix();
	for(int i=0;i<CUBE_COUNT;++i){
		float t=CUBE_COUNT==1 ? 0.0f : (float)i/(CUBE_COUNT-1);
		cubes[i].size=glm::mix(CUBE_FAR_SIZE,CUBE_NEAR_SIZE,t);
		cubes[i].depth=glm::mix(CUBE_FAR_Z,CUBE_NEAR_Z,t);
		cubes[i].position=glm::vec3(0,-BOX_HALF_SIZE+cubes[i].size*.5f,cubes[i].depth);
		cubes[i].velocity=glm::vec3(0);
	}
}

void SpawnBall()
{
	if(doorState!=CLOSED || balls.size()>=BALL_MAX_COUNT)return;
	auto random=[](float a,float b){return uniform_real_distribution<float>(a,b)(randomEngine);};
	glm::vec3 local(random(-.7f,.7f),random(-.5f,.7f),random(BALL_Z_MIN,BALL_Z_MAX));
	glm::vec3 direction=glm::normalize(glm::vec3(random(-1,1),random(-1,1),-1));
	balls.push_back({glm::vec3(boxTransform*glm::vec4(local,1)),direction*BALL_BASE_SPEED,false});
}

glm::vec3 WallVelocity(glm::vec3 local,float dt)
{
	return glm::vec3((boxTransform*glm::vec4(local,1)-previousBoxTransform*glm::vec4(local,1))/dt);
}

void UpdateDoor(float dt)
{
	if(doorState==CLOSED)return;
	doorTime+=dt;
	if(doorState==OPENING){
		doorAngle=DOOR_OPEN_ANGLE*glm::min(doorTime/DOOR_OPEN_SECONDS,1.0f);
		if(doorTime>=DOOR_OPEN_SECONDS){
			doorState=WAITING;doorTime=0;
			for(Ball& ball:balls){
				ball.escaping=true;
				// 문이 완전히 열린 뒤부터 카메라에서 멀어지는 방향으로 낙하시킨다.
				ball.velocity=glm::vec3(0,0,-ESCAPE_SPEED);
			}
		}
	}
	else if(doorState==WAITING){if(doorTime>=DOOR_WAIT_SECONDS){doorState=CLOSING;doorTime=0;}}
	else {
		doorAngle=DOOR_OPEN_ANGLE*(1-glm::min(doorTime/DOOR_CLOSE_SECONDS,1.0f));
		if(doorTime>=DOOR_CLOSE_SECONDS){doorState=CLOSED;doorAngle=doorTime=0;balls.clear();}
	}
}

void ReflectBall(Ball& ball,glm::vec3 normal,glm::vec3 wallVelocity)
{
	glm::vec3 relative=ball.velocity-wallVelocity;
	float incoming=glm::dot(relative,normal);
	if(incoming<0)ball.velocity-=(1+BALL_RESTITUTION)*incoming*normal;
	float speed=glm::length(ball.velocity);
	if(speed>BALL_MAX_SPEED)ball.velocity*=BALL_MAX_SPEED/speed;
}

void PhysicsStep(float dt)
{
	previousBoxTransform=boxTransform;
	float sign=reverseKey ? -1.0f : 1.0f;
	if(movementKey==GLFW_KEY_Z)boxZ=glm::clamp(boxZ+sign*BOX_MOVE_SPEED*dt,BOX_Z_MIN,BOX_Z_MAX);
	if(movementKey==GLFW_KEY_Y)yaw-=sign*BOX_Y_ROTATE_SPEED*dt;
	roll+=glm::clamp(targetRoll-roll,-MAX_ROLL_SPEED*dt,MAX_ROLL_SPEED*dt);
	boxTransform=RoomMatrix();
	glm::mat4 inverse=glm::inverse(boxTransform);
	glm::mat3 rotation(boxTransform);
	UpdateDoor(dt);
	for(Ball& ball:balls){
		if(ball.escaping){
			ball.velocity.z-=ESCAPE_GRAVITY*dt;ball.position+=ball.velocity*dt;continue;
		}
		ball.velocity+=glm::vec3(0,0,BALL_GRAVITY)*dt;
		float speed=glm::length(ball.velocity);
		if(speed>BALL_BASE_SPEED)ball.velocity*=glm::mix(speed,BALL_BASE_SPEED,1-glm::exp(-BALL_SPEED_RETURN*dt))/speed;
		ball.position+=ball.velocity*dt;
		glm::vec3 local(inverse*glm::vec4(ball.position,1));
		for(int axis=0;axis<2;++axis)for(float side:{-1.0f,1.0f}){
			float limit=BOX_HALF_SIZE-BALL_RADIUS;
			if(side*local[axis]>limit){
				local[axis]=side*limit;glm::vec3 normal(0);normal[axis]=-side;
				ReflectBall(ball,rotation*normal,WallVelocity(local,dt));
			}
		}
		// 앞쪽에 벽을 추가하지 않고 공의 깊이 운동만 지정 범위에서 되돌린다.
		if(local.z<BALL_Z_MIN || local.z>BALL_Z_MAX){
			bool atFront=local.z>BALL_Z_MAX;
			local.z=atFront ? BALL_Z_MAX : BALL_Z_MIN;
			glm::vec3 axis=rotation*glm::vec3(0,0,1),wallSpeed=WallVelocity(local,dt);
			float current=glm::dot(ball.velocity-wallSpeed,axis);
			float desired=(atFront ? -1.0f : 1.0f)*glm::max(glm::abs(current),BALL_DEPTH_SPEED);
			ball.velocity+=(desired-current)*axis;
		}
		ball.position=glm::vec3(boxTransform*glm::vec4(local,1));
	}
	for(Cube& cube:cubes){
		cube.velocity+=glm::vec3(0,-CUBE_GRAVITY,0)*dt;
		cube.velocity*=glm::exp(-CUBE_AIR_DRAG*dt);
		cube.position+=cube.velocity*dt;
		glm::vec3 local(inverse*glm::vec4(cube.position,1));local.z=cube.depth;
		// 고정된 깊이 평면의 움직임은 따르되 x/y 방향 관성은 보존한다.
		glm::vec3 depthNormal=rotation*glm::vec3(0,0,1);
		glm::vec3 frameSpeed=WallVelocity(local,dt);
		cube.velocity-=glm::dot(cube.velocity-frameSpeed,depthNormal)*depthNormal;
		for(int axis=0;axis<2;++axis)for(float side:{-1.0f,1.0f}){
			float limit=BOX_HALF_SIZE-cube.size*.5f;
			if(side*local[axis]>=limit){
				local[axis]=side*limit;glm::vec3 normal(0);normal[axis]=-side;normal=rotation*normal;
				glm::vec3 wallSpeed=WallVelocity(local,dt),relative=cube.velocity-wallSpeed;
				float incoming=glm::dot(relative,normal);
				if(incoming<0){
					relative-=(1+CUBE_RESTITUTION)*incoming*normal;
					cube.velocity=wallSpeed+relative;
				}
			}
		}
		cube.position=glm::vec3(boxTransform*glm::vec4(local,1));
	}
}

void UpdateAnimation(double elapsed)
{
	accumulator+=elapsed;
	while(accumulator>=1.0/PHYSICS_FPS){PhysicsStep(1.0f/PHYSICS_FPS);accumulator-=1.0/PHYSICS_FPS;}
}

void DrawMesh(const Mesh& mesh,glm::mat4 model,glm::vec3 color,int first=0,int count=-1)
{
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));glUniform3fv(colorLocation,1,glm::value_ptr(color));
	glBindVertexArray(mesh.vao);glDrawArrays(GL_TRIANGLES,first,count<0 ? mesh.count : count);
}

void DrawScene()
{
	glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(programID);
	glm::mat4 view=glm::lookAt(eye,glm::vec3(0),glm::vec3(0,1,0));
	glm::mat4 projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),(float)WINDOW_WIDTH/WINDOW_HEIGHT,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
	glm::vec3 wallColors[]={{.52f,.53f,.55f},{.60f,.61f,.63f},{.66f,.67f,.69f},{.57f,.58f,.60f},{.73f,.74f,.76f}};
	for(int i=0;i<4;++i)DrawMesh(wallMesh,boxTransform,wallColors[i],i*6,6);
	glm::vec3 hinge(0,-BOX_HALF_SIZE,-BOX_HALF_SIZE);
	glm::mat4 door=glm::translate(boxTransform,hinge);
	door=glm::rotate(door,glm::radians(-doorAngle),glm::vec3(1,0,0));
	door=glm::translate(door,-hinge);DrawMesh(wallMesh,door,wallColors[4],24,6);
	for(const Cube& cube:cubes){
		glm::mat4 model=glm::translate(glm::mat4(1),cube.position)*glm::mat4(glm::mat3(boxTransform));
		DrawMesh(cubeMesh,glm::scale(model,glm::vec3(cube.size)),glm::vec3(.90f,.06f,.04f));
	}
	for(const Ball& ball:balls){
		glm::mat4 model=glm::scale(glm::translate(glm::mat4(1),ball.position),glm::vec3(BALL_RADIUS));
		DrawMesh(sphereMesh,model,glm::vec3(.04f,.12f,.95f));
	}
	glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
	if(action==GLFW_RELEASE && key==movementKey)movementKey=0;
	if(key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT)reverseKey=(mods&GLFW_MOD_SHIFT)!=0;
	if(action!=GLFW_PRESS)return;
	if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE){glfwSetWindowShouldClose(window,true);return;}
	if(key==GLFW_KEY_S){ResetScene();return;}
	if(key==GLFW_KEY_B){SpawnBall();return;}
	if(key==GLFW_KEY_Z || key==GLFW_KEY_Y){movementKey=key;reverseKey=(mods&GLFW_MOD_SHIFT)!=0;}
}

void MouseCallback(GLFWwindow* window,int button,int action,int)
{
	if(button==GLFW_MOUSE_BUTTON_LEFT){
		dragging=action==GLFW_PRESS;
		if(dragging){double y;glfwGetCursorPos(window,&previousMouseX,&y);}
	}
	if(button==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_PRESS && doorState==CLOSED){
		doorState=OPENING;doorTime=0;
	}
}

void CursorCallback(GLFWwindow* window,double x,double)
{
	if(!dragging)return;
	int width,height;glfwGetWindowSize(window,&width,&height);
	if(width>0)targetRoll-=(float)(x-previousMouseX)*DRAG_DEGREES/width;
	previousMouseX=x;
}
