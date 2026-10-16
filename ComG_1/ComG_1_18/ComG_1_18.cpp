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
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CAMERA_X 0.0f
#define CAMERA_Y 1.0f
#define CAMERA_Z 4.0f
#define PERSPECTIVE_FOVY 40.0f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 30.0f
#define VIEW_RANGE 1.50f
#define VIEW_RANGE_MIN 0.20f
#define VIEW_RANGE_MAX 5.0f
#define CAMERA_RADIUS_MIN 1.50f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define OBJECT_MOVE_SPEED 0.50f
#define SUN_RADIUS 0.16f
#define SUN_SCALE_MIN 0.75f
#define SUN_SCALE_MAX 1.25f
#define SUN_CYCLE_SPEED 1.5f
#define SUN_SMALL_RED 0.90f
#define SUN_SMALL_GREEN 0.20f
#define SUN_SMALL_BLUE 0.10f
#define SUN_LARGE_RED 0.90f
#define SUN_LARGE_GREEN 0.80f
#define SUN_LARGE_BLUE 0.10f
#define PLANET_RADIUS 0.075f
#define MOON_RADIUS 0.028f
#define PLANET_ORBIT_1 1.0f
#define PLANET_ORBIT_2 1.0f
#define PLANET_ORBIT_3 1.0f
#define PLANET_SPEED_1 25.0f
#define PLANET_SPEED_2 17.0f
#define PLANET_SPEED_3 11.0f
#define ORBIT_TILT 45.0f
#define MOON_ORBIT_RADIUS 0.15f
#define MOON_SPEED_1 100.0f
#define MOON_SPEED_2 80.0f
#define MOON_SPEED_3 60.0f
#define MOON_TILT_SPEED 35.0f
#define ORBIT_SCALE_MIN 0.85f
#define ORBIT_SCALE_MAX 1.15f
#define ORBIT_CYCLE_SPEED 1.0f
#define SPHERE_SLICES 32
#define SPHERE_STACKS 24
#define PATH_SEGMENTS 128
#define DIFFUSE_STRENGTH 1.0f
#define SPECULAR_STRENGTH 0.45f
#define SHININESS 32.0f

struct Planet { float radius, speed, moonSpeed, tilt, angle, moonAngle; glm::vec3 color; };
Planet planets[3] = {
	{PLANET_ORBIT_1,PLANET_SPEED_1,MOON_SPEED_1,0,0,0,{.9f,.1f,.1f}},
	{PLANET_ORBIT_2,PLANET_SPEED_2,MOON_SPEED_2,ORBIT_TILT,120,120,{.1f,.9f,.1f}},
	{PLANET_ORBIT_3,PLANET_SPEED_3,MOON_SPEED_3,-ORBIT_TILT,240,240,{.1f,.1f,.9f}}
};
glm::vec3 cameraPosition(CAMERA_X,CAMERA_Y,CAMERA_Z), scenePosition(0);
float cameraRadius=glm::length(cameraPosition), cameraYaw=0, cameraPitch=0, viewRange=VIEW_RANGE;
float orbitPhase=0, sunPhase=0, moonTilt=0;
bool perspective=true, wireframe=false, orbitChanging=false, sunChanging=false, moonRotating=false, dragging=false;
double previousMouseX=0, previousMouseY=0;
GLuint programID=0, vao=0, vbo=0;
GLint modelLocation,viewLocation,projectionLocation,colorLocation,shadingLocation;
GLUquadric* qobj=nullptr;

string ReadFile(const char* name);
bool InitShader();
void DrawScene();
void DrawSphere(glm::vec3 center, float radius, glm::vec3 color, bool shaded);
void DrawPath(const glm::mat4& model, glm::vec3 color);
void KeyCallback(GLFWwindow* window,int key,int scancode,int action,int mods);
void MouseCallback(GLFWwindow* window,int button,int action,int mods);
void CursorCallback(GLFWwindow* window,double x,double y);
void ScrollCallback(GLFWwindow* window,double xoffset,double yoffset);

int main()
{
	if(!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
	glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_COMPAT_PROFILE); glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);
	GLFWwindow* window=glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"1-18 Planet Orbits",nullptr,nullptr);
	if(!window){glfwTerminate();return -1;}
	glfwMakeContextCurrent(window); glewExperimental=GL_TRUE;
	if(glewInit()!=GLEW_OK || !InitShader()){glfwDestroyWindow(window);glfwTerminate();return -1;}
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window,KeyCallback); glfwSetMouseButtonCallback(window,MouseCallback);
	glfwSetCursorPosCallback(window,CursorCallback); glfwSetScrollCallback(window,ScrollCallback);
	shadingLocation=glGetUniformLocation(programID,"useShading");
	vector<glm::vec3> path;
	for(int i=0;i<PATH_SEGMENTS;++i){float a=glm::two_pi<float>()*i/PATH_SEGMENTS;path.push_back({glm::cos(a),0,glm::sin(a)});}
	glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
	glBufferData(GL_ARRAY_BUFFER,path.size()*sizeof(glm::vec3),path.data(),GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),nullptr);glEnableVertexAttribArray(0);glBindVertexArray(0);
	qobj=gluNewQuadric();if(!qobj){glfwDestroyWindow(window);glfwTerminate();return -1;}
	gluQuadricNormals(qobj,GLU_SMOOTH);gluQuadricOrientation(qobj,GLU_OUTSIDE);
	cout<<"p/P: ORTHO/PERSP | m/M: SOLID/WIRE | WASD, +/-: move | 1/2/3: orbit size/sun/moon tilt | q: quit"<<endl;
	double previousTime=glfwGetTime();
	while(!glfwWindowShouldClose(window)){
		glfwPollEvents();double now=glfwGetTime();float dt=(float)glm::min(now-previousTime,0.1);previousTime=now;
		glm::vec3 movement(0);
		if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS)movement.x-=1;
		if(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS)movement.x+=1;
		if(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)movement.y+=1;
		if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS)movement.y-=1;
		bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
		if(glfwGetKey(window,GLFW_KEY_KP_ADD)==GLFW_PRESS || (shift && glfwGetKey(window,GLFW_KEY_EQUAL)==GLFW_PRESS))movement.z+=1;
		if(glfwGetKey(window,GLFW_KEY_KP_SUBTRACT)==GLFW_PRESS || glfwGetKey(window,GLFW_KEY_MINUS)==GLFW_PRESS)movement.z-=1;
		if(glm::length(movement)>0)scenePosition+=glm::normalize(movement)*OBJECT_MOVE_SPEED*dt;
		for(Planet& p:planets){p.angle=glm::mod(p.angle+p.speed*dt,360.0f);p.moonAngle=glm::mod(p.moonAngle+p.moonSpeed*dt,360.0f);}
		if(orbitChanging)orbitPhase=glm::mod(orbitPhase+ORBIT_CYCLE_SPEED*dt,glm::two_pi<float>());
		if(sunChanging)sunPhase=glm::mod(sunPhase+SUN_CYCLE_SPEED*dt,glm::two_pi<float>());
		if(moonRotating)moonTilt=glm::mod(moonTilt+MOON_TILT_SPEED*dt,360.0f);
		int width,height;glfwGetFramebufferSize(window,&width,&height);glViewport(0,0,width,height);
		DrawScene();glfwSwapBuffers(window);
	}
	gluDeleteQuadric(qobj);glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(programID);
	glfwDestroyWindow(window);glfwTerminate();return 0;
}

void DrawPath(const glm::mat4& model,glm::vec3 color)
{
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
	glUniform3fv(colorLocation,1,glm::value_ptr(color));glUniform1i(shadingLocation,GL_FALSE);
	glBindVertexArray(vao);glDrawArrays(GL_LINE_LOOP,0,PATH_SEGMENTS);glBindVertexArray(0);
}

void DrawSphere(glm::vec3 center,float radius,glm::vec3 color,bool shaded)
{
	glm::mat4 model=glm::translate(glm::mat4(1),center);
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(model));
	glUniform3fv(colorLocation,1,glm::value_ptr(color));glUniform1i(shadingLocation,shaded);
	gluSphere(qobj,radius,SPHERE_SLICES,SPHERE_STACKS);
}

void DrawScene()
{
	glClearColor(1,1,1,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(programID);
	glm::mat4 view=glm::lookAt(cameraPosition,glm::vec3(0),glm::vec3(0,1,0));
	glm::mat4 projection=perspective ? glm::perspective(glm::radians(PERSPECTIVE_FOVY),PERSPECTIVE_ASPECT,PERSPECTIVE_NEAR,PERSPECTIVE_FAR)
		: glm::ortho(-viewRange*PERSPECTIVE_ASPECT,viewRange*PERSPECTIVE_ASPECT,-viewRange,viewRange,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
	glUniform3fv(glGetUniformLocation(programID,"lightPosition"),1,glm::value_ptr(scenePosition));
	glUniform3fv(glGetUniformLocation(programID,"eyePosition"),1,glm::value_ptr(cameraPosition));
	glUniform1f(glGetUniformLocation(programID,"diffuseStrength"),DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"specularStrength"),SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"shininess"),SHININESS);
	gluQuadricDrawStyle(qobj,wireframe ? GLU_LINE : GLU_FILL);
	float t=(glm::sin(sunPhase)+1)*.5f;
	glm::vec3 sunColor=glm::mix(glm::vec3(SUN_SMALL_RED,SUN_SMALL_GREEN,SUN_SMALL_BLUE),glm::vec3(SUN_LARGE_RED,SUN_LARGE_GREEN,SUN_LARGE_BLUE),t);
	DrawSphere(scenePosition,SUN_RADIUS*glm::mix(SUN_SCALE_MIN,SUN_SCALE_MAX,t),sunColor,false);
	float orbitScale=glm::mix(ORBIT_SCALE_MIN,ORBIT_SCALE_MAX,(glm::sin(orbitPhase)+1)*.5f);
	for(const Planet& p:planets){
		glm::mat4 tilt=glm::rotate(glm::mat4(1),glm::radians(p.tilt),glm::vec3(0,0,1));
		glm::mat4 orbit=glm::translate(glm::mat4(1),scenePosition)*tilt;
		DrawPath(glm::scale(orbit,glm::vec3(p.radius*orbitScale)),p.color);
		float a=glm::radians(p.angle);
		glm::vec3 center=glm::vec3(orbit*glm::vec4(p.radius*orbitScale*glm::cos(a),0,p.radius*orbitScale*glm::sin(a),1));
		DrawSphere(center,PLANET_RADIUS,p.color,true);
		// Rotate the initially world-XZ moon orbit about the planet orbit's local Z.
		glm::vec3 localZ=glm::vec3(tilt*glm::vec4(0,0,1,0));
		glm::mat4 moonOrbit=glm::rotate(glm::translate(glm::mat4(1),center),glm::radians(moonTilt),localZ);
		float radius=MOON_ORBIT_RADIUS*orbitScale;glm::vec3 moonColor=glm::vec3(1)-p.color;
		DrawPath(glm::scale(moonOrbit,glm::vec3(radius)),moonColor);
		a=glm::radians(p.moonAngle);
		glm::vec3 moonCenter=glm::vec3(moonOrbit*glm::vec4(radius*glm::cos(a),0,radius*glm::sin(a),1));
		DrawSphere(moonCenter,MOON_RADIUS,moonColor,true);
	}
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
	if(action!=GLFW_PRESS)return;
	if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
	if(key==GLFW_KEY_P)perspective=(mods&GLFW_MOD_SHIFT)!=0;
	if(key==GLFW_KEY_M)wireframe=(mods&GLFW_MOD_SHIFT)!=0;
	if(key==GLFW_KEY_1)orbitChanging=!orbitChanging;
	if(key==GLFW_KEY_2)sunChanging=!sunChanging;
	if(key==GLFW_KEY_3)moonRotating=!moonRotating;
}

void MouseCallback(GLFWwindow* window,int button,int action,int)
{
	if(button!=GLFW_MOUSE_BUTTON_LEFT)return;
	dragging=action==GLFW_PRESS;
	if(dragging){glfwGetCursorPos(window,&previousMouseX,&previousMouseY);cameraYaw=glm::atan(cameraPosition.x,cameraPosition.z);cameraPitch=glm::asin(cameraPosition.y/cameraRadius);}
}

void CursorCallback(GLFWwindow* window,double x,double y)
{
	if(!dragging)return;
	int width,height;glfwGetWindowSize(window,&width,&height);if(width<=0 || height<=0)return;
	cameraYaw-=glm::radians(ORBIT_DEGREES)*(float)(x-previousMouseX)/width;
	cameraPitch+=glm::radians(ORBIT_DEGREES)*(float)(y-previousMouseY)/height;
	previousMouseX=x;previousMouseY=y;cameraYaw=glm::mod(cameraYaw,glm::two_pi<float>());
	cameraPitch=glm::clamp(cameraPitch,-glm::radians(ORBIT_PITCH_LIMIT),glm::radians(ORBIT_PITCH_LIMIT));
	cameraPosition=cameraRadius*glm::vec3(glm::cos(cameraPitch)*glm::sin(cameraYaw),glm::sin(cameraPitch),glm::cos(cameraPitch)*glm::cos(cameraYaw));
}

void ScrollCallback(GLFWwindow*,double,double yoffset)
{
	if(perspective){cameraRadius=glm::clamp(cameraRadius-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);cameraPosition=glm::normalize(cameraPosition)*cameraRadius;}
	else viewRange=glm::clamp(viewRange-(float)yoffset*ZOOM_STEP,VIEW_RANGE_MIN,VIEW_RANGE_MAX);
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
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0;
}

