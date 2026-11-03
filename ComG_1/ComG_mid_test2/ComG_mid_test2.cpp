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
#define BOX_COUNT 20
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
struct BoxState { bool enabled=false; glm::mat4 model{1}; glm::vec3 color{.5f}; };

// ============================================================================
// 수정 영역 ①: 번호 0~19의 활성화, 전체 크기, 중심 좌표, RGB 색상, 초기 회전각
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

// Move(번호,{dx,dy,dz},시간): 해당 단계 시작 위치에서 상대 이동합니다. 시간은 초 단위입니다.
Command Move(int id,glm::vec3 delta,float seconds=0)
{
    Command c;c.kind=Kind::Move;c.id=id;c.delta=delta;c.seconds=seconds;return c;
}
// Rotate(번호,축,각도,기준점,시간): -360~+360도. 기준점 생략 시 자기 중심.
// Center 회전은 매 순간 현재 중심을 따라갑니다. 이동이나 공전과 함께 실행해도 제자리 자전합니다.
// 축 방향은 조립 공간의 고정 축입니다. 객체의 로컬 축을 사용하는 것은 아닙니다.
// Point는 지정 좌표, 면 기준점은 회전 시작 시의 위치를 계속 사용합니다.
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
// 회전한 객체도 조립 공간의 고정 축으로 신축합니다. 기준점 생략 시 자기 중심.
// 1. 번호: boxSettings의 객체 번호(0~BOX_COUNT-1). 활성화된 객체만 화면에서 변환됩니다.
// 2. 축: Axis::X / Axis::Y / Axis::Z. 선택한 조립 공간 축 방향만 신축하고 나머지 방향은 유지합니다.
//    객체가 기울어져 있어도 로컬 축이 아닌 고정 축을 사용하므로 모양이 비스듬히 변형될 수 있습니다.
// 3. 배율증감: 최종 배율은 1+값입니다. 현재 크기에 대한 상대 배율이며 최종 크기를 직접 넣는 것이 아닙니다.
//    +.5f는 1.5배, +1.0f는 2배, -.5f는 절반입니다. 0은 변화가 없으며 -1 이하는 허용하지 않습니다.
//    Repeat(Scale(...),횟수)는 배율이 누적됩니다. 원래 크기로 돌아오려면 PingPong(Scale(...))을 사용합니다.
// 4. 기준점(생략 가능): Pivot::Center가 기본값입니다. 신축 시작 시의 중심을 기준으로 양쪽으로 늘어납니다.
//    회전의 Center와 달리 신축 기준점은 실행 중 현재 중심을 따라가지 않고 시작 위치로 고정됩니다.
//    Left / Right / Bottom / Top / Front / Back은 시작 시 해당 면 중심을 기준으로 사용합니다.
//    회전하지 않은 객체에 Axis::Y와 Pivot::Bottom을 지정하면 아래쪽을 고정하고 높이를 변경합니다.
//    Point(x,y,z)는 조립 공간의 지정 좌표를 기준으로 신축하므로 객체의 중심 위치도 바뀔 수 있습니다.
// 5. 시간(생략 가능): 편도 실행 시간(초). 생략하거나 0이면 abs(배율증감)/SCALE_SPEED초가 걸립니다.
//    양수 시간을 지정하면 해당 시간에 신축을 완료합니다. 음수는 허용하지 않습니다.
//    시간을 지정하려면 기준점도 적어야 합니다. PingPong은 같은 시간 동안 원래 크기로 돌아옵니다.
// 예: Scale(0,Axis::X,.5f) -> 시작 중심을 기준으로 X방향 크기를 현재의 1.5배로 늘립니다.
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
            if(child.id<0 || child.id>=BOX_COUNT)throw runtime_error("Box ID must be 0~19");
            if(!boxes[child.id].enabled)throw runtime_error("Swap requires enabled objects");
        }
    }
    if(c.kind==Kind::Move || c.kind==Kind::Rotate || c.kind==Kind::Scale)
    {
        if(c.id<0 || c.id>=BOX_COUNT)throw runtime_error("Box ID must be 0~19");
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
            // 이전 진행값을 유지해 반대 방향으로 되돌립니다. Center 회전은 복귀 중에도 현재 중심을 사용합니다.
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
            // 중심 자전만 이동하는 객체를 따라가며, 신축과 다른 회전 기준점은 기존 방식을 유지합니다.
            glm::vec3 pivotPosition=c.pivotPosition;
            if(c.kind==Kind::Rotate && !c.pivot.point
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
    cout << "Arrows: assembly | x/X,z/Z: camera axes | y/Y: orbit\n"
         << "a~l: custom commands | w: wire | t: projection | e: hidden surfaces\n"
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
    assemblyPosition=glm::vec3(0);
    cameraPosition={CAMERA_X,CAMERA_Y,CAMERA_Z};
    cameraTarget={CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z};
    cameraOrbit=0;dragging=false;
    for(int i=0;i<BOX_COUNT;++i)
    {
        const auto& setting=boxSettings[i];
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
    }
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
            if(box.enabled)
            {
                any=true;
                for(float x:{-.5f,.5f})for(float y:{-.5f,.5f})for(float z:{-.5f,.5f})
                {
                    glm::vec3 p=assemblyPosition+glm::vec3(box.model*glm::vec4(x,y,z,1));
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
        if(box.enabled)DrawBox(assembly*box.model,glm::vec3(1),box.color);
    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glBindVertexArray(0);
}

// ============================================================================
// 수정 영역 ②: 예비 키 9개. 아래 주석을 참고해 원하는 명령만 작성합니다.
// 명령의 객체 번호는 수정 영역 ①의 0~19와 같습니다.
//
// 단일 실행: Run(Move(0,{1,0,0}));
// 스왑: Run(Swap(0,1,2.0f)); // 두 객체의 중심 위치를 2초 동안 교환합니다.
// 스왑 반복: slots[0]=Run(Repeat(Swap(0,1,2.0f),Forever)); // 2초마다 교환하며 4초마다 원위치로 돌아옵니다.
// 직선 스왑: Run(Swap(0,1,2.0f,SwapMode::Direct));
// XZ 반원 스왑: Run(Swap(0,1,2.0f,SwapMode::ArcXZ));
// XZ ㄷ자 스왑: Run(Swap(0,1,2.0f,SwapMode::DetourXZ));
// XY ㄷ자 스왑: Run(Swap(0,1,2.0f,SwapMode::DetourXY));
// 방식 지정 반복: slots[0]=Run(Repeat(Swap(0,1,2.0f,SwapMode::ArcXZ),Forever));
// 시간 지정: Run(Move(0,{1,0,0},2.0f)); // 편도 2초
// 회전 시간 지정: Run(Rotate(0,Axis::Y,90,Pivot::Center,3.0f)); // 편도 3초
// 신축 시간 지정: Run(Scale(0,Axis::Y,.5f,Pivot::Bottom,1.0f)); // 편도 1초
// 시간을 생략하거나 0으로 지정하면 MOVE_SPEED / ROTATE_SPEED / SCALE_SPEED를 사용합니다.
// Rotate와 Scale의 시간을 지정할 때는 기준점도 반드시 적습니다.
// PingPong의 왕복 시간은 편도 시간의 두 배입니다.
// 동시 왕복 주기 맞추기: 아래 명령은 함께 1초 동안 변환하고 1초 동안 돌아옵니다.
// slots[0]=Run(Repeat(Parallel({PingPong(Scale(0,Axis::Y,.5f,Pivot::Bottom,1.0f)),PingPong(Move(1,{0,.05f,0},1.0f))}),Forever));
// Repeat 자체가 시간을 맞추지는 않습니다. 동시에 왕복할 변환의 편도 시간을 같게 지정합니다.
// 동시 실행: Run(Parallel({Move(0,{1,0,0}),Rotate(1,Axis::Y,90)}));
// 순차 실행: Run(Sequence({Move(0,{1,0,0}),Rotate(0,Axis::Y,90),Scale(0,Axis::Y,+.5f,Pivot::Bottom)}));
// 3회 반복: Run(Repeat(Move(0,{.2f,0,0}),3)); // 상대 이동 누적
// 3회 왕복: Run(Repeat(PingPong(Move(0,{1,0,0})),3));
// 무한 왕복: slots[0]=Run(Repeat(PingPong(Rotate(0,Axis::Y,90)),Forever));
// 정지/재개: TogglePause(slots[0]); // Pause, Resume를 따로 사용해도 됩니다.
// 완료 확인: if(IsFinished(slots[0])) slots[0]=Run(Move(0,{1,0,0}));
// 현재 상태에서 취소: Cancel(slots[0]);
// 시작 또는 정지/재개:
// if(IsFinished(slots[0])) slots[0]=Run(Repeat(PingPong(Move(0,{1,0,0})),Forever));
// else TogglePause(slots[0]);
//
// 하나처럼 회전: 모든 객체에 같은 Point를 지정합니다. 각자 Center를 사용하면 제자리 회전입니다.
// Center 자전은 현재 중심을 따라가므로 Move, Swap, Point 기준 공전과 동시에 사용할 수 있습니다.
// 공전하며 자전: Run(Parallel({Rotate(0,Axis::Y,360,Point(0,0,0),4.0f),Repeat(Rotate(0,Axis::X,360,Pivot::Center,1.0f),4)}));
// Run(Parallel({Rotate(0,Axis::Y,90,Point(0,0,0)),Rotate(1,Axis::Y,90,Point(0,0,0))}));
// 같은 객체에 동시 변환을 주면 고정 시간 간격마다 목록 순서로 합성합니다.
// 무한반복을 Sequence 중간에 넣으면 뒤 명령은 실행되지 않습니다.
// 비활성 객체는 보이지 않고 변환도 적용하지 않습니다. 명령의 진행 시간은 유지합니다.
// ============================================================================
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
