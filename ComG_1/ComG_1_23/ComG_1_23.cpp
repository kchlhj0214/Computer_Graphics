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

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define ROOM_WIDTH 4.0f
#define ROOM_HEIGHT 3.0f
#define ROOM_DEPTH 4.0f
#define CAMERA_X 4.5f
#define CAMERA_Y 4.0f
#define CAMERA_Z 6.5f
#define CAMERA_MOVE_SPEED 1.5f
#define CAMERA_ROTATE_SPEED 35.0f
#define CAMERA_RADIUS_MIN 5.5f
#define CAMERA_RADIUS_MAX 20.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_NEAR 0.1f
#define PERSPECTIVE_FAR 100.0f
#define ORBIT_ENABLED 1
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 80.0f
#define ZOOM_STEP 0.05f
#define BODY_WIDTH .24f
#define BODY_HEIGHT .32f
#define BODY_DEPTH .16f
#define HEAD_SIZE .19f
#define LIMB_WIDTH .075f
#define ARM_LENGTH .29f
#define LEG_LENGTH .30f
#define LIMB_GAP .025f
#define NOSE_SIZE .055f
#define WALK_SPEED .65f
#define WALK_SPEED_MIN .20f
#define WALK_SPEED_MAX 1.30f
#define WALK_SPEED_STEP .10f
#define WALK_ANGLE_MIN 12.0f
#define WALK_ANGLE_MAX 38.0f
#define WALK_PHASE_PER_DISTANCE 10.0f
#define ROBOT_RADIUS .24f
#define JUMP_HEIGHT .85f
#define JUMP_SECONDS 1.0f
#define DESCEND_SPEED 1.5f
#define PHYSICS_FPS 240
#define OBSTACLE_COUNT 3
#define OBSTACLE_SIZE_MIN .30f
#define OBSTACLE_SIZE_MAX .65f
#define OBSTACLE_HEIGHT_MIN .20f
#define OBSTACLE_HEIGHT_MAX .50f
#define OBSTACLE_GAP .25f
#define CURTAIN_SECONDS .8f
#define CURTAIN_OPEN_INITIAL 0
#define KEY_OPEN GLFW_KEY_O
#define KEY_JUMP GLFW_KEY_SPACE
#define KEY_RESET GLFW_KEY_I

struct Obstacle { glm::vec3 position, size, color; };
vector<Obstacle> obstacles;
mt19937 randomEngine(random_device{}());
glm::vec3 roomCenter(0,ROOM_HEIGHT*.5f,0);
glm::vec3 robotPosition(0), walkDirection(0,0,-1);
glm::vec3 cameraPosition(CAMERA_X,CAMERA_Y,CAMERA_Z), cameraTarget=roomCenter;
float walkSpeed=WALK_SPEED, walkPhase=0, jumpTime=0, jumpStart=0;
float curtainOpen=CURTAIN_OPEN_INITIAL;
bool walking=false, jumping=false, curtainTarget=CURTAIN_OPEN_INITIAL, dragging=false;
int cameraOrbit=0;
double previousMouseX=0,previousMouseY=0,accumulator=0;
GLuint programID=0,vao=0,vbo=0;
GLint modelLocation,viewLocation,projectionLocation,colorLocation;
string ReadFile(const char* name);
bool InitShader();
void ResetScene();
void UpdateScene(GLFWwindow* window,float dt);
void DrawScene(int width,int height);
void DrawBox(const glm::mat4& model,glm::vec3 size,glm::vec3 color);
void KeyCallback(GLFWwindow*,int,int,int,int);
void MouseCallback(GLFWwindow*,int,int,int);
void CursorCallback(GLFWwindow*,double,double);
void ScrollCallback(GLFWwindow*,double,double);

int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-23 Walking Robot", nullptr, nullptr);
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
    ResetScene();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    cout << "WASD: walk | +/-: speed | Space: jump | o: curtain | i: reset\n"
         << "x/X,z/Z: camera | y/Y: orbit | drag/wheel: camera | q: quit" << endl;
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents(); double now = glfwGetTime(); float dt = (float)glm::min(now - previousTime, 0.1); previousTime = now;
		UpdateScene(window, dt);
		int width, height; glfwGetFramebufferSize(window, &width, &height);
		DrawScene(width, height); glfwSwapBuffers(window);
	}
	glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

void ResetScene()
{
    robotPosition=glm::vec3(0);
    walkDirection={0,0,-1};
    walking=jumping=dragging=false;
    walkSpeed=WALK_SPEED;
    walkPhase=jumpTime=jumpStart=0;
    accumulator=0;
    curtainOpen=CURTAIN_OPEN_INITIAL;
    curtainTarget=CURTAIN_OPEN_INITIAL;
    cameraPosition={CAMERA_X,CAMERA_Y,CAMERA_Z};
    cameraTarget=roomCenter;
    cameraOrbit=0;
    obstacles.clear();
    glm::vec3 colors[]={{.85f,.45f,.12f},{.50f,.25f,.70f},{.55f,.60f,.65f}};
    auto random=[](float low,float high)
    {
        return uniform_real_distribution<float>(low,high)(randomEngine);
    };
    for(int i=0;i<OBSTACLE_COUNT;++i)
    {
        Obstacle box;
        box.size={random(OBSTACLE_SIZE_MIN,OBSTACLE_SIZE_MAX),
            random(OBSTACLE_HEIGHT_MIN,OBSTACLE_HEIGHT_MAX),random(OBSTACLE_SIZE_MIN,OBSTACLE_SIZE_MAX)};
        box.color=colors[i%3];
        bool placed=false;
        for(int attempt=0;attempt<1000 && !placed;++attempt)
        {
            box.position={random(-ROOM_WIDTH*.5f+box.size.x,ROOM_WIDTH*.5f-box.size.x),
                box.size.y*.5f,random(-ROOM_DEPTH*.5f+box.size.z,ROOM_DEPTH*.5f-box.size.z)};
            placed=glm::abs(box.position.x)>box.size.x*.5f+ROBOT_RADIUS+OBSTACLE_GAP
                || glm::abs(box.position.z)>box.size.z*.5f+ROBOT_RADIUS+OBSTACLE_GAP;
            for(const auto& other:obstacles)
                if(glm::abs(box.position.x-other.position.x)<(box.size.x+other.size.x)*.5f+OBSTACLE_GAP
                    && glm::abs(box.position.z-other.position.z)<(box.size.z+other.size.z)*.5f+OBSTACLE_GAP)
                    placed=false;
        }
        if(!placed)
        {
            cerr << "Obstacle placement failed: reduce sizes/count." << endl;
            break;
        }
        obstacles.push_back(box);
    }
}

void UpdateScene(GLFWwindow* window,float dt)
{
    bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS
        || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    glm::vec3 movement(0);
    float sign=shift ? -1.0f : 1.0f;
    if(glfwGetKey(window,GLFW_KEY_X)==GLFW_PRESS)movement.x=sign;
    if(glfwGetKey(window,GLFW_KEY_Z)==GLFW_PRESS)movement.z=sign;
    if(glm::length(movement)>0)
    {
        movement=glm::normalize(movement)*CAMERA_MOVE_SPEED*dt;
        cameraPosition+=movement;
        cameraTarget+=movement;
    }
    if(cameraOrbit)
    {
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(cameraOrbit*CAMERA_ROTATE_SPEED*dt),glm::vec3(0,1,0));
        cameraPosition=roomCenter+glm::vec3(rotation*glm::vec4(cameraPosition-roomCenter,0));
        cameraTarget=roomCenter+glm::vec3(rotation*glm::vec4(cameraTarget-roomCenter,0));
    }
    // 닫힌 무대 안으로 카메라가 진입하지 않게 제한한다.
    glm::vec3 fromCenter=cameraPosition-roomCenter;
    if(glm::length(fromCenter)<CAMERA_RADIUS_MIN)
        cameraPosition=roomCenter+(glm::length(fromCenter)>.001f ? glm::normalize(fromCenter) : glm::vec3(0,0,1))*CAMERA_RADIUS_MIN;
    float target=curtainTarget ? 1.0f : 0.0f;
    curtainOpen+=glm::clamp(target-curtainOpen,-dt/CURTAIN_SECONDS,dt/CURTAIN_SECONDS);
    accumulator+=dt;
    const float step=1.0f/PHYSICS_FPS;
    while(accumulator>=step)
    {
        accumulator-=step;
        float previousY=robotPosition.y;
        float nextY=previousY;
        if(jumping)
        {
            jumpTime=glm::min(jumpTime+step,JUMP_SECONDS);
            float t=jumpTime/JUMP_SECONDS;
            nextY=jumpStart+JUMP_HEIGHT*(t<.5f ? 2.0f*t : 2.0f*(1.0f-t));
        }
        else
            nextY=glm::max(0.0f,previousY-DESCEND_SPEED*step);
        glm::vec3 candidate=robotPosition;
        if(walking)
        {
            candidate+=walkDirection*walkSpeed*step;
            walkPhase=glm::mod(walkPhase+walkSpeed*WALK_PHASE_PER_DISTANCE*step,glm::two_pi<float>());
        }
        bool blocked=glm::abs(candidate.x)>ROOM_WIDTH*.5f-ROBOT_RADIUS
            || glm::abs(candidate.z)>ROOM_DEPTH*.5f-ROBOT_RADIUS;
        for(const auto& box:obstacles)
        {
            bool overlap=glm::abs(candidate.x-box.position.x)<box.size.x*.5f+ROBOT_RADIUS
                && glm::abs(candidate.z-box.position.z)<box.size.z*.5f+ROBOT_RADIUS;
            // 현재 높이 또는 점프 높이가 윗면 이상이면 위로 통과할 수 있다.
            if(overlap && glm::max(previousY,nextY)<box.size.y-.0001f)
                blocked=true;
        }
        if(blocked && walking)
            walkDirection=-walkDirection;
        else
        {
            robotPosition.x=candidate.x;
            robotPosition.z=candidate.z;
        }
        float support=0;
        for(const auto& box:obstacles)
        {
            bool overlap=glm::abs(robotPosition.x-box.position.x)<box.size.x*.5f+ROBOT_RADIUS
                && glm::abs(robotPosition.z-box.position.z)<box.size.z*.5f+ROBOT_RADIUS;
            if(overlap && previousY>=box.size.y-.0001f)
                support=glm::max(support,box.size.y);
        }
        bool descending=!jumping || jumpTime>=JUMP_SECONDS*.5f;
        if(descending && nextY<=support)
        {
            nextY=support;
            jumping=false;
        }
        if(jumping && jumpTime>=JUMP_SECONDS)
            jumping=false;
        robotPosition.y=nextY;
    }
}

void DrawBox(const glm::mat4& model,glm::vec3 size,glm::vec3 color)
{
    glm::mat4 transform=glm::scale(model,size);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(transform));
    glUniform3fv(colorLocation,1,glm::value_ptr(color));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES,0,36);
}

void DrawScene(int width,int height)
{
    if(width<=0 || height<=0)return;
    glViewport(0,0,width,height);
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    glUniform1f(glGetUniformLocation(programID,"coverCutoff"),-1000.0f);
    glm::mat4 view=glm::lookAt(cameraPosition,cameraTarget,glm::vec3(0,1,0));
    glm::mat4 projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),(float)width/height,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
    glm::mat4 room=glm::scale(glm::translate(glm::mat4(1),roomCenter),glm::vec3(ROOM_WIDTH,ROOM_HEIGHT,ROOM_DEPTH));
    glm::vec3 wallColors[]={{.65f,.25f,.25f},{.28f,.40f,.65f},{.25f,.55f,.30f},
        {.60f,.45f,.25f},{.55f,.45f,.65f},{.45f,.60f,.40f}};
    // 바깥쪽 면을 컬링하면 가까운 벽은 사라지고 내부를 향한 반대쪽 벽이 보인다.
    glCullFace(GL_FRONT);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(room));
    glBindVertexArray(vao);
    for(int i=0;i<6;++i)
    {
        glUniform3fv(colorLocation,1,glm::value_ptr(wallColors[i]));
        glDrawArrays(GL_TRIANGLES,i*6,6);
    }
    glCullFace(GL_BACK);
    for(const auto& box:obstacles)
        DrawBox(glm::translate(glm::mat4(1),box.position),box.size,box.color);
    glm::mat4 robot=glm::translate(glm::mat4(1),robotPosition);
    float heading=glm::atan(-walkDirection.x,-walkDirection.z);
    robot=glm::rotate(robot,heading,glm::vec3(0,1,0));
    DrawBox(glm::translate(robot,{0,LEG_LENGTH+BODY_HEIGHT*.5f,0}),
        {BODY_WIDTH,BODY_HEIGHT,BODY_DEPTH},{.15f,.65f,.80f});
    float headY=LEG_LENGTH+BODY_HEIGHT+HEAD_SIZE*.5f;
    DrawBox(glm::translate(robot,{0,headY,0}),glm::vec3(HEAD_SIZE),{.85f,.70f,.50f});
    DrawBox(glm::translate(robot,{0,headY,-(HEAD_SIZE+NOSE_SIZE)*.5f}),glm::vec3(NOSE_SIZE),{.90f,.45f,.20f});
    float rate=(walkSpeed-WALK_SPEED_MIN)/(WALK_SPEED_MAX-WALK_SPEED_MIN);
    float swing=walking ? glm::sin(walkPhase)*glm::radians(glm::mix(WALK_ANGLE_MIN,WALK_ANGLE_MAX,rate)) : 0;
    for(int i=0;i<2;++i)
    {
        float side=i==0 ? -1.0f : 1.0f;
        glm::mat4 leg=glm::translate(robot,{side*(LIMB_WIDTH+LIMB_GAP)*.5f,LEG_LENGTH,0});
        leg=glm::rotate(leg,side*swing,glm::vec3(1,0,0));
        DrawBox(glm::translate(leg,{0,-LEG_LENGTH*.5f,0}),{LIMB_WIDTH,LEG_LENGTH,LIMB_WIDTH},
            i==0 ? glm::vec3(.30f,.25f,.80f) : glm::vec3(.65f,.20f,.70f));
        glm::mat4 arm=glm::translate(robot,{side*(BODY_WIDTH*.5f+LIMB_GAP+LIMB_WIDTH*.5f),LEG_LENGTH+BODY_HEIGHT,0});
        arm=glm::rotate(arm,-side*swing,glm::vec3(1,0,0));
        DrawBox(glm::translate(arm,{0,-ARM_LENGTH*.5f,0}),{LIMB_WIDTH,ARM_LENGTH,LIMB_WIDTH},
            i==0 ? glm::vec3(.85f,.25f,.20f) : glm::vec3(.20f,.40f,.85f));
    }
    // 육면체의 외부 면만 아래에서 위로 걷어 올려 내부를 드러낸다.
    // 닫혔을 때는 원래 육면체 모양 그대로 모든 방향에서 내부를 가린다.
    if(curtainOpen<1.0f)
    {
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(room));
        glUniform1f(glGetUniformLocation(programID,"coverCutoff"),ROOM_HEIGHT*curtainOpen);
        for(int i=0;i<6;++i)
        {
            glUniform3fv(colorLocation,1,glm::value_ptr(wallColors[i]));
            glDrawArrays(GL_TRIANGLES,i*6,6);
        }
        glUniform1f(glGetUniformLocation(programID,"coverCutoff"),-1000.0f);
    }
    glBindVertexArray(0);
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
    if(key==GLFW_KEY_W){walkDirection={0,0,-1};walking=true;}
    if(key==GLFW_KEY_S){walkDirection={0,0,1};walking=true;}
    if(key==GLFW_KEY_A){walkDirection={-1,0,0};walking=true;}
    if(key==GLFW_KEY_D){walkDirection={1,0,0};walking=true;}
    if(key==GLFW_KEY_EQUAL || key==GLFW_KEY_KP_ADD)
        walkSpeed=glm::min(walkSpeed+WALK_SPEED_STEP,WALK_SPEED_MAX);
    if(key==GLFW_KEY_MINUS || key==GLFW_KEY_KP_SUBTRACT)
        walkSpeed=glm::max(walkSpeed-WALK_SPEED_STEP,WALK_SPEED_MIN);
    if(key==KEY_OPEN)curtainTarget=!curtainTarget;
    if(key==KEY_JUMP && !jumping)
    {
        // 바닥이나 장애물 윗면에 서 있을 때만 새 점프를 허용한다.
        float support=0;
        for(const auto& box:obstacles)
            if(glm::abs(robotPosition.x-box.position.x)<box.size.x*.5f+ROBOT_RADIUS
                && glm::abs(robotPosition.z-box.position.z)<box.size.z*.5f+ROBOT_RADIUS
                && robotPosition.y>=box.size.y-.0001f)
                support=glm::max(support,box.size.y);
        if(glm::abs(robotPosition.y-support)<.001f)
        {
            jumping=true;
            jumpTime=0;
            jumpStart=robotPosition.y;
        }
    }
    if(key==GLFW_KEY_Y)
    {
        int direction=(mods&GLFW_MOD_SHIFT) ? 1 : -1;
        cameraOrbit=cameraOrbit==direction ? 0 : direction;
    }
    if(key==KEY_RESET)ResetScene();
}

void MouseCallback(GLFWwindow* window,int button,int action,int)
{
    if(button!=GLFW_MOUSE_BUTTON_LEFT)return;
    dragging=ORBIT_ENABLED && action==GLFW_PRESS;
    if(dragging)glfwGetCursorPos(window,&previousMouseX,&previousMouseY);
}

void CursorCallback(GLFWwindow* window,double x,double y)
{
    if(!dragging)return;
    int width,height;
    glfwGetWindowSize(window,&width,&height);
    if(width<=0 || height<=0)return;
    glm::vec3 offset=cameraPosition-cameraTarget;
    float radius=glm::length(offset);
    if(radius<.001f)return;
    float yaw=glm::atan(offset.x,offset.z);
    float pitch=glm::asin(glm::clamp(offset.y/radius,-1.0f,1.0f));
    yaw-=glm::radians(ORBIT_DEGREES)*(float)(x-previousMouseX)/width;
    pitch+=glm::radians(ORBIT_DEGREES)*(float)(y-previousMouseY)/height;
    pitch=glm::clamp(pitch,-glm::radians(ORBIT_PITCH_LIMIT),glm::radians(ORBIT_PITCH_LIMIT));
    cameraPosition=cameraTarget+radius*glm::vec3(glm::cos(pitch)*glm::sin(yaw),glm::sin(pitch),glm::cos(pitch)*glm::cos(yaw));
    previousMouseX=x;
    previousMouseY=y;
}

void ScrollCallback(GLFWwindow*,double,double yoffset)
{
    glm::vec3 offset=cameraPosition-cameraTarget;
    float length=glm::length(offset);
    if(length<.001f)return;
    float radius=glm::clamp(length-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
    cameraPosition=cameraTarget+offset*(radius/length);
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
