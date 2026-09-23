// 링커 -> 명령줄: opengl32.lib glew32.lib glfw3.lib
#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <vector>

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 1200
#define RECT_MIN_SIZE 100.0f
#define RECT_MAX_SIZE 200.0f
#define INITIAL_MIN_RECTS 5
#define INITIAL_MAX_RECTS 10
#define COLOR_MIN 0.10f
#define COLOR_MAX 0.90f
#define ANIMATION_TIME 5.0
#define MOVE_SPEED 140.0f

using namespace std;

struct Color { float r, g, b; };
struct Rectangle {
    float x = 0, y = 0; // 중심: 좌측 상단 원점, 아래쪽이 +y
    float size = 0, startSize = 0;
    float velocityX = 0, velocityY = 0;
    Color color{}, startColor{};
    double startTime = 0;
    bool moving = false;
};

vector<Rectangle> rectangles;
int initialTarget = 0;
double previousTime = 0;
random_device rd;
mt19937 g(rd());

void ResetScene();
void AddRectangle(float x, float y);
void ClampRectangle(Rectangle& r);
bool IsOverlapping(const Rectangle& candidate);
void SplitRectangle(int index);
void AddPiece(float x, float y, float size, Color color, float directionX, float directionY);
void InputProcess(GLFWwindow* window);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UpdateScene(GLFWwindow* window);
void DrawScene();
void RefreshCallback(GLFWwindow* window);

int main() {
    if (!glfwInit()) {
        cerr << "GLFW initialization failed!" << endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(
        WINDOW_WIDTH, WINDOW_HEIGHT, "1-6 Spreading Rectangle Animation", nullptr, nullptr);
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
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetWindowRefreshCallback(window, RefreshCallback);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    ResetScene();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        InputProcess(window);
        UpdateScene(window);
        RefreshCallback(window);
        glfwWaitEventsTimeout(0.01);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

//--- 초기 실행 및 r 명령
void ResetScene() {
    rectangles.clear();
    rectangles.reserve(INITIAL_MAX_RECTS * 8);
    uniform_int_distribution<int> countDistribution(INITIAL_MIN_RECTS, INITIAL_MAX_RECTS);
    uniform_real_distribution<float> xDistribution(
        RECT_MAX_SIZE / 2, WINDOW_WIDTH - RECT_MAX_SIZE / 2);
    uniform_real_distribution<float> yDistribution(
        RECT_MAX_SIZE / 2, WINDOW_HEIGHT - RECT_MAX_SIZE / 2);
    initialTarget = countDistribution(g);

    int attempts = 0;
    while (static_cast<int>(rectangles.size()) < initialTarget && attempts < 10000) {
        AddRectangle(xDistribution(g), yDistribution(g));
        ++attempts;
    }
    previousTime = glfwGetTime();
}

void ClampRectangle(Rectangle& r) {
    const float half = r.size / 2;
    r.x = clamp(r.x, half, WINDOW_WIDTH - half);
    r.y = clamp(r.y, half, WINDOW_HEIGHT - half);
}

bool IsOverlapping(const Rectangle& candidate) {
    for (const Rectangle& r : rectangles) {
        if (r.moving) continue;
        const float distance = (candidate.size + r.size) / 2;
        if (abs(candidate.x - r.x) < distance && abs(candidate.y - r.y) < distance)
            return true;
    }
    return false;
}

void AddRectangle(float x, float y) {
    Rectangle r;
    uniform_real_distribution<float> sizeDistribution(RECT_MIN_SIZE, RECT_MAX_SIZE);
    uniform_real_distribution<float> colorDistribution(COLOR_MIN, COLOR_MAX);
    r.size = r.startSize = sizeDistribution(g);
    r.x = x;
    r.y = y;
    r.color = r.startColor = {
        colorDistribution(g), colorDistribution(g), colorDistribution(g)
    };
    ClampRectangle(r);
    if (!IsOverlapping(r)) rectangles.push_back(r);
}

void AddPiece(float x, float y, float size, Color color,
    float directionX, float directionY) {
    Rectangle piece;
    piece.x = x;
    piece.y = y;
    piece.size = piece.startSize = size;
    piece.color = piece.startColor = color;
    piece.velocityX = directionX * MOVE_SPEED;
    piece.velocityY = directionY * MOVE_SPEED;
    piece.startTime = glfwGetTime();
    piece.moving = true;
    rectangles.push_back(piece);
}

void SplitRectangle(int index) {
    const Rectangle source = rectangles[index];
    rectangles.erase(rectangles.begin() + index);
    uniform_int_distribution<int> modeDistribution(0, 3);
    const int mode = modeDistribution(g);

    // 우측 위, 우측 아래, 좌측 아래, 좌측 위 순서
    const float positionX[4] = { 1, 1, -1, -1 };
    const float positionY[4] = { -1, 1, 1, -1 };
    const float straightX[4] = { 0, 1, 0, -1 };
    const float straightY[4] = { -1, 0, 1, 0 };
    const float diagonalX[4] = { 1, 1, -1, -1 };
    const float diagonalY[4] = { -1, 1, 1, -1 };

    if (mode < 3) {
        float commonX = 0, commonY = 0;
        if (mode == 2) {
            static const float eightX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
            static const float eightY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
            uniform_int_distribution<int> directionDistribution(0, 7);
            const int direction = directionDistribution(g);
            commonX = eightX[direction];
            commonY = eightY[direction];
        }
        const float pieceSize = source.size / 2;
        for (int i = 0; i < 4; ++i) {
            const float directionX = mode == 0 ? straightX[i] :
                (mode == 1 ? diagonalX[i] : commonX);
            const float directionY = mode == 0 ? straightY[i] :
                (mode == 1 ? diagonalY[i] : commonY);
            AddPiece(source.x + positionX[i] * source.size / 4,
                source.y + positionY[i] * source.size / 4,
                pieceSize, source.color, directionX, directionY);
        }
    }
    else {
        // 3 x 3으로 나눈 뒤 가운데를 제외한 8개를 바깥 방향으로 이동한다.
        const float pieceSize = source.size / 3;
        for (int row = -1; row <= 1; ++row)
            for (int column = -1; column <= 1; ++column) {
                if (row == 0 && column == 0) continue;
                AddPiece(source.x + column * pieceSize,
                    source.y + row * pieceSize, pieceSize, source.color,
                    static_cast<float>(column), static_cast<float>(row));
            }
    }
}

void InputProcess(GLFWwindow* window) {
    static bool previousR = false;
    const bool currentR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (currentR && !previousR) ResetScene();
    previousR = currentR;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    for (int i = static_cast<int>(rectangles.size()) - 1; i >= 0; --i) {
        const Rectangle& r = rectangles[i];
        if (r.moving) continue;
        if (abs(static_cast<float>(mouseX) - r.x) <= r.size / 2 &&
            abs(static_cast<float>(mouseY) - r.y) <= r.size / 2) {
            SplitRectangle(i);
            break;
        }
    }
}

void UpdateScene(GLFWwindow*) {
    const double now = glfwGetTime();
    float deltaTime = static_cast<float>(now - previousTime);
    previousTime = now;
    deltaTime = min(deltaTime, 0.1f);

    for (Rectangle& r : rectangles) {
        if (!r.moving) continue;
        const float progress = clamp(
            static_cast<float>((now - r.startTime) / ANIMATION_TIME), 0.0f, 1.0f);
        r.size = r.startSize * (1.0f - progress);
        r.color = {
            r.startColor.r + (1.0f - r.startColor.r) * progress,
            r.startColor.g + (1.0f - r.startColor.g) * progress,
            r.startColor.b + (1.0f - r.startColor.b) * progress
        };
        r.x += r.velocityX * deltaTime;
        r.y += r.velocityY * deltaTime;

        const float half = r.size / 2;
        if (r.x - half < 0) {
            r.x = half;
            r.velocityX = abs(r.velocityX);
        }
        else if (r.x + half > WINDOW_WIDTH) {
            r.x = WINDOW_WIDTH - half;
            r.velocityX = -abs(r.velocityX);
        }
        if (r.y - half < 0) {
            r.y = half;
            r.velocityY = abs(r.velocityY);
        }
        else if (r.y + half > WINDOW_HEIGHT) {
            r.y = WINDOW_HEIGHT - half;
            r.velocityY = -abs(r.velocityY);
        }
    }

    rectangles.erase(remove_if(rectangles.begin(), rectangles.end(),
        [now](const Rectangle& r) {
            return r.moving && now - r.startTime >= ANIMATION_TIME;
        }), rectangles.end());
}

void DrawScene() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClear(GL_COLOR_BUFFER_BIT);
    for (const Rectangle& r : rectangles) {
        glColor3f(r.color.r, r.color.g, r.color.b);
        glRectf(r.x - r.size / 2, r.y - r.size / 2,
            r.x + r.size / 2, r.y + r.size / 2);
    }
}

void RefreshCallback(GLFWwindow* window) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    DrawScene();
    glfwSwapBuffers(window);
}
