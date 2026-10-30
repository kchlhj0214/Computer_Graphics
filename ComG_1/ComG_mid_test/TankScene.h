#pragma once
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <random>
#include <algorithm>
#include <cmath>

// ============================================================================
// 1. 도형과 설정 자료형 (위치는 중심, 크기는 전체 폭/높이/깊이)
// ============================================================================
enum class ShapeKind { Box, Cone, Cylinder, Sphere, Pyramid };
enum class Mode { Once, Loop, PingPong };
enum class Anchor { Center, Left, Right, Bottom, Top, Front, Back };
enum class ActionKind { Move, Rotate, Scale, Swap };

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
    std::vector<int> supports;
    Anchor ownAnchor = center, parentAnchor = center;
    glm::vec3 attachmentOffset{0};
    bool inheritRotation = true;
    bool floorBound = true;
    glm::mat4 geometry{1}, frame{1};
    glm::quat worldRotation{1, 0, 0, 0};

    // angles는 생성 후 선택적으로 설정하는 초기 X/Y/Z 각도(도)입니다.
    Shape(int id, int part, ShapeKind kind, glm::vec3 position, glm::vec3 size, Color color)
        : id(id), part(part), kind(kind), position(position), size(size), color(color)
    {
    }
};

// 아래 클래스는 모두 같은 셰이더/연결/동작 코드를 사용합니다.
// 예: scene.Add(Cylinder(10, WHEELS, {0,0,0}, {.3f,.2f,.3f}, Color::Random()));
// Box(ID,파트,위치,크기,색상): 직육면체를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Box : Shape
{
    Box(int id, int part, glm::vec3 position, glm::vec3 size, Color color)
        : Shape(id, part, ShapeKind::Box, position, size, color)
    {
    }
};

// Cone(ID,파트,위치,크기,색상): 원뿔를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Cone : Shape
{
    Cone(int id, int part, glm::vec3 position, glm::vec3 size, Color color)
        : Shape(id, part, ShapeKind::Cone, position, size, color)
    {
    }
};

// Cylinder(ID,파트,위치,크기,색상): 원기둥를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Cylinder : Shape
{
    Cylinder(int id, int part, glm::vec3 position, glm::vec3 size, Color color)
        : Shape(id, part, ShapeKind::Cylinder, position, size, color)
    {
    }
};

// Sphere(ID,파트,위치,크기,색상): 구를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Sphere : Shape
{
    Sphere(int id, int part, glm::vec3 position, glm::vec3 size, Color color)
        : Shape(id, part, ShapeKind::Sphere, position, size, color)
    {
    }
};

// Pyramid(ID,파트,위치,크기,색상): 사각뿔를 생성합니다. 실제 등록은 scene.Add로 합니다.
struct Pyramid : Shape
{
    Pyramid(int id, int part, glm::vec3 position, glm::vec3 size, Color color)
        : Shape(id, part, ShapeKind::Pyramid, position, size, color)
    {
    }
};

// Id(10): 한 도형. Part(WHEELS): 해당 파트 전체. 모든 도형을 만든 뒤 연결/기능을 등록합니다.
struct Target
{
    int value;
    bool part;
};
// Id(번호): 기능을 개별 도형에만 적용할 때 사용합니다.
inline Target Id(int id)
{
    return {id, false};
}
// Part(파트번호): 기능을 파트 전체에 동시에 적용할 때 사용합니다.
inline Target Part(int part)
{
    return {part, true};
}

struct Action
{
    ActionKind kind;
    Target target;
    int key;
    glm::vec3 axis{0, 1, 0};
    Anchor pivot = Shape::center;
    Mode mode = Mode::Once;
    float speed = 1, amount = 1, low = 0.5f, high = 1.5f;
    float value = 0, progress = 0, start = 0, sign = 1;
    float direction = 1;
    bool active = false;
    int other = -1, outwardReference = -1;
    glm::vec3 swapDelta{0};
};

// ============================================================================
// 2. 도형 등록 / 연결 / 기능 등록
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

    // Attach(자식, 부모, 자식접점, 부모접점, 여유위치, 회전상속).
    // 자식.position은 접점에서의 추가 위치입니다. 부모 크기는 상속하지 않습니다.
    // 포신: Attach(gun, turret, Shape::back, Shape::center).
    void Attach(int child, int parent, Anchor own = Shape::bottom,
                Anchor parentPoint = Shape::top, glm::vec3 offset = {0,0,0}, bool rotate = true)
    {
        Get(parent);
        Shape& s = Get(child);
        s.parent = parent;
        s.supports.clear();
        s.ownAnchor = own;
        s.parentAnchor = parentPoint;
        s.attachmentOffset = offset;
        s.inheritRotation = rotate;
    }

    // AttachOnTop(몸통, Part(WHEELS)): 지지 도형들의 월드 바운딩 박스 중 최고 높이 사용.
    // XZ는 지지 도형 중심의 평균, 몸통은 수평 유지. 서로 다른 높이도 처리합니다.
    // 각 지지 도형의 회전은 상속하지 않습니다. 일반 부모 회전 상속은 Attach를 사용합니다.
    void AttachOnTop(int child, Target supports, glm::vec3 offset = {0,0,0})
    {
        Shape& s = Get(child);
        s.parent = -1;
        s.supports = Resolve(supports);
        s.ownAnchor = Shape::bottom;
        s.attachmentOffset = offset;
        s.inheritRotation = false;
    }

    // AddMovement(대상,키,방향,거리,초당거리,반복방식).
    // 방향은 부모 좌표 기준이며 자동 정규화합니다. 파트 전체는 같은 진행값을 씁니다.
    int AddMovement(Target target, int key, glm::vec3 direction, float distance,
                    float speed, Mode mode = Mode::Once)
    {
        Action a{ActionKind::Move, target, key};
        a.axis = Unit(direction);
        a.amount = distance;
        a.speed = speed;
        a.mode = mode;
        return AddAction(a);
    }

    // AddRotation(대상,키,회전축,기준점,+1/-1 방향,초당각도,사이클각도,반복방식).
    // 기준점은 신축 후 도형의 면 중심입니다. 부모를 유지하면서 자신의 축으로 회전합니다.
    int AddRotation(Target target, int key, glm::vec3 axis, Anchor pivot,
                    float direction, float speed, float cycle = 360, Mode mode = Mode::Once)
    {
        Action a{ActionKind::Rotate, target, key};
        a.axis = Unit(axis);
        a.pivot = pivot;
        a.direction = direction < 0 ? -1.0f : 1.0f;
        a.sign = a.direction;
        a.speed = speed;
        a.amount = cycle;
        a.mode = mode;
        return AddAction(a);
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
        Get(reference);
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
            Resolve(a.target);
            if (a.kind == ActionKind::Swap)
            {
                Shape& x = Get(a.target.value);
                Shape& y = Get(a.other);
                if (x.parent != y.parent || x.ownAnchor != y.ownAnchor || x.parentAnchor != y.parentAnchor ||
                    !x.supports.empty() || !y.supports.empty() || x.attachmentOffset != y.attachmentOffset)
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
            if (a.key != key)
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
            if (a.outwardReference >= 0)
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
            if (a.kind == ActionKind::Scale)
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
            if (!s.floorBound)
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

    // Resolve(target): ID 또는 파트에 속한 도형 ID 목록을 구합니다.
    std::vector<int> Resolve(Target target)
    {
        std::vector<int> result;
        for (const auto& [id, s] : shapes)
            if ((!target.part && id==target.value) || (target.part && s.part==target.value))
                result.push_back(id);
        if (result.empty())
            throw std::runtime_error("Empty target: " + std::to_string(target.value));
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
        return a.target.part ? s.part==a.target.value : s.id==a.target.value;
    }

    // Movement: 개별 이동과 위치 교환을 합산합니다. 부모 좌표계 기준입니다.
    glm::vec3 Movement(int id)
    {
        glm::vec3 result(0);
        const Shape& s=Get(id);
        for (const Action& a : actions)
        {
            if (a.kind==ActionKind::Move && Matches(a,s))
                result+=a.axis*a.value;
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
        glm::vec3 origin=position;
        glm::quat inherited(1,0,0,0);
        glm::quat initial=glm::quat(glm::radians(s.angles));
        if (s.parent>=0)
        {
            EvaluateOne(s.parent,state);
            Shape& parent=Get(s.parent);
            origin=glm::vec3(parent.geometry*glm::vec4(AnchorPoint(s.parentAnchor),1));
            if (s.inheritRotation)
                inherited=parent.worldRotation;
        }
        else if (!s.supports.empty())
        {
            origin=glm::vec3(0);
            float top=-1e9f;
            for (int support : s.supports)
            {
                EvaluateOne(support,state);
                Shape& parent=Get(support);
                origin+=glm::vec3(parent.geometry[3]);
                top=glm::max(top,Bounds(parent.geometry).second.y);
            }
            origin/=(float)s.supports.size();
            origin.y=top;
        }
        glm::vec3 ownPoint(0);
        if (s.parent>=0 || !s.supports.empty())
            ownPoint=initial*(AnchorPoint(s.ownAnchor)*s.size);
        origin+=inherited*(s.position+s.attachmentOffset-ownPoint+Movement(id));

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
                rotationAnchor=a.pivot;
            }
        }
        glm::vec3 scaleShift=scalePivot*(glm::vec3(1)-scale);
        glm::vec3 rotationPivot=initial*(AnchorPoint(rotationAnchor)*s.size*scale+scaleShift);
        glm::quat rotation=glm::angleAxis(glm::radians(glm::mod(angle,360.0f)), initial*rotationAxis);
        s.worldRotation=inherited*rotation*initial;
        s.frame=glm::translate(glm::mat4(1),origin)*glm::mat4_cast(inherited)
            *glm::translate(glm::mat4(1),rotationPivot)*glm::mat4_cast(rotation)
            *glm::translate(glm::mat4(1),-rotationPivot)*glm::mat4_cast(initial)
            *glm::translate(glm::mat4(1),scaleShift);
        s.geometry=glm::scale(s.frame,s.size*scale);
        state[id]=2;
    }
};
