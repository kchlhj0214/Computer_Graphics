// 0 = 시험용 중간대비2 원본, 1 = 인형뽑기, 2 = 중간대비1 그네
// 아래 숫자 하나를 바꾸고 다시 빌드합니다.
#define MID_TEST_EXAMPLE 1
#if MID_TEST_EXAMPLE == 0
//프로젝트속성->링커->명령줄에다음과같이3개의라이브러리추가
//-> opengl32.lib glew32.lib glfw3.lib glu32.lib

#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
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
#define CAMERA_X 0.0f
#define CAMERA_Y 2.6f
#define CAMERA_Z -4.5f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y .5f
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
#define BACKGROUND_RED 1.0f
#define BACKGROUND_GREEN 1.0f
#define BACKGROUND_BLUE 1.0f

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
};
enum class Orientation { Inherit, KeepUpright, KeepWorld }; // 부모 회전 전체 / Y축 방향만 / 자식 자체 방향만
struct Attachment
{
    bool enabled=false;
    Pivot parentPoint=Pivot::Center,childPoint=Pivot::Center;
    Orientation orientation=Orientation::Inherit;
    glm::vec3 offset{0}; // Move로 바꿀 수 있는 부모 기준 연결점의 이동량
};
struct FollowLagState
{
    float seconds=0,maxAngle=0;
    Pivot hinge=Pivot::Top;
    bool sampled=false;
    glm::mat4 previousParent{1};
    glm::vec3 trailingOffset{0};
};
struct BoxState
{
    bool enabled=false,visible=false;
    glm::mat4 model{1},world{1},animation{1}; // model: 부모 기준 변환, world: 상속을 합성한 조립 공간 변환
    glm::vec3 color{.5f},baseSize{1};
    int parent=-1;
    bool inheritScale=false;
    Attachment attachment;
    FollowLagState followLag;
};

// ============================================================================
// 수정 영역 ①: 번호 0~39의 활성화, 전체 크기, 중심 좌표, RGB 색상, 초기 회전각
// {활성화,{크기},{위치},색상,{초기각도}}만 지정합니다.
// 부모/자식 관계와 신축 상속은 ResetScene의 Attach에서 설정합니다.
// 연결점 생략형 Attach에서는 위치가 부모 기준 상대 좌표입니다.
// 연결점 지정형 Attach에서는 두 연결점이 맞닿도록 위치를 계산합니다.
// true인 객체만 표시됩니다. 크기/좌표/색상은 아래 한 곳에서만 설정합니다.
// 초기 회전각은 {X각,Y각,Z각}, 기본 {0,0,0}입니다. 크기는 양수로 지정합니다.
// 색상 인자 예: { .9f,.2f,.2f } / Color::Random() / Color::Random(1)
// Random(1)을 사용한 객체는 모두 같은 랜덤 색입니다. 다른 그룹은 Random(2) 등으로 지정합니다.
// r 리셋 시 랜덤 색은 다시 추첨하며 같은 그룹끼리는 계속 같은 색을 공유합니다.
// ============================================================================
const array<BoxSetting,BOX_COUNT> boxSettings={{
    {true, {.10f,.10f,.50f}, {0.2,.05f,0}, Color::Random(1), {0,0,0}}, // 0번
    {true, {.10f,.10f,.50f}, {-0.2,.05f,0}, Color::Random(1), {0,0,0}}, // 1번
    {true, {.50f,.10f,.50f}, {0,.15f,0}, Color::Random(), {0,0,0}}, // 2번
    {true, {.15f,.10f,.15f}, {0.15f,.25f,0}, Color::Random(2), {0,0,0}}, // 3번
    {true, {.15f,.10f,.15f}, {-0.15f,.25f,0}, Color::Random(2), {0,0,0}}, // 4번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 5번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 6번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 7번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 8번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 9번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 10번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 11번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 12번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 13번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 14번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 15번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 16번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 17번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 18번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 19번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 20번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 21번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 22번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 23번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 24번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 25번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 26번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 27번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 28번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 29번
    {false, {.30f,.30f,.30f}, {0,.15f,0}, {.25f,.65f,.85f}, {0,0,0}}, // 30번
    {true, {.26f,.32f,.16f}, {.9f,.48f,.8f}, {.15f,.65f,.80f}, {0,0,0}}, // 31 몸통
    {true, {.075f,.30f,.075f}, {-.195f,0,0}, {.85f,.25f,.20f}, {0,0,0}}, // 32 왼팔
    {true, {.075f,.30f,.075f}, {.195f,0,0}, {.20f,.40f,.85f}, {0,0,0}}, // 33 오른팔
    {true, {.09f,.32f,.09f}, {-.065f,-.32f,0}, {.30f,.25f,.80f}, {0,0,0}}, // 34 왼다리
    {true, {.09f,.32f,.09f}, {.065f,-.32f,0}, {.65f,.20f,.70f}, {0,0,0}}, // 35 오른다리
    {true, {.20f,.20f,.20f}, {0,.26f,0}, {.85f,.70f,.50f}, {0,0,0}}, // 36 머리
    {true, {.045f,.045f,.055f}, {0,-.015f,-.1275f}, {.90f,.45f,.20f}, {0,0,0}}, // 37 코
    {true, {.035f,.025f,.012f}, {-.045f,.035f,-.106f}, {.12f,.12f,.18f}, {0,0,0}}, // 38 왼눈
    {true, {.035f,.025f,.012f}, {.045f,.035f,-.106f}, {.12f,.12f,.18f}, {0,0,0}}, // 39 오른눈

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
    float delay=0,remainingDelay=0;
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

// 연결점 지정형: Attach(부모,자식,부모연결점,자식연결점,방향상속,신축상속).
// 방향상속 기본값은 Orientation::Inherit, 신축상속 기본값은 false입니다.
// false는 자식 크기 유지, true는 부모 신축 상속입니다. 연결 위치는 두 경우 모두 부모를 따릅니다.
// 예: Attach(0,1,Pivot::Bottom,Pivot::Top,Orientation::Inherit,true);
// 신축상속을 지정하려면 앞의 방향상속도 작성합니다. 부모 관계는 boxSettings에 쓰지 않습니다.
// Pivot::Top/Bottom 등은 각 객체의 면 중심, Point(x,y,z)는 해당 객체 중심 기준 길이입니다.
// 연결점이 맞닿도록 중심을 계산합니다. Move는 부모 기준 연결점 오프셋을 옮기고,
// 자식 Rotate/Scale은 자신의 변환을 적용한 뒤 연결점을 다시 맞춥니다.
void Attach(int parent,int child,Pivot parentPoint,Pivot childPoint,Orientation orientation=Orientation::Inherit,bool inheritScale=false)
{
    if(child<0 || child>=BOX_COUNT || parent<0 || parent>=BOX_COUNT)
        throw runtime_error("Attachment box ID must be 0~39");
    for(int id=parent,depth=0;id!=-1;++depth)
    {
        if(id==child || depth>=BOX_COUNT)throw runtime_error("Attachment would create a cycle");
        if(id<0 || id>=BOX_COUNT)throw runtime_error("Parent ID must be -1 or 0~39");
        id=boxes[id].parent;
    }
    auto& box=boxes[child];
    box.parent=parent;
    box.inheritScale=inheritScale;
    box.attachment={true,parentPoint,childPoint,orientation};
    box.followLag.sampled=false;
    box.followLag.trailingOffset=glm::vec3(0);
}

// 연결점 생략형: Attach(부모,자식,신축상속=false).
// 예: Attach(0,1); / Attach(0,1,true);
// 현재 model의 위치를 부모 기준 상대 위치로 해석하므로 연결 순간 월드 위치가 바뀔 수 있습니다.
// 초기화 직후에는 boxSettings의 center가 상대 위치입니다. 부모 회전은 전체 상속합니다.
// 면을 고정하지 않으므로 기존 몸통/팔다리의 상대 이동과 관절 회전을 유지합니다.
void Attach(int parent,int child,bool inheritScale=false)
{
    Attach(parent,child,Pivot::Center,Pivot::Center,Orientation::Inherit,inheritScale);
    boxes[child].attachment.enabled=false;
}

// FollowLag(자식,연결점,반응 시간(초),최대 기울기(도)). 0도를 지정하면 효과를 끕니다.
// 연결점 지정형 Attach는 자식 연결점을 축으로 사용하고, 생략형은 FollowLag의 연결점을 사용합니다.
// 연결점은 부모를 즉시 따르고,
// 몸체는 움직임 반대쪽으로 기울다가 복귀합니다. 별도 Run이나 키 입력 처리가 필요 없습니다.
void FollowLag(int child,Pivot hinge=Pivot::Top,float seconds=.25f,float maxAngle=25.0f)
{
    if(child<0 || child>=BOX_COUNT)throw runtime_error("FollowLag box ID must be 0~39");
    if(!std::isfinite(seconds) || seconds<=0 || !std::isfinite(maxAngle) || maxAngle<0 || maxAngle>=90)
        throw runtime_error("FollowLag requires positive seconds and 0 <= angle < 90");
    if(maxAngle>0 && boxes[child].parent==-1)throw runtime_error("FollowLag requires a parent");
    auto& lag=boxes[child].followLag;
    lag={seconds,maxAngle,hinge};
}

glm::vec4 AttachmentPoint(const BoxState& box,Pivot point)
{
    return glm::vec4(point.point ? point.value/box.baseSize : point.value,1);
}

// 카메라와 무관한 조립 공간에서 부모가 옮긴 만큼 자식의 상대 지연량을 누적하고 감쇠합니다.
void ApplyFollowLag(BoxState& box,const glm::mat4& parentFrame,float dt)
{
    auto& lag=box.followLag;
    if(lag.maxAngle<=0 || !box.visible)return;
    glm::mat4 frame=parentFrame;
    frame[3]+=glm::vec4(assemblyPosition,0);
    if(!lag.sampled)
    {
        lag.previousParent=frame;
        lag.sampled=true;
    }
    else if(dt>0)
    {
        // 이전과 현재의 부모 프레임에 같은 점을 넣어 자식 자체의 이동은 지연의 원인에서 제외합니다.
        glm::vec4 sample=glm::inverse(parentFrame)*box.world[3];
        glm::vec3 parentDelta(frame*sample-lag.previousParent*sample);
        lag.trailingOffset=(lag.trailingOffset-parentDelta)*std::exp(-dt/lag.seconds);
        lag.previousParent=frame;
    }

    Pivot hinge=box.attachment.enabled ? box.attachment.childPoint : lag.hinge;
    glm::vec3 pivot(box.world*AttachmentPoint(box,hinge));
    glm::vec3 arm=glm::vec3(box.world[3])-pivot;
    float length=glm::length(arm);
    if(length<.00001f)return; // 연결점이 중심이면 기울일 방향을 정할 수 없습니다.
    glm::vec3 axis=arm/length;
    glm::vec3 sideways=lag.trailingOffset-axis*glm::dot(lag.trailingOffset,axis);
    float distance=glm::length(sideways);
    if(distance<.00001f)return;
    float angle=glm::min(glm::atan(distance,length),glm::radians(lag.maxAngle));
    glm::mat4 swing=glm::translate(glm::mat4(1),pivot);
    swing=glm::rotate(swing,angle,glm::normalize(glm::cross(axis,sideways)));
    box.world=glm::translate(swing,-pivot)*box.world;
}

// EvaluateHierarchy: 부모를 먼저 계산합니다. dt가 양수일 때만 따라오는 효과의 시간을 갱신합니다.
void EvaluateHierarchy(float dt=0)
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
            Orientation orientation=box.attachment.orientation;
            if(box.inheritScale && orientation==Orientation::Inherit)
                box.world=frame*local;
            else
            {
                // 크기는 제거하고 회전만 추출합니다. 연결 위치는 신축된 부모를 따라갑니다.
                glm::vec3 x=glm::normalize(glm::vec3(frame[0]));
                glm::vec3 y=glm::vec3(frame[1])-x*glm::dot(x,glm::vec3(frame[1]));
                y=glm::normalize(y);
                glm::vec3 z=glm::normalize(glm::cross(x,y));
                glm::mat4 rotation(1);
                if(orientation==Orientation::Inherit)
                {
                    rotation[0]=glm::vec4(x,0);rotation[1]=glm::vec4(y,0);rotation[2]=glm::vec4(z,0);
                }
                else if(orientation==Orientation::KeepUpright)
                {
                    glm::vec3 horizontal(z.x,0,z.z);
                    if(glm::length(horizontal)<.00001f)horizontal={-x.z,0,x.x};
                    horizontal=glm::normalize(horizontal);
                    rotation[0]=glm::vec4(horizontal.z,0,-horizontal.x,0);
                    rotation[2]=glm::vec4(horizontal,0);
                }
                // 회전을 제한할 때 크기 상속이 켜져 있으면 부모의 축별 크기 변화만 적용합니다.
                if(box.inheritScale)
                    rotation=glm::scale(rotation,glm::vec3(glm::length(glm::vec3(frame[0])),
                        glm::length(glm::vec3(frame[1])),glm::length(glm::vec3(frame[2]))));
                box.world=rotation*local;
                box.world[3]=frame*glm::vec4(glm::vec3(local[3]),1);
            }
            if(box.attachment.enabled)
            {
                const auto& attachment=box.attachment;
                glm::vec4 target=parent.world*AttachmentPoint(parent,attachment.parentPoint)
                    +frame*glm::vec4(attachment.offset,0);
                glm::vec4 current=box.world*AttachmentPoint(box,attachment.childPoint);
                box.world[3]+=target-current;
            }
            ApplyFollowLag(box,frame,dt);
        }
        state[id]=2;
    };
    for(int i=0;i<BOX_COUNT;++i)evaluate(evaluate,i);
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

// 끝에 시간 하나를 쓰면 실행 시간, 두 개를 쓰면 대기 시간과 실행 시간입니다. 단위는 초입니다.
// Move(3,{1,0,0},.0f,2.0f): 즉시 시작해 2초간 이동합니다.
// 대기 시간은 .0f로 두면 변환을 바로 시작합니다.
Command Move(int id,glm::vec3 delta,float delay,float seconds)
{
    Command c;c.kind=Kind::Move;c.id=id;c.delta=delta;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Move(int id,glm::vec3 delta,float seconds=0)
{
    return Move(id,delta,0,seconds);
}
// Rotate(번호,축,각도,기준점,대기 시간,실행 시간): -360~+360도. 기준점 생략 시 자기 중심.
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
//    시간을 지정하려면 기준점도 적습니다. PingPong의 한 주기는 대기 시간 + 편도 실행 시간의 두 배입니다.
// 예: Rotate(0,Axis::X,90,Pivot::Center,.0f,.0f) -> 현재 중심에서 고정 X축 방향으로 +90도 자전합니다.
// 예: Rotate(0,Axis::Y,-180,Point(0,0,0),.0f,2.0f) -> 원점을 지나는 Y축 주위로 2초 동안 -180도 공전합니다.
// 예: Rotate(0,Axis::Z,45,Pivot::Top,.0f,1.0f) -> 시작 시 윗면 중심을 지나는 Z축 주위로 1초 동안 +45도 회전합니다.
// Rotate(3,Axis::Y,90,Pivot::Center,.0f,2.0f): 대기 없이 2초간 회전합니다.
Command Rotate(int id,Axis axis,float degrees,Pivot pivot,float delay,float seconds)
{
    Command c;c.kind=Kind::Rotate;c.id=id;c.axis=axis;c.amount=degrees;c.pivot=pivot;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Rotate(int id,Axis axis,float degrees,Pivot pivot=Pivot::Center,float seconds=0)
{
    return Rotate(id,axis,degrees,pivot,0,seconds);
}
// Scale(번호,축,배율증감,기준점,대기 시간,실행 시간): +0.5는 1.5배, -0.5는 0.5배. -1 이하는 거부.
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
// 예: Scale(0,Axis::X,.5f,Pivot::Center,.0f,.0f) -> 매 순간 현재 중심을 기준으로 X방향 크기를 현재의 1.5배로 늘립니다.
// 예: Run(Parallel({Move(0,{1,0,0},.0f,2.0f),Scale(0,Axis::X,.5f,Pivot::Center,.0f,2.0f)}));
//     2초 동안 X방향으로 1만큼 이동하며 X방향 크기를 1.5배로 늘립니다. 신축은 이동 거리를 바꾸지 않습니다.
// 예: Scale(0,Axis::Y,-.5f,Pivot::Bottom,.0f,2.0f) -> 아래 면 중심을 기준으로 2초 동안 Y방향 크기를 절반으로 줄입니다.
// 예: Scale(0,Axis::X,1.0f,Point(0,0,0),.0f,1.0f) -> 1초 동안 X방향 크기와 원점에서의 X방향 거리를 두 배로 만듭니다.
// Scale(3,Axis::Y,.5f,Pivot::Bottom,.0f,2.0f): 대기 없이 2초간 신축합니다.
Command Scale(int id,Axis axis,float change,Pivot pivot,float delay,float seconds)
{
    Command c;c.kind=Kind::Scale;c.id=id;c.axis=axis;c.amount=change;c.pivot=pivot;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Scale(int id,Axis axis,float change,Pivot pivot=Pivot::Center,float seconds=0)
{
    return Scale(id,axis,change,pivot,0,seconds);
}
// 변환의 마지막 인자는 편도 시간(초)입니다. 생략하거나 0이면 기존 속도로 계산합니다.
// PingPong은 처음 한 번 대기한 뒤 편도 시간만큼 갔다가 즉시 돌아옵니다.
// Repeat로 감싸면 새 반복을 시작할 때 대기 시간도 다시 적용됩니다.
// Swap(번호1,번호2,대기 시간,실행 시간,방식): 지정한 초 동안 두 중심 위치를 교환합니다. 방식 생략 시 직선입니다.
// 회전과 크기는 유지합니다. 서로 다른 활성 객체와 양수 시간을 지정합니다.
// ArcXZ는 중점 주위 XZ 반원, DetourXZ/DetourXY는 각 평면의 ㄷ자 경로입니다.
// ㄷ자 우회 거리는 두 중심 거리의 절반이며 두 객체는 서로 반대쪽으로 우회합니다.
// Swap(번호1,번호2,대기 시간,실행 시간,방식)으로 교환을 늦게 시작할 수 있습니다.
Command Swap(int first,int second,float delay,float seconds,SwapMode mode=SwapMode::Direct)
{
    Command c;c.kind=Kind::Swap;c.seconds=seconds;c.swapMode=mode;
    c.delay=c.remainingDelay=delay;
    c.children={Move(first,glm::vec3(0),seconds),Move(second,glm::vec3(0),seconds)};
    return c;
}
Command Swap(int first,int second,float seconds,SwapMode mode=SwapMode::Direct)
{
    return Swap(first,second,0,seconds,mode);
}
// Parallel({명령,...}): 각 명령의 대기 시간은 함께 흐르며, 모든 명령이 끝나야 완료합니다.
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
    if(!isfinite(c.delay) || c.delay<0)throw runtime_error("Delay must be finite and nonnegative");
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
        if(first.attachment.enabled || second.attachment.enabled)
            throw runtime_error("Swap requires boxes without Attach; use Move to slide an attachment");
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
    c.remainingDelay=c.delay;
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
    if(c.remainingDelay>0)
    {
        // 대기를 끝내고 남은 시간으로 변환을 시작합니다. 기준점도 이때 기록합니다.
        float waiting=glm::min(dt,c.remainingDelay);
        c.remainingDelay-=waiting;
        dt-=waiting;
        if(c.remainingDelay>0)return 0;
    }
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
    if(box.enabled && c.kind==Kind::Move && box.attachment.enabled)
        box.attachment.offset+=c.delta*(value-c.previous);
    else if(box.enabled)
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
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Midterm 2 - Box Assembly", nullptr, nullptr);
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
    cout << "Arrows: custom | x/X,z/Z: camera axes | y/Y: orbit\n"
         << "v/b/n/m: walk | Space: jump | a~l,0~9: custom commands | w: wire | t: projection | e: hidden surfaces\n"
         << "r: reset | q: quit | drag/wheel: camera" << endl;
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
        boxes[i].parent=-1;
        boxes[i].inheritScale=false;
        boxes[i].attachment={};
        boxes[i].followLag={};
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
    // 부모 관계와 신축 상속은 이곳의 Attach에서만 설정합니다.
    // 연결점 생략형: boxSettings의 위치를 부모 기준 상대 위치로 사용합니다.
    for(int child=32;child<=36;++child)Attach(31,child,false);
    for(int child=37;child<=39;++child)Attach(36,child,false);
    // 수정 영역 ①-추가: boxSettings에서 객체를 켠 다음 필요한 줄만 주석 해제합니다.
    // 21번 윗면을 20번 밑면에 고정합니다. 21번 중심 위치 대신 두 연결점이 배치를 정합니다.
    // Attach(20,21,Pivot::Bottom,Pivot::Top);
    // FollowLag(21,Pivot::Top,.25f,25.0f); // 20번이 움직이면 21번이 기울고, 멈추면 복귀
    // Attach(21,22,Pivot::Bottom,Pivot::Top,Orientation::KeepUpright); // 22번은 21번 끝에 붙어 수평 유지
    // Orientation::Inherit: 부모 회전 전체, KeepUpright: 수평 방향만, KeepWorld: 자식 자체 방향만.
    // 자식 자체의 초기 각도/Rotate는 각 모드에서 유지됩니다. 신축 상속은 Attach의 마지막 bool 인자로 정합니다.
    // Point(0,.2f,0)처럼 면 중심 대신 각 부품의 중심에서 잰 위치를 지정할 수 있습니다.
    // FollowLag 사용 전 Attach로 부모를 지정하세요. 연결점 생략형 Attach도 사용할 수 있습니다.
    // 반응 시간을 늘리면 기울어진 상태가 오래 남습니다. 수평을 유지할 손에는 FollowLag를 생략합니다.
    // 변환 예: Run(Move(20,{1,0,0},.0f,1.0f)); // 자식에게 이동 명령을 다시 줄 필요 없이 따라옵니다.
    // 연결점을 미는 예: Run(Move(21,{.2f,0,0},.0f,.5f)); // 21번 연결점을 부모 기준 +X로 이동
    EvaluateHierarchy();
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
        UpdatePlayer(step);
    }
    EvaluateHierarchy();
    // 시험용 방향키는 CustomKey에서 설정합니다. 전체 이동이 필요하면 아래 주석을 풉니다.
    /*
    glm::vec3 movement(0);
    if(glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS)movement.x-=1;
    if(glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)movement.x+=1;
    if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)movement.z-=1;
    if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)movement.z+=1;
    if(glm::length(movement)>0)
    {
        movement=glm::normalize(movement)*GROUP_MOVE_SPEED*dt;
        glm::vec3 low(1e20f),high(-1e20f);
        bool any=false;
        for(const auto& box:boxes)
            if(box.visible)
            {
                any=true;
                for(float x:{-.5f,.5f})for(float y:{-.5f,.5f})for(float z:{-.5f,.5f})
                {
                    glm::vec3 p=assemblyPosition+glm::vec3(box.world*glm::vec4(x,y,z,1));
                    low=glm::min(low,p);high=glm::max(high,p);
                }
            }
        if(any)
        {
            // 명령 변환으로 이미 밖에 있다면 더 나가는 이동을 막고 안쪽 복귀만 허용합니다.
            for(int axis:{0,2})
            {
                float edge=(axis==0 ? FLOOR_WIDTH : FLOOR_DEPTH)*.5f;
                if(movement[axis]>0)movement[axis]=glm::min(movement[axis],glm::max(0.0f,edge-high[axis]));
                if(movement[axis]<0)movement[axis]=glm::max(movement[axis],glm::min(0.0f,-edge-low[axis]));
            }
            assemblyPosition+=movement;
        }
    }
    */
    // 지연 효과의 시간을 프레임마다 한 번만 진행합니다.
    EvaluateHierarchy(dt);
    // 카메라 로컬 +X는 오른쪽, 로컬 +Z는 뒤쪽입니다. Shift는 음의 방향입니다.
    glm::vec3 forward=glm::normalize(cameraTarget-cameraPosition);
    glm::vec3 right=glm::normalize(glm::cross(forward,glm::vec3(0,1,0)));
    bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS
        || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    float direction=shift ? -1.0f : 1.0f;
    glm::vec3 cameraMovement(0);
    if(glfwGetKey(window,GLFW_KEY_X)==GLFW_PRESS)cameraMovement+=right*direction;
    if(glfwGetKey(window,GLFW_KEY_Z)==GLFW_PRESS)cameraMovement-=forward*direction;
    if(glm::length(cameraMovement)>0)
    {
        cameraMovement=glm::normalize(cameraMovement)*CAMERA_MOVE_SPEED*dt;
        cameraPosition+=cameraMovement;cameraTarget+=cameraMovement;
    }
    if(cameraOrbit)
    {
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(cameraOrbit*CAMERA_ROTATE_SPEED*dt),glm::vec3(0,1,0));
        cameraPosition=glm::vec3(rotation*glm::vec4(cameraPosition,1));
        cameraTarget=glm::vec3(rotation*glm::vec4(cameraTarget,1));
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
            {FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH},{.82f,.88f,.94f});
    glm::mat4 assembly=glm::translate(glm::mat4(1),assemblyPosition);
    glPolygonMode(GL_FRONT_AND_BACK,wireframe ? GL_LINE : GL_FILL);
    for(const auto& box:boxes)
        if(box.visible)DrawBox(assembly*box.world,glm::vec3(1),box.color);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindVertexArray(0);
}

// ============================================================================
// [키 입력에서 사용하는 변환 함수 요약]
// 아래 번호는 boxSettings의 활성 객체 번호(0~39)입니다. 시간 단위는 초입니다.
// Move/Rotate/Scale/Swap은 명령을 만들며, Run(명령)을 호출해야 실행됩니다.
// Attach/FollowLag는 설정 함수이므로 Run으로 감싸지 않습니다.
//
// 1) 이동: Run(Move(번호,{X이동량,Y이동량,Z이동량},대기 시간,실행 시간));
//    Run(Move(0,{1,0,0},.0f,2.0f)); // 현재 위치에서 +X로 1만큼, 2초 동안 이동
//    좌표는 목적지가 아니라 상대 이동량입니다. 부모가 있으면 부모 좌표계 기준입니다.
//    연결점 지정형 Attach는 부모 연결점 offset을 이동하고, 생략형은 부모 기준 상대 위치를 이동합니다.
//
// 2) 회전: Run(Rotate(번호,축,각도,기준점,대기 시간,실행 시간));
//    Run(Rotate(0,Axis::Y,90,Pivot::Center,.0f,2.0f)); // 현재 중심에서 90도 자전
//    Run(Rotate(0,Axis::Y,180,Point(0,0,0),.0f,2.0f)); // 조립 공간 원점 주위 공전
//    각도는 현재 상태에 추가하는 도 단위 값(-360~360), 양수는 오른손 법칙입니다.
//    Axis::X/Y/Z는 조립 공간의 고정 축이며, 부모가 있으면 부모 좌표계의 축입니다.
//    Center는 이동 중인 중심을 계속 따라갑니다. 객체 자체의 회전된 로컬 축은 아닙니다.
//
// 3) 신축: Run(Scale(번호,축,배율증감,기준점,대기 시간,실행 시간));
//    Run(Scale(0,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f)); // 아래 면 기준 높이를 1.5배
//    최종 배율 = 1 + 배율증감: .5는 1.5배, 1은 2배, -.5는 절반. -1 이하는 금지.
//    Center는 현재 중심을 따라가므로 이동과 함께 써도 신축 때문에 중심이 밀리지 않습니다.
//    축은 회전과 마찬가지로 조립/부모 공간 기준입니다. 기울어진 객체는 비스듬히 변형될 수 있습니다.
//
// 4) 기준점: Pivot::Center/Left/Right/Bottom/Top/Front/Back 또는 Point(x,y,z)
//    Front는 -Z 면, Back은 +Z 면입니다. Center 생략 가능, 기본값은 Center입니다.
//    회전/신축의 면 기준점은 시작할 때 계산하여 고정합니다. Center만 현재 중심을 따라갑니다.
//    변환의 Point는 조립/부모 공간 좌표입니다. 조립 공간 전체 이동 시 함께 움직입니다.
//    시간을 지정할 때는 Rotate/Scale의 기준점 인자도 반드시 작성합니다.
//    대기 시간 .0f는 즉시 시작입니다. Move/Rotate/Scale의 실행 시간 .0f는 기본 속도로 계산합니다.
//
// 5) 연결: Attach(부모번호,자식번호,부모연결점,자식연결점,방향상속,신축상속);
//    Attach(0,1,Pivot::Bottom,Pivot::Top); // 부모 아래와 자식 위를 맞붙임
//    Attach(0,1,Point(.2f,0,0),Pivot::Top,Orientation::KeepUpright);
//    Attach의 Point는 해당 객체 중심 기준 길이이며, 변환 함수의 Point와 의미가 다릅니다.
//    Orientation::Inherit: 부모 회전 전체 상속(기본값)
//    Orientation::KeepUpright: 부모의 Y축 방향만 상속, 기울기는 따라가지 않음
//    Orientation::KeepWorld: 부모 회전을 상속하지 않음. 자식 자체 회전은 유지
//    자식 Rotate/Scale 후에도 연결점을 다시 맞춥니다. 연결 순간 위치가 바뀔 수 있습니다.
//    Attach(0,1,Pivot::Bottom,Pivot::Top,Orientation::Inherit,true); // 부모 신축도 상속
//    마지막 bool 생략/false: 자식 크기 유지, true: 부모 신축도 상속
//    연결점 위치는 false여도 부모의 신축을 따라갑니다.
//    Attach(0,1,false); // 연결점 고정 없이 부모 관계만 설정, 기존 상대 위치 사용
//    Attach(0,1,true);  // 상대 위치 사용 + 부모 신축 상속
//    전체 이동/회전은 부모에만 주면 자식도 따라옵니다. 부모와 자식에 중복 적용하지 마세요.
//    초기 연결은 ResetScene에서 설정하고, 키 입력 중 Attach하면 그 시점에 연결됩니다.
//
// 6) 따라 흔들림: FollowLag(자식번호,연결점,반응시간,최대기울기);
//    FollowLag(1,Pivot::Top,.25f,25.0f); // 부모 움직임 반대쪽으로 기울다가 복귀
//    먼저 Attach로 부모를 지정합니다. 연결점 지정형은 Attach의 자식 연결점을 사용합니다.
//    연결점 생략형은 FollowLag의 연결점을 사용합니다. 예: Attach(0,1); FollowLag(1,Pivot::Top);
//    반응시간은 지연 감쇠 시간이며 완료 시간이 아닙니다. 클수록 천천히 복귀합니다.
//    최대기울기는 0 이상 90도 미만입니다. 연결점이 중심이면 기울기 효과는 없습니다.
//    최대기울기 0으로 효과를 끕니다. 물리 충돌 계산이 아닌 따라 흔들리는 효과입니다.
//
// 7) 위치 교환: Run(Swap(번호1,번호2,대기 시간,실행 시간,방식));
//    Run(Swap(0,1,.0f,2.0f,SwapMode::ArcXZ)); // 자세와 크기는 유지하며 위치 교환
//    Direct: 직선(기본), ArcXZ: XZ 반원, DetourXZ: XZ ㄷ자, DetourXY: XY ㄷ자
//    서로 다른 활성 객체, 같은 부모와 inheritScale 설정, 양수 시간이 필요합니다.
//    연결점 지정형 Attach는 Swap 불가입니다. 생략형은 같은 부모/신축상속 조건이면 가능합니다.
//
// 8) 조합: Parallel은 동시 실행, Sequence는 앞 명령 완료 후 다음 실행입니다.
//    Run(Parallel({Move(0,{1,0,0},.0f,2.0f),Rotate(0,Axis::Y,90,Pivot::Center,.0f,2.0f)}));
//    Run(Sequence({Move(0,{1,0,0},.0f,1.0f),Rotate(0,Axis::Y,90,Pivot::Center,.0f,1.0f)}));
//    같은 객체의 동시 변환은 하나의 Run(Parallel({...}))로 묶습니다.
//    다른 Run이 같은 객체를 사용 중이면 일시정지 상태라도 새 작업을 시작할 수 없습니다.
//
// 9) 반복/왕복: Repeat(명령,횟수), 무한반복은 Forever입니다.
//    Run(Repeat(Move(0,{.2f,0,0},.0f,1.0f),3)); // 총 +X .6 이동: 상대 변환이 누적됨
//    Run(Repeat(PingPong(Move(0,{1,0,0},.0f,1.0f)),Forever)); // 1초 이동 + 1초 복귀
//    PingPong에는 Move/Rotate/Scale 한 개만 넣습니다. Parallel/Swap은 직접 넣지 않습니다.
//    두 객체가 같은 주기로 신축: 각각 편도 1초, 왕복 2초로 맞춥니다.
//    slots[0]=Run(Repeat(Parallel({
//        PingPong(Scale(0,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f)),
//        PingPong(Scale(1,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f))
//    }),Forever));
//    Repeat는 시간을 자동으로 맞추지 않습니다. 각 변환의 시간을 같게 지정하세요.
//    스왑 반복: slots[0]=Run(Repeat(Swap(0,1,.0f,2.0f,SwapMode::ArcXZ),Forever));
//    신축만 Repeat하면 계속 커집니다. 크기 복귀가 필요하면 PingPong을 사용하세요.
//
// 10) 키를 다시 누르면 일시정지/재개하는 기본 형태:
//    if(IsFinished(slots[0]))
//        slots[0]=Run(Repeat(PingPong(Move(0,{1,0,0},.0f,1.0f)),Forever));
//    else TogglePause(slots[0]);
//    slots에는 객체 번호가 아니라 Run이 반환한 작업 번호를 저장합니다.
//    Pause/Resume/TogglePause/Cancel은 작업 전체에 적용합니다.
//    Cancel(slots[0])은 현재 모습을 유지하며 작업만 종료합니다. 초기화는 r입니다.
//    키마다 slots[0]~slots[8]을 나누되, 같은 객체의 작업 충돌은 별도로 피해야 합니다.
//    일반 키는 누를 때 한 번 실행됩니다. 누르는 동안의 이동은 갱신 함수에서 처리합니다.

// 11) 복합 예시: 이동 + 자전 + 신축 왕복 → XZ 반원 스왑 → 전체 무한반복
// 전제: 0번과 1번은 활성화된 독립 객체이며, 다른 작업에서 사용하고 있지 않아야 합니다.
// 인형뽑기 모드의 0/1번은 이미 연결되어 있으므로, 이 예시에는 별도의 독립 객체 번호를 넣으세요.
// 원하는 키 처리 안에 아래 if/else를 넣습니다. 다시 누르면 전체 작업이 일시정지/재개됩니다.
// if(IsFinished(slots[0])) {
//     slots[0]=Run(Repeat(Sequence({
//         Parallel({
//             PingPong(Move(0,{0,.5f,0},.0f,1.0f)),
//             PingPong(Rotate(0,Axis::Y,90,Pivot::Center,.0f,1.0f)),
//             PingPong(Scale(0,Axis::Y,.5f,Pivot::Center,.0f,1.0f)),
//             PingPong(Move(1,{0,.5f,0},.0f,1.0f)),
//             PingPong(Rotate(1,Axis::Y,-90,Pivot::Center,.0f,1.0f)),
//             PingPong(Scale(1,Axis::X,.5f,Pivot::Center,.0f,1.0f))
//         }),
//         Swap(0,1,.0f,2.0f,SwapMode::ArcXZ)
//     }),Forever));
// } else {
//     TogglePause(slots[0]);
// }
// 처음 1초: 두 객체가 위로 이동하며 반대 방향으로 자전하고 각자 지정 축으로 1.5배 신축합니다.
// 다음 1초: 이동·회전·크기가 복귀합니다. Center 기준이므로 이동 중에도 중심에서 변환됩니다.
// 다음 2초: 자세와 크기를 유지하며 서로의 위치로 반원 이동합니다. 한 반복은 총 4초입니다.
// 4초 뒤에는 서로의 자리에서 같은 동작을 반복하며, 8초 뒤 두 객체의 위치도 처음으로 돌아옵니다.
// Repeat 안에 Sequence를 넣었으므로 왕복 완료 후 스왑하며, Parallel 안의 변환은 동시에 시작합니다.
// slots[0]은 이 예시 전용으로 사용하세요. 다른 객체 번호로 바꾸면 Move/Rotate/Scale/Swap을 모두 바꾸세요.

// 12) 키를 누르는 동안만 실행: 누르면 시작/재개, 떼면 현재 상태에서 일시정지
// 아래 예시는 CustomKey가 아니라 KeyCallback 안의 if(action!=GLFW_PRESS)return; 바로 위에 넣습니다.
// 그 아래에 넣으면 GLFW_RELEASE가 걸러져 키를 떼어도 멈추지 않습니다.
// A와 slots[0]은 예시입니다. 기존 키/작업과 겹치지 않게 변경하세요.
// 특히 인형뽑기의 A는 이동키이므로 사용하지 않는 키를 선택해야 합니다.
// if(key==GLFW_KEY_A) {
//     if(action==GLFW_PRESS) {
//         if(IsFinished(slots[0])) {
//             slots[0]=Run(Repeat(
//                 Rotate(0,Axis::Y,360,Pivot::Center,.0f,4.0f),
//                 Forever
//             ));
//         } else {
//             Resume(slots[0]);
//         }
//     } else if(action==GLFW_RELEASE) {
//         Pause(slots[0]);
//     }
//     return; // 같은 키가 기존 CustomKey로 전달되어 중복 실행되는 것을 막습니다.
// }
// GLFW_REPEAT에서는 새 작업을 만들지 않습니다. 이미 등록한 작업이 갱신 함수에서 계속 실행됩니다.
// 다시 누르면 처음부터 시작하지 않고 멈춘 지점부터 이어집니다.
// 위 Run(...)을 11번의 복합 작업으로 바꾸면 그 작업 전체가 함께 정지/재개됩니다.
// 유한 작업이 끝난 후 다시 누르면 새 작업을 시작합니다. 무한 동작은 Repeat(...,Forever)를 사용합니다.
// return은 콜백의 중복 처리만 막습니다. 갱신 함수에서 같은 키를 glfwGetKey로 읽는 동작은 별도로 남습니다.


//
void CustomKey(int key)
{
    switch(key)
    {
    case GLFW_KEY_A:
        // a 명령
        if (IsFinished(slots[0])) {
            slots[0] = Run(Repeat(Parallel({
                PingPong(Scale(0, Axis::Y, +.5f, Pivot::Bottom, 1.0f)),
                PingPong(Scale(1, Axis::Y, +.5f, Pivot::Bottom, 1.0f)),
                PingPong(Move(2, {0, 0.05f, 0}, 1.0f)),
                PingPong(Move(3, {0, 0.05f, 0}, 1.0f)),
                PingPong(Move(4, {0, 0.05f, 0}, 1.0f))
            }), Forever));
        }
        else {
            TogglePause(slots[0]);
        }
        break;
    case GLFW_KEY_S:
        Run(Parallel({ Swap(3, 4, 1.0f, SwapMode::Direct) }));
        break;
    case GLFW_KEY_D:
        // d 명령
        break;
    case GLFW_KEY_F:
        // f 명령
        break;
    case GLFW_KEY_G:
        // g 명령
        break;
    case GLFW_KEY_H:
        // h 명령
        break;
    case GLFW_KEY_J:
        // j 명령
        break;
    case GLFW_KEY_K:
        // k 명령
        break;
    case GLFW_KEY_L:
        // l 명령
        break;

    // 키보드 윗줄의 숫자키입니다. 각 블록에서 Run 등을 한 번 호출할 수 있습니다.
    case GLFW_KEY_0:
    {
        // 숫자 0 명령
        break;
    }
    case GLFW_KEY_1:
    {
        // 숫자 1 명령
        break;
    }

    case GLFW_KEY_UP:
    {
        // 위 방향키 명령
        break;
    }
    case GLFW_KEY_DOWN:
    {
        // 아래 방향키 명령
        break;
    }
    case GLFW_KEY_LEFT:
    {
        // 왼쪽 방향키 명령
        break;
    }
    case GLFW_KEY_RIGHT:
    {
        // 오른쪽 방향키 명령
        break;
    }
    }
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
    // 이동키를 한 번 누르면 계속 걷습니다. 초기 상태는 정지입니다.
    if(key==GLFW_KEY_V){playerDirection={0,0,-1};playerWalking=true;}
    if(key==GLFW_KEY_B){playerDirection={0,0,1};playerWalking=true;}
    if(key==GLFW_KEY_N){playerDirection={-1,0,0};playerWalking=true;}
    if(key==GLFW_KEY_M){playerDirection={1,0,0};playerWalking=true;}
    if(key==GLFW_KEY_SPACE && boxes[PLAYER_BODY].enabled && !playerJumping)
    {
        PosePlayer();
        float feet=playerInitialFoot+playerOffset.y;
        float radius=0;
        glm::vec3 center(boxes[PLAYER_BODY].world[3]);
        for(int id=0;id<BOX_COUNT;++id)
            if(boxes[id].visible && IsPlayerPart(id))
            {
                auto [low,high]=BoxBounds(boxes[id].world);
                radius=glm::max(radius,glm::max(glm::max(center.x-low.x,high.x-center.x),glm::max(center.z-low.z,high.z-center.z)));
            }
        if(glm::abs(feet-PlayerSupport(center,radius,feet))<.001f)
        {
            playerJumping=true;playerJumpTime=0;playerJumpStart=feet;
        }
    }
    if(key==GLFW_KEY_R)ResetScene();
    if(key==GLFW_KEY_W)wireframe=!wireframe;
    if(key==GLFW_KEY_T)perspective=!perspective;
    if(key==GLFW_KEY_E)hiddenSurface=!hiddenSurface;
    if(key==GLFW_KEY_Y)
    {
        int direction=(mods&GLFW_MOD_SHIFT) ? 1 : -1;
        cameraOrbit=cameraOrbit==direction ? 0 : direction;
    }
    try { CustomKey(key); }
    catch(const exception& error) { cerr << "Command error: " << error.what() << endl; }
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
#elif MID_TEST_EXAMPLE == 1
// 인형뽑기 예제
//프로젝트속성->링커->명령줄에다음과같이3개의라이브러리추가
//-> opengl32.lib glew32.lib glfw3.lib glu32.lib

#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
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
#define CAMERA_X 2.9f
#define CAMERA_Y 2.75f
#define CAMERA_Z -5.9f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 1.05f
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
#define BACKGROUND_RED 1.0f
#define BACKGROUND_GREEN 1.0f
#define BACKGROUND_BLUE 1.0f

// 인형뽑기 크기와 동작 설정
#define CLAW_COUNT 2         // 몸체 둘레의 집게발 수: 2, 3, 4
#define CLAW_SEGMENT_COUNT 2  // 집게발 한 쪽의 육면체 수: 1, 2
#define CLAW_CEILING_Y 2.50f
#define CLAW_CEILING_THICKNESS .06f
#define CLAW_HEAD_HEIGHT .12f
#define CLAW_ROD_LENGTH .35f
#define CLAW_BAR_WIDTH .54f
#define CLAW_BAR_HEIGHT .10f
#define CLAW_UPPER_LENGTH .29f
#define CLAW_LOWER_LENGTH .28f
#define CLAW_JAW_WIDTH .065f
#define CLAW_SINGLE_LENGTH .55f
#define CLAW_SINGLE_OPEN_ANGLE 16.0f
#define CLAW_SINGLE_CLOSED_ANGLE -17.0f
#define CLAW_DOUBLE_OPEN_ANGLE 24.0f
#define CLAW_ELBOW_ANGLE 36.0f
#define CLAW_TIP_CLEARANCE .012f
#define CLAW_MOVE_SPEED .95f
#define CLAW_EDGE_MARGIN .60f
#define CLAW_DROP_X (FLOOR_WIDTH*.5f-CLAW_EDGE_MARGIN)
#define CLAW_DROP_Z (-FLOOR_DEPTH*.5f+CLAW_EDGE_MARGIN)
#define CLAW_LOWER_SECONDS 1.35f
#define CLAW_CLOSE_SECONDS .40f
#define CLAW_LIFT_SECONDS 1.15f
#define CLAW_SETTLE_SECONDS .42f
#define CLAW_RELEASE_SECONDS .40f
#define CLAW_ROD_LAG_SECONDS .22f
#define CLAW_ROD_LAG_ANGLE 13.0f
#define CLAW_BODY_LAG_SECONDS .32f
#define CLAW_BODY_LAG_ANGLE 4.0f
#define PRIZE_SIZE .20f
#define PRIZE_COUNT 6
#define PRIZE_GRAVITY 6.0f

// 집게발마다 두 칸을 예약해 개수를 바꾸더라도 경품 번호는 11번부터 시작합니다.
enum ClawPart { CLAW_HEAD, CLAW_ROD, CLAW_BODY, CLAW_FIRST_FINGER };
constexpr int CLAW_MAX_COUNT=4;
constexpr int PRIZE_FIRST=CLAW_FIRST_FINGER+CLAW_MAX_COUNT*2;
constexpr float CLAW_FIRST_LENGTH=CLAW_SEGMENT_COUNT==2 ? CLAW_UPPER_LENGTH : CLAW_SINGLE_LENGTH;
constexpr float CLAW_OPEN_ANGLE=CLAW_SEGMENT_COUNT==2 ? CLAW_DOUBLE_OPEN_ANGLE : CLAW_SINGLE_OPEN_ANGLE;
constexpr float CLAW_CLOSED_ANGLE=CLAW_SEGMENT_COUNT==2 ? 0.0f : CLAW_SINGLE_CLOSED_ANGLE;
constexpr float CLAW_BAR_DEPTH=CLAW_COUNT==2 ? .14f : CLAW_BAR_WIDTH;
const float CLAW_AZIMUTH_STEP=glm::two_pi<float>()/CLAW_COUNT;
static_assert(CLAW_COUNT>=2 && CLAW_COUNT<=4,"Use 2, 3, or 4 claws");
static_assert(CLAW_SEGMENT_COUNT==1 || CLAW_SEGMENT_COUNT==2,"Use 1 or 2 segments per claw");

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
};
enum class Orientation { Inherit, KeepUpright, KeepWorld }; // 부모 회전 전체 / Y축 방향만 / 자식 자체 방향만
struct Attachment
{
    bool enabled=false;
    Pivot parentPoint=Pivot::Center,childPoint=Pivot::Center;
    Orientation orientation=Orientation::Inherit;
    glm::vec3 offset{0}; // Move로 바꿀 수 있는 부모 기준 연결점의 이동량
};
struct FollowLagState
{
    float seconds=0,maxAngle=0;
    Pivot hinge=Pivot::Top;
    bool sampled=false;
    glm::mat4 previousParent{1};
    glm::vec3 trailingOffset{0};
};
struct BoxState
{
    bool enabled=false,visible=false;
    glm::mat4 model{1},world{1},animation{1}; // model: 부모 기준 변환, world: 상속을 합성한 조립 공간 변환
    glm::vec3 color{.5f},baseSize{1};
    int parent=-1;
    bool inheritScale=false;
    Attachment attachment;
    FollowLagState followLag;
};

// ============================================================================
// 수정 영역 ①: 번호 0~39의 활성화, 전체 크기, 중심 좌표, RGB 색상, 초기 회전각
// {활성화,{크기},{위치},색상,{초기각도}}만 지정합니다.
// 부모/자식 관계와 신축 상속은 ResetScene의 Attach에서 설정합니다.
// 연결점 생략형 Attach에서는 위치가 부모 기준 상대 좌표입니다.
// 연결점 지정형 Attach에서는 두 연결점이 맞닿도록 위치를 계산합니다.
// true인 객체만 표시됩니다. 크기/좌표/색상은 아래 한 곳에서만 설정합니다.
// 초기 회전각은 {X각,Y각,Z각}, 기본 {0,0,0}입니다. 크기는 양수로 지정합니다.
// 색상 인자 예: { .9f,.2f,.2f } / Color::Random() / Color::Random(1)
// Random(1)을 사용한 객체는 모두 같은 랜덤 색입니다. 다른 그룹은 Random(2) 등으로 지정합니다.
// r 리셋 시 랜덤 색은 다시 추첨하며 같은 그룹끼리는 계속 같은 색을 공유합니다.
// ============================================================================
const array<BoxSetting,BOX_COUNT> boxSettings={{
    {true,{.26f,CLAW_HEAD_HEIGHT,.26f},{0,CLAW_CEILING_Y-CLAW_HEAD_HEIGHT*.5f,0},{.15f,.31f,.48f}}, // 이동부
    {true,{.085f,CLAW_ROD_LENGTH,.085f},{0,0,0},{.38f,.48f,.57f}}, // 세로 막대
    {true,{CLAW_BAR_WIDTH,CLAW_BAR_HEIGHT,CLAW_BAR_DEPTH},{0,0,0},{.18f,.34f,.50f}}, // 집게 몸체
    {true,{CLAW_JAW_WIDTH,CLAW_FIRST_LENGTH,.09f},{0,0,0},{.97f,.57f,.17f}}, // 집게발 1 첫 마디
    {CLAW_SEGMENT_COUNT==2,{CLAW_JAW_WIDTH,CLAW_LOWER_LENGTH,.09f},{0,0,0},{.96f,.40f,.10f},{0,0,-CLAW_ELBOW_ANGLE}}, // 집게발 1 끝 마디
    {true,{CLAW_JAW_WIDTH,CLAW_FIRST_LENGTH,.09f},{0,0,0},{.97f,.57f,.17f}}, // 집게발 2 첫 마디
    {CLAW_SEGMENT_COUNT==2,{CLAW_JAW_WIDTH,CLAW_LOWER_LENGTH,.09f},{0,0,0},{.96f,.40f,.10f},{0,0,-CLAW_ELBOW_ANGLE}}, // 집게발 2 끝 마디
    {CLAW_COUNT>=3,{CLAW_JAW_WIDTH,CLAW_FIRST_LENGTH,.09f},{0,0,0},{.97f,.57f,.17f}}, // 집게발 3 첫 마디
    {CLAW_COUNT>=3 && CLAW_SEGMENT_COUNT==2,{CLAW_JAW_WIDTH,CLAW_LOWER_LENGTH,.09f},{0,0,0},{.96f,.40f,.10f},{0,0,-CLAW_ELBOW_ANGLE}}, // 집게발 3 끝 마디
    {CLAW_COUNT>=4,{CLAW_JAW_WIDTH,CLAW_FIRST_LENGTH,.09f},{0,0,0},{.97f,.57f,.17f}}, // 집게발 4 첫 마디
    {CLAW_COUNT>=4 && CLAW_SEGMENT_COUNT==2,{CLAW_JAW_WIDTH,CLAW_LOWER_LENGTH,.09f},{0,0,0},{.96f,.40f,.10f},{0,0,-CLAW_ELBOW_ANGLE}}, // 집게발 4 끝 마디
    {true,glm::vec3(PRIZE_SIZE),{0,PRIZE_SIZE*.5f,0},{.87f,.25f,.35f}},
    {true,glm::vec3(PRIZE_SIZE),{-.92f,PRIZE_SIZE*.5f,-.61f},{.26f,.61f,.87f}},
    {true,glm::vec3(PRIZE_SIZE),{.88f,PRIZE_SIZE*.5f,.50f},{.24f,.73f,.52f}},
    {true,glm::vec3(PRIZE_SIZE),{-.53f,PRIZE_SIZE*.5f,.92f},{.68f,.40f,.87f}},
    {true,glm::vec3(PRIZE_SIZE),{.49f,PRIZE_SIZE*.5f,-.76f},{.95f,.71f,.22f}},
    {true,glm::vec3(PRIZE_SIZE),{-1.24f,PRIZE_SIZE*.5f,.45f},{.96f,.45f,.20f}},
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
    float delay=0,remainingDelay=0;
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

// 연결점 지정형: Attach(부모,자식,부모연결점,자식연결점,방향상속,신축상속).
// 방향상속 기본값은 Orientation::Inherit, 신축상속 기본값은 false입니다.
// false는 자식 크기 유지, true는 부모 신축 상속입니다. 연결 위치는 두 경우 모두 부모를 따릅니다.
// 예: Attach(0,1,Pivot::Bottom,Pivot::Top,Orientation::Inherit,true);
// 신축상속을 지정하려면 앞의 방향상속도 작성합니다. 부모 관계는 boxSettings에 쓰지 않습니다.
// Pivot::Top/Bottom 등은 각 객체의 면 중심, Point(x,y,z)는 해당 객체 중심 기준 길이입니다.
// 연결점이 맞닿도록 중심을 계산합니다. Move는 부모 기준 연결점 오프셋을 옮기고,
// 자식 Rotate/Scale은 자신의 변환을 적용한 뒤 연결점을 다시 맞춥니다.
void Attach(int parent,int child,Pivot parentPoint,Pivot childPoint,Orientation orientation=Orientation::Inherit,bool inheritScale=false)
{
    if(child<0 || child>=BOX_COUNT || parent<0 || parent>=BOX_COUNT)
        throw runtime_error("Attachment box ID must be 0~39");
    for(int id=parent,depth=0;id!=-1;++depth)
    {
        if(id==child || depth>=BOX_COUNT)throw runtime_error("Attachment would create a cycle");
        if(id<0 || id>=BOX_COUNT)throw runtime_error("Parent ID must be -1 or 0~39");
        id=boxes[id].parent;
    }
    auto& box=boxes[child];
    box.parent=parent;
    box.inheritScale=inheritScale;
    box.attachment={true,parentPoint,childPoint,orientation};
    box.followLag.sampled=false;
    box.followLag.trailingOffset=glm::vec3(0);
}

// 연결점 생략형: Attach(부모,자식,신축상속=false).
// 예: Attach(0,1); / Attach(0,1,true);
// 현재 model의 위치를 부모 기준 상대 위치로 해석하므로 연결 순간 월드 위치가 바뀔 수 있습니다.
// 초기화 직후에는 boxSettings의 center가 상대 위치입니다. 부모 회전은 전체 상속합니다.
// 면을 고정하지 않으므로 기존 몸통/팔다리의 상대 이동과 관절 회전을 유지합니다.
void Attach(int parent,int child,bool inheritScale=false)
{
    Attach(parent,child,Pivot::Center,Pivot::Center,Orientation::Inherit,inheritScale);
    boxes[child].attachment.enabled=false;
}

// FollowLag(자식,연결점,반응 시간(초),최대 기울기(도)). 0도를 지정하면 효과를 끕니다.
// 연결점 지정형 Attach는 자식 연결점을 축으로 사용하고, 생략형은 FollowLag의 연결점을 사용합니다.
// 연결점은 부모를 즉시 따르고,
// 몸체는 움직임 반대쪽으로 기울다가 복귀합니다. 별도 Run이나 키 입력 처리가 필요 없습니다.
void FollowLag(int child,Pivot hinge=Pivot::Top,float seconds=.25f,float maxAngle=25.0f)
{
    if(child<0 || child>=BOX_COUNT)throw runtime_error("FollowLag box ID must be 0~39");
    if(!std::isfinite(seconds) || seconds<=0 || !std::isfinite(maxAngle) || maxAngle<0 || maxAngle>=90)
        throw runtime_error("FollowLag requires positive seconds and 0 <= angle < 90");
    if(maxAngle>0 && boxes[child].parent==-1)throw runtime_error("FollowLag requires a parent");
    auto& lag=boxes[child].followLag;
    lag={seconds,maxAngle,hinge};
}

glm::vec4 AttachmentPoint(const BoxState& box,Pivot point)
{
    return glm::vec4(point.point ? point.value/box.baseSize : point.value,1);
}

// 카메라와 무관한 조립 공간에서 부모가 옮긴 만큼 자식의 상대 지연량을 누적하고 감쇠합니다.
void ApplyFollowLag(BoxState& box,const glm::mat4& parentFrame,float dt)
{
    auto& lag=box.followLag;
    if(lag.maxAngle<=0 || !box.visible)return;
    glm::mat4 frame=parentFrame;
    frame[3]+=glm::vec4(assemblyPosition,0);
    if(!lag.sampled)
    {
        lag.previousParent=frame;
        lag.sampled=true;
    }
    else if(dt>0)
    {
        // 이전과 현재의 부모 프레임에 같은 점을 넣어 자식 자체의 이동은 지연의 원인에서 제외합니다.
        glm::vec4 sample=glm::inverse(parentFrame)*box.world[3];
        glm::vec3 parentDelta(frame*sample-lag.previousParent*sample);
        lag.trailingOffset=(lag.trailingOffset-parentDelta)*std::exp(-dt/lag.seconds);
        lag.previousParent=frame;
    }

    Pivot hinge=box.attachment.enabled ? box.attachment.childPoint : lag.hinge;
    glm::vec3 pivot(box.world*AttachmentPoint(box,hinge));
    glm::vec3 arm=glm::vec3(box.world[3])-pivot;
    float length=glm::length(arm);
    if(length<.00001f)return; // 연결점이 중심이면 기울일 방향을 정할 수 없습니다.
    glm::vec3 axis=arm/length;
    glm::vec3 sideways=lag.trailingOffset-axis*glm::dot(lag.trailingOffset,axis);
    float distance=glm::length(sideways);
    if(distance<.00001f)return;
    float angle=glm::min(glm::atan(distance,length),glm::radians(lag.maxAngle));
    glm::mat4 swing=glm::translate(glm::mat4(1),pivot);
    swing=glm::rotate(swing,angle,glm::normalize(glm::cross(axis,sideways)));
    box.world=glm::translate(swing,-pivot)*box.world;
}

// EvaluateHierarchy: 부모를 먼저 계산합니다. dt가 양수일 때만 따라오는 효과의 시간을 갱신합니다.
void EvaluateHierarchy(float dt=0)
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
            Orientation orientation=box.attachment.orientation;
            if(box.inheritScale && orientation==Orientation::Inherit)
                box.world=frame*local;
            else
            {
                // 크기는 제거하고 회전만 추출합니다. 연결 위치는 신축된 부모를 따라갑니다.
                glm::vec3 x=glm::normalize(glm::vec3(frame[0]));
                glm::vec3 y=glm::vec3(frame[1])-x*glm::dot(x,glm::vec3(frame[1]));
                y=glm::normalize(y);
                glm::vec3 z=glm::normalize(glm::cross(x,y));
                glm::mat4 rotation(1);
                if(orientation==Orientation::Inherit)
                {
                    rotation[0]=glm::vec4(x,0);rotation[1]=glm::vec4(y,0);rotation[2]=glm::vec4(z,0);
                }
                else if(orientation==Orientation::KeepUpright)
                {
                    glm::vec3 horizontal(z.x,0,z.z);
                    if(glm::length(horizontal)<.00001f)horizontal={-x.z,0,x.x};
                    horizontal=glm::normalize(horizontal);
                    rotation[0]=glm::vec4(horizontal.z,0,-horizontal.x,0);
                    rotation[2]=glm::vec4(horizontal,0);
                }
                // 회전을 제한할 때 크기 상속이 켜져 있으면 부모의 축별 크기 변화만 적용합니다.
                if(box.inheritScale)
                    rotation=glm::scale(rotation,glm::vec3(glm::length(glm::vec3(frame[0])),
                        glm::length(glm::vec3(frame[1])),glm::length(glm::vec3(frame[2]))));
                box.world=rotation*local;
                box.world[3]=frame*glm::vec4(glm::vec3(local[3]),1);
            }
            if(box.attachment.enabled)
            {
                const auto& attachment=box.attachment;
                glm::vec4 target=parent.world*AttachmentPoint(parent,attachment.parentPoint)
                    +frame*glm::vec4(attachment.offset,0);
                glm::vec4 current=box.world*AttachmentPoint(box,attachment.childPoint);
                box.world[3]+=target-current;
            }
            ApplyFollowLag(box,frame,dt);
        }
        state[id]=2;
    };
    for(int i=0;i<BOX_COUNT;++i)evaluate(evaluate,i);
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

// 끝에 시간 하나를 쓰면 실행 시간, 두 개를 쓰면 대기 시간과 실행 시간입니다. 단위는 초입니다.
// Move(3,{1,0,0},.0f,2.0f): 즉시 시작해 2초간 이동합니다.
// 대기 시간은 .0f로 두면 변환을 바로 시작합니다.
Command Move(int id,glm::vec3 delta,float delay,float seconds)
{
    Command c;c.kind=Kind::Move;c.id=id;c.delta=delta;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Move(int id,glm::vec3 delta,float seconds=0)
{
    return Move(id,delta,0,seconds);
}
// Rotate(번호,축,각도,기준점,대기 시간,실행 시간): -360~+360도. 기준점 생략 시 자기 중심.
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
//    시간을 지정하려면 기준점도 적습니다. PingPong의 한 주기는 대기 시간 + 편도 실행 시간의 두 배입니다.
// 예: Rotate(0,Axis::X,90,Pivot::Center,.0f,.0f) -> 현재 중심에서 고정 X축 방향으로 +90도 자전합니다.
// 예: Rotate(0,Axis::Y,-180,Point(0,0,0),.0f,2.0f) -> 원점을 지나는 Y축 주위로 2초 동안 -180도 공전합니다.
// 예: Rotate(0,Axis::Z,45,Pivot::Top,.0f,1.0f) -> 시작 시 윗면 중심을 지나는 Z축 주위로 1초 동안 +45도 회전합니다.
// Rotate(3,Axis::Y,90,Pivot::Center,.0f,2.0f): 대기 없이 2초간 회전합니다.
Command Rotate(int id,Axis axis,float degrees,Pivot pivot,float delay,float seconds)
{
    Command c;c.kind=Kind::Rotate;c.id=id;c.axis=axis;c.amount=degrees;c.pivot=pivot;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Rotate(int id,Axis axis,float degrees,Pivot pivot=Pivot::Center,float seconds=0)
{
    return Rotate(id,axis,degrees,pivot,0,seconds);
}
// Scale(번호,축,배율증감,기준점,대기 시간,실행 시간): +0.5는 1.5배, -0.5는 0.5배. -1 이하는 거부.
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
// 예: Scale(0,Axis::X,.5f,Pivot::Center,.0f,.0f) -> 매 순간 현재 중심을 기준으로 X방향 크기를 현재의 1.5배로 늘립니다.
// 예: Run(Parallel({Move(0,{1,0,0},.0f,2.0f),Scale(0,Axis::X,.5f,Pivot::Center,.0f,2.0f)}));
//     2초 동안 X방향으로 1만큼 이동하며 X방향 크기를 1.5배로 늘립니다. 신축은 이동 거리를 바꾸지 않습니다.
// 예: Scale(0,Axis::Y,-.5f,Pivot::Bottom,.0f,2.0f) -> 아래 면 중심을 기준으로 2초 동안 Y방향 크기를 절반으로 줄입니다.
// 예: Scale(0,Axis::X,1.0f,Point(0,0,0),.0f,1.0f) -> 1초 동안 X방향 크기와 원점에서의 X방향 거리를 두 배로 만듭니다.
// Scale(3,Axis::Y,.5f,Pivot::Bottom,.0f,2.0f): 대기 없이 2초간 신축합니다.
Command Scale(int id,Axis axis,float change,Pivot pivot,float delay,float seconds)
{
    Command c;c.kind=Kind::Scale;c.id=id;c.axis=axis;c.amount=change;c.pivot=pivot;c.seconds=seconds;
    c.delay=c.remainingDelay=delay;
    return c;
}
Command Scale(int id,Axis axis,float change,Pivot pivot=Pivot::Center,float seconds=0)
{
    return Scale(id,axis,change,pivot,0,seconds);
}
// 변환의 마지막 인자는 편도 시간(초)입니다. 생략하거나 0이면 기존 속도로 계산합니다.
// PingPong은 처음 한 번 대기한 뒤 편도 시간만큼 갔다가 즉시 돌아옵니다.
// Repeat로 감싸면 새 반복을 시작할 때 대기 시간도 다시 적용됩니다.
// Swap(번호1,번호2,대기 시간,실행 시간,방식): 지정한 초 동안 두 중심 위치를 교환합니다. 방식 생략 시 직선입니다.
// 회전과 크기는 유지합니다. 서로 다른 활성 객체와 양수 시간을 지정합니다.
// ArcXZ는 중점 주위 XZ 반원, DetourXZ/DetourXY는 각 평면의 ㄷ자 경로입니다.
// ㄷ자 우회 거리는 두 중심 거리의 절반이며 두 객체는 서로 반대쪽으로 우회합니다.
// Swap(번호1,번호2,대기 시간,실행 시간,방식)으로 교환을 늦게 시작할 수 있습니다.
Command Swap(int first,int second,float delay,float seconds,SwapMode mode=SwapMode::Direct)
{
    Command c;c.kind=Kind::Swap;c.seconds=seconds;c.swapMode=mode;
    c.delay=c.remainingDelay=delay;
    c.children={Move(first,glm::vec3(0),seconds),Move(second,glm::vec3(0),seconds)};
    return c;
}
Command Swap(int first,int second,float seconds,SwapMode mode=SwapMode::Direct)
{
    return Swap(first,second,0,seconds,mode);
}
// Parallel({명령,...}): 각 명령의 대기 시간은 함께 흐르며, 모든 명령이 끝나야 완료합니다.
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
    if(!isfinite(c.delay) || c.delay<0)throw runtime_error("Delay must be finite and nonnegative");
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
        if(first.attachment.enabled || second.attachment.enabled)
            throw runtime_error("Swap requires boxes without Attach; use Move to slide an attachment");
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
    c.remainingDelay=c.delay;
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
    if(c.remainingDelay>0)
    {
        // 대기를 끝내고 남은 시간으로 변환을 시작합니다. 기준점도 이때 기록합니다.
        float waiting=glm::min(dt,c.remainingDelay);
        c.remainingDelay-=waiting;
        dt-=waiting;
        if(c.remainingDelay>0)return 0;
    }
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
    if(box.enabled && c.kind==Kind::Move && box.attachment.enabled)
        box.attachment.offset+=c.delta*(value-c.previous);
    else if(box.enabled)
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
// 명령 설정이 키 입력과 매 갱신 시점을 함께 받을 수 있게 연결합니다.
void CustomKey(GLFWwindow* window,int key,float dt=0);

int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Midterm 2 - Claw Machine", nullptr, nullptr);
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
    cout << "Hold arrows: move claw | Space: grab and deliver\n"
         << "x/X,z/Z: camera | y/Y: orbit | w: wire | t: projection | e: hidden surfaces\n"
         << "r: reset | q: quit | drag/wheel: camera" << endl;
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
        boxes[i].parent=-1;
        boxes[i].inheritScale=false;
        boxes[i].attachment={};
        boxes[i].followLag={};
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
    // 수정 영역 ①-추가: 부모 관계는 아래 Attach에서만 설정합니다.
    // Attach(부모,자식,부모연결점,자식연결점,Orientation::Inherit,false);
    // 마지막 true는 부모 신축 상속, false 또는 생략은 자식 크기 유지입니다.
    // 연결점 없이 상대 위치로 배치하려면 Attach(부모,자식,신축상속)을 사용합니다.
    Attach(CLAW_HEAD,CLAW_ROD,Pivot::Bottom,Pivot::Top);
    Attach(CLAW_ROD,CLAW_BODY,Pivot::Bottom,Pivot::Top);
    for(int finger=0;finger<CLAW_COUNT;++finger)
    {
        int upper=CLAW_FIRST_FINGER+2*finger;
        float azimuth=glm::pi<float>()+CLAW_AZIMUTH_STEP*finger;
        glm::vec3 outward(glm::cos(azimuth),0,glm::sin(azimuth));
        glm::vec3 hinge=outward*(CLAW_BAR_WIDTH*.5f)+glm::vec3(0,-CLAW_BAR_HEIGHT*.5f,0);
        Attach(CLAW_BODY,upper,Point(hinge.x,hinge.y,hinge.z),Pivot::Top);
        if(CLAW_SEGMENT_COUNT==2)
            Attach(upper,upper+1,Pivot::Bottom,Pivot::Top);

        // 로컬 +X가 바깥 방향을 보게 한 뒤 기본 닫힘 각도를 적용합니다.
        glm::mat4 facing=glm::rotate(glm::mat4(1),-azimuth,glm::vec3(0,1,0));
        boxes[upper].model=glm::rotate(facing,glm::radians(CLAW_CLOSED_ANGLE),glm::vec3(0,0,1))
            *boxes[upper].model;
    }
    // 천장 이동부 → 막대 → 집게 몸체에 각각 지연을 적용합니다.
    FollowLag(CLAW_ROD,Pivot::Top,CLAW_ROD_LAG_SECONDS,CLAW_ROD_LAG_ANGLE);
    FollowLag(CLAW_BODY,Pivot::Top,CLAW_BODY_LAG_SECONDS,CLAW_BODY_LAG_ANGLE);
    CustomKey(nullptr,GLFW_KEY_R);
    EvaluateHierarchy();
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
        CustomKey(window,GLFW_KEY_UNKNOWN,step);
        EvaluateHierarchy(step);
    }
    // 카메라 로컬 +X는 오른쪽, 로컬 +Z는 뒤쪽입니다. Shift는 음의 방향입니다.
    glm::vec3 forward=glm::normalize(cameraTarget-cameraPosition);
    glm::vec3 right=glm::normalize(glm::cross(forward,glm::vec3(0,1,0)));
    bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS
        || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    float direction=shift ? -1.0f : 1.0f;
    glm::vec3 cameraMovement(0);
    if(glfwGetKey(window,GLFW_KEY_X)==GLFW_PRESS)cameraMovement+=right*direction;
    if(glfwGetKey(window,GLFW_KEY_Z)==GLFW_PRESS)cameraMovement-=forward*direction;
    if(glm::length(cameraMovement)>0)
    {
        cameraMovement=glm::normalize(cameraMovement)*CAMERA_MOVE_SPEED*dt;
        cameraPosition+=cameraMovement;cameraTarget+=cameraMovement;
    }
    if(cameraOrbit)
    {
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(cameraOrbit*CAMERA_ROTATE_SPEED*dt),glm::vec3(0,1,0));
        cameraPosition=glm::vec3(rotation*glm::vec4(cameraPosition,1));
        cameraTarget=glm::vec3(rotation*glm::vec4(cameraTarget,1));
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
            {FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH},{.82f,.88f,.94f});
    glm::mat4 assembly=glm::translate(glm::mat4(1),assemblyPosition);
    glPolygonMode(GL_FRONT_AND_BACK,wireframe ? GL_LINE : GL_FILL);
    for(const auto& box:boxes)
        if(box.visible)DrawBox(assembly*box.world,glm::vec3(1),box.color);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    // 낙하 구역을 표시하고, 안쪽 구조가 보이도록 천장은 반투명으로 그립니다.
    DrawBox(glm::translate(glm::mat4(1),{CLAW_DROP_X,.004f,CLAW_DROP_Z}),
        {.74f,.008f,.74f},{.59f,.80f,.91f});
    glEnable(GL_BLEND);
    glBlendColor(0,0,0,.22f);
    glBlendFunc(GL_CONSTANT_ALPHA,GL_ONE_MINUS_CONSTANT_ALPHA);
    glDepthMask(GL_FALSE);
    DrawBox(glm::translate(glm::mat4(1),{0,CLAW_CEILING_Y+CLAW_CEILING_THICKNESS*.5f,0}),
        {FLOOR_WIDTH,CLAW_CEILING_THICKNESS,FLOOR_DEPTH},{.30f,.53f,.68f});
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

// ============================================================================
// 수정 영역 ②: 키 입력과 단계별 변환 설정
// GLFW_KEY_UNKNOWN은 일반 키 대신 고정 시간 갱신을 뜻합니다.
// [키 입력에서 사용하는 변환 함수 요약]
// 아래 번호는 boxSettings의 활성 객체 번호(0~39)입니다. 시간 단위는 초입니다.
// Move/Rotate/Scale/Swap은 명령을 만들며, Run(명령)을 호출해야 실행됩니다.
// Attach/FollowLag는 설정 함수이므로 Run으로 감싸지 않습니다.
//
// 1) 이동: Run(Move(번호,{X이동량,Y이동량,Z이동량},대기 시간,실행 시간));
//    Run(Move(0,{1,0,0},.0f,2.0f)); // 현재 위치에서 +X로 1만큼, 2초 동안 이동
//    좌표는 목적지가 아니라 상대 이동량입니다. 부모가 있으면 부모 좌표계 기준입니다.
//    연결점 지정형 Attach는 부모 연결점 offset을 이동하고, 생략형은 부모 기준 상대 위치를 이동합니다.
//
// 2) 회전: Run(Rotate(번호,축,각도,기준점,대기 시간,실행 시간));
//    Run(Rotate(0,Axis::Y,90,Pivot::Center,.0f,2.0f)); // 현재 중심에서 90도 자전
//    Run(Rotate(0,Axis::Y,180,Point(0,0,0),.0f,2.0f)); // 조립 공간 원점 주위 공전
//    각도는 현재 상태에 추가하는 도 단위 값(-360~360), 양수는 오른손 법칙입니다.
//    Axis::X/Y/Z는 조립 공간의 고정 축이며, 부모가 있으면 부모 좌표계의 축입니다.
//    Center는 이동 중인 중심을 계속 따라갑니다. 객체 자체의 회전된 로컬 축은 아닙니다.
//
// 3) 신축: Run(Scale(번호,축,배율증감,기준점,대기 시간,실행 시간));
//    Run(Scale(0,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f)); // 아래 면 기준 높이를 1.5배
//    최종 배율 = 1 + 배율증감: .5는 1.5배, 1은 2배, -.5는 절반. -1 이하는 금지.
//    Center는 현재 중심을 따라가므로 이동과 함께 써도 신축 때문에 중심이 밀리지 않습니다.
//    축은 회전과 마찬가지로 조립/부모 공간 기준입니다. 기울어진 객체는 비스듬히 변형될 수 있습니다.
//
// 4) 기준점: Pivot::Center/Left/Right/Bottom/Top/Front/Back 또는 Point(x,y,z)
//    Front는 -Z 면, Back은 +Z 면입니다. Center 생략 가능, 기본값은 Center입니다.
//    회전/신축의 면 기준점은 시작할 때 계산하여 고정합니다. Center만 현재 중심을 따라갑니다.
//    변환의 Point는 조립/부모 공간 좌표입니다. 조립 공간 전체 이동 시 함께 움직입니다.
//    시간을 지정할 때는 Rotate/Scale의 기준점 인자도 반드시 작성합니다.
//    대기 시간 .0f는 즉시 시작입니다. Move/Rotate/Scale의 실행 시간 .0f는 기본 속도로 계산합니다.
//
// 5) 연결: Attach(부모번호,자식번호,부모연결점,자식연결점,방향상속,신축상속);
//    Attach(0,1,Pivot::Bottom,Pivot::Top); // 부모 아래와 자식 위를 맞붙임
//    Attach(0,1,Point(.2f,0,0),Pivot::Top,Orientation::KeepUpright);
//    Attach의 Point는 해당 객체 중심 기준 길이이며, 변환 함수의 Point와 의미가 다릅니다.
//    Orientation::Inherit: 부모 회전 전체 상속(기본값)
//    Orientation::KeepUpright: 부모의 Y축 방향만 상속, 기울기는 따라가지 않음
//    Orientation::KeepWorld: 부모 회전을 상속하지 않음. 자식 자체 회전은 유지
//    자식 Rotate/Scale 후에도 연결점을 다시 맞춥니다. 연결 순간 위치가 바뀔 수 있습니다.
//    Attach(0,1,Pivot::Bottom,Pivot::Top,Orientation::Inherit,true); // 부모 신축도 상속
//    마지막 bool 생략/false: 자식 크기 유지, true: 부모 신축도 상속
//    연결점 위치는 false여도 부모의 신축을 따라갑니다.
//    Attach(0,1,false); // 연결점 고정 없이 부모 관계만 설정, 기존 상대 위치 사용
//    Attach(0,1,true);  // 상대 위치 사용 + 부모 신축 상속
//    전체 이동/회전은 부모에만 주면 자식도 따라옵니다. 부모와 자식에 중복 적용하지 마세요.
//    초기 연결은 ResetScene에서 설정하고, 키 입력 중 Attach하면 그 시점에 연결됩니다.
//
// 6) 따라 흔들림: FollowLag(자식번호,연결점,반응시간,최대기울기);
//    FollowLag(1,Pivot::Top,.25f,25.0f); // 부모 움직임 반대쪽으로 기울다가 복귀
//    먼저 Attach로 부모를 지정합니다. 연결점 지정형은 Attach의 자식 연결점을 사용합니다.
//    연결점 생략형은 FollowLag의 연결점을 사용합니다. 예: Attach(0,1); FollowLag(1,Pivot::Top);
//    반응시간은 지연 감쇠 시간이며 완료 시간이 아닙니다. 클수록 천천히 복귀합니다.
//    최대기울기는 0 이상 90도 미만입니다. 연결점이 중심이면 기울기 효과는 없습니다.
//    최대기울기 0으로 효과를 끕니다. 물리 충돌 계산이 아닌 따라 흔들리는 효과입니다.
//
// 7) 위치 교환: Run(Swap(번호1,번호2,대기 시간,실행 시간,방식));
//    Run(Swap(0,1,.0f,2.0f,SwapMode::ArcXZ)); // 자세와 크기는 유지하며 위치 교환
//    Direct: 직선(기본), ArcXZ: XZ 반원, DetourXZ: XZ ㄷ자, DetourXY: XY ㄷ자
//    서로 다른 활성 객체, 같은 부모와 inheritScale 설정, 양수 시간이 필요합니다.
//    연결점 지정형 Attach는 Swap 불가입니다. 생략형은 같은 부모/신축상속 조건이면 가능합니다.
//
// 8) 조합: Parallel은 동시 실행, Sequence는 앞 명령 완료 후 다음 실행입니다.
//    Run(Parallel({Move(0,{1,0,0},.0f,2.0f),Rotate(0,Axis::Y,90,Pivot::Center,.0f,2.0f)}));
//    Run(Sequence({Move(0,{1,0,0},.0f,1.0f),Rotate(0,Axis::Y,90,Pivot::Center,.0f,1.0f)}));
//    같은 객체의 동시 변환은 하나의 Run(Parallel({...}))로 묶습니다.
//    다른 Run이 같은 객체를 사용 중이면 일시정지 상태라도 새 작업을 시작할 수 없습니다.
//
// 9) 반복/왕복: Repeat(명령,횟수), 무한반복은 Forever입니다.
//    Run(Repeat(Move(0,{.2f,0,0},.0f,1.0f),3)); // 총 +X .6 이동: 상대 변환이 누적됨
//    Run(Repeat(PingPong(Move(0,{1,0,0},.0f,1.0f)),Forever)); // 1초 이동 + 1초 복귀
//    PingPong에는 Move/Rotate/Scale 한 개만 넣습니다. Parallel/Swap은 직접 넣지 않습니다.
//    두 객체가 같은 주기로 신축: 각각 편도 1초, 왕복 2초로 맞춥니다.
//    slots[0]=Run(Repeat(Parallel({
//        PingPong(Scale(0,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f)),
//        PingPong(Scale(1,Axis::Y,.5f,Pivot::Bottom,.0f,1.0f))
//    }),Forever));
//    Repeat는 시간을 자동으로 맞추지 않습니다. 각 변환의 시간을 같게 지정하세요.
//    스왑 반복: slots[0]=Run(Repeat(Swap(0,1,.0f,2.0f,SwapMode::ArcXZ),Forever));
//    신축만 Repeat하면 계속 커집니다. 크기 복귀가 필요하면 PingPong을 사용하세요.
//
// 10) 키를 다시 누르면 일시정지/재개하는 기본 형태:
//    if(IsFinished(slots[0]))
//        slots[0]=Run(Repeat(PingPong(Move(0,{1,0,0},.0f,1.0f)),Forever));
//    else TogglePause(slots[0]);
//    slots에는 객체 번호가 아니라 Run이 반환한 작업 번호를 저장합니다.
//    Pause/Resume/TogglePause/Cancel은 작업 전체에 적용합니다.
//    Cancel(slots[0])은 현재 모습을 유지하며 작업만 종료합니다. 초기화는 r입니다.
//    키마다 slots[0]~slots[8]을 나누되, 같은 객체의 작업 충돌은 별도로 피해야 합니다.
//    일반 키는 누를 때 한 번 실행됩니다. 누르는 동안의 이동은 갱신 함수에서 처리합니다.

// 11) 복합 예시: 이동 + 자전 + 신축 왕복 → XZ 반원 스왑 → 전체 무한반복
// 전제: 0번과 1번은 활성화된 독립 객체이며, 다른 작업에서 사용하고 있지 않아야 합니다.
// 인형뽑기 모드의 0/1번은 이미 연결되어 있으므로, 이 예시에는 별도의 독립 객체 번호를 넣으세요.
// 원하는 키 처리 안에 아래 if/else를 넣습니다. 다시 누르면 전체 작업이 일시정지/재개됩니다.
// if(IsFinished(slots[0])) {
//     slots[0]=Run(Repeat(Sequence({
//         Parallel({
//             PingPong(Move(0,{0,.5f,0},.0f,1.0f)),
//             PingPong(Rotate(0,Axis::Y,90,Pivot::Center,.0f,1.0f)),
//             PingPong(Scale(0,Axis::Y,.5f,Pivot::Center,.0f,1.0f)),
//             PingPong(Move(1,{0,.5f,0},.0f,1.0f)),
//             PingPong(Rotate(1,Axis::Y,-90,Pivot::Center,.0f,1.0f)),
//             PingPong(Scale(1,Axis::X,.5f,Pivot::Center,.0f,1.0f))
//         }),
//         Swap(0,1,.0f,2.0f,SwapMode::ArcXZ)
//     }),Forever));
// } else {
//     TogglePause(slots[0]);
// }
// 처음 1초: 두 객체가 위로 이동하며 반대 방향으로 자전하고 각자 지정 축으로 1.5배 신축합니다.
// 다음 1초: 이동·회전·크기가 복귀합니다. Center 기준이므로 이동 중에도 중심에서 변환됩니다.
// 다음 2초: 자세와 크기를 유지하며 서로의 위치로 반원 이동합니다. 한 반복은 총 4초입니다.
// 4초 뒤에는 서로의 자리에서 같은 동작을 반복하며, 8초 뒤 두 객체의 위치도 처음으로 돌아옵니다.
// Repeat 안에 Sequence를 넣었으므로 왕복 완료 후 스왑하며, Parallel 안의 변환은 동시에 시작합니다.
// slots[0]은 이 예시 전용으로 사용하세요. 다른 객체 번호로 바꾸면 Move/Rotate/Scale/Swap을 모두 바꾸세요.

// 12) 키를 누르는 동안만 실행: 누르면 시작/재개, 떼면 현재 상태에서 일시정지
// 아래 예시는 CustomKey가 아니라 KeyCallback 안의 if(action!=GLFW_PRESS)return; 바로 위에 넣습니다.
// 그 아래에 넣으면 GLFW_RELEASE가 걸러져 키를 떼어도 멈추지 않습니다.
// A와 slots[0]은 예시입니다. 기존 키/작업과 겹치지 않게 변경하세요.
// 특히 인형뽑기의 A는 이동키이므로 사용하지 않는 키를 선택해야 합니다.
// if(key==GLFW_KEY_A) {
//     if(action==GLFW_PRESS) {
//         if(IsFinished(slots[0])) {
//             slots[0]=Run(Repeat(
//                 Rotate(0,Axis::Y,360,Pivot::Center,.0f,4.0f),
//                 Forever
//             ));
//         } else {
//             Resume(slots[0]);
//         }
//     } else if(action==GLFW_RELEASE) {
//         Pause(slots[0]);
//     }
//     return; // 같은 키가 기존 CustomKey로 전달되어 중복 실행되는 것을 막습니다.
// }
// GLFW_REPEAT에서는 새 작업을 만들지 않습니다. 이미 등록한 작업이 갱신 함수에서 계속 실행됩니다.
// 다시 누르면 처음부터 시작하지 않고 멈춘 지점부터 이어집니다.
// 위 Run(...)을 11번의 복합 작업으로 바꾸면 그 작업 전체가 함께 정지/재개됩니다.
// 유한 작업이 끝난 후 다시 누르면 새 작업을 시작합니다. 무한 동작은 Repeat(...,Forever)를 사용합니다.
// return은 콜백의 중복 처리만 막습니다. 갱신 함수에서 같은 키를 glfwGetKey로 읽는 동작은 별도로 남습니다.


//
void CustomKey(GLFWwindow* window,int key,float dt)
{
    enum class Phase { Ready, Grabbing, Delivering, Opening, Dropping };
    static Phase phase=Phase::Ready;
    static float jawAngle=CLAW_CLOSED_ANGLE,openingStart=CLAW_CLOSED_ANGLE,phaseTime=0;
    static int job=0,heldPrize=-1;
    static float rodExtension=1,fallSpeed=0,landingY=0;

    if(key==GLFW_KEY_R)
    {
        phase=Phase::Ready;
        jawAngle=openingStart=CLAW_CLOSED_ANGLE;phaseTime=0;
        job=0;heldPrize=-1;
        rodExtension=1;fallSpeed=0;landingY=0;
        return;
    }

    if(key==GLFW_KEY_SPACE && phase==Phase::Ready)
    {
        // 여닫는 동안 집게발이 가장 아래로 뻗는 길이를 구해 바닥 관통을 막습니다.
        float reach=0;
        for(int i=0;i<=64;++i)
        {
            float angle=glm::radians(glm::mix(CLAW_CLOSED_ANGLE,CLAW_OPEN_ANGLE,i/64.0f));
            float first=CLAW_FIRST_LENGTH*glm::cos(angle);
            reach=glm::max(reach,first+CLAW_JAW_WIDTH*.5f*glm::abs(glm::sin(angle)));
            if(CLAW_SEGMENT_COUNT==2)
            {
                float lower=angle-glm::radians(CLAW_ELBOW_ANGLE);
                reach=glm::max(reach,first+CLAW_LOWER_LENGTH*glm::cos(lower)
                    +CLAW_JAW_WIDTH*.5f*glm::abs(glm::sin(lower)));
            }
        }
        rodExtension=(CLAW_CEILING_Y-CLAW_HEAD_HEIGHT-CLAW_BAR_HEIGHT-reach-CLAW_TIP_CLEARANCE)
            /CLAW_ROD_LENGTH;
        openingStart=jawAngle;
        phaseTime=0;
        job=Run(Scale(CLAW_ROD,Axis::Y,rodExtension-1,Pivot::Top,CLAW_LOWER_SECONDS));
        phase=Phase::Grabbing;
        return;
    }
    if(key!=GLFW_KEY_UNKNOWN || dt<=0)return;

    phaseTime+=dt;
    if(phase==Phase::Grabbing)
    {
        if(phaseTime<CLAW_LOWER_SECONDS)
            jawAngle=glm::mix(openingStart,CLAW_OPEN_ANGLE,phaseTime/CLAW_LOWER_SECONDS);
        else
            jawAngle=glm::mix(CLAW_OPEN_ANGLE,CLAW_CLOSED_ANGLE,
                glm::min((phaseTime-CLAW_LOWER_SECONDS)/CLAW_CLOSE_SECONDS,1.0f));
    }
    else if(phase==Phase::Opening)
        jawAngle=glm::mix(CLAW_CLOSED_ANGLE,CLAW_OPEN_ANGLE,glm::min(phaseTime/CLAW_RELEASE_SECONDS,1.0f));

    // 부모 공간에서 각 집게발의 접선 축을 기준으로 같은 각도를 적용합니다.
    // Attach의 윗면 연결점이 고정되므로 집게발 아래만 움직입니다.
    for(int finger=0;finger<CLAW_COUNT;++finger)
    {
        int upper=CLAW_FIRST_FINGER+2*finger;
        float azimuth=glm::pi<float>()+CLAW_AZIMUTH_STEP*finger;
        glm::vec3 axis(-glm::sin(azimuth),0,glm::cos(azimuth));
        boxes[upper].animation=glm::rotate(glm::mat4(1),glm::radians(jawAngle-CLAW_CLOSED_ANGLE),axis);
    }

    switch(phase)
    {
    case Phase::Ready:
    {
        // 키를 누르는 동안만 천장 이동부에 짧은 Move를 적용합니다.
        glm::vec3 direction(0);
        if(glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS)direction.x+=1;
        if(glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)direction.x-=1;
        if(glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)direction.z+=1;
        if(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)direction.z-=1;
        if(glm::length(direction)==0)break;
        glm::vec3 center(boxes[CLAW_HEAD].model[3]);
        glm::vec3 end=center+glm::normalize(direction)*CLAW_MOVE_SPEED*dt;
        float halfX=FLOOR_WIDTH*.5f-CLAW_EDGE_MARGIN;
        float halfZ=FLOOR_DEPTH*.5f-CLAW_EDGE_MARGIN;
        end.x=glm::clamp(end.x,-halfX,halfX);
        end.z=glm::clamp(end.z,-halfZ,halfZ);
        auto move=Move(CLAW_HEAD,end-center,dt);
        Advance(move,dt);
        break;
    }
    case Phase::Grabbing:
    {
        if(!IsFinished(job) || phaseTime<CLAW_LOWER_SECONDS+CLAW_CLOSE_SECONDS)break;
        EvaluateHierarchy();

        // 집게 기준 범위 안에서 가장 가운데에 있는 큐브 하나를 붙입니다.
        glm::mat4 toGrip=glm::inverse(boxes[CLAW_BODY].world);
        glm::vec3 gripPoint(0);
        float closest=1e10f;
        for(int id=PRIZE_FIRST;id<PRIZE_FIRST+PRIZE_COUNT;++id)
        {
            glm::vec3 local=glm::vec3(toGrip*boxes[id].world[3])*boxes[CLAW_BODY].baseSize;
            float distance=local.x*local.x+local.z*local.z;
            bool inGrip=CLAW_COUNT==2 ? glm::abs(local.x)<=.13f && glm::abs(local.z)<=.16f
                : distance<=.14f*.14f;
            float fingerLength=CLAW_FIRST_LENGTH+(CLAW_SEGMENT_COUNT==2 ? CLAW_LOWER_LENGTH : 0.0f);
            if(!inGrip || local.y<-fingerLength || local.y>-.16f)continue;
            if(distance>=closest)continue;
            closest=distance;heldPrize=id;gripPoint=local;
        }
        if(heldPrize>=0)
            Attach(CLAW_BODY,heldPrize,Point(gripPoint.x,gripPoint.y,gripPoint.z),
                Pivot::Center,Orientation::KeepWorld);

        glm::vec3 center(boxes[CLAW_HEAD].model[3]);
        glm::vec3 delta=glm::vec3(CLAW_DROP_X,center.y,CLAW_DROP_Z)-center;
        float travelSeconds=glm::max(.25f,glm::length(delta)/CLAW_MOVE_SPEED);

        // 상승 → 구석 이동 → 관성이 잦아드는 동안 대기
        job=Run(Sequence({
            Scale(CLAW_ROD,Axis::Y,1/rodExtension-1,Pivot::Top,CLAW_LIFT_SECONDS),
            Move(CLAW_HEAD,delta,travelSeconds),
            Move(CLAW_HEAD,{0,0,0},CLAW_SETTLE_SECONDS)
        }));
        phase=Phase::Delivering;
        break;
    }
    case Phase::Delivering:
        if(!IsFinished(job))break;
        phaseTime=0;
        phase=Phase::Opening;
        break;
    case Phase::Opening:
    {
        if(phaseTime<CLAW_RELEASE_SECONDS)break;
        if(heldPrize<0)
        {
            phase=Phase::Ready;
            break;
        }
        EvaluateHierarchy();

        // 현재 자세에서 부모를 해제하고 아래의 바닥 또는 큐브의 높이를 구합니다.
        auto& prize=boxes[heldPrize];
        prize.model=prize.world;
        prize.parent=-1;
        prize.attachment={};
        glm::vec3 center(prize.model[3]);
        landingY=PRIZE_SIZE*.5f;
        for(int id=PRIZE_FIRST;id<PRIZE_FIRST+PRIZE_COUNT;++id)
        {
            if(id==heldPrize)continue;
            glm::vec3 other(boxes[id].world[3]);
            if(glm::abs(other.x-center.x)<PRIZE_SIZE && glm::abs(other.z-center.z)<PRIZE_SIZE
                && other.y<center.y)
                landingY=glm::max(landingY,other.y+PRIZE_SIZE);
        }
        fallSpeed=0;
        phase=Phase::Dropping;
        break;
    }
    case Phase::Dropping:
    {
        // model의 4번째 열은 이동 성분입니다. Y만 갱신해 제자리에서 떨어뜨립니다.
        fallSpeed+=PRIZE_GRAVITY*dt;
        auto& height=boxes[heldPrize].model[3].y;
        height=glm::max(landingY,height-fallSpeed*dt);
        if(height<=landingY)
        {
            heldPrize=-1;
            phase=Phase::Ready;
        }
        break;
    }
    }
}

void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
    if(action!=GLFW_PRESS)return;
    if(key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window,true);
    if(key==GLFW_KEY_R)ResetScene();
    if(key==GLFW_KEY_W)wireframe=!wireframe;
    if(key==GLFW_KEY_T)perspective=!perspective;
    if(key==GLFW_KEY_E)hiddenSurface=!hiddenSurface;
    if(key==GLFW_KEY_Y)
    {
        int direction=(mods&GLFW_MOD_SHIFT) ? 1 : -1;
        cameraOrbit=cameraOrbit==direction ? 0 : direction;
    }
    try { CustomKey(window,key); }
    catch(const exception& error) { cerr << "Command error: " << error.what() << endl; }
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
#elif MID_TEST_EXAMPLE == 2
// 중간대비1 그네 예제 원본
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
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
#else
#error MID_TEST_EXAMPLE must be 0, 1, or 2
#endif

// 시험용 셰이더 복사용 원본

/* vertex.glsl
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;
void main(void)
{
    gl_Position = projectionTransform * viewTransform * modelTransform * vec4(position, 1.0);
}
*/

/* fragment.glsl
#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main(void)
{
    FragColor = vec4(uColor, 1.0);
}
*/
