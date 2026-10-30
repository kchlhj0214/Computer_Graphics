#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "TankScene.h"
#include "ShapeMeshes.h"
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
// 탱크 부품 크기
// ===========================================================================
#define LOWER_WIDTH 0.26f
#define LOWER_HEIGHT 0.24f
#define LOWER_DEPTH 0.90f
#define LOWER_GAP 0.26f
#define MIDDLE_WIDTH 0.90f
#define MIDDLE_HEIGHT 0.12f
#define MIDDLE_DEPTH 0.62f
#define UPPER_WIDTH 0.28f
#define UPPER_HEIGHT 0.22f
#define UPPER_DEPTH 0.32f
#define UPPER_GAP 0.16f
#define BARREL_WIDTH 0.045f
#define BARREL_HEIGHT 0.045f
#define BARREL_LENGTH 0.40f
#define POLE_WIDTH 0.035f
#define POLE_HEIGHT 0.42f
#define POLE_DEPTH 0.035f
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
// 2. 파트와 도형 ID: 숫자는 식별용이며, 높이/부모 순서는 Attach로 지정합니다.
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
// 3. 탱크 조립 및 기능 설정: 실습 때 주로 이 함수만 바꾸면 됩니다.
// ============================================================================

// BuildTank(): 도형 생성 -> 연결 -> 기능 부여 -> Seal 순서로 설정합니다.
// 종류만 Box에서 Cylinder/Sphere/Cone/Pyramid로 바꿔도 같은 기능을 사용할 수 있습니다.
void BuildTank()
{
    scene.colorMin=COLOR_MIN;
    scene.colorMax=COLOR_MAX;

    // 3-1. 도형 생성: (고유ID, 파트, 초기중심/접점기준위치, 전체크기, 색상)
    // 같은 Random(파트번호)는 같은 랜덤색을 사용합니다.
    scene.Add(Box(WHEEL_LEFT,WHEELS,
        {-(LOWER_WIDTH+LOWER_GAP)*.5f,LOWER_HEIGHT*.5f,0},
        {LOWER_WIDTH,LOWER_HEIGHT,LOWER_DEPTH},Color::Random(WHEELS)));
    scene.Add(Box(WHEEL_RIGHT,WHEELS,
        {(LOWER_WIDTH+LOWER_GAP)*.5f,LOWER_HEIGHT*.5f,0},
        {LOWER_WIDTH,LOWER_HEIGHT,LOWER_DEPTH},Color::Random(WHEELS)));
    scene.Add(Box(BODY_MAIN,BODY,{0,0,0},
        {MIDDLE_WIDTH,MIDDLE_HEIGHT,MIDDLE_DEPTH},Color::Random(BODY)));
    scene.Add(Box(TURRET_LEFT,TURRETS,{-(UPPER_WIDTH+UPPER_GAP)*.5f,0,0},
        {UPPER_WIDTH,UPPER_HEIGHT,UPPER_DEPTH},Color::Random(TURRETS)));
    scene.Add(Box(TURRET_RIGHT,TURRETS,{(UPPER_WIDTH+UPPER_GAP)*.5f,0,0},
        {UPPER_WIDTH,UPPER_HEIGHT,UPPER_DEPTH},Color::Random(TURRETS)));
    scene.Add(Box(GUN_LEFT,BARRELS,{0,0,0},
        {BARREL_WIDTH,BARREL_HEIGHT,BARREL_LENGTH},Color::Random(BARRELS)));
    scene.Add(Box(GUN_RIGHT,BARRELS,{0,0,0},
        {BARREL_WIDTH,BARREL_HEIGHT,BARREL_LENGTH},Color::Random(BARRELS)));
    scene.Add(Box(POLE_LEFT,POLES,{0,0,0},
        {POLE_WIDTH,POLE_HEIGHT,POLE_DEPTH},Color::Random(POLES)));
    scene.Add(Box(POLE_RIGHT,POLES,{0,0,0},
        {POLE_WIDTH,POLE_HEIGHT,POLE_DEPTH},Color::Random(POLES)));

    // 3-2. 접점 연결: 부모 신축은 높이만, 부모 이동/회전은 전체 자식에 전달됩니다.
    scene.AttachOnTop(BODY_MAIN,Part(WHEELS));
    scene.Attach(TURRET_LEFT,BODY_MAIN,Shape::bottom,Shape::top);
    scene.Attach(TURRET_RIGHT,BODY_MAIN,Shape::bottom,Shape::top);
    scene.Attach(GUN_LEFT,TURRET_LEFT,Shape::back,Shape::center);
    scene.Attach(GUN_RIGHT,TURRET_RIGHT,Shape::back,Shape::center);
    scene.Attach(POLE_LEFT,TURRET_LEFT,Shape::bottom,Shape::center);
    scene.Attach(POLE_RIGHT,TURRET_RIGHT,Shape::bottom,Shape::center);

    // 3-3. 파트 전체 신축: 바퀴 두 개는 한 기능의 동일한 배율을 공유합니다.
    scene.AddScale(Part(WHEELS),GLFW_KEY_D,{0,1,0},Shape::bottom,
        LOWER_SCALE_MIN,LOWER_SCALE_MAX,LOWER_SCALE_SPEED,Mode::PingPong);

    // 3-4. 개별 회전: 부모 회전에 더해서 자기 기준점으로 독립 회전합니다.
    scene.AddRotation(Id(BODY_MAIN),GLFW_KEY_T,{0,1,0},Shape::center,
        1,MIDDLE_ROTATE_SPEED,MIDDLE_CYCLE_ANGLE,Mode::Once);
    int leftGun=scene.AddRotation(Id(GUN_LEFT),GLFW_KEY_G,{0,1,0},Shape::back,
        1,BARREL_ROTATE_SPEED,BARREL_CYCLE_ANGLE,Mode::Once);
    int rightGun=scene.AddRotation(Id(GUN_RIGHT),GLFW_KEY_G,{0,1,0},Shape::back,
        -1,BARREL_ROTATE_SPEED,BARREL_CYCLE_ANGLE,Mode::Once);
    scene.SetOutwardRotation(leftGun,TURRET_LEFT);
    scene.SetOutwardRotation(rightGun,TURRET_RIGHT);
    scene.AddRotation(Id(POLE_LEFT),GLFW_KEY_P,{1,0,0},Shape::bottom,
        -1,POLE_ROTATE_SPEED,POLE_CYCLE_ANGLE,Mode::Once);
    scene.AddRotation(Id(POLE_RIGHT),GLFW_KEY_P,{1,0,0},Shape::bottom,
        1,POLE_ROTATE_SPEED,POLE_CYCLE_ANGLE,Mode::Once);

    // 3-5. 위치 교환: 각 상부에 연결한 포신과 깃대도 함께 이동합니다.
    scene.AddSwap(TURRET_LEFT,TURRET_RIGHT,GLFW_KEY_L,SWAP_DURATION);

    // 추가 예시 (필요할 때 주석 해제): u 키로 상부 파트 전체를 X방향 0.2만큼 왕복.
    // scene.AddMovement(Part(TURRETS),GLFW_KEY_U,{1,0,0},.2f,.1f,Mode::PingPong);
    scene.Seal();
}

// ============================================================================
// 4. 함수 선언
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
// 5. 초기화 / 메인 반복문 / 종료
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
// 6. 매 프레임 처리
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
// 7. 공통 셰이더로 그리기
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
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,glm::value_ptr(shape.geometry));
        glUniform3fv(colorLocation,1,glm::value_ptr(shape.color.value));
        MeshRange range=meshRanges.at(shape.kind);
        glDrawArrays(GL_TRIANGLES,range.first,range.count);
    }
    glBindVertexArray(0);
}

// ============================================================================
// 8. 키보드와 마우스 입력
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
// 9. 셰이더 파일 로딩과 컴파일
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
