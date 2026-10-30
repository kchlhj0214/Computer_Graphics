#include <GL/glew.h>
#include <GL/glfw3.h>
int VerifyFrame(GLFWwindow* window);
GLFWwindow* HiddenWindow(int w,int h,const char* title,GLFWmonitor* monitor,GLFWwindow* share)
{
    glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    return glfwCreateWindow(w,h,title,monitor,share);
}
#define main TankApplicationMain
#define glfwCreateWindow HiddenWindow
#define glfwWindowShouldClose VerifyFrame
#include "ComG_mid_test.cpp"
#undef main
#undef glfwCreateWindow
#undef glfwWindowShouldClose

// Check: Release 빌드에서도 실패를 놓치지 않도록 예외로 검사합니다.
void Check(bool condition,const char* message)
{
    if (!condition)
        throw runtime_error(message);
}
bool Near(float a,float b)
{
    return std::abs(a-b)<0.0001f;
}
glm::vec3 Point(int id,Anchor anchor)
{
    return glm::vec3(scene.Get(id).geometry*glm::vec4(AnchorPoint(anchor),1));
}

// CheckScene: 실제 BuildTank의 설정을 사용해 동작과 연결 불변조건을 검사합니다.
void CheckScene()
{
    BuildTank();
    scene.Trigger(GLFW_KEY_D);
    scene.Update(.5f);
    Check(Near(TankScene::Bounds(scene.Get(WHEEL_LEFT).geometry).first.y,0),"wheel bottom drift");
    Check(Near(glm::length(glm::vec3(scene.Get(BODY_MAIN).geometry[1])),scene.Get(BODY_MAIN).size.y),"child inherited scale");
    Check(Near(Point(WHEEL_LEFT,Shape::top).y,Point(WHEEL_RIGHT,Shape::top).y),"part scale drift");
    Check(Near(Point(BODY_MAIN,Shape::bottom).y,Point(WHEEL_LEFT,Shape::top).y),"support lost contact");
    float height=Point(BODY_MAIN,Shape::bottom).y;
    scene.Trigger(GLFW_KEY_D);
    scene.Update(1);
    Check(Near(Point(BODY_MAIN,Shape::bottom).y,height),"scale pause failed");
    scene.Trigger(GLFW_KEY_T);
    scene.Update(1);
    glm::vec3 pin=Point(TURRET_LEFT,Shape::center);
    scene.Trigger(GLFW_KEY_G);
    scene.Update(.5f);
    Check(glm::length(Point(GUN_LEFT,Shape::back)-Point(TURRET_LEFT,Shape::center))<.0001f,"barrel detached");
    Check(glm::length(Point(GUN_RIGHT,Shape::back)-Point(TURRET_RIGHT,Shape::center))<.0001f,"right barrel detached");
    scene.Update(10);
    for (const auto& a : scene.actions)
        Check(!a.active,"Once did not stop");
    scene.Reset();
    for (int repeat=0;repeat<4;++repeat)
    {
        scene.Trigger(GLFW_KEY_G);
        scene.Update(1);
        auto& left=scene.Get(GUN_LEFT);
        glm::vec3 tip=glm::vec3(glm::inverse(scene.Get(TURRET_LEFT).frame)*left.geometry*glm::vec4(0,0,-.5f,1));
        Check(tip.x<0,"barrel turned inward");
        scene.Update(1);
    }
    scene.Trigger(GLFW_KEY_L);
    scene.Update(SWAP_DURATION);
    Check(Point(TURRET_LEFT,Shape::center).x>0,"swap did not finish");
    scene.Trigger(GLFW_KEY_G);
    scene.Update(1);
    glm::vec3 tip=glm::vec3(glm::inverse(scene.Get(TURRET_LEFT).frame)*scene.Get(GUN_LEFT).geometry*glm::vec4(0,0,-.5f,1));
    Check(tip.x>0,"outward rotation after swap failed");
    scene.Reset();
    scene.position={100,0,-100};
    scene.Evaluate();
    scene.KeepOnFloor(FLOOR_WIDTH,FLOOR_DEPTH);
    for (const auto& [id,s] : scene.shapes)
    {
        auto [lo,hi]=TankScene::Bounds(s.geometry);
        Check(lo.x>=-FLOOR_WIDTH*.5f-.001f && hi.x<=FLOOR_WIDTH*.5f+.001f,"floor X boundary");
        Check(lo.z>=-FLOOR_DEPTH*.5f-.001f && hi.z<=FLOOR_DEPTH*.5f+.001f,"floor Z boundary");
    }
    scene.RandomizeColors();
    Check(scene.Get(WHEEL_LEFT).color.value==scene.Get(WHEEL_RIGHT).color.value,"color group mismatch");
    scene.Reset();
    Check(scene.Get(WHEEL_LEFT).color.value==scene.Get(WHEEL_LEFT).initialColor,"reset colors failed");
    Check(glm::length(scene.position)==0,"reset position failed");
}

// CheckCustom: 추가 도형/비순차 ID/서로 다른 높이/순환 연결/모드들을 검사합니다.
void CheckCustom()
{
    TankScene custom;
    custom.Add(Sphere(100,1,{0,0,0},{1,1,1},Color::Fixed(.5f,.5f,.5f)));
    custom.Add(Cylinder(30,0,{-.5f,.5f,0},{.5f,1,.5f},Color::Random(0)));
    custom.Add(Box(5,0,{.5f,1,0},{.5f,2,.5f},Color::Random(0)));
    custom.AttachOnTop(100,Part(0));
    custom.AddScale(Id(30),'d',{0,1,0},Shape::bottom,.5f,1.5f,.5f);
    custom.AddRotation(Id(100),'t',{0,1,0},Shape::center,1,90,180,Mode::Loop);
    custom.AddMovement(Id(100),'u',{1,0,0},1,1,Mode::PingPong);
    custom.Seal();
    Check(Near(TankScene::Bounds(custom.Get(100).geometry).first.y,2),"highest support failed");
    custom.Trigger('t');
    custom.Trigger('u');
    custom.Update(3);
    Check(Near(custom.actions[1].value,270),"loop rotation failed");
    Check(Near(custom.actions[2].value,1),"pingpong failed");
    custom.Trigger('t');
    custom.Update(2);
    Check(Near(custom.actions[1].value,270),"loop pause failed");
    custom.Attach(30,100);
    bool rejected=false;
    try
    {
        custom.Evaluate();
    }
    catch (const exception&)
    {
        rejected=true;
    }
    Check(rejected,"cycle not rejected");
    for (ShapeKind kind : {ShapeKind::Box,ShapeKind::Cone,ShapeKind::Cylinder,ShapeKind::Sphere,ShapeKind::Pyramid})
    {
        auto mesh=MakeMesh(kind,32,20);
        Check(!mesh.empty() && mesh.size()%3==0,"invalid mesh triangles");
        for (auto p : mesh)
            Check(glm::all(glm::lessThanEqual(glm::abs(p),glm::vec3(.5001f))),"mesh bounds failed");
    }
}

// CheckDirections: 여섯 윗방향, 바닥 배치, 지지대 개수, 좌표계별 회전을 검사합니다.
void CheckDirections()
{
    TopDirection directions[]={TopDirection::X,TopDirection::Y,TopDirection::Z,
        TopDirection::MinusX,TopDirection::MinusY,TopDirection::MinusZ};
    glm::vec3 expected[]={{1,0,0},{0,1,0},{0,0,1},{-1,0,0},{0,-1,0},{0,0,-1}};
    for (int i=0;i<6;++i)
    {
        TankScene t;
        t.Add(Cylinder(1,0,{0,0,0},{.2f,.8f,.2f},Color::Random(),directions[i]));
        t.Add(Cone(2,1,{1,0,0},{.3f,.6f,.3f},Color::Random(),directions[i]));
        t.PlaceOnFloor(Part(0));
        t.PlaceOnFloor(Part(1));
        t.Seal();
        for (int id : {1,2})
        {
            auto& shape=t.Get(id);
            glm::vec3 up=glm::normalize(glm::vec3(shape.geometry*glm::vec4(0,1,0,0)));
            Check(glm::length(up-expected[i])<.0001f,"initial top direction failed");
            Check(Near(TankScene::Bounds(shape.geometry).first.y,0),"oriented ground placement failed");
        }
    }
    for (int count : {1,3})
    {
        TankScene t;
        for (int i=0;i<count;++i)
            t.Add(Cylinder(i,0,{(i-(count-1)*.5f)*.6f,0,0},{.3f,.2f,.3f},Color::Random(0),TopDirection::X));
        t.PlaceOnFloor(Part(0));
        t.Add(Pyramid(10,1,{0,0,0},{1,.4f,.4f},Color::Random()));
        t.AttachOnTop(10,Part(0));
        t.AddScale(Part(0),'d',{1,0,1},Shape::center,.5f,1.5f,.5f);
        t.Seal();
        t.Trigger('d');
        t.Update(.5f);
        Check(Near(TankScene::Bounds(t.Get(10).geometry).first.y,TankScene::Bounds(t.Get(0).geometry).second.y),"variable support count failed");
    }
    for (TransformSpace space : {TransformSpace::Local,TransformSpace::Parent,TransformSpace::World})
    {
        TankScene t;
        t.Add(Box(1,0,{0,0,0},{1,1,1},Color::Random()));
        t.Get(1).angles={0,0,90};
        t.Add(Cylinder(2,1,{0,0,0},{.2f,1,.2f},Color::Random(),TopDirection::Z));
        t.Attach(2,1,Shape::center,Shape::center);
        t.AddRotation(Id(2),'r',{0,1,0},Shape::center,1,90,90,Mode::Once,space);
        t.Seal();
        glm::quat parent=t.Get(1).worldRotation;
        glm::quat start=t.Get(2).worldRotation;
        glm::vec3 worldAxis=space==TransformSpace::World ? glm::vec3(0,1,0) :
            (space==TransformSpace::Parent ? parent*glm::vec3(0,1,0) : start*glm::vec3(0,1,0));
        glm::quat wanted=glm::angleAxis(glm::radians(90.0f),worldAxis)*start;
        t.Trigger('r');
        t.Update(1);
        Check(std::abs(glm::dot(wanted,t.Get(2).worldRotation))>.9999f,"rotation coordinate space failed");
        int move=t.AddMovement(Id(2),'m',{0,1,0},1,1,Mode::Once,space);
        glm::vec3 before(t.Get(2).geometry[3]);
        t.Trigger('m');
        t.Update(1);
        // Local movement is based on the initial orientation, not the animation angle.
        glm::vec3 displacement=glm::vec3(t.Get(2).geometry[3])-before;
        Check(glm::length(displacement-worldAxis)<.0001f,"movement coordinate space failed");
    }
}

// VerifyFrame: 숨겨진 OpenGL 창에서 다섯 종류와 조합 애니메이션을 실제 렌더링합니다.
int VerifyFrame(GLFWwindow* window)
{
    while (glGetError()!=GL_NO_ERROR)
    {
    }
    for (ShapeKind kind : {ShapeKind::Box,ShapeKind::Cone,ShapeKind::Cylinder,ShapeKind::Sphere,ShapeKind::Pyramid})
    {
        scene.Get(BODY_MAIN).kind=kind;
        DrawScene();
        glFinish();
        Check(glGetError()==GL_NO_ERROR,"OpenGL rendering error");
    }
    scene.Get(BODY_MAIN).kind=ShapeKind::Box;
    scene.Trigger(GLFW_KEY_D);
    scene.Trigger(GLFW_KEY_T);
    scene.Trigger(GLFW_KEY_G);
    scene.Trigger(GLFW_KEY_P);
    scene.Trigger(GLFW_KEY_L);
    for (int i=0;i<30;++i)
    {
        scene.Update(.1f);
        DrawScene();
    }
    Check(glGetError()==GL_NO_ERROR,"combined animation rendering failed");
    scene.Reset();
    DrawScene();
    glFinish();
    vector<unsigned char> pixels(WINDOW_WIDTH*WINDOW_HEIGHT*3);
    glReadPixels(0,0,WINDOW_WIDTH,WINDOW_HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    ofstream file("midterm-preview.ppm",ios::binary);
    file << "P6\n" << WINDOW_WIDTH << " " << WINDOW_HEIGHT << "\n255\n";
    for (int y=WINDOW_HEIGHT-1;y>=0;--y)
        file.write((char*)pixels.data()+y*WINDOW_WIDTH*3,WINDOW_WIDTH*3);
    cout << "PASS: all shape meshes, attachments, scale isolation, group sync, cycles, swap, reset, rendering" << endl;
    return 1;
}
int main()
{
    try
    {
        CheckScene();
        CheckCustom();
        CheckDirections();
        scene=TankScene();
        return TankApplicationMain();
    }
    catch (const exception& error)
    {
        cerr << "TEST FAILED: " << error.what() << endl;
        return 1;
    }
}
