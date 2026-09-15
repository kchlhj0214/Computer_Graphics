// 링커 -> 명령줄: opengl32.lib glew32.lib glfw3.lib
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <random>

//--- 변경 가능한 설정
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define REGION_WIDTH (WINDOW_WIDTH / 2.0f)
#define REGION_HEIGHT (WINDOW_HEIGHT / 2.0f)
#define RECT_CREATE_MIN_SIZE 30
#define RECT_CREATE_MAX_SIZE 300
#define RECT_MIN_SIZE 10
#define RECT_SCALE_UP 1.1f
#define RECT_SCALE_DOWN 0.9f
#define MAX_RECTS_PER_REGION 5
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define SELECTION_LINE_WIDTH 3.0f

using namespace std;

struct Color { float r, g, b; };
struct Square {
    float x, y; // 좌측 하단 좌표
    float size;
    Color color;
};
struct Region {
    float x, y;
    Color color;
    Square squares[MAX_RECTS_PER_REGION];
    int count;
};

random_device rd;
mt19937 generator(rd());
uniform_real_distribution<float> distribution(COLOR_MIN, COLOR_MAX);
Region regions[4] = {};
int selectedRegion = -1;
int selectedSquare = -1;

void InputProcess(GLFWwindow* window);
void DrawScene();
Color RandomColor();
void ResetScene();
void AddSquare(int index);
void SelectSquare(GLFWwindow* window);
void ResizeSelected(float scale);
bool KeyPressed(GLFWwindow* window, int key);

int main() {
    //--- GLFW 초기화
    if (!glfwInit()) {
        cerr << "GLFW 초기화 실패!" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // glRectf 등 기초 함수를 사용하기 위한 호환 프로파일
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Window", nullptr, nullptr);
    if (!window) {
        cerr << "윈도우 생성 실패!" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        cerr << "GLEW 초기화 실패!" << endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    //--- 좌측 하단 (0, 0), 우측 상단 (WINDOW_WIDTH, WINDOW_HEIGHT)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    ResetScene();

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        InputProcess(window);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

Color RandomColor() {
    return { distribution(generator), distribution(generator), distribution(generator) };
}

void ResetScene() {
    selectedRegion = -1;
    selectedSquare = -1;
    // 왼쪽 위부터 반시계 방향으로 1, 2, 3, 4
    const float x[4] = { 0, 0, REGION_WIDTH, REGION_WIDTH };
    const float y[4] = { REGION_HEIGHT, 0, 0, REGION_HEIGHT };
    for (int i = 0; i < 4; ++i) {
        regions[i].x = x[i];
        regions[i].y = y[i];
        regions[i].count = 0;
        bool duplicate;
        do {
            regions[i].color = RandomColor();
            duplicate = false;
            for (int j = 0; j < i; ++j) {
                Color a = regions[i].color;
                Color b = regions[j].color;
                if (a.r == b.r && a.g == b.g && a.b == b.b)
                    duplicate = true;
            }
        } while (duplicate);
    }
}

//--- 길게 눌러도 한 번만 실행
bool KeyPressed(GLFWwindow* window, int key) {
    static bool previous[GLFW_KEY_LAST + 1] = {};
    bool current = glfwGetKey(window, key) == GLFW_PRESS;
    bool pressed = current && !previous[key];
    previous[key] = current;
    return pressed;
}

void InputProcess(GLFWwindow* window) {
    for (int i = 0; i < 4; ++i) {
        if (KeyPressed(window, GLFW_KEY_1 + i)) AddSquare(i);
    }
    static bool previousLeft = false;
    bool currentLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (currentLeft && !previousLeft) SelectSquare(window);
    previousLeft = currentLeft;

    bool equalPressed = KeyPressed(window, GLFW_KEY_EQUAL);
    bool addPressed = KeyPressed(window, GLFW_KEY_KP_ADD);
    bool minusPressed = KeyPressed(window, GLFW_KEY_MINUS);
    bool subtractPressed = KeyPressed(window, GLFW_KEY_KP_SUBTRACT);
    bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if ((equalPressed && shift) || addPressed) ResizeSelected(RECT_SCALE_UP);
    if (minusPressed || subtractPressed) ResizeSelected(RECT_SCALE_DOWN);
    if (KeyPressed(window, GLFW_KEY_C) && selectedRegion != -1)
        regions[selectedRegion].squares[selectedSquare].color = RandomColor();
    if (KeyPressed(window, GLFW_KEY_R)) ResetScene();
    if (KeyPressed(window, GLFW_KEY_Q)) glfwSetWindowShouldClose(window, true);
}

void AddSquare(int index) {
    Region& region = regions[index];
    if (region.count >= MAX_RECTS_PER_REGION) return;
    uniform_real_distribution<float> sizeDistribution(
        RECT_CREATE_MIN_SIZE, RECT_CREATE_MAX_SIZE);
    Square& square = region.squares[region.count];
    square.size = sizeDistribution(generator);
    // 창 크기를 작게 설정해도 영역 안에 배치
    if (square.size > REGION_WIDTH) square.size = REGION_WIDTH;
    if (square.size > REGION_HEIGHT) square.size = REGION_HEIGHT;
    // 크기를 먼저 정한 뒤, 사각형 전체가 들어갈 좌측 하단 좌표 범위 계산
    float minX = region.x;
    float maxX = region.x + REGION_WIDTH - square.size;
    float minY = region.y;
    float maxY = region.y + REGION_HEIGHT - square.size;
    uniform_real_distribution<float> xDistribution(minX, maxX);
    uniform_real_distribution<float> yDistribution(minY, maxY);
    square.x = xDistribution(generator);
    square.y = yDistribution(generator);
    square.color = RandomColor();
    ++region.count;
}

void SelectSquare(GLFWwindow* window) {
    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    // 마우스의 좌측 상단 원점을 OpenGL의 좌측 하단 원점으로 변환
    float x = static_cast<float>(mouseX / width * WINDOW_WIDTH);
    float y = static_cast<float>((height - mouseY) / height * WINDOW_HEIGHT);
    selectedRegion = -1;
    selectedSquare = -1;
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    int index;
    if (x < REGION_WIDTH) index = y >= REGION_HEIGHT ? 0 : 1;
    else index = y >= REGION_HEIGHT ? 3 : 2;
    Region& region = regions[index];
    // 나중에 그린 도형부터 검사
    for (int i = region.count - 1; i >= 0; --i) {
        Square& square = region.squares[i];
        if (x >= square.x && x <= square.x + square.size &&
            y >= square.y && y <= square.y + square.size) {
            selectedRegion = index;
            selectedSquare = i;
            return;
        }
    }
}

void ResizeSelected(float scale) {
    if (selectedRegion == -1) return;
    Region& region = regions[selectedRegion];
    Square& square = region.squares[selectedSquare];
    float centerX = square.x + square.size / 2.0f;
    float centerY = square.y + square.size / 2.0f;
    float size = square.size * scale;
    if (size < RECT_MIN_SIZE) size = RECT_MIN_SIZE;
    if (size > REGION_WIDTH) size = REGION_WIDTH;
    if (size > REGION_HEIGHT) size = REGION_HEIGHT;
    square.size = size;
    square.x = centerX - size / 2.0f;
    square.y = centerY - size / 2.0f;
    // 경계에 닿아도 확대를 멈추지 않고 위치를 안쪽으로 보정
    if (square.x < region.x) square.x = region.x;
    if (square.y < region.y) square.y = region.y;
    if (square.x + size > region.x + REGION_WIDTH)
        square.x = region.x + REGION_WIDTH - size;
    if (square.y + size > region.y + REGION_HEIGHT)
        square.y = region.y + REGION_HEIGHT - size;
}

void DrawScene() {
    glClear(GL_COLOR_BUFFER_BIT);
    for (int i = 0; i < 4; ++i) {
        Region& region = regions[i];
        glColor3f(region.color.r, region.color.g, region.color.b);
        glRectf(region.x, region.y,
            region.x + REGION_WIDTH, region.y + REGION_HEIGHT);
        // 추가 순서대로 그려서 나중에 추가한 사각형이 위에 표시
        for (int j = 0; j < region.count; ++j) {
            Square& square = region.squares[j];
            glColor3f(square.color.r, square.color.g, square.color.b);
            glRectf(square.x, square.y,
                square.x + square.size, square.y + square.size);
            if (i == selectedRegion && j == selectedSquare) {
                // 선을 반 두께만큼 안쪽으로 넣어 영역 경계를 넘지 않도록 함
                GLint viewport[4];
                glGetIntegerv(GL_VIEWPORT, viewport);
                float insetX = viewport[2] > 0 ?
                    SELECTION_LINE_WIDTH * WINDOW_WIDTH / (2.0f * viewport[2]) : 0;
                float insetY = viewport[3] > 0 ?
                    SELECTION_LINE_WIDTH * WINDOW_HEIGHT / (2.0f * viewport[3]) : 0;
                glColor3f(0.0f, 0.0f, 0.0f);
                glLineWidth(SELECTION_LINE_WIDTH);
                glBegin(GL_LINE_LOOP);
                glVertex2f(square.x + insetX, square.y + insetY);
                glVertex2f(square.x + square.size - insetX, square.y + insetY);
                glVertex2f(square.x + square.size - insetX, square.y + square.size - insetY);
                glVertex2f(square.x + insetX, square.y + square.size - insetY);
                glEnd();
                glLineWidth(1.0f);
            }
        }
    }
}
