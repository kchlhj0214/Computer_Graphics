//프로젝트속성->링커->명령줄에다음과같이3개의라이브러리추가
//-> opengl32.lib glew32.lib glfw3.lib glu32.lib

#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <cmath>
#include <stdexcept>
#include <random>
#include <map>
using namespace std;

// ============================================================================
// 1. 프로그램 설정
// ============================================================================
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define BOX_COUNT 40
// 저울형 크레인: 조절값은 여기에서 변경합니다.
#define CRANE_INITIAL_TILT 10.0f
#define CRANE_TILT_PER_STEP 3.0f
#define CRANE_SLIDE_STEPS 5
#define CRANE_SLIDE_STEP .10f
#define CRANE_SLIDE_SPEED .45f
#define CRANE_TILT_SPEED 30.0f
#define CRANE_ARM_AMPLITUDE 60.0f
#define CRANE_ARM_PHASE_SPEED 75.0f
#define CRANE_ARM_RETURN_SPEED 120.0f
#define CRANE_YAW_SPEED 50.0f
#define CRANE_SPREAD_ANGLE 22.0f
#define CRANE_SPREAD_SPEED 55.0f
#define PLAYER_BODY 31
#define PLAYER_SPEED .65f
#define PLAYER_WALK_ANGLE 25.0f
#define PLAYER_PHASE_SPEED 10.0f
#define PLAYER_JUMP_HEIGHT .85f
#define PLAYER_JUMP_SECONDS 1.0f
#define PLAYER_DESCEND_SPEED 1.5f
#define RANDOM_COLOR_MIN .10f
#define RANDOM_COLOR_MAX .90f
#define FLOOR_ENABLED 1 // 0: 바닥 숨김, 1: 표시. 방향키 경계는 표시와 무관하게 적용합니다.
#define FLOOR_WIDTH 4.0f
#define FLOOR_DEPTH 4.0f
#define FLOOR_HEIGHT .04f
#define MOVE_SPEED .75f // 초당 이동 거리
#define ROTATE_SPEED 90.0f // 초당 회전 각도
#define SCALE_SPEED .50f // 초당 배율 변화량: +0.5 신축은 1초
#define GROUP_MOVE_SPEED 1.0f
#define ANIMATION_FPS 240
#define CAMERA_X 2.4f
#define CAMERA_Y 2.7f
#define CAMERA_Z 5.0f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 1.0f
#define CAMERA_TARGET_Z 0.0f
#define CAMERA_MOVE_SPEED 1.0f
#define CAMERA_ROTATE_SPEED 35.0f
#define CAMERA_RADIUS_MIN .30f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP .05f
#define ORBIT_ENABLED 1
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_NEAR .1f
#define PERSPECTIVE_FAR 100.0f
#define WIREFRAME_INITIAL 0 // 0: 솔리드, 1: 와이어 (객체에 적용)
#define PERSPECTIVE_INITIAL 1 // 0: 직교, 1: 원근
#define HIDDEN_SURFACE_INITIAL 1 // 0: 은면 제거 끔, 1: 깊이 검사와 뒷면 컬링 켬
// 배경색을 바꾸려면 아래 RGB 값을 0~1 범위에서 수정합니다.
#define BACKGROUND_RED .55f
#define BACKGROUND_GREEN .74f
#define BACKGROUND_BLUE .92f

static_assert(MOVE_SPEED>0 && ROTATE_SPEED>0 && SCALE_SPEED>0 && ANIMATION_FPS>0,
    "Animation speeds must be positive");

// ============================================================================
// 2. 공통 자료형
// ============================================================================
enum class Axis { X, Y, Z }; // 변환 축 / 조립 공간의 고정 X, Y, Z축
struct Pivot
{
    bool point=false;
    glm::vec3 value{0};
    static const Pivot Center,Left,Right,Bottom,Top,Front,Back;
};
const Pivot Pivot::Center{false,{0,0,0}};
const Pivot Pivot::Left{false,{-.5f,0,0}};
const Pivot Pivot::Right{false,{.5f,0,0}};
const Pivot Pivot::Bottom{false,{0,-.5f,0}};
const Pivot Pivot::Top{false,{0,.5f,0}};
const Pivot Pivot::Front{false,{0,0,-.5f}};
const Pivot Pivot::Back{false,{0,0,.5f}};
// Point(x,y,z): 방향키 전체 이동을 따라가는 조립 공간의 회전/신축 기준점.
Pivot Point(float x,float y,float z) { return {true,{x,y,z}}; }
// Color::Random(): 객체별 랜덤 색상. Random(그룹번호): 같은 번호끼리 색상 공유.
// 고정 색상은 기존 {R,G,B} 또는 Color::Fixed(R,G,B)를 그대로 사용합니다.
struct Color
{
    glm::vec3 value;
    bool random=false;
    int group=-1;
    Color(float r=.5f,float g=.5f,float b=.5f) : value(r,g,b) {}
    static Color Fixed(float r,float g,float b) { return Color(r,g,b); }
    static Color Random(int group=-1)
    {
        Color color;
        color.random=true;
        color.group=group;
        return color;
    }
};
struct BoxSetting
{
    bool enabled=false;
    glm::vec3 size{.3f},center{0};
    Color color;
    glm::vec3 angles{0};
    int parent=-1; // -1: 기존 독립 객체, 0~39: 부모 객체 번호
    bool inheritScale=false; // false: 자기 크기 유지, true: 부모 신축도 상속
};
struct BoxState
{
    bool enabled=false,visible=false;
    glm::mat4 model{1},world{1},animation{1}; // model: 부모 기준 변환, world: 상속을 합성한 조립 공간 변환
    glm::vec3 color{.5f},baseSize{1};
    int parent=-1;
    bool inheritScale=false;
};

// ============================================================================
// 수정 영역 ①: 번호 0~39의 활성화, 전체 크기, 중심 좌표, RGB 색상, 초기 회전각
// 마지막에 부모 번호와 신축 상속을 선택적으로 추가합니다. 생략하면 기존과 완전히 같습니다.
// {true,{크기},{위치},색상,{각도},부모번호,신축상속}
// 부모가 있으면 위치·Move·회전축·Point 좌표는 부모 중심 기준이며, 단위는 기존과 같은 길이입니다.
// 부모 신축 시 위치는 함께 이동합니다. false이면 자식 크기는 유지하고 true이면 크기도 상속합니다.
// 예: {true,{.3f,.5f,.2f},{0,.7f,0},Color::Random(),{0,0,0},-1,false}, // 몸통 0번
// 예: {true,{.1f,.4f,.1f},{-.23f,0,0},Color::Random(),{0,0,0},0,false}, // 왼팔 1번
// 이 예에서는 팔 윗면이 몸통 중심보다 .2 위에 있습니다. 상대 좌표로 부착 위치를 조정합니다.
// 부모의 이동·회전은 모든 후손에게 전달됩니다. 자식 변환은 부모에 영향을 주지 않습니다.
// 부모가 비활성화되면 자식도 숨깁니다. 자기 자신/순환 연결/범위 밖 부모 번호는 허용하지 않습니다.
// true인 객체만 표시됩니다. 크기/좌표/색상은 아래 한 곳에서만 설정합니다.
// 초기 회전각은 {X각,Y각,Z각}, 기본 {0,0,0}입니다. 크기는 양수로 지정합니다.
// 색상 인자 예: { .9f,.2f,.2f } / Color::Random() / Color::Random(1)
// Random(1)을 사용한 객체는 모두 같은 랜덤 색입니다. 다른 그룹은 Random(2) 등으로 지정합니다.
// r 리셋 시 랜덤 색은 다시 추첨하며 같은 그룹끼리는 계속 같은 색을 공유합니다.
// ============================================================================
// 0 받침, 1 중앙 기둥, 2 위 몸체, 3/4 팔, 5/6 손. 나머지는 비활성 예비 칸입니다.
// 부모 중심 기준 위치를 사용하며 팔과 손은 PoseCrane에서 연결점을 계산합니다.
const array<BoxSetting,BOX_COUNT> boxSettings={{
    {true,{.95f,.16f,.65f},{0,.08f,0},{.25f,.25f,.50f},{0,0,0}}, // 받침
    {true,{.16f,1.75f,.18f},{0,.955f,0},{.15f,.36f,.58f},{0,0,0},0,false}, // 기둥
    {true,{2.75f,.12f,.20f},{0,.935f,0},{.65f,.30f,.08f},{0,0,0},1,false}, // 위 몸체
    {true,{.055f,.95f,.055f},{-1.20f,-.535f,0},{.75f,.18f,.25f},{0,0,0},2,false}, // 팔1
    {true,{.055f,.95f,.055f},{1.20f,-.535f,0},{.75f,.65f,.10f},{0,0,0},2,false}, // 팔2
    {true,{.45f,.055f,.32f},{0,-.5025f,0},{.72f,.72f,.72f},{0,0,0},3,false}, // 손1
    {true,{.45f,.055f,.32f},{0,-.5025f,0},{.72f,.72f,.72f},{0,0,0},4,false}, // 손2
}};

// ============================================================================
// 3. 명령 생성: 이 함수들은 애니메이션을 즉시 실행하지 않고 명령을 반환합니다.
// Run()이 실행을 등록하며, 매 프레임 UpdateScene()에서 진행합니다.
// ============================================================================
enum class Kind { Move, Rotate, Scale, Swap, Parallel, Sequence, Repeat, PingPong }; // 이동, 회전, 신축, 교환, 동시, 순차, 반복, 왕복
constexpr int Forever=-1;
enum class SwapMode { Direct, ArcXZ, DetourXZ, DetourXY };
struct Command
{
    Kind kind=Kind::Move;
    int id=0,count=1,index=0,cycles=0;
    glm::vec3 delta{0},pivotPosition{0};
    Axis axis=Axis::Y;
    Pivot pivot=Pivot::Center;
    float amount=0,elapsed=0,duration=0,previous=0,seconds=0;
    bool started=false,done=false,backward=false;
    SwapMode swapMode=SwapMode::Direct;
    glm::vec3 swapStart[2]{glm::vec3(0),glm::vec3(0)};
    vector<Command> children;
};
struct Job { int id; Command command; bool paused=false; };
array<BoxState,BOX_COUNT> boxes;
bool playerWalking=false,playerJumping=false;
glm::vec3 playerOffset(0),playerDirection(0,0,-1);
float playerPhase=0,playerJumpTime=0,playerJumpStart=0,playerInitialFoot=0;
vector<Job> jobs;
int nextJob=1;
array<int,9> slots{}; // 예비 키의 작업 번호 저장용. slots[0]~slots[8]
glm::vec3 assemblyPosition(0);
glm::vec3 cameraPosition(CAMERA_X,CAMERA_Y,CAMERA_Z),cameraTarget(CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z);
bool dragging=false;
bool wireframe=WIREFRAME_INITIAL,perspective=PERSPECTIVE_INITIAL,hiddenSurface=HIDDEN_SURFACE_INITIAL;
int cameraOrbit=0;
double previousMouseX=0,previousMouseY=0,accumulator=0;
GLuint programID=0,vao=0,vbo=0;
GLint modelLocation,viewLocation,projectionLocation,colorLocation;

// 크레인 상태: 선택 0=왼쪽(팔1), 1=오른쪽(팔2).
int selectedArm=0,slideStep[2]={0,0};
float craneYaw=0,beamTilt=CRANE_INITIAL_TILT,spreadAngle=0;
float armPhase[2]={0,0},armAngle[2]={0,0},armSlide[2]={0,0};
bool armRunning[2]={false,false};
void PoseCrane();

// EvaluateHierarchy: 부모를 먼저 계산합니다. 부모의 초기 크기는 좌표 단위에 곱하지 않습니다.
void EvaluateHierarchy()
{
    array<int,BOX_COUNT> state{};
    auto evaluate=[&](auto&& self,int id)->void
    {
        if(state[id]==2)return;
        if(state[id]==1)throw runtime_error("Parent hierarchy contains a cycle");
        state[id]=1;
        auto& box=boxes[id];
        box.visible=box.enabled;
        glm::mat4 local=box.animation*box.model;
        box.world=local;
        if(box.parent!=-1)
        {
            if(box.parent<0 || box.parent>=BOX_COUNT)throw runtime_error("Parent ID must be -1 or 0~39");
            self(self,box.parent);
            const auto& parent=boxes[box.parent];
            box.visible=box.visible && parent.visible;
            glm::mat4 frame=parent.world*glm::scale(glm::mat4(1),glm::vec3(1)/parent.baseSize);
            if(box.inheritScale)
                box.world=frame*local;
            else
            {
                // 크기는 제거하고 회전만 추출합니다. 연결 위치는 신축된 부모를 따라갑니다.
                glm::vec3 x=glm::normalize(glm::vec3(frame[0]));
                glm::vec3 y=glm::vec3(frame[1])-x*glm::dot(x,glm::vec3(frame[1]));
                y=glm::normalize(y);
                glm::vec3 z=glm::normalize(glm::cross(x,y));
                glm::mat4 rotation(1);
                rotation[0]=glm::vec4(x,0);rotation[1]=glm::vec4(y,0);rotation[2]=glm::vec4(z,0);
                box.world=rotation*local;
                box.world[3]=frame*glm::vec4(glm::vec3(local[3]),1);
            }
        }
        state[id]=2;
    };
    for(int i=0;i<BOX_COUNT;++i)evaluate(evaluate,i);
}

// PoseCrane: 최종 팔 끝을 손의 연결점으로 사용하고 손의 기울기는 역회전으로 상쇄합니다.
// 중앙 기둥의 Y축 회전은 부모 상속으로 위 몸체/팔/손 전체에 전달됩니다.
void PoseCrane()
{
    glm::vec3 pillarCenter(boxes[1].model[3]);
    boxes[1].animation=glm::translate(glm::mat4(1),pillarCenter)
        *glm::rotate(glm::mat4(1),glm::radians(craneYaw),glm::vec3(0,1,0))
        *glm::translate(glm::mat4(1),-pillarCenter);
    glm::vec3 beamCenter(boxes[2].model[3]);
    boxes[2].animation=glm::translate(glm::mat4(1),beamCenter)
        *glm::rotate(glm::mat4(1),glm::radians(beamTilt),glm::vec3(0,0,1))
        *glm::translate(glm::mat4(1),-beamCenter);
    for(int i=0;i<2;++i)
    {
        int arm=3+i,hand=5+i;
        float side=i==0 ? -1.0f : 1.0f;
        float length=boxSettings[arm].size.y;
        float attachmentX=boxSettings[arm].center.x-side*armSlide[i];
        glm::vec3 attachment(attachmentX,-boxSettings[2].size.y*.5f,0);
        glm::mat4 sway=glm::rotate(glm::mat4(1),glm::radians(side*spreadAngle),glm::vec3(0,0,1))
            *glm::rotate(glm::mat4(1),glm::radians(armAngle[i]),glm::vec3(1,0,0));
        // 위 몸체가 기울어져도 정지한 팔은 아래쪽 수직을 유지합니다.
        glm::mat4 relative=glm::rotate(glm::mat4(1),glm::radians(-beamTilt),glm::vec3(0,0,1))*sway;
        glm::mat4 armModel=glm::translate(glm::mat4(1),attachment)*relative
            *glm::translate(glm::mat4(1),glm::vec3(0,-length*.5f,0))
            *glm::scale(glm::mat4(1),boxSettings[arm].size);
        boxes[arm].animation=armModel*glm::inverse(boxes[arm].model);
        // 팔 끝은 상속하되 팔의 앞뒤/바깥쪽 기울기만 취소해 손을 XZ 평면과 평행하게 둡니다.
        glm::mat4 handModel=glm::translate(glm::mat4(1),glm::vec3(0,-length*.5f,0))
            *glm::inverse(sway)
            *glm::translate(glm::mat4(1),glm::vec3(0,-boxSettings[hand].size.y*.5f,0))
            *glm::scale(glm::mat4(1),boxSettings[hand].size);
        boxes[hand].animation=handModel*glm::inverse(boxes[hand].model);
        boxes[hand].color=i==selectedArm ? glm::vec3(.95f,.85f,.20f) : boxSettings[hand].color.value;
    }
    EvaluateHierarchy();
}

// UpdateCrane: 5/6을 누르는 동안 회전하고, 원심력 모양의 벌어짐을 적용합니다.
void UpdateCrane(GLFWwindow* window,float dt)
{
    int yawDirection=0;
    if(glfwGetKey(window,GLFW_KEY_5)==GLFW_PRESS)++yawDirection;
    if(glfwGetKey(window,GLFW_KEY_6)==GLFW_PRESS)--yawDirection;
    craneYaw=glm::mod(craneYaw+yawDirection*CRANE_YAW_SPEED*dt,360.0f);
    float spreadTarget=yawDirection!=0 ? CRANE_SPREAD_ANGLE : 0;
    spreadAngle+=glm::clamp(spreadTarget-spreadAngle,-CRANE_SPREAD_SPEED*dt,CRANE_SPREAD_SPEED*dt);
    float tiltTarget=CRANE_INITIAL_TILT+(slideStep[1]-slideStep[0])*CRANE_TILT_PER_STEP;
    beamTilt+=glm::clamp(tiltTarget-beamTilt,-CRANE_TILT_SPEED*dt,CRANE_TILT_SPEED*dt);
    for(int i=0;i<2;++i)
    {
        float slideTarget=slideStep[i]*CRANE_SLIDE_STEP;
        armSlide[i]+=glm::clamp(slideTarget-armSlide[i],-CRANE_SLIDE_SPEED*dt,CRANE_SLIDE_SPEED*dt);
        if(armRunning[i])
        {
            armPhase[i]=glm::mod(armPhase[i]+glm::radians(CRANE_ARM_PHASE_SPEED)*dt,glm::two_pi<float>());
            armAngle[i]=CRANE_ARM_AMPLITUDE*glm::sin(armPhase[i]);
        }
        else
        {
            armAngle[i]+=glm::clamp(-armAngle[i],-CRANE_ARM_RETURN_SPEED*dt,CRANE_ARM_RETURN_SPEED*dt);
            armPhase[i]=glm::asin(glm::clamp(armAngle[i]/CRANE_ARM_AMPLITUDE,-1.0f,1.0f));
        }
    }
    PoseCrane();
}

// 사람 샘플: 31 몸통 → 32~36 팔다리/머리 → 37~39 코/눈.
// 기본 model을 덮어쓰지 않고 별도 animation 행렬로 걷기·점프를 합성합니다.
// 크기/색상/상대 위치는 위의 31~39 설정에서 수정합니다. 몸통을 끄면 전체가 숨습니다.
bool IsPlayerPart(int id)
{
    for(int depth=0;id>=0 && depth<BOX_COUNT;++depth)
    {
        if(id==PLAYER_BODY)return true;
        id=boxes[id].parent;
    }
    return false;
}

pair<glm::vec3,glm::vec3> BoxBounds(const glm::mat4& model)
{
    glm::vec3 low(1e20f),high(-1e20f);
    for(float x:{-.5f,.5f})for(float y:{-.5f,.5f})for(float z:{-.5f,.5f})
    {
        glm::vec3 p(model*glm::vec4(x,y,z,1));
        low=glm::min(low,p);high=glm::max(high,p);
    }
    return {low,high};
}

void PosePlayer()
{
    glm::vec3 center(boxes[PLAYER_BODY].model[3]);
    glm::mat4 pose=glm::translate(glm::mat4(1),playerOffset+center);
    pose=glm::rotate(pose,glm::atan(-playerDirection.x,-playerDirection.z),glm::vec3(0,1,0));
    boxes[PLAYER_BODY].animation=glm::translate(pose,-center);
    float angle=playerWalking ? glm::radians(PLAYER_WALK_ANGLE)*glm::sin(playerPhase) : 0;
    for(int id=32;id<=35;++id)
    {
        float sign=(id==32 || id==35) ? 1.0f : -1.0f;
        glm::vec3 top(boxes[id].model*glm::vec4(0,.5f,0,1));
        glm::mat4 swing=glm::translate(glm::mat4(1),top);
        swing=glm::rotate(swing,sign*angle,glm::vec3(1,0,0));
        boxes[id].animation=glm::translate(swing,-top);
    }
    EvaluateHierarchy();
}

// PlayerSupport: 발 아래에 있는 장애물 윗면 높이를 구합니다.
float PlayerSupport(glm::vec3 center,float radius,float feet)
{
    float support=0;
    for(int i=0;i<BOX_COUNT;++i)
        if(boxes[i].visible && !IsPlayerPart(i))
        {
            auto [low,high]=BoxBounds(boxes[i].world);
            if(center.x+radius>low.x && center.x-radius<high.x
                && center.z+radius>low.z && center.z-radius<high.z && feet>=high.y-.0001f)
                support=glm::max(support,high.y);
        }
    return support;
}

void UpdatePlayer(float dt)
{
    if(!boxes[PLAYER_BODY].enabled)return;
    PosePlayer();
    glm::vec3 center(boxes[PLAYER_BODY].world[3]);
    float radius=.01f,height=.01f;
    float feet=playerInitialFoot+playerOffset.y;
    for(int id=0;id<BOX_COUNT;++id)
        if(boxes[id].visible && IsPlayerPart(id))
        {
            auto [low,high]=BoxBounds(boxes[id].world);
            radius=glm::max(radius,glm::max(glm::max(center.x-low.x,high.x-center.x),glm::max(center.z-low.z,high.z-center.z)));
            height=glm::max(height,high.y-feet);
        }
    float nextFeet=glm::max(0.0f,feet-PLAYER_DESCEND_SPEED*dt);
    if(playerJumping)
    {
        playerJumpTime=glm::min(playerJumpTime+dt,PLAYER_JUMP_SECONDS);
        float t=playerJumpTime/PLAYER_JUMP_SECONDS;
        nextFeet=playerJumpStart+PLAYER_JUMP_HEIGHT*(t<.5f ? 2*t : 2*(1-t));
    }
    glm::vec3 candidate=center;
    if(playerWalking)candidate+=playerDirection*PLAYER_SPEED*dt;
    glm::vec3 worldCenter=candidate+assemblyPosition;
    bool blocked=glm::abs(worldCenter.x)>FLOOR_WIDTH*.5f-radius || glm::abs(worldCenter.z)>FLOOR_DEPTH*.5f-radius;
    for(int id=0;id<BOX_COUNT;++id)
        if(boxes[id].visible && !IsPlayerPart(id))
        {
            auto [low,high]=BoxBounds(boxes[id].world);
            if(candidate.x+radius>low.x && candidate.x-radius<high.x
                && candidate.z+radius>low.z && candidate.z-radius<high.z
                && glm::max(feet,nextFeet)<high.y-.0001f && glm::min(feet,nextFeet)+height>low.y)
                blocked=true;
        }
    if(blocked && playerWalking)playerDirection=-playerDirection;
    else {playerOffset.x+=candidate.x-center.x;playerOffset.z+=candidate.z-center.z;center=candidate;}
    float support=PlayerSupport(center,radius,feet);
    bool descending=!playerJumping || playerJumpTime>=PLAYER_JUMP_SECONDS*.5f;
    if(descending && nextFeet<=support){nextFeet=support;playerJumping=false;}
    if(playerJumping && playerJumpTime>=PLAYER_JUMP_SECONDS)playerJumping=false;
    playerOffset.y=nextFeet-playerInitialFoot;
    if(playerWalking)playerPhase=glm::mod(playerPhase+PLAYER_SPEED*PLAYER_PHASE_SPEED*dt,glm::two_pi<float>());
    PosePlayer();
}

// Move(번호,{dx,dy,dz},시간): 해당 단계 시작 위치에서 상대 이동합니다. 시간은 초 단위입니다.
Command Move(int id,glm::vec3 delta,float seconds=0)
{
    Command c;c.kind=Kind::Move;c.id=id;c.delta=delta;c.seconds=seconds;return c;
}
// Rotate(번호,축,각도,기준점,시간): -360~+360도. 기준점 생략 시 자기 중심.
// Center 회전은 매 순간 현재 중심을 따라갑니다. 이동이나 공전과 함께 실행해도 제자리 자전합니다.
// 축 방향은 부모가 없으면 조립 공간의 고정 축, 있으면 부모를 따라 회전하는 축입니다.
// Point는 지정 좌표, 면 기준점은 회전 시작 시의 위치를 계속 사용합니다.
// 부모가 있는 경우 아래 설명의 조립 공간은 해당 부모 좌표계를 뜻합니다.
// 1. 번호: boxSettings의 객체 번호(0~BOX_COUNT-1). 활성화된 객체만 화면에서 변환됩니다.
// 2. 축: Axis::X / Axis::Y / Axis::Z. 기준점을 지나며 조립 공간의 해당 축과 평행한 회전축입니다.
// 3. 각도: 도 단위 상대 회전량(-360~360). 최종 절대 각도가 아니라 현재 자세에서 추가로 회전할 양입니다.
//    양수는 오른손 법칙 방향, 음수는 반대 방향입니다. 화면의 시계/반시계 방향과 항상 같지는 않습니다.
// 4. 기준점(생략 가능): Pivot::Center가 기본값이며 현재 객체 중심을 매 순간 따라갑니다.
//    Pivot::Left / Right / Bottom / Top / Front / Back은 객체의 해당 면 중심입니다.
//    면 기준점은 객체의 회전과 크기를 반영해 시작 시 한 번 계산하고, 이후에는 그 위치로 고정됩니다.
//    Point(x,y,z)는 조립 공간의 좌표를 직접 지정합니다. Point(0,0,0)은 조립 공간 원점입니다.
//    방향키 전체 이동은 조립 공간 전체에 적용되므로 Point 기준점도 함께 이동합니다.
// 5. 시간(생략 가능): 편도 실행 시간(초). 양수를 넣으면 회전 속도는 각도/시간으로 정해집니다.
//    생략하거나 0이면 ROTATE_SPEED를 사용합니다. 음수는 허용하지 않습니다.
//    시간을 지정하려면 앞의 기준점도 적어야 합니다. PingPong으로 감싸면 왕복 시간은 두 배입니다.
// 예: Rotate(0,Axis::X,90) -> 현재 중심에서 고정 X축 방향으로 +90도 자전합니다.
// 예: Rotate(0,Axis::Y,-180,Point(0,0,0),2.0f) -> 원점을 지나는 Y축 주위로 2초 동안 -180도 공전합니다.
// 예: Rotate(0,Axis::Z,45,Pivot::Top,1.0f) -> 시작 시 윗면 중심을 지나는 Z축 주위로 1초 동안 +45도 회전합니다.
Command Rotate(int id,Axis axis,float degrees,Pivot pivot=Pivot::Center,float seconds=0)
{
    Command c;c.kind=Kind::Rotate;c.id=id;c.axis=axis;c.amount=degrees;c.pivot=pivot;c.seconds=seconds;return c;
}
// Scale(번호,축,배율증감,기준점,시간): +0.5는 1.5배, -0.5는 0.5배. -1 이하는 거부.
// 부모가 없으면 조립 공간 축, 있으면 부모 좌표계 축으로 신축합니다. 기본 기준점은 자기 중심.
// 부모가 있는 경우 아래 설명의 조립 공간은 해당 부모 좌표계를 뜻합니다.
// 1. 번호: boxSettings의 객체 번호(0~BOX_COUNT-1). 활성화된 객체만 화면에서 변환됩니다.
// 2. 축: Axis::X / Axis::Y / Axis::Z. 선택한 조립 공간 축 방향만 신축하고 나머지 방향은 유지합니다.
//    객체가 기울어져 있어도 로컬 축이 아닌 고정 축을 사용하므로 모양이 비스듬히 변형될 수 있습니다.
// 3. 배율증감: 최종 배율은 1+값입니다. 현재 크기에 대한 상대 배율이며 최종 크기를 직접 넣는 것이 아닙니다.
//    +.5f는 1.5배, +1.0f는 2배, -.5f는 절반입니다. 0은 변화가 없으며 -1 이하는 허용하지 않습니다.
//    Repeat(Scale(...),횟수)는 배율이 누적됩니다. 원래 크기로 돌아오려면 PingPong(Scale(...))을 사용합니다.
// 4. 기준점(생략 가능): Pivot::Center가 기본값입니다. 매 순간 현재 중심을 기준으로 양쪽으로 늘어납니다.
//    Move나 Swap, 공전과 함께 실행해도 신축 자체가 중심을 밀거나 당기지 않고 크기만 변경합니다.
//    Left / Right / Bottom / Top / Front / Back은 시작 시 해당 면 중심을 기준으로 사용합니다.
//    회전하지 않은 객체에 Axis::Y와 Pivot::Bottom을 지정하면 아래쪽을 고정하고 높이를 변경합니다.
//    Point(x,y,z)는 조립 공간의 지정 좌표를 기준으로 신축하므로 객체의 중심 위치도 바뀔 수 있습니다.
// 5. 시간(생략 가능): 편도 실행 시간(초). 생략하거나 0이면 abs(배율증감)/SCALE_SPEED초가 걸립니다.
//    양수 시간을 지정하면 해당 시간에 신축을 완료합니다. 음수는 허용하지 않습니다.
//    시간을 지정하려면 기준점도 적어야 합니다. PingPong은 같은 시간 동안 원래 크기로 돌아옵니다.
// 예: Scale(0,Axis::X,.5f) -> 매 순간 현재 중심을 기준으로 X방향 크기를 현재의 1.5배로 늘립니다.
// 예: Run(Parallel({Move(0,{1,0,0},2.0f),Scale(0,Axis::X,.5f,Pivot::Center,2.0f)}));
//     2초 동안 X방향으로 1만큼 이동하며 X방향 크기를 1.5배로 늘립니다. 신축은 이동 거리를 바꾸지 않습니다.
// 예: Scale(0,Axis::Y,-.5f,Pivot::Bottom,2.0f) -> 아래 면 중심을 기준으로 2초 동안 Y방향 크기를 절반으로 줄입니다.
// 예: Scale(0,Axis::X,1.0f,Point(0,0,0),1.0f) -> 1초 동안 X방향 크기와 원점에서의 X방향 거리를 두 배로 만듭니다.
Command Scale(int id,Axis axis,float change,Pivot pivot=Pivot::Center,float seconds=0)
{
    Command c;c.kind=Kind::Scale;c.id=id;c.axis=axis;c.amount=change;c.pivot=pivot;c.seconds=seconds;return c;
}
// 변환의 마지막 인자는 편도 시간(초)입니다. 생략하거나 0이면 기존 속도로 계산합니다.
// PingPong은 같은 시간 동안 돌아오므로 한 주기는 지정 시간의 두 배입니다.
// Swap(번호1,번호2,시간,방식): 지정한 초 동안 두 중심 위치를 교환합니다. 방식 생략 시 직선입니다.
// 회전과 크기는 유지합니다. 서로 다른 활성 객체와 양수 시간을 지정합니다.
// ArcXZ는 중점 주위 XZ 반원, DetourXZ/DetourXY는 각 평면의 ㄷ자 경로입니다.
// ㄷ자 우회 거리는 두 중심 거리의 절반이며 두 객체는 서로 반대쪽으로 우회합니다.
Command Swap(int first,int second,float seconds,SwapMode mode=SwapMode::Direct)
{
    Command c;c.kind=Kind::Swap;c.seconds=seconds;c.swapMode=mode;
    c.children={Move(first,glm::vec3(0),seconds),Move(second,glm::vec3(0),seconds)};
    return c;
}
// Parallel({명령,...}): 모두 동시에 시작하고 가장 긴 명령이 끝나면 완료합니다.
Command Parallel(initializer_list<Command> commands)
{
    Command c;c.kind=Kind::Parallel;c.children=commands;return c;
}
// Sequence({명령,...}): 앞 명령 완료 후 다음 명령을 시작합니다.
Command Sequence(initializer_list<Command> commands)
{
    Command c;c.kind=Kind::Sequence;c.children=commands;return c;
}
// Repeat(명령,횟수): 매번 현재 상태에서 상대 변환을 반복합니다. Forever는 무한반복.
Command Repeat(Command command,int count)
{
    Command c;c.kind=Kind::Repeat;c.count=count;c.children.push_back(command);return c;
}
// PingPong(단일 변환): 이동/회전/신축 한 개를 실행한 뒤 역변환으로 돌아옵니다.
// 여러 객체의 동시 왕복은 Parallel({PingPong(...),PingPong(...)})로 구성합니다.
Command PingPong(Command command)
{
    Command c;c.kind=Kind::PingPong;c.children.push_back(command);return c;
}

glm::vec3 AxisVector(Axis axis)
{
    return axis==Axis::X ? glm::vec3(1,0,0) : axis==Axis::Y ? glm::vec3(0,1,0) : glm::vec3(0,0,1);
}

// Validate: 잘못된 설정은 화면을 깨뜨리지 않고 콘솔 오류로 알립니다.
void Validate(const Command& c)
{
    if(c.kind==Kind::Swap)
    {
        if(!isfinite(c.seconds) || c.seconds<=0)throw runtime_error("Swap duration must be finite and positive");
        if(c.children[0].id==c.children[1].id)throw runtime_error("Swap requires two different objects");
        for(const auto& child:c.children)
        {
            if(child.id<0 || child.id>=BOX_COUNT)throw runtime_error("Box ID must be 0~39");
            if(!boxes[child.id].enabled)throw runtime_error("Swap requires enabled objects");
        }
        const auto& first=boxes[c.children[0].id];
        const auto& second=boxes[c.children[1].id];
        if(first.parent!=second.parent || first.inheritScale!=second.inheritScale)
            throw runtime_error("Swap requires the same parent and scale inheritance");
    }
    if(c.kind==Kind::Move || c.kind==Kind::Rotate || c.kind==Kind::Scale)
    {
        if(c.id<0 || c.id>=BOX_COUNT)throw runtime_error("Box ID must be 0~39");
        if(!isfinite(c.seconds) || c.seconds<0)throw runtime_error("Duration must be finite and nonnegative");
        if(!isfinite(c.amount) || !isfinite(c.delta.x) || !isfinite(c.delta.y) || !isfinite(c.delta.z)
            || !isfinite(c.pivot.value.x) || !isfinite(c.pivot.value.y) || !isfinite(c.pivot.value.z))
            throw runtime_error("Command values must be finite");
        if(c.kind==Kind::Rotate && glm::abs(c.amount)>360)throw runtime_error("Rotation must be -360~360");
        if(c.kind==Kind::Scale && c.amount<=-1)throw runtime_error("Scale change must be greater than -1");
    }
    if(c.kind==Kind::Repeat && c.count!=Forever && c.count<1)throw runtime_error("Repeat count must be positive or Forever");
    if(c.kind==Kind::PingPong && (c.children[0].kind!=Kind::Move && c.children[0].kind!=Kind::Rotate && c.children[0].kind!=Kind::Scale))
        throw runtime_error("PingPong accepts one Move/Rotate/Scale; combine them with Parallel or Sequence");
    for(const auto& child:c.children)Validate(child);
}

void CollectIds(const Command& c,set<int>& ids)
{
    if(c.kind==Kind::Move || c.kind==Kind::Rotate || c.kind==Kind::Scale)ids.insert(c.id);
    for(const auto& child:c.children)CollectIds(child,ids);
}

// Run(명령): 작업 번호 반환. 이미 실행/일시정지 중인 객체의 중복 작업은 거부합니다.
// 같은 객체를 동시에 이동·회전하려면 하나의 Parallel 안에 넣습니다.
int Run(Command command)
{
    EvaluateHierarchy();
    Validate(command);
    set<int> wanted;
    CollectIds(command,wanted);
    for(const auto& job:jobs)
        if(!job.command.done)
        {
            set<int> occupied;
            CollectIds(job.command,occupied);
            for(int id:wanted)
                if(occupied.count(id))throw runtime_error("Object already has a job. Use Parallel, or finish/cancel the previous job");
        }
    int id=nextJob++;
    jobs.push_back({id,command,false});
    return id;
}
// Pause/Resume: 작업 전체를 현재 상태에서 정지/재개합니다.
void Pause(int id) { for(auto& j:jobs)if(j.id==id)j.paused=true; }
void Resume(int id) { for(auto& j:jobs)if(j.id==id)j.paused=false; }
void TogglePause(int id) { for(auto& j:jobs)if(j.id==id)j.paused=!j.paused; }
// Cancel: 현재 모습은 유지하고 작업만 끝냅니다. 초기 모습 복원은 r입니다.
void Cancel(int id) { for(auto& j:jobs)if(j.id==id)j.command.done=true; }
// IsFinished: 0/없는 번호도 완료로 처리해 최초 시작 조건으로 사용할 수 있습니다.
bool IsFinished(int id)
{
    for(const auto& j:jobs)if(j.id==id)return j.command.done;
    return true;
}

// Rewind: 반복할 때 애니메이션 진행 상태만 비우며 도형의 현재 모습은 유지합니다.
void Rewind(Command& c)
{
    c.started=c.done=c.backward=false;
    c.elapsed=c.previous=0;c.index=c.cycles=0;
    for(auto& child:c.children)Rewind(child);
}

// Advance: 주어진 시간만큼 진행하고 완료 후 남은 시간을 반환합니다.
// 순차 명령에 남은 시간을 전달하므로 프레임 경계에서 불필요하게 멈추지 않습니다.
glm::vec3 SwapPosition(const Command& c,int index,float progress)
{
    glm::vec3 start=c.swapStart[index],end=c.swapStart[1-index];
    if(progress<=0)return start;
    if(progress>=1)return end;
    if(c.swapMode==SwapMode::Direct)return glm::mix(start,end,progress);
    if(c.swapMode==SwapMode::ArcXZ)
    {
        glm::vec3 center=(start+end)*.5f,offset=start-center;
        float angle=glm::radians(180.0f)*progress;
        // 높이가 다르면 Y는 직선으로 보간하고 XZ 궤도는 반원으로 유지합니다.
        return center+glm::vec3(offset.x*cos(angle)+offset.z*sin(angle),
            offset.y*(1-2*progress),-offset.x*sin(angle)+offset.z*cos(angle));
    }
    glm::vec3 direction=c.swapStart[1]-c.swapStart[0];
    glm::vec3 side=c.swapMode==SwapMode::DetourXZ
        ? glm::vec3(-direction.z,0,direction.x) : glm::vec3(-direction.y,direction.x,0);
    float length=glm::length(side);
    side=length>0 ? side/length : (c.swapMode==SwapMode::DetourXZ ? glm::vec3(0,0,1) : glm::vec3(0,1,0));
    side*=glm::length(direction)*.5f*(index==0 ? 1.0f : -1.0f);
    // 세 구간에 시간을 1:2:1로 배분하여 일정한 속도로 우회합니다.
    if(progress<.25f)return start+side*(progress*4);
    if(progress<.75f)return glm::mix(start,end,(progress-.25f)*2)+side;
    return end+side*((1-progress)*4);
}

float Advance(Command& c,float dt)
{
    if(c.done)return dt;
    if(c.kind==Kind::Swap)
    {
        if(!c.started)
        {
            // 반복할 때마다 현재 중심을 다시 읽습니다.
            for(int i=0;i<2;++i)c.swapStart[i]=glm::vec3(boxes[c.children[i].id].model*glm::vec4(0,0,0,1));
            c.started=true;
        }
        float used=glm::min(dt,glm::max(0.0f,c.seconds-c.elapsed));
        c.elapsed+=used;
        float progress=glm::min(c.elapsed/c.seconds,1.0f);
        for(int i=0;i<2;++i)
        {
            glm::vec3 delta=SwapPosition(c,i,progress)-SwapPosition(c,i,c.previous);
            BoxState& box=boxes[c.children[i].id];
            if(box.enabled)box.model=glm::translate(glm::mat4(1),delta)*box.model;
        }
        c.previous=progress;c.done=progress>=1;
        return c.done ? dt-used : 0;
    }
    if(c.kind==Kind::Parallel)
    {
        bool finished=true;
        float remaining=dt;
        for(auto& child:c.children)
        {
            remaining=glm::min(remaining,Advance(child,dt));
            finished=finished && child.done;
        }
        c.done=finished;
        return finished ? remaining : 0;
    }
    if(c.kind==Kind::Sequence)
    {
        while(c.index<(int)c.children.size())
        {
            dt=Advance(c.children[c.index],dt);
            if(!c.children[c.index].done)return 0;
            ++c.index;
        }
        c.done=true;return dt;
    }
    if(c.kind==Kind::Repeat)
    {
        // 빈 명령/0거리의 무한반복이 프로그램을 멈추지 않도록 한 프레임의 반복 수를 제한합니다.
        for(int guard=0;guard<128;++guard)
        {
            dt=Advance(c.children[0],dt);
            if(!c.children[0].done)return 0;
            ++c.cycles;
            if(c.count!=Forever && c.cycles>=c.count){c.done=true;return dt;}
            Rewind(c.children[0]);
            if(dt<=0)return 0;
        }
        return 0;
    }
    if(c.kind==Kind::PingPong)
    {
        Command& child=c.children[0];
        dt=Advance(child,dt);
        if(!child.done)return 0;
        if(!c.backward)
        {
            c.backward=true;
            child.backward=true;child.done=false;child.elapsed=0;
            // 이전 진행값을 유지해 반대 방향으로 되돌립니다. Center 회전과 신축은 복귀 중에도 현재 중심을 사용합니다.
            // 다른 축의 회전과 함께 실행하면 최초 자세로 정확히 복귀하는 것은 아닙니다.
            dt=Advance(child,dt);
        }
        c.done=child.done;
        return c.done ? dt : 0;
    }
    BoxState& box=boxes[c.id];
    if(!c.started)
    {
        c.started=true;
        c.pivotPosition=c.pivot.point ? c.pivot.value : glm::vec3(box.model*glm::vec4(c.pivot.value,1));
        c.duration=c.seconds>0 ? c.seconds : c.kind==Kind::Move ? glm::length(c.delta)/MOVE_SPEED
            : glm::abs(c.amount)/(c.kind==Kind::Rotate ? ROTATE_SPEED : SCALE_SPEED);
    }
    float used=glm::min(dt,glm::max(0.0f,c.duration-c.elapsed));
    c.elapsed+=used;
    float progress=c.duration>0 ? glm::min(c.elapsed/c.duration,1.0f) : 1.0f;
    float value=c.backward ? 1-progress : progress;
    if(box.enabled)
    {
        glm::mat4 change(1);
        if(c.kind==Kind::Move)
            change=glm::translate(change,c.delta*(value-c.previous));
        else
        {
            // Center 회전과 신축은 현재 중심을 따라갑니다. Point와 면 기준점은 시작 시 위치를 유지합니다.
            glm::vec3 pivotPosition=c.pivotPosition;
            if(!c.pivot.point
                && c.pivot.value.x==0 && c.pivot.value.y==0 && c.pivot.value.z==0)
                pivotPosition=glm::vec3(box.model*glm::vec4(0,0,0,1));
            change=glm::translate(change,pivotPosition);
            if(c.kind==Kind::Rotate)
                change=glm::rotate(change,glm::radians(c.amount*(value-c.previous)),AxisVector(c.axis));
            else
            {
                float factor=(1+c.amount*value)/(1+c.amount*c.previous);
                glm::vec3 scale(1);scale[(int)c.axis]=factor;
                change=glm::scale(change,scale);
            }
            change=glm::translate(change,-pivotPosition);
        }
        box.model=change*box.model;
    }
    c.previous=value;
    c.done=progress>=1;
    return c.done ? dt-used : 0;
}

// ============================================================================
// 4. 기본 함수 선언과 메인 반복문
// ============================================================================
string ReadFile(const char* name);
bool InitShader();
void ResetScene();
void UpdateScene(GLFWwindow*,float);
void DrawScene(int,int);
void DrawBox(const glm::mat4&,glm::vec3,glm::vec3);
void KeyCallback(GLFWwindow*,int,int,int,int);
void MouseCallback(GLFWwindow*,int,int,int);
void CursorCallback(GLFWwindow*,double,double);
void ScrollCallback(GLFWwindow*,double,double);
int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2023182010_kch - Balance Crane", nullptr, nullptr);
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
    cout << "1/2: arm1 start/stop | 3/4: arm2 start/stop | hold 5/6: rotate\n"
         << "l/r: select | Right/Left: in/out (5 steps) | +/-: camera Z | o: orbit\n"
         << "s: reset | q: quit | drag/wheel: camera" << endl;
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

// ResetScene: 설정 영역 ①에서 모든 초기값을 다시 읽습니다.
void ResetScene()
{
    wireframe=WIREFRAME_INITIAL;
    perspective=PERSPECTIVE_INITIAL;
    hiddenSurface=HIDDEN_SURFACE_INITIAL;
    static mt19937 randomEngine(random_device{}());
    uniform_real_distribution<float> randomChannel(RANDOM_COLOR_MIN,RANDOM_COLOR_MAX);
    map<int,glm::vec3> groupColors; // 매 리셋마다 비워 그룹 색상도 새로 추첨합니다.
    auto randomColor=[&]()
    {
        return glm::vec3(randomChannel(randomEngine),randomChannel(randomEngine),randomChannel(randomEngine));
    };
    jobs.clear();nextJob=1;slots.fill(0);accumulator=0;
    playerWalking=playerJumping=false;playerOffset=glm::vec3(0);playerDirection={0,0,-1};
    playerPhase=playerJumpTime=playerJumpStart=0;
    assemblyPosition=glm::vec3(0);
    cameraPosition={CAMERA_X,CAMERA_Y,CAMERA_Z};
    cameraTarget={CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z};
    cameraOrbit=0;dragging=false;
    for(int i=0;i<BOX_COUNT;++i)
    {
        const auto& setting=boxSettings[i];
        if(setting.size.x<=0 || setting.size.y<=0 || setting.size.z<=0)
            throw runtime_error("Initial box size must be positive");
        boxes[i].animation=glm::mat4(1);
        boxes[i].parent=setting.parent;
        boxes[i].inheritScale=setting.inheritScale;
        boxes[i].baseSize=setting.size;
        boxes[i].enabled=setting.enabled;
        boxes[i].color=setting.color.value;
        if(setting.color.random)
        {
            if(setting.color.group<0)
                boxes[i].color=randomColor();
            else
            {
                auto found=groupColors.find(setting.color.group);
                if(found==groupColors.end())
                    found=groupColors.emplace(setting.color.group,randomColor()).first;
                boxes[i].color=found->second;
            }
        }
        glm::mat4 model=glm::translate(glm::mat4(1),setting.center);
        model=model*glm::mat4_cast(glm::quat(glm::radians(setting.angles)));
        boxes[i].model=glm::scale(model,setting.size);
    }
    selectedArm=0;craneYaw=0;beamTilt=CRANE_INITIAL_TILT;spreadAngle=0;
    for(int i=0;i<2;++i)
    {
        slideStep[i]=0;armPhase[i]=armAngle[i]=armSlide[i]=0;armRunning[i]=false;
    }
    PoseCrane();
    playerInitialFoot=1e20f;
    for(int id=0;id<BOX_COUNT;++id)
        if(boxes[id].visible && IsPlayerPart(id))
            playerInitialFoot=glm::min(playerInitialFoot,BoxBounds(boxes[id].world).first.y);
    if(playerInitialFoot==1e20f)playerInitialFoot=0;
}

// ============================================================================
// 5. 애니메이션, 전체 방향키 이동, 카메라
// ============================================================================
void UpdateScene(GLFWwindow* window,float dt)
{
    accumulator+=dt;
    const float step=1.0f/ANIMATION_FPS;
    while(accumulator>=step)
    {
        accumulator-=step;
        for(auto& job:jobs)
            if(!job.paused && !job.command.done)Advance(job.command,step);
        UpdateCrane(window,step);
    }
    float zDirection=0;
    if(glfwGetKey(window,GLFW_KEY_EQUAL)==GLFW_PRESS || glfwGetKey(window,GLFW_KEY_KP_ADD)==GLFW_PRESS)zDirection+=1;
    if(glfwGetKey(window,GLFW_KEY_MINUS)==GLFW_PRESS || glfwGetKey(window,GLFW_KEY_KP_SUBTRACT)==GLFW_PRESS)zDirection-=1;
    glm::vec3 candidate=cameraPosition+glm::vec3(0,0,zDirection*CAMERA_MOVE_SPEED*dt);
    if(glm::length(candidate-cameraTarget)>=CAMERA_RADIUS_MIN)cameraPosition=candidate;
    if(cameraOrbit)
    {
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(CAMERA_ROTATE_SPEED*dt),glm::vec3(0,1,0));
        glm::vec3 center(0,CAMERA_TARGET_Y,0);
        cameraPosition=center+glm::vec3(rotation*glm::vec4(cameraPosition-center,0));
        cameraTarget=center+glm::vec3(rotation*glm::vec4(cameraTarget-center,0));
    }
}

void DrawBox(const glm::mat4& model,glm::vec3 size,glm::vec3 color)
{
    glm::mat4 transform=glm::scale(model,size);
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(transform));
    glUniform3fv(colorLocation,1,glm::value_ptr(color));
    glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,36);
}

void DrawScene(int width,int height)
{
    if(width<=0 || height<=0)return;
    EvaluateHierarchy();
    if(hiddenSurface)
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
    }
    glViewport(0,0,width,height);
    glClearColor(BACKGROUND_RED,BACKGROUND_GREEN,BACKGROUND_BLUE,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    glm::mat4 view=glm::lookAt(cameraPosition,cameraTarget,glm::vec3(0,1,0));
    float aspect=(float)width/height;
    glm::mat4 projection;
    if(perspective)
        projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),aspect,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
    else
    {
        // 주시점 깊이에서 원근 투영과 같은 크기가 되도록 범위를 계산합니다.
        // 카메라 거리와 연동하여 직교 투영에서도 기존 휠 확대·축소가 작동합니다.
        float halfHeight=glm::length(cameraPosition-cameraTarget)*glm::tan(glm::radians(PERSPECTIVE_FOVY)*.5f);
        projection=glm::ortho(-halfHeight*aspect,halfHeight*aspect,-halfHeight,halfHeight,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
    }
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    if(FLOOR_ENABLED)
        DrawBox(glm::translate(glm::mat4(1),{0,-FLOOR_HEIGHT*.5f,0}),
            {FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH},{.30f,.46f,.28f});
    glm::mat4 assembly=glm::translate(glm::mat4(1),assemblyPosition);
    glPolygonMode(GL_FRONT_AND_BACK,wireframe ? GL_LINE : GL_FILL);
    for(const auto& box:boxes)
        if(box.visible)DrawBox(assembly*box.world,glm::vec3(1),box.color);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindVertexArray(0);
}

// ============================================================================
// 수정 영역 ②: 과제 명령. 회전/왕복 중에도 위치 이동과 선택을 함께 사용할 수 있습니다.
// 오른쪽=선택한 팔 안쪽 이동, 왼쪽=바깥쪽 이동. 0~5단계에서 제한합니다.
// 안쪽으로 옮긴 쪽이 올라가고 반대쪽이 내려가도록 위 몸체 각도를 바꿉니다.
// ============================================================================
void CustomKey(int key)
{
    switch(key)
    {
    case GLFW_KEY_1: armRunning[0]=true; break;
    case GLFW_KEY_2: armRunning[0]=false; break;
    case GLFW_KEY_3: armRunning[1]=true; break;
    case GLFW_KEY_4: armRunning[1]=false; break;
    case GLFW_KEY_L: selectedArm=0; break;
    case GLFW_KEY_R: selectedArm=1; break;
    case GLFW_KEY_RIGHT: slideStep[selectedArm]=glm::min(slideStep[selectedArm]+1,CRANE_SLIDE_STEPS); break;
    case GLFW_KEY_LEFT: slideStep[selectedArm]=glm::max(slideStep[selectedArm]-1,0); break;
    case GLFW_KEY_O: cameraOrbit=cameraOrbit ? 0 : 1; break;
    case GLFW_KEY_S: ResetScene(); break;
    }
    PoseCrane();
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int)
{
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
    // 기존 표시 설정 토글도 유지합니다.
    if(key==GLFW_KEY_W)wireframe=!wireframe;
    if(key==GLFW_KEY_T)perspective=!perspective;
    if(key==GLFW_KEY_E)hiddenSurface=!hiddenSurface;
    CustomKey(key);
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
    int width,height;glfwGetWindowSize(window,&width,&height);
    if(width<=0 || height<=0)return;
    glm::vec3 offset=cameraPosition-cameraTarget;
    float radius=glm::length(offset);
    float yaw=glm::atan(offset.x,offset.z),pitch=glm::asin(glm::clamp(offset.y/radius,-1.0f,1.0f));
    yaw-=glm::radians(ORBIT_DEGREES)*(float)(x-previousMouseX)/width;
    pitch+=glm::radians(ORBIT_DEGREES)*(float)(y-previousMouseY)/height;
    pitch=glm::clamp(pitch,-glm::radians(ORBIT_PITCH_LIMIT),glm::radians(ORBIT_PITCH_LIMIT));
    cameraPosition=cameraTarget+radius*glm::vec3(glm::cos(pitch)*glm::sin(yaw),glm::sin(pitch),glm::cos(pitch)*glm::cos(yaw));
    previousMouseX=x;previousMouseY=y;
}
void ScrollCallback(GLFWwindow*,double,double yoffset)
{
    glm::vec3 offset=cameraPosition-cameraTarget;
    float radius=glm::clamp(glm::length(offset)-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
    cameraPosition=cameraTarget+glm::normalize(offset)*radius;
}

// ============================================================================
// 6. 셰이더 읽기와 컴파일
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
