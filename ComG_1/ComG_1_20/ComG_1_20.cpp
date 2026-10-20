#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CAMERA_X 4.0f
#define CAMERA_Y 4.0f
#define CAMERA_Z 6.0f
#define CAMERA_TARGET_X 0.15f
#define CAMERA_TARGET_Y 0.35f
#define CAMERA_TARGET_Z 0.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 50.0f
#define CAMERA_MOVE_SPEED 1.5f
#define CAMERA_RADIUS_MIN 0.50f
#define CAMERA_RADIUS_MAX 25.0f
#define VIEW_RANGE 3.2f
#define VIEW_RANGE_MIN 0.2f
#define VIEW_RANGE_MAX 15.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define FLOOR_WIDTH 6.0f
#define FLOOR_DEPTH 5.0f
#define FLOOR_HEIGHT 0.06f
#define BELT_START_X -1.20f
#define BELT_START_Z 1.60f
#define BELT_HEIGHT 0.65f
#define BELT_WIDTH 0.65f
#define BELT_FIRST_LENGTH 2.0f
#define BELT_SECOND_LENGTH 2.2f
#define BELT_CORNER_RADIUS 0.65f
#define BELT_SEGMENTS 120
#define BELT_SPEED 0.40f
#define BELT_END_MARGIN 0.25f
#define STRIPE_COUNT 32
#define STRIPE_LENGTH 0.055f
#define STRIPE_HEIGHT 0.008f
#define RAIL_WIDTH 0.035f
#define RAIL_HEIGHT 0.045f
#define LEG_WIDTH 0.06f
#define LEG_PAIRS 7
#define BOX_SIZE 0.30f
#define BOX_COUNT 5
#define MARK_WIDTH 0.032f
#define MARK_LENGTH 0.12f
#define MARK_HEIGHT 0.004f
#define MARK_GAP 0.018f
#define CEILING_HEIGHT 4.0f
#define LIGHT_SAMPLES_X 6
#define LIGHT_SAMPLES_Z 6
#define DIFFUSE_STRENGTH 1.25f
#define SPECULAR_STRENGTH 0.30f
#define SHININESS 32.0f

struct Vertex { glm::vec3 position, normal; };
glm::vec3 boxColors[BOX_COUNT] = { {.9f,.1f,.1f},{.9f,.45f,.1f},{.9f,.9f,.1f},{.1f,.8f,.1f},{.1f,.25f,.9f} };
glm::vec3 cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z), cameraTarget(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
float beltDistance = 0, viewRange = VIEW_RANGE;
float pathLength = BELT_FIRST_LENGTH + BELT_CORNER_RADIUS * glm::half_pi<float>() + BELT_SECOND_LENGTH;
bool moving = false, perspective = true, dragging = false;
double previousMouseX = 0, previousMouseY = 0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation, eyeLocation;
int surfaceCount = 0;

string ReadFile(const char* name);
bool InitShader();
glm::mat4 BeltTransform(float distance);
void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-20 Conveyor", nullptr, nullptr);
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
	glm::vec3 normals[] = { {0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
	vector<Vertex> vertices;
	for (int i = 0; i < 36; ++i)vertices.push_back({corners[indices[i]], normals[i/6]});
	// A continuous flat ribbon follows the same rounded path as the boxes.
	for (int i = 0; i < BELT_SEGMENTS; ++i) {
		float d0 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * i / BELT_SEGMENTS;
		float d1 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * (i+1) / BELT_SEGMENTS;
		glm::mat4 a = BeltTransform(d0), b = BeltTransform(d1);
		glm::vec3 p[] = { glm::vec3(a * glm::vec4(-BELT_WIDTH*.5f,0,0,1)), glm::vec3(a * glm::vec4(BELT_WIDTH*.5f,0,0,1)),
			glm::vec3(b * glm::vec4(-BELT_WIDTH*.5f,0,0,1)), glm::vec3(b * glm::vec4(BELT_WIDTH*.5f,0,0,1)) };
		int face[] = {0,1,2,1,3,2};
		for (int index : face)vertices.push_back({p[index],glm::vec3(0,1,0)});
	}
	surfaceCount = (int)vertices.size() - 36;
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position)); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,normal)); glEnableVertexAttribArray(1); glBindVertexArray(0);
	glUseProgram(programID); eyeLocation = glGetUniformLocation(programID,"eyePosition");
	glUniform2f(glGetUniformLocation(programID,"ceilingSize"),FLOOR_WIDTH,FLOOR_DEPTH);
	glUniform1f(glGetUniformLocation(programID,"ceilingHeight"),CEILING_HEIGHT);
	glUniform2i(glGetUniformLocation(programID,"lightSamples"),LIGHT_SAMPLES_X,LIGHT_SAMPLES_Z);
	glUniform1f(glGetUniformLocation(programID,"diffuseStrength"),DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"specularStrength"),SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID,"shininess"),SHININESS);
	cout << "b: start/stop | WASD: camera | drag: orbit | wheel, +/-: zoom | p/P: PERSP/ORTHO | r: reset | q: quit" << endl;
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents(); double now = glfwGetTime(); float dt = (float)glm::min(now-previousTime,0.1); previousTime=now;
		glm::vec3 movement(0);
		glm::vec3 forward = cameraTarget - cameraPosition;
		forward.y = 0;
		if (glm::length(forward)>0)forward = glm::normalize(forward);
		glm::vec3 right = glm::cross(forward,glm::vec3(0,1,0));
		if (glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)movement+=forward;
		if (glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS)movement-=forward;
		if (glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS)movement-=right;
		if (glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS)movement+=right;
		if (glm::length(movement)>0) { movement=glm::normalize(movement)*CAMERA_MOVE_SPEED*dt; cameraPosition+=movement; cameraTarget+=movement; }
		if (moving)beltDistance=glm::mod(beltDistance+BELT_SPEED*dt,pathLength);
		int width,height; glfwGetFramebufferSize(window,&width,&height); glViewport(0,0,width,height);
		DrawScene(); glfwSwapBuffers(window);
	}
	glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

glm::mat4 BeltTransform(float distance)
{
	float arcLength = BELT_CORNER_RADIUS * glm::half_pi<float>();
	glm::vec3 position(BELT_START_X,BELT_HEIGHT,BELT_START_Z);
	float angle=0;
	if (distance<=BELT_FIRST_LENGTH)position.z-=distance;
	else if (distance<BELT_FIRST_LENGTH+arcLength) {
		angle=(distance-BELT_FIRST_LENGTH)/BELT_CORNER_RADIUS;
		position.x+=BELT_CORNER_RADIUS*(1-glm::cos(angle));
		position.z-=BELT_FIRST_LENGTH+BELT_CORNER_RADIUS*glm::sin(angle);
	}
	else {
		angle=glm::half_pi<float>();
		position.x+=BELT_CORNER_RADIUS+distance-BELT_FIRST_LENGTH-arcLength;
		position.z-=BELT_FIRST_LENGTH+BELT_CORNER_RADIUS;
	}
	return glm::rotate(glm::translate(glm::mat4(1),position),-angle,glm::vec3(0,1,0));
}

void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color)
{
	glm::mat4 transform=glm::scale(model,size);
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(transform));
	glUniform3fv(colorLocation,1,glm::value_ptr(color));
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,36); glBindVertexArray(0);
}

void DrawScene()
{
	glClearColor(.70f,.82f,.94f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); glUseProgram(programID);
	glm::mat4 view=glm::lookAt(cameraPosition,cameraTarget,glm::vec3(0,1,0));
	glm::mat4 projection=perspective ? glm::perspective(glm::radians(PERSPECTIVE_FOVY),PERSPECTIVE_ASPECT,PERSPECTIVE_NEAR,PERSPECTIVE_FAR)
		: glm::ortho(-viewRange*PERSPECTIVE_ASPECT,viewRange*PERSPECTIVE_ASPECT,-viewRange,viewRange,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view)); glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
	glUniform3fv(eyeLocation,1,glm::value_ptr(cameraPosition));
	DrawBox(glm::translate(glm::mat4(1),glm::vec3(0,-FLOOR_HEIGHT*.5f,0)),glm::vec3(FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH),glm::vec3(.32f,.34f,.32f));
	glm::mat4 identity(1); glm::vec3 beltColor(.22f,.24f,.27f);
	glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(identity)); glUniform3fv(colorLocation,1,glm::value_ptr(beltColor));
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,36,surfaceCount); glBindVertexArray(0);
	for (int i=0;i<BELT_SEGMENTS;++i) {
		float d0=-BELT_END_MARGIN+(pathLength+2*BELT_END_MARGIN)*i/BELT_SEGMENTS;
		float d1=-BELT_END_MARGIN+(pathLength+2*BELT_END_MARGIN)*(i+1)/BELT_SEGMENTS;
		for (int side=-1;side<=1;side+=2) {
			glm::vec3 a=glm::vec3(BeltTransform(d0)*glm::vec4(side*(BELT_WIDTH+RAIL_WIDTH)*.5f,RAIL_HEIGHT*.5f,0,1));
			glm::vec3 b=glm::vec3(BeltTransform(d1)*glm::vec4(side*(BELT_WIDTH+RAIL_WIDTH)*.5f,RAIL_HEIGHT*.5f,0,1));
			glm::vec3 delta=b-a;
			glm::mat4 rail=glm::rotate(glm::translate(glm::mat4(1),(a+b)*.5f),glm::atan(-delta.x,-delta.z),glm::vec3(0,1,0));
			DrawBox(rail,glm::vec3(RAIL_WIDTH,RAIL_HEIGHT,glm::length(delta)+.002f),glm::vec3(.85f,.70f,.12f));
		}
	}
	for (int i=0;i<LEG_PAIRS;++i) {
		glm::mat4 frame=BeltTransform(pathLength*i/(LEG_PAIRS-1));
		for (int side=-1;side<=1;side+=2)
			DrawBox(glm::translate(frame,glm::vec3(side*(BELT_WIDTH-LEG_WIDTH)*.5f,-BELT_HEIGHT*.5f,0)),glm::vec3(LEG_WIDTH,BELT_HEIGHT,LEG_WIDTH),glm::vec3(.15f));
	}
	for (int i=0;i<STRIPE_COUNT;++i) {
		float distance=glm::mod(beltDistance+pathLength*i/STRIPE_COUNT,pathLength);
		DrawBox(glm::translate(BeltTransform(distance),glm::vec3(0,STRIPE_HEIGHT*.5f,0)),glm::vec3(BELT_WIDTH*.97f,STRIPE_HEIGHT,STRIPE_LENGTH),glm::vec3(.50f,.52f,.55f));
	}
	for (int i=0;i<BOX_COUNT;++i) {
		float distance=glm::mod(beltDistance+pathLength*i/BOX_COUNT,pathLength);
		glm::mat4 box=glm::translate(BeltTransform(distance),glm::vec3(0,STRIPE_HEIGHT+BOX_SIZE*.5f,0));
		DrawBox(box,glm::vec3(BOX_SIZE),boxColors[i]);
		for (int mark=0;mark<=i;++mark) {
			float x=(mark-i*.5f)*(MARK_WIDTH+MARK_GAP);
			DrawBox(glm::translate(box,glm::vec3(x,(BOX_SIZE+MARK_HEIGHT)*.5f,0)),glm::vec3(MARK_WIDTH,MARK_HEIGHT,MARK_LENGTH),glm::vec3(.10f));
		}
	}
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
	if (action!=GLFW_PRESS && action!=GLFW_REPEAT)return;
	if (key==GLFW_KEY_KP_ADD || key==GLFW_KEY_EQUAL)ScrollCallback(window,0,1);
	if (key==GLFW_KEY_KP_SUBTRACT || key==GLFW_KEY_MINUS)ScrollCallback(window,0,-1);
	if (action!=GLFW_PRESS)return;
	if (key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
	if (key==GLFW_KEY_B)moving=!moving;
	if (key==GLFW_KEY_P)perspective=(mods&GLFW_MOD_SHIFT)==0;
	if (key==GLFW_KEY_R) {
		beltDistance=0; moving=false; perspective=true; dragging=false; viewRange=VIEW_RANGE;
		cameraPosition=glm::vec3(CAMERA_X,CAMERA_Y,CAMERA_Z); cameraTarget=glm::vec3(CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z);
	}
}

void ScrollCallback(GLFWwindow*,double,double yoffset)
{
	if (perspective) {
		glm::vec3 offset=cameraPosition-cameraTarget;
		float radius=glm::clamp(glm::length(offset)-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
		cameraPosition=cameraTarget+glm::normalize(offset)*radius;
	}
	else viewRange=glm::clamp(viewRange-(float)yoffset*ZOOM_STEP,VIEW_RANGE_MIN,VIEW_RANGE_MAX);
}

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





