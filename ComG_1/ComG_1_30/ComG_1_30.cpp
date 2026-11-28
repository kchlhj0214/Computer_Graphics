#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glu.h>
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
#include <GL/glm/gtc/matrix_transform.hpp>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#define STBI_ONLY_JPEG
#include "stb_image.h"
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
#define AXIS_LENGTH 1.0f
#define CONE_RADIUS 0.0225f
#define CONE_HEIGHT (CONE_RADIUS * 2.0f)
#define CONE_SLICES 24
#define CONE_STACKS 16
#define PERSPECTIVE_FOVY 27.0f
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 30.0f
#define ZOOM_STEP 0.05f
#define CAMERA_RADIUS_MIN 1.50f
#define CAMERA_RADIUS_MAX 20.0f
#define CAMERA_X 3.0f
#define CAMERA_Y 2.4f
#define CAMERA_Z 4.0f
#define CAMERA_TARGET_X 0.0f
#define CAMERA_TARGET_Y 0.0f
#define CAMERA_TARGET_Z 0.0f
#define CAMERA_MOVE_SPEED 1.0f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
#define OBJECT_SIZE 0.75f
#define OBJECT_ROTATE_SPEED 90.0f
#define TEXTURE_COUNT 6

struct Vertex {
    glm::vec3 position, normal;
    glm::vec2 uv;
};

struct Face {
    GLint first;
    GLsizei count;
    GLuint textureIndex;
};

enum class ObjectType { Cube, Pyramid };
ObjectType objectType = ObjectType::Cube;
glm::mat4 objectRotation(1.0f);
const char* const textureFiles[TEXTURE_COUNT] = {
    "cats.jpeg",
    "고양이 인생, 힘이 든다.jpeg",
    "다운로드 (2).jpeg",
    "다운로드.jpeg",
    "에베베베베ㅔ베ㅔ베벱.jpeg",
    "이달의 소녀(LOOΠΔ) (@loonatheworld) on X.jpeg",
};
GLuint textures[TEXTURE_COUNT]{};
vector<Face> cubeFaces, pyramidFaces;
GLuint objectVao = 0, objectVbo = 0;
GLint textureEnabledLocation = -1;

glm::vec3 cameraTarget(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
glm::vec3 cameraPosition = cameraTarget + glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z);
float cameraRadius = glm::length(cameraPosition - cameraTarget);
float cameraYaw = 0.0f, cameraPitch = 0.0f;
double previousMouseX = 0.0, previousMouseY = 0.0;
bool dragging = false;


GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation = -1, viewLocation = -1, projectionLocation = -1, colorLocation = -1;
GLUquadric* qobj = nullptr;

string ReadFile(const char* name);
bool InitShader();
void InitBuffer();
bool InitTexture();
void DrawObject();
void DrawScene(int width, int height);
void KeyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void CursorCallback(GLFWwindow* window, double x, double y);

int main()
{
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // GLU로 기존 좌표축 원뿔을 그리기 위해 호환 프로필을 사용합니다.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-30 Texture Mapping", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK || !InitShader() || !InitTexture()) {
        glDeleteTextures(TEXTURE_COUNT, textures);
        if (programID) glDeleteProgram(programID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    InitBuffer();
    glfwSetKeyCallback(window, KeyCallback);
    glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
    glfwSetMouseButtonCallback(window, MouseCallback);
    glfwSetCursorPosCallback(window, CursorCallback);
    glfwSetScrollCallback(window, ScrollCallback);
    float vertices[] = {
        -AXIS_LENGTH,0,0, AXIS_LENGTH,0,0,
        0,-AXIS_LENGTH,0, 0,AXIS_LENGTH,0,
        0,0,-AXIS_LENGTH, 0,0,AXIS_LENGTH
    };
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
    glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0); glBindVertexArray(0);
    qobj = gluNewQuadric();
    if (!qobj) {
        glDeleteTextures(TEXTURE_COUNT, textures);
        glDeleteBuffers(1, &objectVbo); glDeleteVertexArrays(1, &objectVao);
        glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao);
        glDeleteProgram(programID);
        glfwDestroyWindow(window); glfwTerminate(); return -1;
    }
    gluQuadricDrawStyle(qobj, GLU_FILL);
    gluQuadricNormals(qobj, GLU_NONE);
    gluQuadricOrientation(qobj, GLU_OUTSIDE);
    cout << "c: cube | p: pyramid | x/X, y/Y held: rotate | s: reset\n"
         << "drag/wheel/arrows: camera | q: quit" << endl;
    double cameraMoveTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // 카메라와 시선 중심을 함께 XZ 평면에서 이동합니다.
        double cameraNow = glfwGetTime();
        float cameraDelta = (float)glm::min(cameraNow - cameraMoveTime, 0.1);
        cameraMoveTime = cameraNow;
        bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
            || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        float angle = glm::radians((shift ? -1.0f : 1.0f) * OBJECT_ROTATE_SPEED * cameraDelta);
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            objectRotation = glm::rotate(glm::mat4(1), angle, glm::vec3(1, 0, 0)) * objectRotation;
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
            objectRotation = glm::rotate(glm::mat4(1), angle, glm::vec3(0, 1, 0)) * objectRotation;
        glm::vec3 direction(0.0f);
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) direction.z -= 1;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) direction.z += 1;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) direction.x += 1;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) direction.x -= 1;
        if (glm::length(direction) > 0) {
            glm::vec3 movement = glm::normalize(direction) * CAMERA_MOVE_SPEED * cameraDelta;
            cameraTarget += movement; cameraPosition += movement;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        int width, height; glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        if (width > 0 && height > 0) DrawScene(width, height);
        glfwSwapBuffers(window);
    }
    gluDeleteQuadric(qobj);
    glDeleteTextures(TEXTURE_COUNT, textures);
    glDeleteBuffers(1, &objectVbo); glDeleteVertexArrays(1, &objectVao);
    glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
    glfwDestroyWindow(window); glfwTerminate(); return 0;
}

string ReadFile(const char* name)
{
    ifstream file(name);
    if (!file) { cerr << "Cannot open shader: " << name << endl; return ""; }
    stringstream stream; stream << file.rdbuf(); return stream.str();
}

// 각 면의 정점을 연속으로 저장하고 위치, 법선, UV를 속성으로 등록합니다.
void InitBuffer()
{
    vector<Vertex> vertices;
    const glm::vec2 uv[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
    auto addTriangle = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c,
        glm::vec2 uvA, glm::vec2 uvB, glm::vec2 uvC) {
        glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
        vertices.push_back({a, normal, uvA});
        vertices.push_back({b, normal, uvB});
        vertices.push_back({c, normal, uvC});
    };
    auto addQuad = [&](vector<Face>& faces, GLuint index, glm::vec3 a, glm::vec3 b,
        glm::vec3 c, glm::vec3 d) {
        faces.push_back({(GLint)vertices.size(), 6, index});
        addTriangle(a, b, c, uv[0], uv[1], uv[2]);
        addTriangle(a, c, d, uv[0], uv[2], uv[3]);
    };

    const glm::vec3 a(-.5f, -.5f, .5f), b(.5f, -.5f, .5f);
    const glm::vec3 c(.5f, -.5f, -.5f), d(-.5f, -.5f, -.5f);
    const glm::vec3 e(-.5f, .5f, .5f), f(.5f, .5f, .5f);
    const glm::vec3 g(.5f, .5f, -.5f), h(-.5f, .5f, -.5f);
    addQuad(cubeFaces, 0, a, b, f, e);
    addQuad(cubeFaces, 1, b, c, g, f);
    addQuad(cubeFaces, 2, c, d, h, g);
    addQuad(cubeFaces, 3, d, a, e, h);
    addQuad(cubeFaces, 4, e, f, g, h);
    addQuad(cubeFaces, 5, d, c, b, a);

    const glm::vec3 base[4] = {a, b, c, d};
    const glm::vec3 apex(0, .5f, 0);
    for (GLuint i = 0; i < 4; ++i) {
        pyramidFaces.push_back({(GLint)vertices.size(), 3, i});
        // 이미지의 아래 두 모서리와 위쪽 중심을 옆면에 대응시킵니다.
        addTriangle(base[i], base[(i + 1) % 4], apex, uv[0], uv[1], glm::vec2(.5f, 1));
    }
    addQuad(pyramidFaces, 4, d, c, b, a);

    glGenVertexArrays(1, &objectVao);
    glGenBuffers(1, &objectVbo);
    glBindVertexArray(objectVao);
    glBindBuffer(GL_ARRAY_BUFFER, objectVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

bool InitTexture()
{
    stbi_set_flip_vertically_on_load(true);
    glGenTextures(TEXTURE_COUNT, textures);
    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        int width, height, channels;
        unsigned char* pixels = stbi_load(textureFiles[i], &width, &height, &channels, STBI_rgb);
        if (!pixels) {
            cerr << "Cannot load texture: " << textureFiles[i] << " (" << stbi_failure_reason() << ")" << endl;
            return false;
        }
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // RGB 픽셀 행을 여백 없이 업로드합니다.
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        stbi_image_free(pixels);
        if (glGetError() != GL_NO_ERROR) {
            cerr << "Cannot upload texture: " << textureFiles[i] << endl;
            return false;
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
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
    textureEnabledLocation = glGetUniformLocation(programID, "useTexture");
    GLint samplerLocation = glGetUniformLocation(programID, "outTexture");
    if (modelLocation < 0 || viewLocation < 0 || projectionLocation < 0 || colorLocation < 0
        || textureEnabledLocation < 0 || samplerLocation < 0) return false;
    glUseProgram(programID);
    glUniform1i(samplerLocation, 0);
    return true;
}

void DrawScene(int width, int height)
{
    glClearColor(1, 1, 1, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(programID);
    glUniform1i(textureEnabledLocation, GL_FALSE);
    glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, glm::vec3(0, 1, 0));
    glm::mat4 projection = glm::perspective(glm::radians(PERSPECTIVE_FOVY), (float)width / height, PERSPECTIVE_NEAR, PERSPECTIVE_FAR);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
    glm::vec3 colors[3] = { {0.85f,0.12f,0.12f},{0.12f,0.65f,0.20f},{0.15f,0.30f,0.90f} };
    glm::vec3 ends[3] = { {AXIS_LENGTH,0,0},{0,AXIS_LENGTH,0},{0,0,AXIS_LENGTH} };
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glBindVertexArray(vao); glLineWidth(1);
    for (int i = 0; i < 3; ++i) {
        glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
        glDrawArrays(GL_LINES, i * 2, 2);
    }
    glBindVertexArray(0);
    for (int i = 0; i < 3; ++i) {
        model = glm::mat4(1.0f);
        model = glm::translate(model, ends[i]);
        // 원뿔이 각 좌표축의 양의 방향을 가리키도록 회전합니다.
        if (i == 0) model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 1, 0));
        else if (i == 1) model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
        model = glm::translate(model, glm::vec3(0, 0, -CONE_HEIGHT * 0.5f));
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLocation, 1, glm::value_ptr(colors[i]));
        gluCylinder(qobj, CONE_RADIUS, 0.0, CONE_HEIGHT, CONE_SLICES, CONE_STACKS);
        // 원뿔의 밑면을 바깥 방향으로 닫습니다.
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        gluDisk(qobj, 0.0, CONE_RADIUS, CONE_SLICES, 1);
    }
    DrawObject();
}

void DrawObject()
{
    glm::mat4 model = glm::scale(objectRotation, glm::vec3(OBJECT_SIZE));
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(textureEnabledLocation, GL_TRUE);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(objectVao);
    const auto& faces = objectType == ObjectType::Cube ? cubeFaces : pyramidFaces;
    for (const Face& face : faces) {
        // 면을 그릴 때마다 같은 텍스처 유닛에 해당 이미지를 바인딩합니다.
        glBindTexture(GL_TEXTURE_2D, textures[face.textureIndex]);
        glDrawArrays(GL_TRIANGLES, face.first, face.count);
    }
    glBindVertexArray(0);
}

void KeyCallback(GLFWwindow*, int key, int, int action, int)
{
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_C) objectType = ObjectType::Cube;
    if (key == GLFW_KEY_P) objectType = ObjectType::Pyramid;
    if (key == GLFW_KEY_S) {
        objectType = ObjectType::Cube;
        objectRotation = glm::mat4(1);
        cameraTarget = glm::vec3(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
        cameraPosition = cameraTarget + glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z);
        cameraRadius = glm::length(cameraPosition - cameraTarget);
        dragging = false;
    }
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action == GLFW_PRESS) {
        dragging = true;
        glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
        cameraYaw = glm::atan(cameraPosition.x - cameraTarget.x, cameraPosition.z - cameraTarget.z);
        cameraPitch = glm::asin((cameraPosition.y - cameraTarget.y) / cameraRadius);
    }
    else if (action == GLFW_RELEASE) {
        dragging = false;
        cameraPosition = cameraTarget + glm::normalize(glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z)) * cameraRadius;
    }
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraRadius = glm::clamp(cameraRadius - (float)yoffset * ZOOM_STEP, CAMERA_RADIUS_MIN, CAMERA_RADIUS_MAX);
    cameraPosition = cameraTarget + glm::normalize(cameraPosition - cameraTarget) * cameraRadius;
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
    if (!dragging) return;
    int width, height; glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return;
    // 드래그의 가로·세로 이동량으로 카메라의 방위각과 고도각을 바꿉니다.
    cameraYaw -= glm::radians(ORBIT_DEGREES) * (float)(x - previousMouseX) / width;
    cameraPitch += glm::radians(ORBIT_DEGREES) * (float)(y - previousMouseY) / height;
    previousMouseX = x; previousMouseY = y;
    cameraYaw = glm::mod(cameraYaw, glm::radians(360.0f));
    cameraPitch = glm::clamp(cameraPitch, -glm::radians(ORBIT_PITCH_LIMIT), glm::radians(ORBIT_PITCH_LIMIT));
    cameraPosition = cameraTarget + cameraRadius * glm::vec3(
        glm::cos(cameraPitch) * glm::sin(cameraYaw),
        glm::sin(cameraPitch),
        glm::cos(cameraPitch) * glm::cos(cameraYaw));
}
