#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>
using namespace std;

// ============================================================================
// 1. 프로그램 설정: 크기, 속도, 카메라 설정은 여기에서 수정합니다.
// ============================================================================
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define CAMERA_X 2.8f
#define CAMERA_Y 2.4f
#define CAMERA_Z -4.0f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 0.0f
#define CAMERA_TARGET_Z 0.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 50.0f
#define CAMERA_MOVE_SPEED 1.0f
#define CAMERA_ROTATE_SPEED 35.0f
#define CAMERA_RADIUS_MIN 0.30f
#define CAMERA_RADIUS_MAX 20.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
// ===========================================================================
// 바닥 설정
// ===========================================================================
#define FLOOR_WIDTH 4.0f
#define FLOOR_DEPTH 4.0f
#define FLOOR_HEIGHT 0.04f
#define FLOOR_RED 0.82f
#define FLOOR_GREEN 0.88f
#define FLOOR_BLUE 0.94f
// ===========================================================================
// 부품 크기는 BuildTank()의 각 생성문에서 지정합니다.
// ===========================================================================
// ===========================================================================
// 애니메이션과 색상 설정
// ===========================================================================
#define TANK_MOVE_SPEED 0.65f
#define LOWER_SCALE_MIN 0.50f
#define LOWER_SCALE_MAX 1.50f
#define MIDDLE_ROTATE_SPEED 90.0f
#define BARREL_ROTATE_SPEED 90.0f
#define POLE_ROTATE_SPEED 120.0f
#define MIDDLE_CYCLE_ANGLE 180.0f
#define BARREL_CYCLE_ANGLE 180.0f
#define POLE_CYCLE_ANGLE 360.0f
#define SWAP_DURATION 1.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f


#define MESH_SLICES 32
#define MESH_STACKS 20
#define LOWER_SCALE_SPEED 0.60f

// ============================================================================
// 2. 도형과 설정 자료형 (위치는 중심, 크기는 전체 폭/높이/깊이)
// ============================================================================
enum class ShapeKind { Box, Cone, Cylinder, Sphere, Pyramid }; // 모양 종류 / 육면체, 원뿔, 원기둥, 구, 사각뿔
enum class Mode { Once, Loop, PingPong }; // 반복 방식 / 한 사이클, 계속 진행, 왕복
enum class Anchor { Center, Left, Right, Bottom, Top, Front, Back }; // 로컬 기준점 / 중심, 왼쪽, 오른쪽, 아래, 위, 앞(-Z), 뒤(+Z)
enum class ActionKind { Move, Rotate, Scale, Swap }; // 기능 종류 / 이동, 회전, 신축, 위치 교환

enum class TransformSpace { Local, Parent, World }; // 변환 축 좌표계 / 도형, 부모, 월드
// 신축 축과 접점은 도형 로컬 좌표입니다. 이동·회전에만 TransformSpace를 사용합니다.
enum class MissingSupport { Floor, KeepPosition }; // 수동 지지대가 없을 때 / 바닥, 생성 위치 유지
// 생성 각도는 {X회전,Y회전,Z회전}, 단위는 도입니다. 행렬은 Rz * Ry * Rx 순서입니다.
// 기본 윗방향 +Y를 돌리는 예: +X {0,0,-90}, -X {0,0,90}, +Y {0,0,0},
// -Y {180,0,0}, +Z {90,0,0}, -Z {-90,0,0}. {25,40,15} 같은 임의 각도도 가능합니다.

// 앞은 -Z, 뒤는 +Z입니다. AnchorPoint(Shape::bottom)는 (0,-0.5,0)입니다.
inline glm::vec3 AnchorPoint(Anchor a)
{
    switch (a)
    {
    case Anchor::Left:   return {-0.5f, 0, 0};
    case Anchor::Right:  return {0.5f, 0, 0};
    case Anchor::Bottom: return {0, -0.5f, 0};
    case Anchor::Top:    return {0, 0.5f, 0};
    case Anchor::Front:  return {0, 0, -0.5f};
    case Anchor::Back:   return {0, 0, 0.5f};
    default:            return {0, 0, 0};
    }
}

// Color::Fixed(r,g,b): 고정색. Color::Random(그룹번호): 같은 번호는 같은 랜덤색.
// Random(-1)은 도형마다 독립 추첨합니다. o는 고정색도 그룹별로 다시 추첨합니다.
struct Color
{
    glm::vec3 value{0.7f};
    bool random = false;
    int group = -1;

    static Color Fixed(float r, float g, float b, int group = -1)
    {
        return {{r, g, b}, false, group};
    }

    static Color Random(int group = -1)
    {
        return {glm::vec3(0.7f), true, group};
    }
};

struct Shape
{
    static constexpr Anchor center = Anchor::Center;
    static constexpr Anchor left = Anchor::Left;
    static constexpr Anchor right = Anchor::Right;
    static constexpr Anchor bottom = Anchor::Bottom;
    static constexpr Anchor top = Anchor::Top;
    static constexpr Anchor front = Anchor::Front;
    static constexpr Anchor back = Anchor::Back;

    int id, part;
    ShapeKind kind;
    glm::vec3 position, size, angles{0};
    Color color;
    glm::vec3 initialColor{0};
    int parent = -1;
    int layer = 0;                 // 0: 자유 도형/부속, 양수: 쌓이는 층 번호
    int layerMotionParent = -1;    // 층 위치와 별개로 이동·회전을 따라갈 도형(없으면 무시)
    bool keepLayerContact = true;  // 층 아래쪽을 받침 높이에 맞출지 여부
    bool visible = true;           // 부모가 없는 부속은 자식까지 숨깁니다.
    std::vector<int> supports;
    bool supportAttachment = false;
    MissingSupport missingSupport = MissingSupport::Floor;
    Anchor ownAnchor = center, parentAnchor = center;
    glm::vec3 attachmentOffset{0};
    bool inheritRotation = true;
    bool floorBound = true;
    glm::mat4 geometry{1}, frame{1};
    glm::quat worldRotation{1, 0, 0, 0};

    // 마지막 angles는 초기 X/Y/Z 각도(도)입니다. 생성 시 입력하거나 Get(id).angles로 설정합니다.
    Shape(int id, int part, ShapeKind kind, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : id(id), part(part), kind(kind), position(position), size(size), angles(angles), color(color)
    {
    }
};

// 아래 클래스는 모두 같은 셰이더/연결/동작 코드를 사용합니다.
// 예: scene.Add(Cylinder(10, WHEELS, {0,0,0}, {.3f,.2f,.3f}, Color::Random()));
// Box(ID,파트,위치,크기,색상,각도={0,0,0}): 직육면체를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Box : Shape
{
    Box(int id, int part, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : Shape(id, part, ShapeKind::Box, position, size, color, angles)
    {
    }
};

// Cone(ID,파트,위치,크기,색상,각도={0,0,0}): 원뿔을 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Cone : Shape
{
    Cone(int id, int part, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : Shape(id, part, ShapeKind::Cone, position, size, color, angles)
    {
    }
};

// Cylinder(ID,파트,위치,크기,색상,각도={0,0,0}): 원기둥을 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Cylinder : Shape
{
    Cylinder(int id, int part, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : Shape(id, part, ShapeKind::Cylinder, position, size, color, angles)
    {
    }
};

// Sphere(ID,파트,위치,크기,색상,각도={0,0,0}): 구를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Sphere : Shape
{
    Sphere(int id, int part, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : Shape(id, part, ShapeKind::Sphere, position, size, color, angles)
    {
    }
};

// Pyramid(ID,파트,위치,크기,색상,각도={0,0,0}): 사각뿔을 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Pyramid : Shape
{
    Pyramid(int id, int part, glm::vec3 position, glm::vec3 size, Color color, glm::vec3 angles = {0,0,0})
        : Shape(id, part, ShapeKind::Pyramid, position, size, color, angles)
    {
    }
};

// Id(10): 한 도형. Part(WHEELS): 해당 파트 전체. 모든 도형을 만든 뒤 연결/기능을 등록합니다.
enum class TargetKind { Object, Part, Layer }; // 기능 대상 / 도형 ID, 파트, 층
struct Target
{
    int value;
    TargetKind kind;
};
// Id(번호): 기능을 개별 도형에만 적용할 때 사용합니다.
inline Target Id(int id)
{
    return {id, TargetKind::Object};
}
// Part(파트번호): 기능을 파트 전체에 동시에 적용할 때 사용합니다.
inline Target Part(int part)
{
    return {part, TargetKind::Part};
}

// Layer(층번호): 그 층의 기본 도형만 선택합니다. 부속 도형은 자동으로 포함하지 않습니다.
inline Target Layer(int layer)
{
    return {layer, TargetKind::Layer};
}

struct Action
{
    ActionKind kind;
    Target target;
    int key;
    TransformSpace space = TransformSpace::Parent;
    glm::vec3 axis{0, 1, 0};
    Anchor pivot = Shape::center;
    Mode mode = Mode::Once;
    float speed = 1, amount = 1, low = 0.5f, high = 1.5f;
    float value = 0, progress = 0, start = 0, sign = 1;
    float direction = 1;
    bool active = false;
    bool angleRange = false; // true이면 low~high 사이의 부호 있는 각도로 왕복
    int other = -1, outwardReference = -1;
    glm::vec3 swapDelta{0};
};

// ============================================================================
// 3. 도형 등록 / 연결 / 기능 등록
// ============================================================================
class TankScene
{
public:
    std::map<int, Shape> shapes;
    std::vector<Action> actions;
    glm::vec3 position{0};
    float colorMin = 0.1f, colorMax = 0.9f;

    // Add(Box(...)): ID는 고유해야 합니다. 등록 순서와 연결 순서는 무관합니다.
    Shape& Add(Shape shape)
    {
        if (shape.id < 0 || shapes.count(shape.id))
            throw std::runtime_error("Shape ID must be unique and nonnegative");
        if (glm::any(glm::lessThanEqual(shape.size, glm::vec3(0))))
            throw std::runtime_error("Shape size must be positive");
        return shapes.emplace(shape.id, shape).first->second;
    }

    // Get(10).angles = {0,0,90}; 처럼 초기 설정을 변경합니다. 설정은 Seal() 전에 합니다.
    Shape& Get(int id)
    {
        auto it = shapes.find(id);
        if (it == shapes.end())
            throw std::runtime_error("Unknown shape ID: " + std::to_string(id));
        return it->second;
    }

    // SetLayer(대상,층번호,이동회전참조ID=-1,접촉유지=true).
    // 비어 있지 않은 바로 아래층 위에 쌓습니다. XZ 평균 정렬은 하지 않습니다.
    // 참조ID가 존재하면 그 도형의 이동/회전을 따르지만, 없어져도 이 층은 남습니다.
    // 접촉유지=false이면 초기 높이만 정렬하고 개별 회전/신축 기준점의 움직임을 허용합니다.
    void SetLayer(Target target, int number, int motionParent = -1, bool keepContact = true)
    {
        if (number<=0)
            throw std::runtime_error("Layer number must be positive");
        for (int id : Resolve(target))
        {
            Shape& s=Get(id);
            s.layer=number;
            s.layerMotionParent=motionParent;
            s.keepLayerContact=keepContact;
            s.parent=-1;
            s.supports.clear();
            s.supportAttachment=false;
            s.attachmentOffset=glm::vec3(0);
        }
    }

    // PlaceOnFloor(대상): 루트 도형을 생성 크기/초기 방향에 맞춰 바닥 Y=0에 놓습니다.
    // 모든 루트 도형을 생성한 뒤, 연결과 동작 설정 전에 호출합니다.
    void PlaceOnFloor(Target target)
    {
        Evaluate();
        for (int id : Resolve(target))
        {
            Shape& s=Get(id);
            if (s.parent>=0 || s.supportAttachment || s.layer>0)
                throw std::runtime_error("PlaceOnFloor requires unattached root shapes");
            s.position.y-=Bounds(s.geometry).first.y;
        }
        Evaluate();
    }

    // Attach(자식, 부모, 자식접점, 부모접점, 여유위치, 회전상속).
    // 자식.position은 접점에서의 추가 위치입니다. 부모 크기는 상속하지 않습니다.
    // 없는 자식은 무시합니다. 부모가 없거나 숨겨지면 이 부속과 그 자식도 숨깁니다.
    // 포신: Attach(gun, turret, Shape::back, Shape::center).
    void Attach(int child, int parent, Anchor own = Shape::bottom,
                Anchor parentPoint = Shape::top, glm::vec3 offset = {0,0,0}, bool rotate = true)
    {
        if (!shapes.count(child))
            return;
        Shape& s = Get(child);
        s.supportAttachment = false;
        s.layer = 0;
        s.layerMotionParent = -1;
        s.parent = parent;
        s.supports.clear();
        s.ownAnchor = own;
        s.parentAnchor = parentPoint;
        s.attachmentOffset = offset;
        s.inheritRotation = rotate;
    }

    // AttachOnTop(몸통, Part(WHEELS)): 지지 도형들의 월드 바운딩 박스 중 최고 높이 사용.
    // XZ는 지지 도형 중심의 평균, 몸통은 수평 유지. 서로 다른 높이도 처리합니다.
    // 지지대가 없으면 기본적으로 바닥에 놓습니다. KeepPosition은 생성 위치를 유지합니다.
    // 없는 자식 연결은 무시합니다. 지지 도형의 회전은 상속하지 않습니다.
    void AttachOnTop(int child, Target supports, glm::vec3 offset = {0,0,0},
                     MissingSupport missing = MissingSupport::Floor)
    {
        if (!shapes.count(child))
            return;
        Shape& s = Get(child);
        s.layer = 0;
        s.layerMotionParent = -1;
        s.supportAttachment = true;
        s.missingSupport = missing;
        s.parent = -1;
        s.supports = Resolve(supports);
        s.ownAnchor = Shape::bottom;
        s.attachmentOffset = offset;
        s.inheritRotation = false;
    }

    // AddMovement(대상,키,방향,거리,초당거리,반복방식).
    // 마지막 space로 Local/Parent/World를 선택합니다. 기본은 Parent이며 방향은 자동 정규화합니다.
    int AddMovement(Target target, int key, glm::vec3 direction, float distance,
                    float speed, Mode mode = Mode::Once, TransformSpace space = TransformSpace::Parent)
    {
        Action a{ActionKind::Move, target, key};
        a.space = space;
        a.axis = Unit(direction);
        a.amount = distance;
        a.speed = speed;
        a.mode = mode;
        return AddAction(a);
    }

    // AddRotation(대상,키,회전축,기준점,+1/-1 방향,초당각도,사이클각도,반복방식).
    // 기준점은 신축 후 로컬 면 중심입니다. 마지막 space는 Local/Parent/World(기본 Local)입니다.
    int AddRotation(Target target, int key, glm::vec3 axis, Anchor pivot,
                    float direction, float speed, float cycle = 360, Mode mode = Mode::Once,
                    TransformSpace space = TransformSpace::Local)
    {
        Action a{ActionKind::Rotate, target, key};
        a.space = space;
        a.axis = Unit(axis);
        a.pivot = pivot;
        a.direction = direction < 0 ? -1.0f : 1.0f;
        a.sign = a.direction;
        a.speed = speed;
        a.amount = cycle;
        a.mode = mode;
        return AddAction(a);
    }

    // AddSwingRotation(대상,키,축,기준점,최소각도,최대각도,초당각도,좌표계).
    // 0도에서 시작해 최대각도와 최소각도 사이를 왕복합니다. 다시 누르면 정지/재개합니다.
    // 예: (-30,30,45)는 수직에서 시작해 앞뒤 30도, 초당 45도로 움직입니다.
    int AddSwingRotation(Target target, int key, glm::vec3 axis, Anchor pivot,
                         float low, float high, float speed,
                         TransformSpace space = TransformSpace::Local)
    {
        if (low>0 || high<0 || low>=high)
            throw std::runtime_error("Swing range must include zero and min < max");
        int index=AddRotation(target,key,axis,pivot,1,speed,high-low,Mode::PingPong,space);
        Action& a=actions[index];
        a.angleRange=true;
        a.low=low;
        a.high=high;
        return index;
    }

    // AddScale(대상,키,축마스크,고정점,최소배율,최대배율,초당배율,반복방식).
    // {0,1,0}은 Y만 신축, {1,1,1}은 전체 신축. bottom이면 바닥은 고정됩니다.
    // Once: 현재->최대, Loop: 최소~최대 반복(경계에서 되돌림), PingPong: 연속 왕복.
    int AddScale(Target target, int key, glm::vec3 axes, Anchor pivot,
                 float low, float high, float speed, Mode mode = Mode::PingPong)
    {
        if (low <= 0 || low > 1 || high < 1 || low >= high ||
            glm::any(glm::lessThan(axes, glm::vec3(0))) ||
            glm::any(glm::greaterThan(axes, glm::vec3(1))) || glm::length(axes) == 0)
            throw std::runtime_error("Scale requires 0 < min <= 1 <= max and axis mask in [0,1]");
        Action a{ActionKind::Scale, target, key};
        a.axis = axes;
        a.pivot = pivot;
        a.low = low;
        a.high = high;
        a.speed = speed;
        a.mode = mode;
        a.value = 1;
        return AddAction(a);
    }

    // AddSwap(상부1,상부2,키,소요초): 같은 부모/접점의 두 도형 위치를 교환합니다.
    // 자식 포신과 깃대는 연결 관계를 통해 자동으로 따라갑니다.
    int AddSwap(int first, int second, int key, float seconds)
    {
        if (seconds <= 0 || first == second)
            throw std::runtime_error("Swap needs two different IDs and positive duration");
        Action a{ActionKind::Swap, Id(first), key};
        a.other = second;
        a.speed = 1 / seconds;
        return AddAction(a);
    }

    // SetOutwardRotation(회전기능번호, 좌우위치를판단할상부ID).
    // Y축/180도/Once 전용. 위치 교환 후에도 바깥쪽 반원을 선택합니다.
    void SetOutwardRotation(int action, int reference)
    {
        Action& a = actions.at(action);
        if (a.kind != ActionKind::Rotate || a.mode != Mode::Once ||
            std::abs(a.amount - 180) > 0.001f || glm::length(a.axis-glm::vec3(0,1,0)) > 0.001f)
            throw std::runtime_error("Outward rotation requires Y axis, Once, 180 degrees");
        a.outwardReference = reference;
    }

    // Seal(): 모든 설정 뒤 한 번 호출. 순환 연결/겹친 회전·신축 기능을 검사하고 초기색 저장.
    // 같은 도형은 이동 여러 개를 합산할 수 있지만 회전·신축 채널은 각각 하나입니다.
    void Seal()
    {
        for (auto& [id, s] : shapes)
        {
            if (s.layer>0 && shapes.count(s.layerMotionParent))
            {
                const Shape& parent=Get(s.layerMotionParent);
                if (parent.layer<=0 || parent.layer>=s.layer)
                    throw std::runtime_error("Layer motion parent must be a lower-layer root");
            }
            int rotateCount = 0, scaleCount = 0;
            for (const Action& a : actions)
            {
                auto ids = Resolve(a.target);
                if (std::find(ids.begin(), ids.end(), id) == ids.end())
                    continue;
                rotateCount += a.kind == ActionKind::Rotate;
                scaleCount += a.kind == ActionKind::Scale;
            }
            if (rotateCount > 1 || scaleCount > 1)
                throw std::runtime_error("Overlapping rotation/scale actions for ID " + std::to_string(id));
        }
        for (Action& a : actions)
        {
            if (Resolve(a.target).empty() || (a.kind == ActionKind::Swap && !shapes.count(a.other)))
                continue;
            if (a.kind == ActionKind::Swap)
            {
                Shape& x = Get(a.target.value);
                Shape& y = Get(a.other);
                if (x.parent != y.parent || x.ownAnchor != y.ownAnchor || x.parentAnchor != y.parentAnchor ||
                    !x.supports.empty() || !y.supports.empty() || x.attachmentOffset != y.attachmentOffset ||
                    x.layer != y.layer || x.layerMotionParent != y.layerMotionParent)
                    throw std::runtime_error("Swap requires matching parent and attachment anchors");
                a.swapDelta = y.position - x.position;
            }
        }
        RandomizeColors(true);
        for (auto& [id, s] : shapes)
            s.initialColor = s.color.value;
        Evaluate();
    }

    // Trigger(key): 키를 한 번 누를 때 호출합니다. 같은 키의 기능은 동시에 시작합니다.
    // Once는 실행 중 재입력을 무시하고 Loop/PingPong은 현재 상태에서 정지·재개합니다.
    void Trigger(int key)
    {
        for (Action& a : actions)
        {
            auto targets=Resolve(a.target);
            bool hasVisible=false;
            for (int id : targets)
                hasVisible=hasVisible || Get(id).visible;
            if (a.key != key || !hasVisible ||
                (a.kind == ActionKind::Swap && (!shapes.count(a.other) || !Get(a.other).visible)))
                continue;
            if (a.mode != Mode::Once)
            {
                a.active = !a.active;
                continue;
            }
            if (a.active)
                continue;
            a.active = true;
            a.start = a.value;
            a.progress = 0;
            a.sign = a.direction;
            if (a.kind == ActionKind::Swap)
                a.sign = a.value < 0.5f ? 1.0f : -1.0f;
            if (a.outwardReference >= 0 && shapes.count(a.outwardReference) && Get(a.outwardReference).visible)
            {
                const Shape& ref = Get(a.outwardReference);
                float x = ref.position.x + Movement(ref.id).x;
                a.sign = x * glm::cos(glm::radians(a.value)) >= 0 ? -1.0f : 1.0f;
            }
        }
    }

    // Update(dt): 초 단위 경과 시간으로 갱신합니다. 파트는 같은 진행값을 공유합니다.
    void Update(float dt)
    {
        for (Action& a : actions)
        {
            if (!a.active)
                continue;
            if (a.kind == ActionKind::Scale || a.angleRange)
            {
                if (a.mode == Mode::Once)
                {
                    a.value = glm::min(a.value + a.speed * dt, a.high);
                    a.active = a.value < a.high;
                }
                else if (a.mode == Mode::Loop)
                    a.value = a.low + glm::mod(a.value-a.low+a.speed*dt, a.high-a.low);
                else
                {
                    float range = a.high-a.low;
                    float phase = a.sign > 0 ? a.value-a.low : 2*range-(a.value-a.low);
                    phase = glm::mod(phase+a.speed*dt, 2*range);
                    a.sign = phase < range ? 1.0f : -1.0f;
                    a.value = a.low + (phase < range ? phase : 2*range-phase);
                }
                continue;
            }
            if (a.mode == Mode::Once)
            {
                a.progress = glm::min(a.progress+a.speed*dt, a.amount);
                a.value = a.start+a.sign*a.progress;
                a.active = a.progress < a.amount;
            }
            else if (a.mode == Mode::Loop)
                a.value += a.direction*a.speed*dt;
            else
            {
                a.progress = glm::mod(a.progress+a.speed*dt, 2*a.amount);
                a.value = a.direction*(a.progress<a.amount ? a.progress : 2*a.amount-a.progress);
            }
        }
        Evaluate();
    }

    // Reset(): 설정은 유지하고 동작·위치·색상을 시작 상태로 복구합니다.
    void Reset()
    {
        position = glm::vec3(0);
        for (Action& a : actions)
        {
            a.active = false;
            a.value = a.kind == ActionKind::Scale ? 1.0f : 0.0f;
            a.progress = a.start = 0;
            a.sign = a.direction;
        }
        for (auto& [id, s] : shapes)
            s.color.value = s.initialColor;
        Evaluate();
    }

    // RandomizeColors(): 같은 color.group끼리 같은 RGB로 재추첨합니다.
    void RandomizeColors(bool initial = false)
    {
        std::map<int, glm::vec3> groups;
        std::uniform_real_distribution<float> dist(colorMin, colorMax);
        for (auto& [id, s] : shapes)
        {
            if (initial && !s.color.random)
                continue;
            if (s.color.group >= 0 && groups.count(s.color.group))
                s.color.value = groups.at(s.color.group);
            else
            {
                s.color.value = {dist(random), dist(random), dist(random)};
                if (s.color.group >= 0)
                    groups[s.color.group] = s.color.value;
            }
        }
    }

    // Evaluate(): 부모부터 연결을 계산합니다. 등록 순서는 무관하며 순환은 오류로 알립니다.
    void Evaluate()
    {
        std::map<int, int> state;
        for (auto& [id, s] : shapes)
            EvaluateOne(id, state);
    }

    // KeepOnFloor(폭,깊이): 모든 부품의 현재 범위를 사용해 바닥 경계를 제한합니다.
    // 탱크가 바닥보다 큰 설정은 불가능하므로 오류로 알립니다.
    void KeepOnFloor(float width, float depth)
    {
        glm::vec3 lo(1e9f), hi(-1e9f);
        for (const auto& [id, s] : shapes)
        {
            if (!s.floorBound || !s.visible)
                continue;
            auto bounds = Bounds(s.geometry);
            lo = glm::min(lo, bounds.first);
            hi = glm::max(hi, bounds.second);
        }
        if (hi.x-lo.x>width || hi.z-lo.z>depth)
            throw std::runtime_error("Tank footprint is larger than the floor");
        glm::vec3 correction(0);
        if (lo.x < -width/2)
            correction.x = -width/2-lo.x;
        if (hi.x > width/2)
            correction.x = width/2-hi.x;
        if (lo.z < -depth/2)
            correction.z = -depth/2-lo.z;
        if (hi.z > depth/2)
            correction.z = depth/2-hi.z;
        position += correction;
        if (glm::length(correction)>0)
            Evaluate();
    }

    // Bounds(matrix): 단위 상자의 여덟 꼭짓점으로 보수적인 월드 범위를 구합니다.
    static std::pair<glm::vec3, glm::vec3> Bounds(const glm::mat4& matrix)
    {
        glm::vec3 lo(1e9f), hi(-1e9f);
        for (int x : {-1,1})
            for (int y : {-1,1})
                for (int z : {-1,1})
                {
                    glm::vec3 p(matrix*glm::vec4(x*.5f,y*.5f,z*.5f,1));
                    lo=glm::min(lo,p);
                    hi=glm::max(hi,p);
                }
        return {lo,hi};
    }

private:
    std::mt19937 random{std::random_device{}()};

    // Unit(v): 방향을 정규화하고 길이가 0인 잘못된 방향을 검사합니다.
    static glm::vec3 Unit(glm::vec3 v)
    {
        if (glm::length(v)<0.00001f)
            throw std::runtime_error("Direction must not be zero");
        return glm::normalize(v);
    }

    // Resolve(target): 존재하는 대상만 반환합니다. 빈 파트/없는 ID는 빈 목록입니다.
    std::vector<int> Resolve(Target target)
    {
        std::vector<int> result;
        for (const auto& [id, s] : shapes)
            if ((target.kind==TargetKind::Object && id==target.value) ||
                (target.kind==TargetKind::Part && s.part==target.value) ||
                (target.kind==TargetKind::Layer && s.layer>0 && s.layer==target.value))
                result.push_back(id);
        // 빈 파트나 생략한 개별 ID는 유효한 빈 선택입니다.
        return result;
    }

    // AddAction: 등록 공통 검사. 반환값은 이후 특수 회전 설정에 사용할 기능 번호입니다.
    int AddAction(Action action)
    {
        if (action.speed<=0 || action.amount<=0)
            throw std::runtime_error("Action speed and amount must be positive");
        actions.push_back(action);
        return (int)actions.size()-1;
    }

    // Matches: 기능의 대상이 현재 도형/파트인지 검사합니다.
    static bool Matches(const Action& a, const Shape& s)
    {
        if (a.target.kind==TargetKind::Part)
            return s.part==a.target.value;
        if (a.target.kind==TargetKind::Layer)
            return s.layer>0 && s.layer==a.target.value;
        return s.id==a.target.value;
    }

    // Movement: 개별 이동과 위치 교환을 합산합니다. 부모 좌표계 기준입니다.
    glm::vec3 Movement(int id, glm::quat inherited = glm::quat(1,0,0,0))
    {
        glm::vec3 result(0);
        const Shape& s=Get(id);
        for (const Action& a : actions)
        {
            if (a.kind==ActionKind::Move && Matches(a,s))
            {
                glm::vec3 direction=a.axis;
                if (a.space==TransformSpace::Local)
                    direction=glm::quat(glm::radians(s.angles))*direction;
                else if (a.space==TransformSpace::World)
                    direction=glm::inverse(inherited)*direction;
                result+=direction*a.value;
            }
            if (a.kind==ActionKind::Swap)
            {
                if (a.target.value==id)
                    result+=a.swapDelta*a.value;
                if (a.other==id)
                    result-=a.swapDelta*a.value;
            }
        }
        return result;
    }

    // EvaluateOne: 부모 이동/회전은 상속하고 신축 배율은 상속하지 않는 핵심 부분입니다.
    void EvaluateOne(int id, std::map<int,int>& state)
    {
        if (state[id]==2)
            return;
        if (state[id]==1)
            throw std::runtime_error("Attachment cycle at ID " + std::to_string(id));
        state[id]=1;
        Shape& s=Get(id);
        s.visible=true;
        glm::vec3 origin=position;
        glm::quat inherited(1,0,0,0);
        glm::quat initial=glm::quat(glm::radians(s.angles));
        float layerTop=position.y;
        bool missingSupport=s.supportAttachment && s.supports.empty();

        if (s.parent>=0)
        {
            if (!shapes.count(s.parent))
            {
                s.visible=false;
                state[id]=2;
                return;
            }
            EvaluateOne(s.parent,state);
            Shape& parent=Get(s.parent);
            if (!parent.visible)
            {
                s.visible=false;
                state[id]=2;
                return;
            }
            origin=glm::vec3(parent.geometry*glm::vec4(AnchorPoint(s.parentAnchor),1));
            if (s.inheritRotation)
                inherited=parent.worldRotation;
        }
        else if (s.layer>0)
        {
            // 번호가 작은 층 중 가장 가까운 층을 선택합니다. 부속 높이는 층 높이에 포함하지 않습니다.
            int below=0;
            for (const auto& [otherId,other] : shapes)
                if (other.layer>0 && other.layer<s.layer)
                    below=std::max(below,other.layer);
            if (below>0)
            {
                layerTop=-1e9f;
                for (const auto& [otherId,other] : shapes)
                {
                    if (other.layer!=below)
                        continue;
                    EvaluateOne(otherId,state);
                    layerTop=glm::max(layerTop,Bounds(Get(otherId).geometry).second.y);
                }
            }
            origin.y=layerTop;
            // 층의 XZ는 설정값 그대로입니다. 선택적으로 지정한 참조의 변환만 추가합니다.
            if (shapes.count(s.layerMotionParent))
            {
                EvaluateOne(s.layerMotionParent,state);
                Shape& parent=Get(s.layerMotionParent);
                inherited=parent.worldRotation;
                glm::vec3 center(parent.geometry[3]);
                glm::vec3 reference(parent.position.x,0,parent.position.z);
                glm::vec3 shifted=center+inherited*(glm::vec3(s.position.x,0,s.position.z)-reference);
                origin.x=shifted.x-(inherited*s.position).x;
                origin.z=shifted.z-(inherited*s.position).z;
            }
        }
        else if (!s.supports.empty())
        {
            origin=glm::vec3(0);
            float top=-1e9f;
            int count=0;
            for (int support : s.supports)
            {
                EvaluateOne(support,state);
                Shape& parent=Get(support);
                if (!parent.visible)
                    continue;
                origin+=glm::vec3(parent.geometry[3]);
                top=glm::max(top,Bounds(parent.geometry).second.y);
                ++count;
            }
            missingSupport=count==0;
            if (count>0)
            {
                origin/=(float)count;
                origin.y=top;
            }
            else
                origin=position;
        }
        glm::vec3 ownPoint(0);
        if (s.parent>=0 || (!s.supports.empty() && !missingSupport))
            ownPoint=initial*(AnchorPoint(s.ownAnchor)*s.size);
        if (s.layer>0)
            ownPoint.y=Bounds(glm::mat4_cast(inherited*initial)*glm::scale(glm::mat4(1),s.size)).first.y;
        // 층 배치의 ownPoint는 월드 Y, 일반 연결은 부모 좌표 기준입니다.
        if (s.layer>0)
            origin+=inherited*(s.position+Movement(id,inherited))-glm::vec3(0,ownPoint.y,0);
        else
            origin+=inherited*(s.position+s.attachmentOffset-ownPoint+Movement(id,inherited));

        glm::vec3 scale(1), scalePivot(0), rotationAxis(0,1,0);
        Anchor rotationAnchor=Shape::center;
        float angle=0;
        for (const Action& a : actions)
        {
            if (!Matches(a,s))
                continue;
            if (a.kind==ActionKind::Scale)
            {
                scale=glm::vec3(1)+a.axis*(a.value-1);
                scalePivot=AnchorPoint(a.pivot)*s.size;
            }
            if (a.kind==ActionKind::Rotate)
            {
                angle=a.value;
                rotationAxis=a.axis;
                if (a.space==TransformSpace::Local)
                    rotationAxis=initial*rotationAxis;
                else if (a.space==TransformSpace::World)
                    rotationAxis=glm::inverse(inherited)*rotationAxis;
                rotationAnchor=a.pivot;
            }
        }
        glm::vec3 scaleShift=scalePivot*(glm::vec3(1)-scale);
        glm::vec3 rotationPivot=initial*(AnchorPoint(rotationAnchor)*s.size*scale+scaleShift);
        glm::quat rotation=glm::angleAxis(glm::radians(glm::mod(angle,360.0f)), rotationAxis);
        s.worldRotation=inherited*rotation*initial;
        s.frame=glm::translate(glm::mat4(1),origin)*glm::mat4_cast(inherited)
            *glm::translate(glm::mat4(1),rotationPivot)*glm::mat4_cast(rotation)
            *glm::translate(glm::mat4(1),-rotationPivot)*glm::mat4_cast(initial)
            *glm::translate(glm::mat4(1),scaleShift);
        s.geometry=glm::scale(s.frame,s.size*scale);
        if ((missingSupport && s.missingSupport==MissingSupport::Floor) || (s.layer>0 && s.keepLayerContact))
        {
            // 기울어진 도형도 실제 변환된 바운딩 박스의 아래쪽을 바닥에 맞춥니다.
            float floorY=(s.layer>0 ? layerTop : position.y)+s.position.y+s.attachmentOffset.y+
                (inherited*Movement(id,inherited)).y;
            float lift=floorY-Bounds(s.geometry).first.y;
            glm::mat4 correction=glm::translate(glm::mat4(1),glm::vec3(0,lift,0));
            s.frame=correction*s.frame;
            s.geometry=correction*s.geometry;
        }
        state[id]=2;
    }
};

// ============================================================================
// 4. 공통 도형 메시 생성
// ============================================================================

// MakeMesh(종류,원둘레분할,구위도분할): 중심 원점, 전체 크기 1인 삼각형 정점 목록.
// 크기·위치·색상·회전은 공통 셰이더 uniform으로 처리하므로 도형별 셰이더는 없습니다.
inline std::vector<glm::vec3> MakeMesh(ShapeKind kind, int slices, int stacks)
{
    if (slices<3 || stacks<2)
        throw std::runtime_error("Mesh needs at least 3 slices and 2 stacks");
    std::vector<glm::vec3> vertices;

    // triangle(a,b,c): 삼각형 하나를 버퍼에 추가하는 지역 함수입니다.
    auto triangle=[&](glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
    };

    if (kind==ShapeKind::Box || kind==ShapeKind::Pyramid)
    {
        glm::vec3 p[]={
            {-.5f,-.5f,-.5f}, {.5f,-.5f,-.5f}, {.5f,.5f,-.5f}, {-.5f,.5f,-.5f},
            {-.5f,-.5f,.5f}, {.5f,-.5f,.5f}, {.5f,.5f,.5f}, {-.5f,.5f,.5f}
        };
        if (kind==ShapeKind::Box)
        {
            int indices[]={0,2,1,0,3,2,4,5,6,4,6,7,0,4,7,0,7,3,
                1,2,6,1,6,5,3,7,6,3,6,2,0,1,5,0,5,4};
            for (int index : indices)
                vertices.push_back(p[index]);
        }
        else
        {
            glm::vec3 top(0,.5f,0);
            triangle(p[0],top,p[1]);
            triangle(p[1],top,p[5]);
            triangle(p[5],top,p[4]);
            triangle(p[4],top,p[0]);
            triangle(p[0],p[1],p[5]);
            triangle(p[0],p[5],p[4]);
        }
    }
    else if (kind==ShapeKind::Sphere)
    {
        // spherePoint: 위도/경도에서 반지름 0.5인 구의 정점을 계산합니다.
        auto spherePoint=[](float latitude, float longitude)
        {
            return .5f*glm::vec3(glm::cos(latitude)*glm::cos(longitude),
                glm::sin(latitude),glm::cos(latitude)*glm::sin(longitude));
        };
        for (int y=0; y<stacks; ++y)
        {
            float v0=-glm::half_pi<float>()+glm::pi<float>()*y/stacks;
            float v1=-glm::half_pi<float>()+glm::pi<float>()*(y+1)/stacks;
            for (int x=0; x<slices; ++x)
            {
                float u0=glm::two_pi<float>()*x/slices;
                float u1=glm::two_pi<float>()*(x+1)/slices;
                glm::vec3 a=spherePoint(v0,u0), b=spherePoint(v0,u1);
                glm::vec3 c=spherePoint(v1,u0), d=spherePoint(v1,u1);
                if (y>0)
                    triangle(a,c,b);
                if (y<stacks-1)
                    triangle(b,c,d);
            }
        }
    }
    else
    {
        for (int i=0; i<slices; ++i)
        {
            float a=glm::two_pi<float>()*i/slices;
            float b=glm::two_pi<float>()*(i+1)/slices;
            glm::vec3 p(.5f*glm::cos(a),-.5f,.5f*glm::sin(a));
            glm::vec3 q(.5f*glm::cos(b),-.5f,.5f*glm::sin(b));
            triangle({0,-.5f,0},p,q);
            if (kind==ShapeKind::Cone)
                triangle(p,{0,.5f,0},q);
            else
            {
                glm::vec3 r(p.x,.5f,p.z), s(q.x,.5f,q.z);
                triangle(p,r,q);
                triangle(q,r,s);
                triangle({0,.5f,0},s,r);
            }
        }
    }
    return vertices;
}

// ============================================================================
// 5. 파트와 도형 ID: 숫자는 식별용이며, 높이/부모 순서는 Attach로 지정합니다.
// ============================================================================
enum TankPart { WHEELS, BODY, TURRETS, BARRELS, POLES };
enum ShapeId { WHEEL_LEFT=10, WHEEL_RIGHT=11, BODY_MAIN=20,
    TURRET_LEFT=30, TURRET_RIGHT=31, GUN_LEFT=40, GUN_RIGHT=41,
    POLE_LEFT=50, POLE_RIGHT=51 };

TankScene scene;
glm::vec3 cameraPosition(CAMERA_X,CAMERA_Y,CAMERA_Z);
glm::vec3 cameraTarget(CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z);
glm::vec3 cameraUp(0,1,0);
bool dragging=false;
int cameraSpin=0, cameraOrbit=0;
double previousMouseX=0, previousMouseY=0;
GLuint programID=0, vao=0, vbo=0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation;
struct MeshRange { int first, count; };
map<ShapeKind,MeshRange> meshRanges;

// ============================================================================
// 6. 탱크 조립 및 기능 설정: 실습 때 주로 이 함수만 바꾸면 됩니다.
// ============================================================================

// BuildTank(): 도형 생성 -> 연결 -> 기능 부여 -> Seal 순서로 설정합니다.
// 종류만 Box에서 Cylinder/Sphere/Cone/Pyramid로 바꿔도 같은 기능을 사용할 수 있습니다.
void BuildTank()
{
    scene.colorMin=COLOR_MIN;
    scene.colorMax=COLOR_MAX;

    // 6-1. 도형 생성: (고유ID, 파트, 초기중심/접점기준위치, 전체크기, 색상)
    // 같은 Random(파트번호)는 같은 랜덤색을 사용합니다.
    scene.Add(Box(WHEEL_LEFT,WHEELS,
        {-0.26f,0,0},
        {0.26f,0.24f,0.90f},Color::Random(WHEELS)));
    scene.Add(Box(WHEEL_RIGHT,WHEELS,
        {0.26f,0,0},
        {0.26f,0.24f,0.90f},Color::Random(WHEELS)));
    scene.Add(Box(BODY_MAIN,BODY,{0,0,0},
        {0.90f,0.12f,0.62f},Color::Random(BODY)));
    scene.Add(Box(TURRET_LEFT,TURRETS,{-0.22f,0,0},
        {0.28f,0.22f,0.32f},Color::Random(TURRETS)));
    scene.Add(Box(TURRET_RIGHT,TURRETS,{0.22f,0,0},
        {0.28f,0.22f,0.32f},Color::Random(TURRETS)));
    scene.Add(Box(GUN_LEFT,BARRELS,{0,0,0},
        {0.045f,0.045f,0.40f},Color::Random(BARRELS)));
    scene.Add(Box(GUN_RIGHT,BARRELS,{0,0,0},
        {0.045f,0.045f,0.40f},Color::Random(BARRELS)));
    scene.Add(Box(POLE_LEFT,POLES,{0,0,0},
        {0.035f,0.42f,0.035f},Color::Random(POLES)));
    scene.Add(Box(POLE_RIGHT,POLES,{0,0,0},
        {0.035f,0.42f,0.035f},Color::Random(POLES)));

    // 층은 높이 순서만 지정합니다. 중간 층의 도형을 지우면 그 층은 자동으로 건너뜁니다.
    scene.SetLayer(Part(WHEELS),1);
    scene.SetLayer(Id(BODY_MAIN),2);
    // BODY_MAIN이 있으면 그 이동/회전을 상속하고, 없어도 3층은 아래층 위에 남습니다.
    scene.SetLayer(Part(TURRETS),3,BODY_MAIN);

    // 6-2. 부속 연결: 각 층의 개별 도형에 붙입니다. 부속에도 생성 각도와 독립 기능을 설정할 수 있습니다.
    // 부속은 지정 부모 도형에만 연결됩니다. 부모가 없어지면 함께 숨깁니다.
    scene.Attach(GUN_LEFT,TURRET_LEFT,Shape::back,Shape::center);
    scene.Attach(GUN_RIGHT,TURRET_RIGHT,Shape::back,Shape::center);
    scene.Attach(POLE_LEFT,TURRET_LEFT,Shape::bottom,Shape::center);
    scene.Attach(POLE_RIGHT,TURRET_RIGHT,Shape::bottom,Shape::center);

    // 임의 각도 부속 추가 예 (사용하려면 두 줄 모두 주석 해제):
    // scene.Add(Cylinder(60,BARRELS,{0,0,0},{.1f,.5f,.1f},Color::Random(),{25,40,15}));
    // scene.Attach(60,WHEEL_LEFT,Shape::bottom,Shape::center);
    // BODY_MAIN 등 어떤 도형을 부모로 지정해도 같은 방식으로 동작합니다.

    // 6-3. 신축 대상은 Layer(1), Part(WHEELS), Id(WHEEL_LEFT) 중 원하는 것으로 바꿉니다.
    scene.AddScale(Layer(1),GLFW_KEY_D,{0,1,0},Shape::bottom,
        LOWER_SCALE_MIN,LOWER_SCALE_MAX,LOWER_SCALE_SPEED,Mode::PingPong);

    // 6-4. 개별 회전: 부모 회전에 더해서 자기 기준점으로 독립 회전합니다.
    scene.AddRotation(Id(BODY_MAIN),GLFW_KEY_T,{0,1,0},Shape::center,
        1,MIDDLE_ROTATE_SPEED,MIDDLE_CYCLE_ANGLE,Mode::Once);
    int leftGun=scene.AddRotation(Id(GUN_LEFT),GLFW_KEY_G,{0,1,0},Shape::back,
        1,BARREL_ROTATE_SPEED,BARREL_CYCLE_ANGLE,Mode::Once);
    int rightGun=scene.AddRotation(Id(GUN_RIGHT),GLFW_KEY_G,{0,1,0},Shape::back,
        -1,BARREL_ROTATE_SPEED,BARREL_CYCLE_ANGLE,Mode::Once);
    // 바깥쪽 전용 보정이 필요 없으면 다음 두 줄을 제거하고 위 방향(+1/-1)을 설정합니다.
    // 같은 방향은 둘 다 +1, 반대 방향은 +1/-1입니다.
    scene.SetOutwardRotation(leftGun,TURRET_LEFT);
    scene.SetOutwardRotation(rightGun,TURRET_RIGHT);
    scene.AddRotation(Id(POLE_LEFT),GLFW_KEY_P,{1,0,0},Shape::bottom,
        -1,POLE_ROTATE_SPEED,POLE_CYCLE_ANGLE,Mode::Once);
    scene.AddRotation(Id(POLE_RIGHT),GLFW_KEY_P,{1,0,0},Shape::bottom,
        1,POLE_ROTATE_SPEED,POLE_CYCLE_ANGLE,Mode::Once);

    // 6-5. 위치 교환: 각 상부에 연결한 포신과 깃대도 함께 이동합니다.
    scene.AddSwap(TURRET_LEFT,TURRET_RIGHT,GLFW_KEY_L,SWAP_DURATION);

    // 추가 예시 (필요할 때 주석 해제): u 키로 상부 파트 전체를 X방향 0.2만큼 왕복.
    // scene.AddMovement(Part(TURRETS),GLFW_KEY_U,{1,0,0},.2f,.1f,Mode::PingPong);
    scene.Seal();
}

// ============================================================================
// 1번 교체 예시 (전체 주석): 아래 함수의 주석을 풀고 기존 BuildTank 대신 사용합니다.
// 방법: main의 BuildTank(); 한 줄만 BuildTankExample1(); 로 바꿉니다.
// 두 함수를 함께 호출하지 않습니다. 크기와 키는 이 예시 안에서 바로 수정합니다.
// d: 1층 신축, u: 1층 이동, f: 2층 신축, t: 2층 회전,
// g: 조종석 각각 회전, p: 원뿔 앞뒤 왕복. c: 전체 초기화.
// ============================================================================
/* EXAMPLE_1_BEGIN
void BuildTankExample1()
{
    scene.colorMin=COLOR_MIN;
    scene.colorMax=COLOR_MAX;

    // 1. 이 예제 안에서만 사용하는 ID와 파트 이름입니다.
    enum ExamplePart { BASE_PART=100, DISC_PART, CAB_PART, CONE_PART };
    enum ExampleId { BASE=1000, DISC=2000, CAB_LEFT=3000, CAB_RIGHT=3001,
        CONE_LEFT=3010, CONE_RIGHT=3011 };

    // 2. 1층: Z방향으로 긴 직육면체 바퀴 하나 (폭 .55 / 높이 .22 / 깊이 1.10).
    scene.Add(Box(BASE,BASE_PART,{0,0,0},{.55f,.22f,1.10f},
        Color::Fixed(.25f,.25f,.30f)));

    // 3. 2층: 윗방향이 +Y인 납작한 원기둥 (지름 1.0 / 높이 .14).
    scene.Add(Cylinder(DISC,DISC_PART,{0,0,0},{1.0f,.14f,1.0f},
        Color::Fixed(.20f,.65f,.85f),{0,0,0}));

    // 4. 3층: 한 변 .28인 정육면체 조종석 두 개.
    scene.Add(Box(CAB_LEFT,CAB_PART,{-.23f,0,0},{.28f,.28f,.28f},
        Color::Fixed(.85f,.35f,.20f)));
    scene.Add(Box(CAB_RIGHT,CAB_PART,{.23f,0,0},{.28f,.28f,.28f},
        Color::Fixed(.85f,.35f,.20f)));

    // 5. 각 조종석의 부속: 윗방향 +Y, 밑면 지름 .10 / 높이 .50의 가는 원뿔.
    scene.Add(Cone(CONE_LEFT,CONE_PART,{0,0,0},{.10f,.50f,.10f},
        Color::Fixed(.85f,.75f,.15f),{0,0,0}));
    scene.Add(Cone(CONE_RIGHT,CONE_PART,{0,0,0},{.10f,.50f,.10f},
        Color::Fixed(.85f,.75f,.15f),{0,0,0}));

    // 6. 층 번호는 높이 순서, 세 번째 인자는 이동/회전을 따라갈 도형입니다.
    // 아래에서 위로만 상속: BASE -> DISC -> 조종석 -> 각 원뿔.
    scene.SetLayer(Id(BASE),1);
    scene.SetLayer(Id(DISC),2,BASE);
    scene.SetLayer(Part(CAB_PART),3,DISC);

    // 7. 원뿔의 밑면 중심을 조종석 윗면 중심에 붙입니다.
    // 원뿔은 층 높이에 포함하지 않으며 자기 조종석이 없으면 함께 숨습니다.
    scene.Attach(CONE_LEFT,CAB_LEFT,Shape::bottom,Shape::top);
    scene.Attach(CONE_RIGHT,CAB_RIGHT,Shape::bottom,Shape::top);

    // 8. 1층: 밑면을 고정한 Y신축 + X방향 .25거리 왕복 이동.
    // 1층의 이동은 위 전체에 전달되고 신축은 위층 높이에 반영됩니다.
    scene.AddScale(Layer(1),GLFW_KEY_D,{0,1,0},Shape::bottom,
        .60f,1.50f,.40f,Mode::PingPong);
    scene.AddMovement(Id(BASE),GLFW_KEY_U,{1,0,0},.25f,.15f,
        Mode::PingPong,TransformSpace::World);

    // 9. 2층: 밑면 고정 Y신축. 원기둥 중심의 Y축으로 180도씩 회전.
    // 1층은 영향을 받지 않고, 두 조종석과 원뿔이 함께 따라갑니다.
    scene.AddScale(Layer(2),GLFW_KEY_F,{0,1,0},Shape::bottom,
        .60f,1.80f,.40f,Mode::PingPong);
    scene.AddRotation(Id(DISC),GLFW_KEY_T,{0,1,0},Shape::center,
        1,60,180,Mode::Once);

    // 10. 3층: 각각의 조종석 중심에서 같은 방향으로 180도씩 회전.
    // 아래 1/2층은 그대로이며 각 원뿔만 자기 조종석을 따라갑니다.
    scene.AddRotation(Part(CAB_PART),GLFW_KEY_G,{0,1,0},Shape::center,
        1,90,180,Mode::Once);

    // 11. 원뿔: 밑면 중심 고정. 로컬 X축으로 -30~+30도 앞뒤 왕복.
    // 0도(+Y)를 향한 상태에서 시작합니다. 로컬 앞뒤는 조종석 회전도 따라갑니다.
    scene.AddSwingRotation(Part(CONE_PART),GLFW_KEY_P,{1,0,0},Shape::bottom,
        -30,30,45,TransformSpace::Local);

    scene.Seal();
}
EXAMPLE_1_END */

// ============================================================================
// 7. 함수 선언
// ============================================================================
string ReadFile(const char* name);
bool InitShader();
void DrawScene();
void UpdateScene(GLFWwindow* window,float dt);
void KeyCallback(GLFWwindow* window,int key,int scancode,int action,int mods);
void MouseCallback(GLFWwindow* window,int button,int action,int mods);
void CursorCallback(GLFWwindow* window,double x,double y);
void ScrollCallback(GLFWwindow* window,double xoffset,double yoffset);

// ============================================================================
// 8. 초기화 / 메인 반복문 / 종료
// ============================================================================

// main(): 공통 메시를 한 번 생성하고, 매 프레임 입력 -> 애니메이션 -> 그리기를 수행합니다.
int main()
{
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);
    GLFWwindow* window=glfwCreateWindow(WINDOW_WIDTH,WINDOW_HEIGHT,"Midterm Tank Builder",nullptr,nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental=GL_TRUE;
    if (glewInit()!=GLEW_OK || !InitShader())
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);
    glfwSetKeyCallback(window,KeyCallback);
    glfwSetMouseButtonCallback(window,MouseCallback);
    glfwSetCursorPosCallback(window,CursorCallback);
    glfwSetScrollCallback(window,ScrollCallback);

    int result=0;
    try
    {
        BuildTank();
        vector<glm::vec3> vertices;
        for (ShapeKind kind : {ShapeKind::Box,ShapeKind::Cone,ShapeKind::Cylinder,ShapeKind::Sphere,ShapeKind::Pyramid})
        {
            auto mesh=MakeMesh(kind,MESH_SLICES,MESH_STACKS);
            meshRanges[kind]={(int)vertices.size(),(int)mesh.size()};
            vertices.insert(vertices.end(),mesh.begin(),mesh.end());
        }
        glGenVertexArrays(1,&vao);
        glGenBuffers(1,&vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,vertices.size()*sizeof(glm::vec3),vertices.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        cout << "Arrows: tank | d: scale | t/g/p: rotate | l: swap | x/X,z/Z: camera\n"
             << "y/Y: spin | r/R: orbit | drag/wheel: camera | o: colors | c: reset | q: quit" << endl;
        double previousTime=glfwGetTime();
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            double now=glfwGetTime();
            float dt=(float)glm::min(now-previousTime,0.1);
            previousTime=now;
            UpdateScene(window,dt);
            int width,height;
            glfwGetFramebufferSize(window,&width,&height);
            glViewport(0,0,width,height);
            DrawScene();
            glfwSwapBuffers(window);
        }
    }
    catch (const exception& error)
    {
        cerr << "Tank configuration error: " << error.what() << endl;
        result=-1;
    }
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glDeleteProgram(programID);
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}

// ============================================================================
// 9. 매 프레임 처리
// ============================================================================

// UpdateScene(window,dt): 탱크/카메라 연속 입력과 등록한 기능들을 처리합니다.
void UpdateScene(GLFWwindow* window,float dt)
{
    glm::vec3 movement(0),cameraMovement(0);
    if (glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS)
        movement.x-=1;
    if (glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)
        movement.x+=1;
    if (glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)
        movement.z-=1;
    if (glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS)
        movement.z+=1;
    if (glm::length(movement)>0)
        scene.position+=glm::normalize(movement)*TANK_MOVE_SPEED*dt;
    bool shift=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS
        || glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
    float direction=shift ? -1.0f : 1.0f;
    if (glfwGetKey(window,GLFW_KEY_X)==GLFW_PRESS)
        cameraMovement.x+=direction;
    if (glfwGetKey(window,GLFW_KEY_Z)==GLFW_PRESS)
        cameraMovement.z+=direction;
    if (glm::length(cameraMovement)>0)
    {
        cameraMovement=glm::normalize(cameraMovement)*CAMERA_MOVE_SPEED*dt;
        cameraPosition+=cameraMovement;
        cameraTarget+=cameraMovement;
    }
    if (cameraOrbit)
    {
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(cameraOrbit*CAMERA_ROTATE_SPEED*dt),glm::vec3(0,1,0));
        cameraPosition=glm::vec3(rotation*glm::vec4(cameraPosition,1));
        cameraTarget=glm::vec3(rotation*glm::vec4(cameraTarget,1));
        cameraUp=glm::vec3(rotation*glm::vec4(cameraUp,0));
    }
    if (cameraSpin)
    {
        // 카메라 자신의 위쪽 축으로 제자리 회전합니다.
        glm::vec3 forward=glm::normalize(cameraTarget-cameraPosition);
        glm::vec3 right=glm::normalize(glm::cross(forward,cameraUp));
        glm::vec3 up=glm::normalize(glm::cross(right,forward));
        glm::mat4 rotation=glm::rotate(glm::mat4(1),glm::radians(cameraSpin*CAMERA_ROTATE_SPEED*dt),up);
        cameraTarget=cameraPosition+glm::vec3(rotation*glm::vec4(cameraTarget-cameraPosition,0));
        cameraUp=up;
    }
    scene.Update(dt);
    scene.KeepOnFloor(FLOOR_WIDTH,FLOOR_DEPTH);
}

// ============================================================================
// 10. 공통 셰이더로 그리기
// ============================================================================

// DrawScene(): 도형 종류에 맞는 메시 구간만 바꾸고, 변환/색상은 같은 uniform으로 전달합니다.
void DrawScene()
{
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    glm::mat4 view=glm::lookAt(cameraPosition,cameraTarget,cameraUp);
    glm::mat4 projection=glm::perspective(glm::radians(PERSPECTIVE_FOVY),PERSPECTIVE_ASPECT,PERSPECTIVE_NEAR,PERSPECTIVE_FAR);
    glUniformMatrix4fv(viewLocation,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation,1,GL_FALSE,glm::value_ptr(projection));
    glBindVertexArray(vao);
    glm::mat4 floor=glm::scale(glm::translate(glm::mat4(1),glm::vec3(0,-FLOOR_HEIGHT*.5f,0)),
        glm::vec3(FLOOR_WIDTH,FLOOR_HEIGHT,FLOOR_DEPTH));
    glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(floor));
    glUniform3f(colorLocation,FLOOR_RED,FLOOR_GREEN,FLOOR_BLUE);
    glDrawArrays(GL_TRIANGLES,meshRanges.at(ShapeKind::Box).first,meshRanges.at(ShapeKind::Box).count);
    for (const auto& [id,shape] : scene.shapes)
    {
        if (!shape.visible)
            continue;
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(shape.geometry));
        glUniform3fv(colorLocation,1,glm::value_ptr(shape.color.value));
        MeshRange range=meshRanges.at(shape.kind);
        glDrawArrays(GL_TRIANGLES,range.first,range.count);
    }
    glBindVertexArray(0);
}

// ============================================================================
// 11. 키보드와 마우스 입력
// ============================================================================

// KeyCallback: 등록한 키는 scene.Trigger(key)로 전달합니다. 기능 추가 시 이 함수는 수정하지 않습니다.
void KeyCallback(GLFWwindow* window,int key,int,int action,int mods)
{
    if (action!=GLFW_PRESS)
        return;
    scene.Trigger(key);
    if (key==GLFW_KEY_Q || key==GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window,true);
    int direction=(mods&GLFW_MOD_SHIFT) ? 1 : -1;
    if (key==GLFW_KEY_Y)
        cameraSpin=cameraSpin==direction ? 0 : direction;
    if (key==GLFW_KEY_R)
        cameraOrbit=cameraOrbit==direction ? 0 : direction;
    if (key==GLFW_KEY_O)
        scene.RandomizeColors();
    if (key==GLFW_KEY_C)
    {
        scene.Reset();
        cameraPosition={CAMERA_X,CAMERA_Y,CAMERA_Z};
        cameraTarget={CAMERA_TARGET_X,CAMERA_TARGET_Y,CAMERA_TARGET_Z};
        cameraSpin=cameraOrbit=0;
        cameraUp={0,1,0};
        dragging=false;
    }
}

// MouseCallback: 좌클릭을 놓아도 현재 시점을 유지합니다.
void MouseCallback(GLFWwindow* window,int button,int action,int)
{
    if (button!=GLFW_MOUSE_BUTTON_LEFT)
        return;
    dragging=action==GLFW_PRESS;
    if (dragging)
        glfwGetCursorPos(window,&previousMouseX,&previousMouseY);
}

// CursorCallback: 주시점을 중심으로 드래그 회전합니다. 창 전체 너비가 ORBIT_DEGREES입니다.
void CursorCallback(GLFWwindow* window,double x,double y)
{
    if (!dragging)
        return;
    int width,height;
    glfwGetWindowSize(window,&width,&height);
    if (width<=0 || height<=0)
        return;
    glm::vec3 offset=cameraPosition-cameraTarget;
    float radius=glm::length(offset);
    float yaw=glm::atan(offset.x,offset.z);
    float pitch=glm::asin(glm::clamp(offset.y/radius,-1.0f,1.0f));
    yaw-=glm::radians(ORBIT_DEGREES)*(float)(x-previousMouseX)/width;
    pitch+=glm::radians(ORBIT_DEGREES)*(float)(y-previousMouseY)/height;
    previousMouseX=x;
    previousMouseY=y;
    cameraUp={0,1,0};
    pitch=glm::clamp(pitch,-glm::radians(ORBIT_PITCH_LIMIT),glm::radians(ORBIT_PITCH_LIMIT));
    cameraPosition=cameraTarget+radius*glm::vec3(glm::cos(pitch)*glm::sin(yaw),glm::sin(pitch),glm::cos(pitch)*glm::cos(yaw));
}

// ScrollCallback: 주시점까지 거리를 조절합니다. 상하한은 define으로 설정합니다.
void ScrollCallback(GLFWwindow*,double,double yoffset)
{
    glm::vec3 offset=cameraPosition-cameraTarget;
    float radius=glm::clamp(glm::length(offset)-(float)yoffset*ZOOM_STEP,CAMERA_RADIUS_MIN,CAMERA_RADIUS_MAX);
    cameraPosition=cameraTarget+glm::normalize(offset)*radius;
}

// ============================================================================
// 12. 셰이더 파일 로딩과 컴파일
// ============================================================================

// ReadFile(name): 프로젝트의 GLSL 파일을 읽습니다. 실패하면 빈 문자열을 반환합니다.
string ReadFile(const char* name)
{
    ifstream file(name);
    if (!file)
    {
        cerr << "Cannot open shader: " << name << endl;
        return "";
    }
    stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

// InitShader(): 모든 도형이 사용하는 공통 셰이더 프로그램을 한 번 생성합니다.
bool InitShader()
{
    string sources[2]={ReadFile("vertex.glsl"),ReadFile("fragment.glsl")};
    if (sources[0].empty() || sources[1].empty())
        return false;
    GLuint shaders[2]={glCreateShader(GL_VERTEX_SHADER),glCreateShader(GL_FRAGMENT_SHADER)};
    for (int i=0; i<2; ++i)
    {
        const char* code=sources[i].c_str();
        glShaderSource(shaders[i],1,&code,nullptr);
        glCompileShader(shaders[i]);
        GLint ok;
        glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);
        if (!ok)
        {
            char log[1024];
            glGetShaderInfoLog(shaders[i],1024,nullptr,log);
            cerr << log << endl;
            return false;
        }
    }
    programID=glCreateProgram();
    glAttachShader(programID,shaders[0]);
    glAttachShader(programID,shaders[1]);
    glLinkProgram(programID);
    GLint ok;
    glGetProgramiv(programID,GL_LINK_STATUS,&ok);
    if (!ok)
    {
        char log[1024];
        glGetProgramInfoLog(programID,1024,nullptr,log);
        cerr << log << endl;
        return false;
    }
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
    modelLocation=glGetUniformLocation(programID,"modelTransform");
    viewLocation=glGetUniformLocation(programID,"viewTransform");
    projectionLocation=glGetUniformLocation(programID,"projectionTransform");
    colorLocation=glGetUniformLocation(programID,"uColor");
    return modelLocation>=0 && viewLocation>=0 && projectionLocation>=0 && colorLocation>=0;
}
