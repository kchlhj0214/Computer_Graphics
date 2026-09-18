// 링커 -> 명령줄: opengl32.lib glew32.lib glfw3.lib
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>

//--- 변경 가능한 설정
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define WINDOW_MIN_WIDTH 300
#define WINDOW_MIN_HEIGHT 300
#define MOVE_STEP 1.0f
#define TIMER_INTERVAL_MS 2
#define SIZE_STEP 1.0f
#define RECT_MIN_SIZE 50.0f
#define RECT_MAX_SIZE 150.0f
#define ZIGZAG_VERTICAL_DISTANCE 100.0f
#define MAX_RECTS 5
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f

using namespace std;

struct Color { float r, g, b; };
enum class Motion { None, Diagonal, Zigzag, Edge, Return };
struct Rectangle {
    float x = 0, y = 0; // 중심: 좌측 상단 원점, 아래쪽이 +y
    float originalX = 0, originalY = 0;
    float width = RECT_MIN_SIZE, height = RECT_MIN_SIZE;
    Color color{};
    int dx = 1, dy = 1;
    bool zigzagVertical = false;
    float verticalRemaining = 0;
    int sizePhase = 0; // 세로 증가, 세로 감소, 가로 증가, 가로 감소
    bool attachedToTop = false;
    double edgeDistance = 0;
};


//--- 전역 상태

Rectangle rectangles[MAX_RECTS]{};
int rectangleCount = 0;
float windowWidth = WINDOW_WIDTH, windowHeight = WINDOW_HEIGHT;
Motion motion = Motion::None, pausedMotion = Motion::None;
bool resizing = false, edgeRunning = false;
float edgeHalfWidth = RECT_MIN_SIZE / 2, edgeHalfHeight = RECT_MIN_SIZE / 2;
mt19937 generator{ random_device{}() };
bool minimized = false;
double nextTick = 0.0;
const double timerInterval = TIMER_INTERVAL_MS / 1000.0;


//--- 함수 선언
Color RandomColor();
void ClampRectangle(Rectangle& r);
void PrepareEdge();
void AddRectangle(float x, float y);
void ResizeWindow(int w, int h);
void ToggleMotion(Motion next);
void StopAnimations();
void ReturnHome();
void ClearRectangles();
void ToggleSize();
bool Approach(float& value, float target, float step);
void UpdateSize(Rectangle& r);
void Bounce(float& value, int& direction, float low, float high, float step);
void UpdateZigzag(Rectangle& r);
void UpdateEdge();
void UpdateAnimation();

void InputProcess(GLFWwindow* window);
void DrawScene();
void TimerFunction();
void WindowSizeCallback(GLFWwindow* window, int width, int height);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void RefreshCallback(GLFWwindow* window);
void IconifyCallback(GLFWwindow* window, int iconified);
void GetMousePosition(GLFWwindow* window, float& x, float& y);
bool KeyPressed(GLFWwindow* window, int key);

int main() {
    //--- GLFW 초기화
    if (!glfwInit()) {
        cerr << "GLFW initialization failed!" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "1-4 Rectangle Animation", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);
    //--- 실제 창 내부 영역의 최소 크기: 300 x 300
    glfwSetWindowSizeLimits(window, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT,
        GLFW_DONT_CARE, GLFW_DONT_CARE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    ResizeWindow(width, height);
    glfwSetWindowSizeCallback(window, WindowSizeCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowRefreshCallback(window, RefreshCallback);
    glfwSetWindowIconifyCallback(window, IconifyCallback);
    nextTick = glfwGetTime() + timerInterval;

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        InputProcess(window);
        TimerFunction();
        if (!minimized) {
            int framebufferWidth, framebufferHeight;
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            DrawScene();
            glfwSwapBuffers(window);
        }
        double wait = nextTick - glfwGetTime();
        if (wait > 0) glfwWaitEventsTimeout(wait);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

//--- 길게 눌러도 한 번만 실행
bool KeyPressed(GLFWwindow* window, int key) {
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool current = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = current && !previous[key];
    previous[key] = current;
    return pressed;
}

void GetMousePosition(GLFWwindow* window, float& x, float& y) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    // 마우스와 투영 모두 좌측 상단 원점. 창 크기의 비율을 곱하지 않는다.
    x = static_cast<float>(mouseX);
    y = static_cast<float>(mouseY);
}

void InputProcess(GLFWwindow* window) {
    if (KeyPressed(window, GLFW_KEY_1)) ToggleMotion(Motion::Diagonal);
    if (KeyPressed(window, GLFW_KEY_2)) ToggleMotion(Motion::Zigzag);
    if (KeyPressed(window, GLFW_KEY_3)) ToggleMotion(Motion::Edge);
    if (KeyPressed(window, GLFW_KEY_4)) ToggleSize();
    if (KeyPressed(window, GLFW_KEY_5))
        for (int i = 0; i < rectangleCount; ++i) rectangles[i].color = RandomColor();
    if (KeyPressed(window, GLFW_KEY_S)) StopAnimations();
    if (KeyPressed(window, GLFW_KEY_M)) ReturnHome();
    if (KeyPressed(window, GLFW_KEY_R)) ClearRectangles();
    if (KeyPressed(window, GLFW_KEY_Q)) glfwSetWindowShouldClose(window, true);

    static bool previousLeft = false;
    bool currentLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (currentLeft && !previousLeft) {
        float mouseX, mouseY;
        GetMousePosition(window, mouseX, mouseY);
        AddRectangle(mouseX, mouseY);
    }
    previousLeft = currentLeft;
}

//--- 고정 간격 타이머: 한 단계마다 MOVE_STEP만큼 이동
void TimerFunction() {
    double now = glfwGetTime();
    if (minimized) { nextTick = now + timerInterval; return; }

    if (now - nextTick > 0.1) nextTick = now;
    while (now >= nextTick) {
        UpdateAnimation();
        nextTick += timerInterval;
    }
}

void WindowSizeCallback(GLFWwindow* window, int width, int height) {
    ResizeWindow(width, height);
    nextTick = glfwGetTime() + timerInterval;
    RefreshCallback(window);
}

void FramebufferSizeCallback(GLFWwindow* window, int, int) {
    RefreshCallback(window);
}

void RefreshCallback(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    DrawScene();
    glfwSwapBuffers(window);
}

void IconifyCallback(GLFWwindow*, int iconified) {
    minimized = iconified == GLFW_TRUE;
    nextTick = glfwGetTime() + timerInterval;
}

void DrawScene() {
    // 현재 창 크기로 좌표계 갱신: 좌측 상단 (0, 0), 아래쪽이 +y
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < rectangleCount; ++i) {
        const Rectangle& rectangle = rectangles[i];
        glColor3f(rectangle.color.r, rectangle.color.g, rectangle.color.b);
        glRectf(rectangle.x - rectangle.width / 2, rectangle.y - rectangle.height / 2,
            rectangle.x + rectangle.width / 2, rectangle.y + rectangle.height / 2);
    }
}
//--- 애니메이션 및 충돌 처리
Color RandomColor() {
    uniform_real_distribution<float> value(COLOR_MIN, COLOR_MAX);
    return { value(generator), value(generator), value(generator) };
}
void ClampRectangle(Rectangle& r) {
    r.x = clamp(r.x, r.width / 2, windowWidth - r.width / 2);
    r.y = clamp(r.y, r.height / 2, windowHeight - r.height / 2);
}
void PrepareEdge() {
    // 크기 변화 중이었어도 모든 사각형을 50 x 50으로 통일한다.
    edgeRunning = false;
    edgeHalfWidth = edgeHalfHeight = RECT_MIN_SIZE / 2;
    for (int i = 0; i < rectangleCount; ++i) {
        rectangles[i].width = rectangles[i].height = RECT_MIN_SIZE;
        rectangles[i].sizePhase = 0;
        rectangles[i].attachedToTop = false;
    }
}
void AddRectangle(float x, float y) {
    if (rectangleCount == MAX_RECTS) return;
    Rectangle r;
    r.x = x; r.y = y;
    ClampRectangle(r);
    r.originalX = r.x; r.originalY = r.y;
    r.color = RandomColor();
    r.dx = (generator() % 2) ? 1 : -1;
    r.dy = (generator() % 2) ? 1 : -1;
    rectangles[rectangleCount++] = r;
    if (motion == Motion::Edge || pausedMotion == Motion::Edge) PrepareEdge();
}
void ResizeWindow(int w, int h) {
    if (w <= 0 || h <= 0) return; // 최소화 제외
    windowWidth = static_cast<float>(max(w, WINDOW_MIN_WIDTH));
    windowHeight = static_cast<float>(max(h, WINDOW_MIN_HEIGHT));
    for (int i = 0; i < rectangleCount; ++i) ClampRectangle(rectangles[i]);
    // 경로 자체가 변했으므로 현재 위치에서 다시 위쪽으로 집결한다.
    if (motion == Motion::Edge || pausedMotion == Motion::Edge) PrepareEdge();
}
void ToggleMotion(Motion next) {
    if (motion == next) { pausedMotion = motion; motion = Motion::None; return; }
    const bool resume = motion == Motion::None && pausedMotion == next;
    motion = next;
    pausedMotion = Motion::None;
    if (next == Motion::Edge) {
        resizing = false;
        if (!resume) PrepareEdge();
    }
    if (next == Motion::Zigzag && !resume)
        for (int i = 0; i < rectangleCount; ++i) rectangles[i].zigzagVertical = false;
}
void StopAnimations() { 
    pausedMotion = motion; motion = Motion::None; resizing = false;
}
void ReturnHome() { 
    motion = Motion::Return; pausedMotion = Motion::None; 
}
void ClearRectangles() { 
    rectangleCount = 0; motion = pausedMotion = Motion::None; resizing = edgeRunning = false; 
}
void ToggleSize() {
    if (motion != Motion::Edge) {
        resizing = !resizing;
        if (resizing && pausedMotion == Motion::Edge) pausedMotion = Motion::None;
    }
}
bool Approach(float& value, float target, float step) {
    if (abs(target - value) <= step) { value = target; return true; }
    value += target > value ? step : -step;
    return false;
}
void UpdateSize(Rectangle& r) {
    float& value = r.sizePhase < 2 ? r.height : r.width;
    const float target = r.sizePhase % 2 == 0 ? RECT_MAX_SIZE : RECT_MIN_SIZE;
    if (Approach(value, target, SIZE_STEP)) r.sizePhase = (r.sizePhase + 1) % 4;
    ClampRectangle(r);
}
void Bounce(float& value, int& direction, float low, float high, float step) {
    if (high <= low) { value = low; return; }
    // 잔여 이동량을 반사시켜 설정값이 커도 경계를 넘지 않는다.
    float remaining = step;
    while (remaining > 0) {
        float distance = direction > 0 ? high - value : value - low;
        float moved = min(remaining, max(0.0f, distance));
        value += direction * moved;
        remaining -= moved;
        if (moved >= distance) direction = -direction;
    }
}
void UpdateZigzag(Rectangle& r) {
    if (!r.zigzagVertical) {
        const float target = r.dx > 0 ? windowWidth - r.width / 2 : r.width / 2;
        if (Approach(r.x, target, MOVE_STEP)) {
            r.dx = -r.dx;
            r.zigzagVertical = true;
            r.verticalRemaining = ZIGZAG_VERTICAL_DISTANCE;
        }
    } else {
        const float step = min(MOVE_STEP, r.verticalRemaining);
        Bounce(r.y, r.dy, r.height / 2, windowHeight - r.height / 2, step);
        r.verticalRemaining -= step;
        if (r.verticalRemaining <= 0) r.zigzagVertical = false;
    }
}
void UpdateEdge() {
    const double horizontal = windowWidth - 2 * edgeHalfWidth;
    const double vertical = windowHeight - 2 * edgeHalfHeight;
    const double perimeter = 2 * (horizontal + vertical);
    if (!edgeRunning) {
        bool allAttached = rectangleCount > 0;
        for (int i = 0; i < rectangleCount; ++i) {
            auto& r = rectangles[i];
            const bool xReady = Approach(r.x, clamp(r.x, edgeHalfWidth,
                windowWidth - edgeHalfWidth), MOVE_STEP);
            const bool yReady = Approach(r.y, edgeHalfHeight, MOVE_STEP);
            r.attachedToTop = xReady && yReady;
            allAttached = allAttached && r.attachedToTop;
        }
        if (allAttached) {
            for (int i = 0; i < rectangleCount; ++i)
                rectangles[i].edgeDistance = rectangles[i].x - edgeHalfWidth;
            edgeRunning = true;
        }
        return;
    }
    if (perimeter <= 0) return;
    for (int i = 0; i < rectangleCount; ++i) {
        auto& r = rectangles[i];
        r.edgeDistance = fmod(r.edgeDistance + MOVE_STEP, perimeter);
        double d = r.edgeDistance;
        if (d < horizontal) { r.x = edgeHalfWidth + static_cast<float>(d); r.y = edgeHalfHeight; }
        else if ((d -= horizontal) < vertical) {
            r.x = windowWidth - edgeHalfWidth; r.y = edgeHalfHeight + static_cast<float>(d);
        } else if ((d -= vertical) < horizontal) {
            r.x = windowWidth - edgeHalfWidth - static_cast<float>(d); r.y = windowHeight - edgeHalfHeight;
        } else {
            d -= horizontal;
            r.x = edgeHalfWidth; r.y = windowHeight - edgeHalfHeight - static_cast<float>(d);
        }
    }
}
void UpdateAnimation() {
    if (resizing) for (int i = 0; i < rectangleCount; ++i) UpdateSize(rectangles[i]);
    if (motion == Motion::Edge) { UpdateEdge(); return; }
    bool allHome = true;
    for (int i = 0; i < rectangleCount; ++i) {
        auto& r = rectangles[i];
        if (motion == Motion::Diagonal) {
            constexpr float diagonalStep = MOVE_STEP * 0.70710678118f;
            Bounce(r.x, r.dx, r.width / 2, windowWidth - r.width / 2, diagonalStep);
            Bounce(r.y, r.dy, r.height / 2, windowHeight - r.height / 2, diagonalStep);
        } else if (motion == Motion::Zigzag) UpdateZigzag(r);
        else if (motion == Motion::Return) {
            const float tx = clamp(r.originalX, r.width / 2, windowWidth - r.width / 2);
            const float ty = clamp(r.originalY, r.height / 2, windowHeight - r.height / 2);
            const float dx = tx - r.x, dy = ty - r.y;
            const float length = sqrt(dx * dx + dy * dy);
            if (length <= MOVE_STEP) { r.x = tx; r.y = ty; }
            else { r.x += MOVE_STEP * dx / length; r.y += MOVE_STEP * dy / length; allHome = false; }
        }
    }
    if (motion == Motion::Return && allHome) motion = Motion::None;
}
