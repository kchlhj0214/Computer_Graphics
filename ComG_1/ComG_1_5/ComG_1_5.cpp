// 링커 -> 명령줄: opengl32.lib glew32.lib glfw3.lib
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

//--- 변경 가능한 설정
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define WINDOW_MIN_WIDTH 300
#define WINDOW_MIN_HEIGHT 300
#define RECT_SIZE 30.0f
#define INITIAL_MIN_RECTS 20
#define INITIAL_MAX_RECTS 40
#define MAX_ADDED_RECTS 10
#define SPAWN_INTERVAL 0.1
#define ERASER_SIZE 60.0f
#define ERASER_SIZE_STEP 5.0f
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define COUNT_TEXT_MARGIN 10.0f
#define COUNT_TEXT_SCALE 2.0f

using namespace std;

struct Color { float r, g, b; };
struct Rectangle {
    float x = 0, y = 0; // 중심: 좌측 상단 원점, 아래쪽이 +y
    float size = RECT_SIZE;
    Color color{};
    bool hidden = false;
};

//--- 전역 상태
vector<Rectangle> rectangles;
Rectangle eraser;
int initialTarget = 0, initialCreated = 0, addedCount = 0;
float windowWidth = WINDOW_WIDTH, windowHeight = WINDOW_HEIGHT;
bool erasing = false;
double nextSpawn = 0;
random_device rd;
mt19937 g(rd());

//--- 함수 선언
void ResetScene();
void AddRectangle(float x, float y);
void ClampRectangle(Rectangle& r);
void InputProcess(GLFWwindow* window);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UpdateScene(GLFWwindow* window);
void DrawScene();
void RefreshCallback(GLFWwindow* window);

int main() {
    //--- GLFW 초기화
    if (!glfwInit()) {
        cerr << "GLFW initialization failed!" << endl;
        return -1;
    }
    //--- OpenGL 버전 설정
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    //--- 윈도우 생성
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "1-5 Rectangle Eraser", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    //--- 컨텍스트 설정 및 GLEW 초기화
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glfwSwapInterval(1);
    glfwSetWindowSizeLimits(window,
        max(WINDOW_MIN_WIDTH, static_cast<int>(ceil(max(RECT_SIZE, ERASER_SIZE)))),
        max(WINDOW_MIN_HEIGHT, static_cast<int>(ceil(max(RECT_SIZE, ERASER_SIZE)))),
        GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetWindowRefreshCallback(window, RefreshCallback);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    ResetScene();
    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        InputProcess(window);
        UpdateScene(window);
        RefreshCallback(window);
        glfwWaitEventsTimeout(0.01);
    }
    //--- 종료 처리
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

//--- 초기 실행 및 r 명령
void ResetScene() {
    rectangles.clear();
    rectangles.reserve(INITIAL_MAX_RECTS + MAX_ADDED_RECTS);
    initialCreated = addedCount = 0;
    uniform_int_distribution<> uid_count{ INITIAL_MIN_RECTS, INITIAL_MAX_RECTS };
    initialTarget = uid_count(g);
    erasing = false;
    eraser = Rectangle{};
    eraser.size = ERASER_SIZE;
    nextSpawn = glfwGetTime() + SPAWN_INTERVAL;
}

void ClampRectangle(Rectangle& r) {
    r.x = clamp(r.x, r.size / 2, windowWidth - r.size / 2);
    r.y = clamp(r.y, r.size / 2, windowHeight - r.size / 2);
}

//--- 자동 생성과 우클릭 생성의 공통 처리
void AddRectangle(float x, float y) {
    if (rectangles.size() >= INITIAL_MAX_RECTS + MAX_ADDED_RECTS) return;
    Rectangle r;
    r.x = x; r.y = y;
    uniform_real_distribution<float> urd_rgb{ COLOR_MIN, COLOR_MAX };
    r.color = { urd_rgb(g), urd_rgb(g), urd_rgb(g) };
    ClampRectangle(r);
    rectangles.push_back(r);
}

void InputProcess(GLFWwindow* window) {
    static bool previousR = false;
    bool currentR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (currentR && !previousR) ResetScene();
    previousR = currentR;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            erasing = true;
            eraser.size = ERASER_SIZE;
            eraser.color = { 0, 0, 0 };
            UpdateScene(window);
        }
        else if (action == GLFW_RELEASE) {
            erasing = false;
            for (Rectangle& r : rectangles) r.hidden = false;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS &&
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE &&
        addedCount < MAX_ADDED_RECTS) {
        AddRectangle(static_cast<float>(mouseX), static_cast<float>(mouseY));
        ++addedCount;
        // 지우개는 숨겨진 상태이며, 다음 좌클릭 시 초기 크기로 돌아간다.
        eraser.size = max(RECT_SIZE, eraser.size - ERASER_SIZE_STEP);
    }
}

void UpdateScene(GLFWwindow* window) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    windowWidth = static_cast<float>(width);
    windowHeight = static_cast<float>(height);
    for (Rectangle& r : rectangles) ClampRectangle(r);

    double now = glfwGetTime();
    if (initialCreated < initialTarget && now >= nextSpawn) {
        uniform_real_distribution<float> urd_x{ RECT_SIZE / 2, windowWidth - RECT_SIZE / 2 };
        uniform_real_distribution<float> urd_y{ RECT_SIZE / 2, windowHeight - RECT_SIZE / 2 };
        AddRectangle(urd_x(g), urd_y(g));
        ++initialCreated;
        nextSpawn = now + SPAWN_INTERVAL; // 지연되었어도 한꺼번에 생성하지 않는다.
    }
    if (!erasing) return;
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    eraser.size = min(eraser.size, min(windowWidth, windowHeight));
    eraser.x = static_cast<float>(mouseX);
    eraser.y = static_cast<float>(mouseY);
    ClampRectangle(eraser);

    // 커진 지우개에 새로 들어온 중심도 같은 갱신에서 처리한다.
    bool collided;
    do {
        collided = false;
        for (Rectangle& r : rectangles) {
            if (!r.hidden && abs(r.x - eraser.x) <= eraser.size / 2 &&
                abs(r.y - eraser.y) <= eraser.size / 2) {
                r.hidden = true;
                eraser.color = r.color;
                eraser.size = min(eraser.size + ERASER_SIZE_STEP, min(windowWidth, windowHeight));
                ClampRectangle(eraser);
                collided = true;
            }
        }
    } while (collided);
}

void DrawScene() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT);
    // 마지막에 지우개를 그려 다른 사각형보다 위에 표시한다.
    for (size_t i = 0; i < rectangles.size() + (erasing ? 1 : 0); ++i) {
        const Rectangle& r = i < rectangles.size() ? rectangles[i] : eraser;
        if (r.hidden) continue;
        glColor3f(r.color.r, r.color.g, r.color.b);
        glRectf(r.x - r.size / 2, r.y - r.size / 2, r.x + r.size / 2, r.y + r.size / 2);
    }

    // 외부 폰트 없이 5 x 7 글자로 현재 보이는 사각형 개수를 표시한다.
    int visibleCount = 0;
    for (const Rectangle& r : rectangles)
        if (!r.hidden) ++visibleCount;
    const string text = "COUNT: " + to_string(visibleCount);
    static const string characters = "0123456789COUNT:";
    static const vector<vector<unsigned char>> letters = {
        { 14, 17, 19, 21, 25, 17, 14 }, // 0
        { 4, 12, 4, 4, 4, 4, 14 },
        { 14, 17, 1, 2, 4, 8, 31 },
        { 30, 1, 1, 14, 1, 1, 30 },
        { 2, 6, 10, 18, 31, 2, 2 },
        { 31, 16, 16, 30, 1, 1, 30 },
        { 14, 16, 16, 30, 17, 17, 14 },
        { 31, 1, 2, 4, 8, 8, 8 },
        { 14, 17, 17, 14, 17, 17, 14 },
        { 14, 17, 17, 15, 1, 1, 14 },
        { 14, 17, 16, 16, 16, 17, 14 }, // C
        { 14, 17, 17, 17, 17, 17, 14 }, // O
        { 17, 17, 17, 17, 17, 17, 14 }, // U
        { 17, 25, 25, 21, 19, 19, 17 }, // N
        { 31, 4, 4, 4, 4, 4, 4 },       // T
        { 0, 4, 4, 0, 4, 4, 0 }        // :
    };
    // 사각형이나 지우개와 겹쳐도 읽을 수 있도록 배경을 먼저 그린다.
    glColor3f(1.0f, 1.0f, 1.0f);
    glRectf(COUNT_TEXT_MARGIN - COUNT_TEXT_SCALE, COUNT_TEXT_MARGIN - COUNT_TEXT_SCALE,
        COUNT_TEXT_MARGIN + static_cast<float>(text.size()) * 6 * COUNT_TEXT_SCALE,
        COUNT_TEXT_MARGIN + 8 * COUNT_TEXT_SCALE);
    glColor3f(0.0f, 0.0f, 0.0f);
    for (size_t i = 0; i < text.size(); ++i) {
        size_t letter = characters.find(text[i]);
        if (letter == string::npos) continue;
        for (int row = 0; row < 7; ++row)
            for (int column = 0; column < 5; ++column) {
                if (!(letters[letter][row] & (1 << (4 - column)))) continue;
                float x = COUNT_TEXT_MARGIN + (static_cast<float>(i) * 6 + column) * COUNT_TEXT_SCALE;
                float y = COUNT_TEXT_MARGIN + row * COUNT_TEXT_SCALE;
                glRectf(x, y, x + COUNT_TEXT_SCALE, y + COUNT_TEXT_SCALE);
            }
    }
}

void RefreshCallback(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    UpdateScene(window);
    glViewport(0, 0, width, height);
    DrawScene();
    glfwSwapBuffers(window);
}
