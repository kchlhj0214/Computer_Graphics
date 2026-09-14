// 링커 -> 명령줄: opengl32.lib glew32.lib glfw3.lib
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <algorithm>
#include <iostream>
#include <random>

//--- 변경 가능한 설정
#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define RECT_MIN_SIZE 50.0f
#define RECT_MAX_SIZE 150.0f
#define MAX_A_RECTS 10
#define MAX_RECTS 20
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define DRAG_LINE_WIDTH 3.0f

using namespace std;

struct Color { float r, g, b; };
struct Rectangle {
    float x, y; // 좌측 하단 좌표
    float width, height;
    Color color;
};

random_device rd;
mt19937 generator(rd());
uniform_real_distribution<float> colorDistribution(COLOR_MIN, COLOR_MAX);
Rectangle rectangles[MAX_RECTS] = {};
int rectangleCount = 0;
bool dragging = false;
int draggedIndex = -1;
float dragOffsetX = 0.0f;
float dragOffsetY = 0.0f;

void InputProcess(GLFWwindow* window);
void DrawScene();
Color RandomColor();
void AddRandomRectangle();
void StartDragging(float mouseX, float mouseY);
void MoveDraggedRectangle(float mouseX, float mouseY);
void FinishDragging();
void SplitRectangle(float mouseX, float mouseY);
void RemoveRectangle(int index);
int FindTopRectangle(float x, float y);
bool IsOverlapping(const Rectangle& a, const Rectangle& b);
bool KeyPressed(GLFWwindow* window, int key);
void GetMousePosition(GLFWwindow* window, float& x, float& y);

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
    return {
        colorDistribution(generator),
        colorDistribution(generator),
        colorDistribution(generator)
    };
}

void GetMousePosition(GLFWwindow* window, float& x, float& y) {
    double mouseX, mouseY;
    int width, height;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) {
        x = 0.0f;
        y = 0.0f;
        return;
    }
    // GLFW의 좌측 상단 원점을 OpenGL의 좌측 하단 원점으로 변환
    x = static_cast<float>(mouseX / width * WINDOW_WIDTH);
    y = static_cast<float>((height - mouseY) / height * WINDOW_HEIGHT);
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
    if (KeyPressed(window, GLFW_KEY_A) && rectangleCount < MAX_A_RECTS)
        AddRandomRectangle();
    if (KeyPressed(window, GLFW_KEY_Q))
        glfwSetWindowShouldClose(window, true);

    float mouseX, mouseY;
    GetMousePosition(window, mouseX, mouseY);

    static bool previousLeft = false;
    bool currentLeft =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (currentLeft && !previousLeft)
        StartDragging(mouseX, mouseY);
    if (currentLeft && dragging)
        MoveDraggedRectangle(mouseX, mouseY);
    if (!currentLeft && previousLeft && dragging)
        FinishDragging();
    previousLeft = currentLeft;

    static bool previousRight = false;
    bool currentRight =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (currentRight && !previousRight && !dragging)
        SplitRectangle(mouseX, mouseY);
    previousRight = currentRight;
}

void AddRandomRectangle() {
    if (rectangleCount >= MAX_RECTS) return;

    uniform_real_distribution<float> sizeDistribution(
        RECT_MIN_SIZE, RECT_MAX_SIZE);
    Rectangle& rectangle = rectangles[rectangleCount];
    rectangle.width = sizeDistribution(generator);
    rectangle.height = sizeDistribution(generator);

    uniform_real_distribution<float> xDistribution(
        0.0f, WINDOW_WIDTH - rectangle.width);
    uniform_real_distribution<float> yDistribution(
        0.0f, WINDOW_HEIGHT - rectangle.height);
    rectangle.x = xDistribution(generator);
    rectangle.y = yDistribution(generator);
    rectangle.color = RandomColor();
    ++rectangleCount;
}

int FindTopRectangle(float x, float y) {
    // 배열의 뒤쪽일수록 나중에 만든 사각형이므로 위에서부터 검사
    for (int i = rectangleCount - 1; i >= 0; --i) {
        const Rectangle& rectangle = rectangles[i];
        if (x >= rectangle.x && x <= rectangle.x + rectangle.width &&
            y >= rectangle.y && y <= rectangle.y + rectangle.height)
            return i;
    }
    return -1;
}

void StartDragging(float mouseX, float mouseY) {
    draggedIndex = FindTopRectangle(mouseX, mouseY);
    if (draggedIndex == -1) return;

    dragging = true;
    dragOffsetX = mouseX - rectangles[draggedIndex].x;
    dragOffsetY = mouseY - rectangles[draggedIndex].y;
}

void MoveDraggedRectangle(float mouseX, float mouseY) {
    Rectangle& rectangle = rectangles[draggedIndex];
    rectangle.x = clamp(
        mouseX - dragOffsetX, 0.0f, WINDOW_WIDTH - rectangle.width);
    rectangle.y = clamp(
        mouseY - dragOffsetY, 0.0f, WINDOW_HEIGHT - rectangle.height);
}

bool IsOverlapping(const Rectangle& a, const Rectangle& b) {
    // 변이나 꼭짓점만 맞닿는 경우도 겹친 것으로 처리
    return a.x <= b.x + b.width && a.x + a.width >= b.x &&
        a.y <= b.y + b.height && a.y + a.height >= b.y;
}

void FinishDragging() {
    // 동시에 여러 개와 겹치면 가장 나중에 만든 사각형 하나만 선택
    int overlappingIndex = -1;
    for (int i = rectangleCount - 1; i >= 0; --i) {
        if (i != draggedIndex &&
            IsOverlapping(rectangles[draggedIndex], rectangles[i])) {
            overlappingIndex = i;
            break;
        }
    }

    if (overlappingIndex != -1) {
        Rectangle dragged = rectangles[draggedIndex];
        Rectangle overlapping = rectangles[overlappingIndex];
        Rectangle merged;
        merged.x = min(dragged.x, overlapping.x);
        merged.y = min(dragged.y, overlapping.y);
        merged.width = max(dragged.x + dragged.width,
            overlapping.x + overlapping.width) - merged.x;
        merged.height = max(dragged.y + dragged.height,
            overlapping.y + overlapping.height) - merged.y;
        merged.color = RandomColor();

        // 인덱스가 큰 원소부터 제거해야 앞쪽 인덱스가 변하지 않음
        RemoveRectangle(max(draggedIndex, overlappingIndex));
        RemoveRectangle(min(draggedIndex, overlappingIndex));
        // 합친 사각형은 새로 생성된 사각형이므로 맨 위에 배치
        rectangles[rectangleCount] = merged;
        ++rectangleCount;
    }

    dragging = false;
    draggedIndex = -1;
}

void SplitRectangle(float mouseX, float mouseY) {
    int index = FindTopRectangle(mouseX, mouseY);
    if (index == -1 || rectangleCount >= MAX_RECTS) return;

    RemoveRectangle(index);
    AddRandomRectangle();
    AddRandomRectangle();
}

void RemoveRectangle(int index) {
    for (int i = index; i < rectangleCount - 1; ++i)
        rectangles[i] = rectangles[i + 1];
    --rectangleCount;
}

void DrawScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 생성 순서대로 그려서 나중에 만든 사각형이 위에 표시
    for (int i = 0; i < rectangleCount; ++i) {
        const Rectangle& rectangle = rectangles[i];
        glColor3f(rectangle.color.r, rectangle.color.g, rectangle.color.b);
        glRectf(rectangle.x, rectangle.y,
            rectangle.x + rectangle.width, rectangle.y + rectangle.height);
    }

    // 선택 표시가 다른 사각형에 가려지지 않도록 모든 면을 그린 뒤 표시
    if (dragging) {
        const Rectangle& rectangle = rectangles[draggedIndex];
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        float insetX = viewport[2] > 0 ?
            DRAG_LINE_WIDTH * WINDOW_WIDTH / (2.0f * viewport[2]) : 0.0f;
        float insetY = viewport[3] > 0 ?
            DRAG_LINE_WIDTH * WINDOW_HEIGHT / (2.0f * viewport[3]) : 0.0f;
        glColor3f(0.0f, 0.0f, 0.0f);
        glLineWidth(DRAG_LINE_WIDTH);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rectangle.x + insetX, rectangle.y + insetY);
        glVertex2f(rectangle.x + rectangle.width - insetX,
            rectangle.y + insetY);
        glVertex2f(rectangle.x + rectangle.width - insetX,
            rectangle.y + rectangle.height - insetY);
        glVertex2f(rectangle.x + insetX,
            rectangle.y + rectangle.height - insetY);
        glEnd();
        glLineWidth(1.0f);
    }
}
