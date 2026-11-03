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

// ============================================================================
// 1. 프로그램 설정: 크기는 전체 길이, 회전각은 도 단위입니다.
// ============================================================================
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define FLOOR_WIDTH 4.0f
#define FLOOR_DEPTH 4.0f
#define FLOOR_HEIGHT .04f
#define SUPPORT_SPACING 2.30f
#define SUPPORT_DEPTH 1.20f
#define SUPPORT_HEIGHT 2.30f
#define SUPPORT_THICKNESS .12f
#define BEAM_THICKNESS .14f
#define ROPE_LENGTH 1.55f
#define ROPE_THICKNESS .025f
#define ROPE_SPACING .62f
#define SEAT_WIDTH .80f
#define SEAT_DEPTH .38f
#define SEAT_THICKNESS .055f
#define PERSON_ENABLED 1 // 0: 사람 숨김, 1: 사람 표시
#define BODY_WIDTH .30f
#define BODY_HEIGHT .44f
#define BODY_DEPTH .18f
#define HEAD_SIZE .24f
#define ARM_WIDTH .09f
#define ARM_LENGTH .38f
#define LEG_WIDTH .11f
#define LEG_LENGTH .43f
#define LIMB_GAP .035f
#define ARM_SPREAD_ANGLE 10.0f
#define LEG_SPREAD_ANGLE 9.0f

// 흔들림 범위는 -진폭~+진폭입니다. 시작 자세와 초기 진폭은 별개입니다.
#define SWING_START_ANGLE 0.0f
#define SWING_INITIAL_AMPLITUDE 25.0f
#define SWING_MIN_AMPLITUDE 5.0f
#define SWING_MAX_AMPLITUDE 45.0f
#define SWING_AMPLITUDE_STEP 5.0f
#define SWING_AMPLITUDE_CHANGE_SPEED 15.0f
// 속도는 사인 함수 위상이 1초에 진행하는 각도입니다. 60이면 한 왕복에 6초입니다.
#define SWING_INITIAL_SPEED 60.0f
#define SWING_MIN_SPEED 15.0f
#define SWING_MAX_SPEED 150.0f
#define SWING_SPEED_STEP 15.0f
#define KEY_SWING GLFW_KEY_G
#define KEY_HEIGHT GLFW_KEY_H
#define KEY_BOARD GLFW_KEY_B
#define SUPPORT_SCALE_MIN 1.0f
#define SUPPORT_SCALE_MAX 1.5f
#define SUPPORT_SCALE_SPEED .20f
#define SWING_MOVE_SPEED .70f
#define WALK_SPEED .65f
#define WALK_SPEED_MIN .20f
#define WALK_SPEED_MAX 1.30f
#define WALK_SPEED_STEP .10f
#define WALK_ANGLE 25.0f
#define WALK_PHASE_PER_DISTANCE 10.0f
#define PERSON_RADIUS .27f
#define SIMULATION_FPS 240
#define KEY_ANGLE_DOWN GLFW_KEY_O
#define KEY_ANGLE_UP GLFW_KEY_P
#define KEY_SPEED_UP GLFW_KEY_EQUAL // 일반 키보드 +/= 키
#define KEY_SPEED_DOWN GLFW_KEY_MINUS
#define KEY_RESET GLFW_KEY_C

#define CAMERA_X 3.2f
#define CAMERA_Y 2.6f
#define CAMERA_Z -4.5f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 1.1f
#define CAMERA_TARGET_Z 0.0f
#define CAMERA_MOVE_SPEED 1.0f
#define CAMERA_ROTATE_SPEED 35.0f
#define CAMERA_RADIUS_MIN .30f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP .05f
#define ORBIT_ENABLED 1 // 0: 마우스 오비트 사용 안 함, 1: 사용
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_NEAR .1f
#define PERSPECTIVE_FAR 100.0f

static_assert(SWING_MIN_AMPLITUDE>0 && SWING_INITIAL_AMPLITUDE>=SWING_MIN_AMPLITUDE
    && SWING_INITIAL_AMPLITUDE<=SWING_MAX_AMPLITUDE,"Check swing amplitude settings");
static_assert(SWING_START_ANGLE>=-SWING_INITIAL_AMPLITUDE && SWING_START_ANGLE<=SWING_INITIAL_AMPLITUDE,
    "Start angle must be within initial amplitude");
static_assert(SWING_MIN_SPEED>0 && SWING_INITIAL_SPEED>=SWING_MIN_SPEED
    && SWING_INITIAL_SPEED<=SWING_MAX_SPEED,"Check swing speed settings");

// ============================================================================
// 2. 상태: 그네 전체가 하나의 회전행렬을 공유합니다.
// ============================================================================
float swingPhase=0, swingAngle=SWING_START_ANGLE;
float amplitude=SWING_INITIAL_AMPLITUDE, targetAmplitude=SWING_INITIAL_AMPLITUDE;
float swingSpeed=SWING_INITIAL_SPEED;
bool swinging=false,dragging=false;
bool riding=true,boardingPending=false,walking=false,heightChanging=false;
float supportScale=1,scaleDirection=1,walkSpeed=WALK_SPEED,walkPhase=0;
glm::vec3 swingPosition(0),personPosition(0),walkDirection(0,0,-1);
double simulationTime=0;
struct Part { glm::mat4 model; glm::vec3 size,color; };
int cameraOrbit=0;
glm::vec3 cameraPosition(CAMERA_X,CAMERA_Y,CAMERA_Z);
glm::vec3 cameraTarget(CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z);
const glm::vec3 orbitCenter(CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z);
double previousMouseX=0,previousMouseY=0;
GLuint programID=0,vao=0,vbo=0;
GLint modelLocation,viewLocation,projectionLocation,colorLocation;
string ReadFile(const char* name);
bool InitShader();
void ResetScene();
vector<Part> SwingParts(glm::vec3 position);
glm::mat4 SeatMatrix();
bool PersonBlocked(glm::vec3 position,glm::vec3 framePosition);
void CompleteBoarding();
void UpdateScene(GLFWwindow* window,float dt);
void DrawBox(const glm::mat4& model,glm::vec3 size,glm::vec3 color);
void DrawScene(int width,int height);
void KeyCallback(GLFWwindow*,int,int,int,int);
void MouseCallback(GLFWwindow*,int,int,int);
void CursorCallback(GLFWwindow*,double,double);
void ScrollCallback(GLFWwindow*,double,double);

// ============================================================================
// 3. 초기화와 메인 반복문
// ============================================================================
int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Midterm 1 - Swing", nullptr, nullptr);
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
    cout << "G: swing | H: height | B: board/exit | WASD: walk | arrows: move swing\n"
         << "O/P: amplitude -/+ | +/-: both speeds | C: reset\n"
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

// ResetScene: 그네의 자세·설정·정지 상태와 카메라를 초기화합니다.
void ResetScene()
{
    amplitude=targetAmplitude=SWING_INITIAL_AMPLITUDE;
    swingSpeed=SWING_INITIAL_SPEED;
    swingPhase=glm::asin(SWING_START_ANGLE/SWING_INITIAL_AMPLITUDE);
    swingAngle=SWING_START_ANGLE;
    swinging=dragging=false;
    riding=true;boardingPending=walking=heightChanging=false;
    supportScale=scaleDirection=1;
    walkSpeed=WALK_SPEED;walkPhase=0;simulationTime=0;
    swingPosition=personPosition=glm::vec3(0);walkDirection={0,0,-1};
    cameraOrbit=0;
    cameraPosition={CAMERA_X,CAMERA_Y,CAMERA_Z};
    cameraTarget=orbitCenter;
}

// SeatMatrix: 좌석 윗면 기준 행렬. 기둥 높이와 전체 이동을 함께 반영합니다.
glm::mat4 SeatMatrix()
{
    glm::mat4 frame=glm::translate(glm::mat4(1),swingPosition+glm::vec3(0,SUPPORT_HEIGHT*supportScale,0));
    frame=glm::rotate(frame,glm::radians(swingAngle),glm::vec3(1,0,0));
    return glm::translate(frame,{0,-ROPE_LENGTH,0});
}

// SwingParts: 그리기와 충돌 검사가 같은 기둥/가로봉/줄/좌석 정보를 사용합니다.
vector<Part> SwingParts(glm::vec3 position)
{
    vector<Part> parts;
    glm::mat4 world=glm::translate(glm::mat4(1),position);
    float height=SUPPORT_HEIGHT*supportScale;
    glm::vec3 wood(.55f,.30f,.16f);
    for(float side:{-1.0f,1.0f})
    {
        for(float front:{-1.0f,1.0f})
            parts.push_back({glm::translate(world,{side*SUPPORT_SPACING*.5f,height*.5f,front*SUPPORT_DEPTH*.5f}),
                {SUPPORT_THICKNESS,height,SUPPORT_THICKNESS},wood});
        parts.push_back({glm::translate(world,{side*SUPPORT_SPACING*.5f,height,0}),
            {SUPPORT_THICKNESS,SUPPORT_THICKNESS,SUPPORT_DEPTH+SUPPORT_THICKNESS},wood});
    }
    parts.push_back({glm::translate(world,{0,height,0}),
        {SUPPORT_SPACING-SUPPORT_THICKNESS,BEAM_THICKNESS,BEAM_THICKNESS},{.65f,.38f,.18f}});
    glm::mat4 swing=glm::rotate(glm::translate(world,{0,height,0}),glm::radians(swingAngle),glm::vec3(1,0,0));
    for(float side:{-1.0f,1.0f})
        parts.push_back({glm::translate(swing,{side*ROPE_SPACING*.5f,-ROPE_LENGTH*.5f,0}),
            {ROPE_THICKNESS,ROPE_LENGTH,ROPE_THICKNESS},{.25f,.25f,.28f}});
    parts.push_back({glm::translate(swing,{0,-ROPE_LENGTH-SEAT_THICKNESS*.5f,0}),
        {SEAT_WIDTH,SEAT_THICKNESS,SEAT_DEPTH},{.80f,.55f,.20f}});
    return parts;
}

// PersonBlocked: 서 있는 사람의 범위와 각 부품의 변환된 AABB를 비교합니다.
bool PersonBlocked(glm::vec3 position,glm::vec3 framePosition)
{
    if(glm::abs(position.x)>FLOOR_WIDTH*.5f-PERSON_RADIUS
        || glm::abs(position.z)>FLOOR_DEPTH*.5f-PERSON_RADIUS)return true;
    float personHeight=LEG_LENGTH+BODY_HEIGHT+HEAD_SIZE;
    for(const auto& part:SwingParts(framePosition))
    {
        glm::mat4 model=glm::scale(part.model,part.size);
        glm::vec3 low(1e10f),high(-1e10f);
        for(float x:{-.5f,.5f})for(float y:{-.5f,.5f})for(float z:{-.5f,.5f})
        {
            glm::vec3 point(model*glm::vec4(x,y,z,1));
            low=glm::min(low,point);high=glm::max(high,point);
        }
        if(low.y<personHeight && high.y>0
            && position.x+PERSON_RADIUS>low.x && position.x-PERSON_RADIUS<high.x
            && position.z+PERSON_RADIUS>low.z && position.z-PERSON_RADIUS<high.z)return true;
    }
    return false;
}

// CompleteBoarding: 중앙에서 순간이동으로 자세를 전환합니다. 하차 지점은 충돌 없는 바닥입니다.
void CompleteBoarding()
{
    if(!riding)
        riding=true;
    else
    {
        glm::vec3 candidate=swingPosition+glm::vec3(0,0,-SEAT_DEPTH*.5f-PERSON_RADIUS-.15f);
        bool found=!PersonBlocked(candidate,swingPosition);
        // 이동한 그네 앞에 공간이 부족하면 가까운 빈 바닥 위치를 찾습니다.
        float best=1e10f;
        if(!found)
            for(float x=-FLOOR_WIDTH*.5f+PERSON_RADIUS;x<=FLOOR_WIDTH*.5f-PERSON_RADIUS;x+=.1f)
                for(float z=-FLOOR_DEPTH*.5f+PERSON_RADIUS;z<=FLOOR_DEPTH*.5f-PERSON_RADIUS;z+=.1f)
                {
                    glm::vec3 point(x,0,z);
                    float distance=glm::length(point-swingPosition);
                    if(distance<best && !PersonBlocked(point,swingPosition))
                    {
                        candidate=point;best=distance;found=true;
                    }
                }
        if(!found)
        {
            cerr << "No free floor position for dismount." << endl;
            boardingPending=false;
            return;
        }
        personPosition=candidate;riding=false;
    }
    boardingPending=false;walking=false;walkPhase=0;walkDirection={0,0,-1};
}

// ============================================================================
// 4. 애니메이션과 카메라 이동
// ============================================================================
void UpdateScene(GLFWwindow* window,float dt)
{
    simulationTime+=dt;
    const float step=1.0f/SIMULATION_FPS;
    while(simulationTime>=step)
    {
        simulationTime-=step;
        if(heightChanging)
        {
            supportScale+=scaleDirection*SUPPORT_SCALE_SPEED*step;
            if(supportScale>=SUPPORT_SCALE_MAX){supportScale=SUPPORT_SCALE_MAX;scaleDirection=-1;}
            if(supportScale<=SUPPORT_SCALE_MIN){supportScale=SUPPORT_SCALE_MIN;scaleDirection=1;}
        }
        if(swinging)
        {
            amplitude+=glm::clamp(targetAmplitude-amplitude,
                -SWING_AMPLITUDE_CHANGE_SPEED*step,SWING_AMPLITUDE_CHANGE_SPEED*step);
            float nextPhase=swingPhase+glm::radians(swingSpeed)*step;
            bool crossed=glm::floor(nextPhase/glm::pi<float>())>glm::floor(swingPhase/glm::pi<float>());
            if(boardingPending && crossed)
            {
                swingPhase=glm::mod(glm::floor(nextPhase/glm::pi<float>())*glm::pi<float>(),glm::two_pi<float>());
                swingAngle=0;swinging=false;
                CompleteBoarding();
            }
            else
            {
                swingPhase=glm::mod(nextPhase,glm::two_pi<float>());
                swingAngle=amplitude*glm::sin(swingPhase);
            }
        }
        glm::vec3 movement(0);
        if(glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS)movement.x-=1;
        if(glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)movement.x+=1;
        if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)movement.z-=1;
        if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)movement.z+=1;
        if(glm::length(movement)>0)
        {
            glm::vec3 candidate=swingPosition+glm::normalize(movement)*SWING_MOVE_SPEED*step;
            // 최대 흔들림과 탑승자의 앞으로 뻗은 다리까지 고려해 경계를 제한합니다.
            float extentX=(SUPPORT_SPACING+SUPPORT_THICKNESS)*.5f;
            float extentZ=glm::max((SUPPORT_DEPTH+SUPPORT_THICKNESS)*.5f,
                ROPE_LENGTH*glm::sin(glm::radians(SWING_MAX_AMPLITUDE))+LEG_LENGTH+BODY_DEPTH);
            candidate.x=glm::clamp(candidate.x,-glm::max(0.0f,FLOOR_WIDTH*.5f-extentX),glm::max(0.0f,FLOOR_WIDTH*.5f-extentX));
            candidate.z=glm::clamp(candidate.z,-glm::max(0.0f,FLOOR_DEPTH*.5f-extentZ),glm::max(0.0f,FLOOR_DEPTH*.5f-extentZ));
            if(!PERSON_ENABLED || riding || !PersonBlocked(personPosition,candidate))
                swingPosition=candidate;
        }
        if(PERSON_ENABLED && !riding && walking)
        {
            walkPhase=glm::mod(walkPhase+walkSpeed*WALK_PHASE_PER_DISTANCE*step,glm::two_pi<float>());
            glm::vec3 candidate=personPosition+walkDirection*walkSpeed*step;
            if(PersonBlocked(candidate,swingPosition))
                walkDirection=-walkDirection;
            else
                personPosition=candidate;
        }
    }
    bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS
        || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    float sign=shift ? -1.0f : 1.0f;
    glm::vec3 movement(0);
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
        glm::vec3 center=orbitCenter+swingPosition;
        cameraPosition=center+glm::vec3(rotation*glm::vec4(cameraPosition-center,0));
        cameraTarget=center+glm::vec3(rotation*glm::vec4(cameraTarget-center,0));
    }
}

// ============================================================================
// 5. 그리기: 기둥 → 그네 회전축 → 좌석 → 사람 순서로 행렬을 연결합니다.
// ============================================================================
// DrawBox: 중심을 기준으로 하는 단위 육면체에 크기·행렬·색상을 적용합니다.
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
    glm::mat4 view=glm::lookAt(cameraPosition,cameraTarget,glm::vec3(0,1,0));
    glm::mat4 projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),(float)width/height,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
    glm::mat4 world(1);
    DrawBox(glm::translate(world,{0,-FLOOR_HEIGHT*.5f,0}),
        {FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH},{.82f,.88f,.94f});
    for(const auto& part:SwingParts(swingPosition))
        DrawBox(part.model,part.size,part.color);
    glm::mat4 seat=SeatMatrix();

    if(PERSON_ENABLED)
    {
        // seat 원점이 의자 윗면입니다. 몸통의 하부를 이 위치에 고정합니다.
        glm::mat4 person=seat;
        if(!riding)
        {
            person=glm::translate(glm::mat4(1),personPosition+glm::vec3(0,LEG_LENGTH,0));
            person=glm::rotate(person,glm::atan(-walkDirection.x,-walkDirection.z),glm::vec3(0,1,0));
        }
        float walkAngle=!riding && walking ? glm::radians(WALK_ANGLE)*glm::sin(walkPhase) : 0;
        DrawBox(glm::translate(person,{0,BODY_HEIGHT*.5f,0}),
            {BODY_WIDTH,BODY_HEIGHT,BODY_DEPTH},{.10f,.65f,.72f});
        DrawBox(glm::translate(person,{0,BODY_HEIGHT+HEAD_SIZE*.5f,0}),
            glm::vec3(HEAD_SIZE),{.78f,.58f,.40f});
        // 앞은 -Z입니다. 작은 눈 두 개도 얇은 육면체로 만듭니다.
        for(float side:{-1.0f,1.0f})
        {
            DrawBox(glm::translate(person,{side*HEAD_SIZE*.22f,BODY_HEIGHT+HEAD_SIZE*.55f,-HEAD_SIZE*.5f-.005f}),
                {HEAD_SIZE*.16f,HEAD_SIZE*.13f,.012f},{.15f,.15f,.20f});
            if(riding)
            {
                glm::mat4 leg=glm::translate(person,{side*(LEG_WIDTH+LIMB_GAP)*.5f,LEG_WIDTH*.5f,-BODY_DEPTH*.25f});
                leg=glm::rotate(leg,glm::radians(-side*LEG_SPREAD_ANGLE),glm::vec3(0,1,0));
                DrawBox(glm::translate(leg,{0,0,-LEG_LENGTH*.5f}),
                    {LEG_WIDTH,LEG_WIDTH,LEG_LENGTH},{.25f,.25f,.65f});
            }
            else
            {
                glm::mat4 leg=glm::translate(person,{side*(LEG_WIDTH+LIMB_GAP)*.5f,0,0});
                leg=glm::rotate(leg,side*walkAngle,glm::vec3(1,0,0));
                DrawBox(glm::translate(leg,{0,-LEG_LENGTH*.5f,0}),
                    {LEG_WIDTH,LEG_LENGTH,LEG_WIDTH},{.25f,.25f,.65f});
            }
            // 팔은 어깨 옆에서 아래로 내려오며 Z축으로 벌립니다.
            glm::mat4 arm=glm::translate(person,{side*(BODY_WIDTH*.5f+ARM_WIDTH*.5f+LIMB_GAP),BODY_HEIGHT,0});
            arm=glm::rotate(arm,glm::radians(side*ARM_SPREAD_ANGLE),glm::vec3(0,0,1));
            arm=glm::rotate(arm,-side*walkAngle,glm::vec3(1,0,0));
            DrawBox(glm::translate(arm,{0,-ARM_LENGTH*.5f,0}),
                {ARM_WIDTH,ARM_LENGTH,ARM_WIDTH},{.78f,.58f,.40f});
        }
    }
    glBindVertexArray(0);
}

// ============================================================================
// 6. 키보드와 마우스
// ============================================================================
void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
    if(key==KEY_SWING && !boardingPending)swinging=!swinging;
    if(key==KEY_HEIGHT)heightChanging=!heightChanging;
    if(key==KEY_BOARD && PERSON_ENABLED && !boardingPending)
    {
        boardingPending=true;
        if(glm::abs(swingAngle)<.001f)
        {
            swingAngle=0;swinging=false;
            CompleteBoarding();
        }
        else
            swinging=true; // 중간 각도에서 일시정지했어도 중앙까지 이동합니다.
    }
    if(!riding && PERSON_ENABLED)
    {
        if(key==GLFW_KEY_W){walkDirection={0,0,-1};walking=true;}
        if(key==GLFW_KEY_S){walkDirection={0,0,1};walking=true;}
        if(key==GLFW_KEY_A){walkDirection={-1,0,0};walking=true;}
        if(key==GLFW_KEY_D){walkDirection={1,0,0};walking=true;}
    }
    if(key==KEY_ANGLE_DOWN)targetAmplitude=glm::max(targetAmplitude-SWING_AMPLITUDE_STEP,SWING_MIN_AMPLITUDE);
    if(key==KEY_ANGLE_UP)targetAmplitude=glm::min(targetAmplitude+SWING_AMPLITUDE_STEP,SWING_MAX_AMPLITUDE);
    if(key==KEY_SPEED_UP || key==GLFW_KEY_KP_ADD)
    {
        swingSpeed=glm::min(swingSpeed+SWING_SPEED_STEP,SWING_MAX_SPEED);
        walkSpeed=glm::min(walkSpeed+WALK_SPEED_STEP,WALK_SPEED_MAX);
    }
    if(key==KEY_SPEED_DOWN || key==GLFW_KEY_KP_SUBTRACT)
    {
        swingSpeed=glm::max(swingSpeed-SWING_SPEED_STEP,SWING_MIN_SPEED);
        walkSpeed=glm::max(walkSpeed-WALK_SPEED_STEP,WALK_SPEED_MIN);
    }
    if(key==GLFW_KEY_Y)
    {
        int direction=(mods&GLFW_MOD_SHIFT) ? 1 : -1;
        cameraOrbit=cameraOrbit==direction ? 0 : direction;
    }
    if(key==KEY_RESET)ResetScene();
    if(key==KEY_ANGLE_DOWN || key==KEY_ANGLE_UP || key==KEY_SPEED_DOWN || key==KEY_SPEED_UP
        || key==GLFW_KEY_KP_ADD || key==GLFW_KEY_KP_SUBTRACT)
        cout << "Amplitude: +/-" << targetAmplitude << " deg | Phase speed: " << swingSpeed << " deg/s | Walk speed: " << walkSpeed << endl;
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
    float radius=glm::clamp(glm::length(offset)-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
    cameraPosition=cameraTarget+glm::normalize(offset)*radius;
}

// ============================================================================
// 7. 셰이더 파일 읽기와 컴파일
// ============================================================================
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
