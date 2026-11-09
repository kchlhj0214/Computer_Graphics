#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm/glm.hpp>
#include <GL/glm/glm/ext.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <limits>
#include <stdexcept>
#include <algorithm>
using namespace std;

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200
// 트럭과 벨트가 시작 화면에 함께 들어오는 시점
#define CAMERA_X 7.0f
#define CAMERA_Y 6.0f
#define CAMERA_Z 9.0f
#define CAMERA_TARGET_X 0.55f
#define CAMERA_TARGET_Y 0.50f
#define CAMERA_TARGET_Z -1.0f
#define PERSPECTIVE_FOVY 45.0f
#define PERSPECTIVE_ASPECT ((float)WINDOW_WIDTH / WINDOW_HEIGHT)
#define PERSPECTIVE_NEAR 0.10f
#define PERSPECTIVE_FAR 50.0f
#define CAMERA_MOVE_SPEED 1.5f
#define CAMERA_RADIUS_MIN 0.50f
#define CAMERA_RADIUS_MAX 25.0f
#define VIEW_RANGE 5.5f
#define VIEW_RANGE_MIN 0.2f
#define VIEW_RANGE_MAX 15.0f
#define ZOOM_STEP 0.05f
#define ORBIT_DEGREES 720.0f
#define ORBIT_PITCH_LIMIT 89.0f
// 바닥 가로(X), 세로(Z): 직전 크기의 2배
#define FLOOR_WIDTH (6.0f * 8.0f)
#define FLOOR_DEPTH (5.0f * 8.0f)
#define FLOOR_HEIGHT 0.06f
#define BELT_START_X -1.20f
#define BELT_START_Z 1.60f
#define BELT_HEIGHT 0.65f
#define BELT_WIDTH 0.65f
#define BELT_FIRST_LENGTH 2.0f
#define BELT_SECOND_LENGTH 2.2f
#define BELT_CORNER_RADIUS 0.65f
#define BELT_SEGMENTS 120
#define BELT_SPEED 0.40f
#define BELT_END_MARGIN 0.25f
#define STRIPE_COUNT 32
#define STRIPE_LENGTH 0.055f
#define STRIPE_HEIGHT 0.008f
#define RAIL_WIDTH 0.035f
#define RAIL_HEIGHT 0.045f
#define LEG_WIDTH 0.06f
#define LEG_PAIRS 7
#define BOX_SIZE 0.30f
#define BOX_COUNT 5
#define MARK_WIDTH 0.032f
#define MARK_LENGTH 0.12f
#define MARK_HEIGHT 0.004f
#define MARK_GAP 0.018f
#define CEILING_HEIGHT 4.0f
// 트럭 양쪽과 벨트 주변까지 넓게 분포하는 천장 조명
#define CEILING_WIDTH FLOOR_WIDTH
#define CEILING_DEPTH FLOOR_DEPTH
// 높이로 크기를 정하며, 길이와 폭은 원본 비율을 유지한다.
#define TRUCK_HEIGHT (BELT_HEIGHT * 2.75f)
#define TRUCK_BELT_GAP 0.65f
#define TRUCK_YAW -90.0f
#define TRUCK_DRIVE_SPEED 1.5f
#define TRUCK_STEER_LIMIT 45.0f
#define TRUCK_STEER_SPEED 65.0f
#define TRUCK_STEER_RETURN_SPEED 90.0f
#define TRUCK_PHYSICS_STEP (1.0f / 120.0f)
#define TRUCK_ARTICULATION_LIMIT 35.0f
#define TRUCK_JOINT_MOBILITY 0.60f
#define TRUCK_BOUNDARY_MARGIN 0.02f
#define TRUCK_VIEW_HEIGHT 6.5f
#define TRUCK_VIEW_FORWARD 6.0f
#define TRACTOR_REAR_Z -17.0275f
#define TRACTOR_FRONT_Z 7.947f
#define TRUCK_HITCH_Z -16.0f
#define TRAILER_AXLE_Z -44.731f
#define FRONT_WHEEL_X 5.5f
#define FRONT_WHEEL_Y 2.347f
#define TRUCK_OBJ_FILE "Truck.obj"
#define TRUCK_MTL_FILE "Truck.mtl"
// O: 옆면과 지붕을 한 덩어리로 열고 닫는다.
#define TRUCK_OPEN_ANGLE 60.0f
#define TRUCK_OPEN_SPEED 45.0f
// 아래 값은 Truck.obj 원본 좌표의 지붕 모서리와 옆판 경계이다.
// 트럭의 크기/위치는 truckModel로 적용되므로 이 값은 함께 바꿀 필요가 없다.
#define TRUCK_HINGE_X -5.784952f
#define TRUCK_HINGE_Y 16.474405f
#define TRUCK_SIDE_X 5.90f
#define TRUCK_SIDE_BOTTOM 5.927f
#define TRUCK_PANEL_FRONT_Z -15.49f
#define TRUCK_PANEL_BACK_Z -58.16f
#define TRUCK_ROOF_BOTTOM 16.32f
#define PARKING_MARGIN 0.50f
// 주차선 바깥부터 맵 가장자리까지 확보할 통로 폭
#define MAP_EDGE_CLEARANCE 3.0f
// 각 지점에서는 가까운 8x8 조명을 계산해 조명 증가에 따른 부하를 제한한다.
#define LIGHT_LOCAL_SAMPLES 8
#define PARKING_LINE_WIDTH 0.08f
#define PARKING_LINE_HEIGHT 0.006f
#define LIGHT_SAMPLES_X 48
#define LIGHT_SAMPLES_Z 40
// 샘플 수는 평균되므로 밝기는 별도로 조절한다.
#define DIFFUSE_STRENGTH 2.5f
#define SPECULAR_STRENGTH 0.30f
#define SHININESS 32.0f
#define BOX_EYE_HEIGHT (BOX_SIZE * .25f)
#define BOX_EYE_FORWARD (BOX_SIZE * .5f + .015f)
#define CARGO_MOVE_SECONDS 2.4f
#define CARGO_GAP 0.025f
#define CARGO_BELT_GAP 0.06f
#define CARGO_LIGHT_STRENGTH 1.8f
#define CARGO_LIGHT_ATTENUATION 0.45f
// 점조명 적용 범위: 적재 여백이 아닌 실제 짐칸 안쪽 표면의 원본 좌표
#define CARGO_LIGHT_MIN_X -5.65f
#define CARGO_LIGHT_MAX_X 5.974f
#define CARGO_LIGHT_MIN_Z -57.108f
#define CARGO_LIGHT_MAX_Z -15.493f
#define CARGO_LIGHT_TOP 16.475f
// 적재 공간: OBJ 원본 좌표. 바닥 윗면부터 닫힌 지붕 안쪽까지.
#define CARGO_MIN_X -5.30f
#define CARGO_MAX_X 5.50f
#define CARGO_FLOOR_Y 5.48f
#define CARGO_CEILING_Y 15.90f
#define CARGO_MIN_Z -56.70f
#define CARGO_MAX_Z -15.90f

struct Vertex { glm::vec3 position, normal; };
glm::vec3 boxColors[BOX_COUNT] = { {.9f,.1f,.1f},{.9f,.45f,.1f},{.9f,.9f,.1f},{.1f,.8f,.1f},{.1f,.25f,.9f} };
glm::vec3 cameraPosition(CAMERA_X, CAMERA_Y, CAMERA_Z), cameraTarget(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z);
float beltDistance = 0, viewRange = VIEW_RANGE;
float pathLength = BELT_FIRST_LENGTH + BELT_CORNER_RADIUS * glm::half_pi<float>() + BELT_SECOND_LENGTH;
bool moving = false, perspective = true, dragging = false;
double previousMouseX = 0, previousMouseY = 0;
GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation, eyeLocation;
int surfaceCount = 0;

// 같은 재질의 삼각형을 한 번에 그리는 구간
struct TruckPart {
	int first, count; glm::vec3 color; bool wing; bool tractor; int frontWheel;
	glm::vec3 minimum{ numeric_limits<float>::max() }, maximum{ numeric_limits<float>::lowest() };
};
vector<TruckPart> truckParts;
GLuint truckVao = 0, truckVbo = 0;
glm::vec3 truckSize(0), truckCenter(0), sceneOffset(0);
glm::mat4 truckModel(1);
glm::mat4 tractorModel(1), initialTruckModel(1);
glm::vec3 tractorRear(0), trailerRear(0), secondParkingCenter(0);
float truckScale = 1, tractorYaw = TRUCK_YAW, trailerYaw = TRUCK_YAW, steeringAngle = 0;
float truckOpenAngle = 0;
bool truckOpen = false;
bool firstPerson = false;
bool truckView = false;
glm::vec3 truckViewOffset(0, TRUCK_VIEW_HEIGHT, -TRUCK_VIEW_FORWARD);
int viewBoxNumber = 0;
enum class CargoState { Belt, WaitingSpawn, Loading, Loaded }; // 벨트 위, 재생성 대기, 적재 이동 중, 적재 완료
struct BeltCargo { float distance = 0; bool requested = false; CargoState state = CargoState::Belt; };
struct StoredCargo { int number; glm::vec3 localCenter; };
BeltCargo beltCargo[BOX_COUNT];
vector<StoredCargo> storedCargo;
vector<glm::vec3> cargoSlots;
int loadingNumber = -1;
float loadingProgress = 0;
glm::vec3 loadingStart(0);
bool closeAfterLoading = false;
GLint cargoLightsLocation, cargoLightStrengthLocation, cargoLightAttenuationLocation;
GLint worldToTruckLocation;

string ReadFile(const char* name);
bool InitShader();
bool InitTruck();
void DrawTruck();
void UpdateTruckDoor(float dt);
void ResetTruckMotion();
void UpdateTruckMotion(GLFWwindow* window, float dt);
void AdvanceTruckMotion(float throttle, float steeringInput, float dt);
glm::mat4 TruckPartTransform(const TruckPart& part);
bool TruckInsideFloor();
bool BoxViewTransform(int number, glm::mat4& model);
void ResetCargo();
void UpdateCargo(float dt);
void DrawCargo();
void DrawNumberedBox(const glm::mat4& model, int number);
glm::mat4 LoadingTransform();
glm::mat4 TruckWingTransform();
glm::mat4 BeltTransform(float distance);
void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color);
void DrawScene();
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE); glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "1-24 Conveyor and Truck", nullptr, nullptr);
	if (!window) { glfwTerminate(); return -1; }
	glfwMakeContextCurrent(window); glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	if (!InitTruck()) { glfwDestroyWindow(window); glfwTerminate(); return -1; }
	cameraPosition += sceneOffset; cameraTarget += sceneOffset;
	// 안쪽(-X) 행부터 채우고, 바닥 한 층이 찬 다음 윗층을 채운다.
	float truckScale = glm::length(glm::vec3(truckModel[0]));
	float localBox = BOX_SIZE / truckScale;
	float localPitch = (BOX_SIZE + CARGO_GAP) / truckScale;
	for (float y = CARGO_FLOOR_Y + localBox * .5f; y + localBox * .5f <= CARGO_CEILING_Y; y += localBox)
		for (float x = CARGO_MIN_X + localBox * .5f; x + localBox * .5f <= CARGO_MAX_X; x += localPitch)
			// 운전석 쪽(현재 화면의 왼쪽)부터 뒤쪽으로 채운다.
			for (float z = CARGO_MAX_Z - localBox * .5f; z - localBox * .5f >= CARGO_MIN_Z; z -= localPitch)
				cargoSlots.push_back(glm::vec3(x, y, z));
	ResetCargo();
	glfwSwapInterval(1); glEnable(GL_DEPTH_TEST);
	glfwSetKeyCallback(window, KeyCallback); glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback); glfwSetScrollCallback(window, ScrollCallback);
	glm::vec3 corners[] = { {-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
		{-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f} };
	int indices[] = { 0,2,1,0,3,2, 4,5,6,4,6,7, 0,4,7,0,7,3,
		1,2,6,1,6,5, 3,7,6,3,6,2, 0,1,5,0,5,4 };
	glm::vec3 normals[] = { {0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,1,0},{0,-1,0} };
	vector<Vertex> vertices;
	for (int i = 0; i < 36; ++i)vertices.push_back({ corners[indices[i]], normals[i / 6] });
	// A continuous flat ribbon follows the same rounded path as the boxes.
	for (int i = 0; i < BELT_SEGMENTS; ++i) {
		float d0 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * i / BELT_SEGMENTS;
		float d1 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * (i + 1) / BELT_SEGMENTS;
		glm::mat4 a = BeltTransform(d0), b = BeltTransform(d1);
		glm::vec3 p[] = { glm::vec3(a * glm::vec4(-BELT_WIDTH * .5f,0,0,1)), glm::vec3(a * glm::vec4(BELT_WIDTH * .5f,0,0,1)),
			glm::vec3(b * glm::vec4(-BELT_WIDTH * .5f,0,0,1)), glm::vec3(b * glm::vec4(BELT_WIDTH * .5f,0,0,1)) };
		int face[] = { 0,1,2,1,3,2 };
		for (int index : face)vertices.push_back({ p[index],glm::vec3(0,1,0) });
	}
	surfaceCount = (int)vertices.size() - 36;
	glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); glEnableVertexAttribArray(1); glBindVertexArray(0);
	glUseProgram(programID); eyeLocation = glGetUniformLocation(programID, "eyePosition");
	cargoLightsLocation = glGetUniformLocation(programID, "cargoLights[0]");
	worldToTruckLocation = glGetUniformLocation(programID, "worldToTruck");
	glUniform3f(glGetUniformLocation(programID, "cargoLightMin"),
		CARGO_LIGHT_MIN_X, CARGO_FLOOR_Y - .005f, CARGO_LIGHT_MIN_Z);
	glUniform3f(glGetUniformLocation(programID, "cargoLightMax"),
		CARGO_LIGHT_MAX_X, CARGO_LIGHT_TOP, CARGO_LIGHT_MAX_Z);
	cargoLightStrengthLocation = glGetUniformLocation(programID, "cargoLightStrength");
	cargoLightAttenuationLocation = glGetUniformLocation(programID, "cargoLightAttenuation");
	glUniform1f(cargoLightStrengthLocation, CARGO_LIGHT_STRENGTH);
	glUniform1f(cargoLightAttenuationLocation, CARGO_LIGHT_ATTENUATION);
	glUniform2f(glGetUniformLocation(programID, "ceilingSize"), CEILING_WIDTH, CEILING_DEPTH);
	glUniform1f(glGetUniformLocation(programID, "ceilingHeight"), CEILING_HEIGHT);
	glUniform2i(glGetUniformLocation(programID, "lightSamples"), LIGHT_SAMPLES_X, LIGHT_SAMPLES_Z);
	glUniform1i(glGetUniformLocation(programID, "localSamples"), LIGHT_LOCAL_SAMPLES);
	glUniform1f(glGetUniformLocation(programID, "diffuseStrength"), DIFFUSE_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "specularStrength"), SPECULAR_STRENGTH);
	glUniform1f(glGetUniformLocation(programID, "shininess"), SHININESS);
	cout << "v: view | n/N: next/previous box | 1-5: load/respawn | o: truck open/close | b: start/stop | arrows: camera | drag: orbit | wheel, +/-: zoom | p/P: PERSP/ORTHO | r: reset | q: quit" << endl;
	cout << "W/S: truck forward/reverse | A/D: front-wheel steering (45 deg max, automatic return) | T: truck 3/4 view" << endl;
	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents(); double now = glfwGetTime(); float dt = (float)glm::min(now - previousTime, 0.1); previousTime = now;
		glm::vec3 movement(0);
		glm::vec3 forward = cameraTarget - cameraPosition;
		forward.y = 0;
		if (glm::length(forward) > 0)forward = glm::normalize(forward);
		glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)movement += forward;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)movement -= forward;
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)movement -= right;
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)movement += right;
		if (!firstPerson && !truckView && glm::length(movement) > 0) { movement = glm::normalize(movement) * CAMERA_MOVE_SPEED * dt; cameraPosition += movement; cameraTarget += movement; }
		if (moving)beltDistance = glm::mod(beltDistance + BELT_SPEED * dt, pathLength);
		UpdateTruckDoor(dt);
		UpdateTruckMotion(window, dt);
		UpdateCargo(dt);
		int width, height; glfwGetFramebufferSize(window, &width, &height); glViewport(0, 0, width, height);
		DrawScene(); glfwSwapBuffers(window);
	}
	glDeleteBuffers(1, &truckVbo); glDeleteVertexArrays(1, &truckVao);
	glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); glDeleteProgram(programID);
	glfwDestroyWindow(window); glfwTerminate(); return 0;
}

// OBJ의 면/노멀과 MTL의 Kd(기본 색상)를 읽는다.
// 현재 트럭처럼 이미지 텍스처 없이 단색 재질을 사용하는 모델용이다.
bool InitTruck()
{
	try {
		ifstream materialFile(TRUCK_MTL_FILE), objectFile(TRUCK_OBJ_FILE);
		if (!materialFile || !objectFile)
			throw runtime_error("Cannot open Truck.obj / Truck.mtl");

		map<string, glm::vec3> colors;
		string line, name, objectName;
		while (getline(materialFile, line)) {
			istringstream input(line);
			string type; input >> type;
			if (type == "newmtl") input >> name;
			else if (type == "Kd") input >> colors[name].r >> colors[name].g >> colors[name].b;
		}

		vector<glm::vec3> positions, normals;
		vector<Vertex> vertices;
		glm::vec3 color(.8f);
		glm::vec3 minimum(numeric_limits<float>::max()), maximum(numeric_limits<float>::lowest());
		while (getline(objectFile, line)) {
			istringstream input(line);
			string type; input >> type;
			if (type == "v" || type == "vn") {
				glm::vec3 value; input >> value.x >> value.y >> value.z;
				(type == "v" ? positions : normals).push_back(value);
			}
			else if (type == "o") input >> objectName;
			else if (type == "usemtl") {
				input >> name;
				auto found = colors.find(name);
				if (found == colors.end()) throw runtime_error("Unknown truck material: " + name);
				color = found->second;
			}
			else if (type == "f") {
				vector<Vertex> face;
				string token;
				while (input >> token) {
					if (token[0] == '#') break;
					size_t slash = token.find('/');
					int index = stoi(token.substr(0, slash));
					int resolved = index > 0 ? index - 1 : (int)positions.size() + index;
					Vertex vertex{ positions.at(resolved), glm::vec3(0) };
					size_t last = token.rfind('/');
					if (slash != string::npos && last != slash && last + 1 < token.size()) {
						int normal = stoi(token.substr(last + 1));
						vertex.normal = normals.at(normal > 0 ? normal - 1 : (int)normals.size() + normal);
					}
					face.push_back(vertex);
				}
				// 내보내기 때 삼각형 변환을 하지 않은 면도 나눠서 저장한다.
				for (size_t i = 1; i + 1 < face.size(); ++i) {
					Vertex triangle[] = { face[0], face[i], face[i + 1] };
					glm::vec3 normal = glm::cross(triangle[1].position - triangle[0].position,
						triangle[2].position - triangle[0].position);
					if (glm::length(normal) < .000001f) continue;
					normal = glm::normalize(normal);
					// 짐칸의 +X 옆판(현재 벨트 쪽)과 지붕만 움직인다.
					// 뒤/앞 벽과 하부 프레임은 그대로 둔다. 원본 OBJ는 수정하지 않는다.
					glm::vec3 low = glm::min(triangle[0].position, glm::min(triangle[1].position, triangle[2].position));
					glm::vec3 high = glm::max(triangle[0].position, glm::max(triangle[1].position, triangle[2].position));
					bool roof = low.y >= TRUCK_ROOF_BOTTOM;
					bool side = low.x >= TRUCK_SIDE_X && low.y >= TRUCK_SIDE_BOTTOM
						&& low.z >= TRUCK_PANEL_BACK_Z && high.z <= TRUCK_PANEL_FRONT_Z;
					bool wing = objectName == "Cube" && (roof || side);
					bool tractor = objectName != "Cube" && objectName != "Wheels.001"
						&& objectName != "Cube.001" && objectName != "Cube.002"
						&& objectName != "Cube.003" && objectName != "Cube.004";
					int frontWheel = objectName == "Wheels" && low.z > 0 ? (low.x > 0 ? 1 : 0) : -1;
					if (truckParts.empty() || truckParts.back().color != color || truckParts.back().wing != wing
						|| truckParts.back().tractor != tractor || truckParts.back().frontWheel != frontWheel)
						truckParts.push_back({ (int)vertices.size(), 0, color, wing, tractor, frontWheel });
					for (Vertex vertex : triangle) {
						if (glm::length(vertex.normal) < .000001f) vertex.normal = normal;
						vertices.push_back(vertex);
						truckParts.back().minimum = glm::min(truckParts.back().minimum, vertex.position);
						truckParts.back().maximum = glm::max(truckParts.back().maximum, vertex.position);
						minimum = glm::min(minimum, vertex.position);
						maximum = glm::max(maximum, vertex.position);
					}
					truckParts.back().count += 3;
				}
			}
		}
		if (vertices.empty() || maximum.y <= minimum.y)
			throw runtime_error("Truck mesh is empty or has no height");

		// 면에 실제로 사용되는 정점만으로 크기를 구해 고립 정점은 제외한다.
		float scale = TRUCK_HEIGHT / (maximum.y - minimum.y);
		glm::vec3 size = (maximum - minimum) * scale;
		glm::mat4 rotation = glm::rotate(glm::mat4(1), glm::radians(TRUCK_YAW), glm::vec3(0, 1, 0));
		truckSize = glm::abs(glm::vec3(rotation[0])) * size.x
			+ glm::abs(glm::vec3(rotation[1])) * size.y + glm::abs(glm::vec3(rotation[2])) * size.z;
		float lastStartX = BELT_START_X + BELT_CORNER_RADIUS;
		float lastZ = BELT_START_Z - BELT_FIRST_LENGTH - BELT_CORNER_RADIUS;
		truckCenter = glm::vec3(lastStartX + BELT_SECOND_LENGTH * .5f, 0,
			lastZ - BELT_WIDTH * .5f - RAIL_WIDTH - TRUCK_BELT_GAP - truckSize.z * .5f);
		// 트럭이 -Z 가장자리 쪽, 벨트가 맵 안쪽에 오도록 함께 평행 이동한다.
		float parkingBack = truckCenter.z - truckSize.z * .5f - PARKING_MARGIN - PARKING_LINE_WIDTH * .5f;
		sceneOffset.z = -FLOOR_DEPTH * .5f + MAP_EDGE_CLEARANCE - parkingBack;
		truckCenter += sceneOffset;
		glm::vec3 origin((minimum.x + maximum.x) * .5f, minimum.y, (minimum.z + maximum.z) * .5f);
		truckModel = glm::translate(glm::mat4(1), truckCenter) * rotation
			* glm::scale(glm::mat4(1), glm::vec3(scale)) * glm::translate(glm::mat4(1), -origin);
		truckScale = scale;
		initialTruckModel = truckModel;
		secondParkingCenter = glm::vec3(-truckCenter.x, 0, -truckCenter.z);
		ResetTruckMotion();

		glGenVertexArrays(1, &truckVao); glGenBuffers(1, &truckVbo);
		glBindVertexArray(truckVao); glBindBuffer(GL_ARRAY_BUFFER, truckVbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(1); glBindVertexArray(0);
		cout << "Truck size: " << truckSize.x << ", " << truckSize.y << ", " << truckSize.z << endl;
		return true;
	}
	catch (const exception& error) {
		cerr << error.what() << endl;
		return false;
	}
}

// 리셋 시 적재 이력과 대기 번호를 비우고 최초 다섯 상자를 복원한다.
void ResetCargo()
{
	storedCargo.clear();
	loadingNumber = -1; loadingProgress = 0; closeAfterLoading = false;
	for (int i = 0; i < BOX_COUNT; ++i)
		beltCargo[i] = { pathLength * i / BOX_COUNT, false, CargoState::Belt };
}

// 현재 트럭 위치를 기준으로 목적지를 계산하므로 나중에 트럭이 이동해도 적재함을 따라간다.
glm::mat4 LoadingTransform()
{
	glm::vec3 local = cargoSlots[storedCargo.size()];
	glm::vec3 target = glm::vec3(truckModel * glm::vec4(local, 1));
	// 열린 옆면 바깥에서 목표 높이/길이 위치로 맞춘 뒤 안으로 진입한다.
	glm::vec3 entry = glm::vec3(truckModel * glm::vec4(8.2f, local.y, local.z, 1));
	float t = loadingProgress;
	glm::vec3 position;
	if (t < .55f) {
		float u = t / .55f; u = u * u * (3 - 2 * u);
		position = glm::mix(loadingStart, entry, u);
	}
	else {
		float u = (t - .55f) / .45f; u = u * u * (3 - 2 * u);
		position = glm::mix(entry, target, u);
	}
	// 상자의 정면은 로컬 -Z. 최종적으로 트럭 로컬 +X(열리는 왼쪽 면)를 바라본다.
	float u = glm::min(t / .55f, 1.0f); u = u * u * (3 - 2 * u);
	glm::quat from = glm::angleAxis(-glm::half_pi<float>(), glm::vec3(0, 1, 0));
	glm::quat to = glm::angleAxis(glm::radians(trailerYaw - 90.0f), glm::vec3(0, 1, 0));
	return glm::translate(glm::mat4(1), position) * glm::mat4_cast(glm::slerp(from, to, u));
}

void UpdateCargo(float dt)
{
	// 이미 벨트를 떠난 상자는 이송을 마친다. 벨트 정지는 새로운 적재 시작만 막는다.
	if (loadingNumber >= 0) {
		loadingProgress = glm::min(loadingProgress + dt / CARGO_MOVE_SECONDS, 1.0f);
		if (loadingProgress >= 1) {
			storedCargo.push_back({ loadingNumber, cargoSlots[storedCargo.size()] });
			beltCargo[loadingNumber].state = CargoState::Loaded;
			beltCargo[loadingNumber].requested = false;
			loadingNumber = -1;
			if (closeAfterLoading) { truckOpen = false; closeAfterLoading = false; }
		}
	}
	float gap = BOX_SIZE + CARGO_BELT_GAP;
	if (!moving) return;

	// 선두부터 처리해 끝에서 대기하는 상자 뒤로 다른 상자가 겹치지 않도록 줄을 세운다.
	vector<int> order;
	for (int i = 0; i < BOX_COUNT; ++i)
		if (beltCargo[i].state == CargoState::Belt) order.push_back(i);
	sort(order.begin(), order.end(), [](int a, int b) { return beltCargo[a].distance > beltCargo[b].distance; });
	float limit = pathLength;
	for (int number : order) {
		BeltCargo& box = beltCargo[number];
		box.distance = glm::min(box.distance + BELT_SPEED * dt, limit);
		if (box.distance >= pathLength - .0001f) {
			if (box.requested) {
				if (loadingNumber < 0 && truckOpen && !closeAfterLoading
					&& truckOpenAngle >= TRUCK_OPEN_ANGLE - .001f && storedCargo.size() < cargoSlots.size()) {
					loadingNumber = number; loadingProgress = 0; box.state = CargoState::Loading;
					loadingStart = glm::vec3(BeltTransform(pathLength) * glm::vec4(0, STRIPE_HEIGHT + BOX_SIZE * .5f, 0, 1));
					continue;
				}
			}
			else {
				bool clear = true;
				for (int other : order)
					if (other != number && beltCargo[other].state == CargoState::Belt && beltCargo[other].distance < gap) clear = false;
				if (clear) { box.distance = 0; continue; }
			}
		}
		limit = glm::max(0.0f, box.distance - gap);
	}
	// 번호별 최초 간격에 해당하는 가상의 자리가 시작점을 통과할 때만 재생성한다.
	// 벨트 정지 중에는 기다리며, 시작점이 막혔으면 다음 순환에서 다시 시도한다.
	for (int i = 0; i < BOX_COUNT; ++i) {
		if (beltCargo[i].state != CargoState::WaitingSpawn) continue;
		float phase = glm::mod(beltDistance + pathLength * i / BOX_COUNT, pathLength);
		if (dt <= 0 || phase >= BELT_SPEED * dt) continue;
		bool clear = true;
		for (const BeltCargo& other : beltCargo)
			if (other.state == CargoState::Belt && glm::abs(other.distance - phase) < gap) clear = false;
		if (clear) {
			beltCargo[i].state = CargoState::Belt;
			beltCargo[i].distance = phase;
		}
	}

}

void DrawNumberedBox(const glm::mat4& model, int number)
{
	DrawBox(model, glm::vec3(BOX_SIZE), boxColors[number]);
	for (int mark = 0; mark <= number; ++mark) {
		float x = (mark - number * .5f) * (MARK_WIDTH + MARK_GAP);
		DrawBox(glm::translate(model, glm::vec3(x, (BOX_SIZE + MARK_HEIGHT) * .5f, 0)),
			glm::vec3(MARK_WIDTH, MARK_HEIGHT, MARK_LENGTH), glm::vec3(.10f));
	}
	// 앞면의 작은 검은 표시로 적재 후 바라보는 방향을 확인한다.
	DrawBox(glm::translate(model, glm::vec3(0, 0, -(BOX_SIZE + MARK_HEIGHT) * .5f)),
		glm::vec3(MARK_LENGTH, MARK_WIDTH, MARK_HEIGHT), glm::vec3(.10f));
}

// 벨트 위 또는 이송 중인 상자는 같은 번호의 적재된 상자보다 최근에 생성된 개체이다.
// 재생성 대기 중에는 아직 새 개체가 없으므로 가장 최근 적재 개체를 따라간다.
bool BoxViewTransform(int number, glm::mat4& model)
{
	if (beltCargo[number].state == CargoState::Belt) {
		model = glm::translate(BeltTransform(beltCargo[number].distance),
			glm::vec3(0, STRIPE_HEIGHT + BOX_SIZE * .5f, 0));
		return true;
	}
	if (loadingNumber == number) {
		model = LoadingTransform();
		return true;
	}
	for (auto it = storedCargo.rbegin(); it != storedCargo.rend(); ++it)
		if (it->number == number) {
			glm::vec3 center = glm::vec3(truckModel * glm::vec4(it->localCenter, 1));
			model = glm::rotate(glm::translate(glm::mat4(1), center),
				glm::radians(trailerYaw - 90.0f), glm::vec3(0, 1, 0));
			return true;
		}
	return false;
}

void DrawCargo()
{
	for (int i = 0; i < BOX_COUNT; ++i)
		if (beltCargo[i].state == CargoState::Belt)
			DrawNumberedBox(glm::translate(BeltTransform(beltCargo[i].distance),
				glm::vec3(0, STRIPE_HEIGHT + BOX_SIZE * .5f, 0)), i);
	for (const StoredCargo& box : storedCargo) {
		glm::vec3 center = glm::vec3(truckModel * glm::vec4(box.localCenter, 1));
		DrawNumberedBox(glm::rotate(glm::translate(glm::mat4(1), center),
			glm::radians(trailerYaw - 90.0f), glm::vec3(0, 1, 0)), box.number);
	}
	if (loadingNumber >= 0) DrawNumberedBox(LoadingTransform(), loadingNumber);
}

// 목표 각도까지 일정한 속도로 이동한다. 도중에 O를 누르면 현재 각도에서 방향을 바꾼다.
void UpdateTruckDoor(float dt)
{
	float previousAngle = truckOpenAngle;
	float target = truckOpen ? TRUCK_OPEN_ANGLE : 0.0f;
	float step = TRUCK_OPEN_SPEED * dt;
	truckOpenAngle += glm::clamp(target - truckOpenAngle, -step, step);
	if (!TruckInsideFloor()) truckOpenAngle = previousAngle;
}

void ResetTruckMotion()
{
	tractorYaw = trailerYaw = TRUCK_YAW;
	steeringAngle = 0;
	tractorRear = glm::vec3(initialTruckModel * glm::vec4(0, 0, TRACTOR_REAR_Z, 1));
	trailerRear = glm::vec3(initialTruckModel * glm::vec4(0, 0, TRAILER_AXLE_Z, 1));
	tractorModel = truckModel = initialTruckModel;
}

// 앞 차체는 조향각에 따라 주행하고, 뒤 차축은 연결점까지의 거리를 유지하며 끌려옵니다.
void AdvanceTruckMotion(float throttle, float steeringInput, float dt)
{
	while (dt > .000001f) {
		float step = glm::min(dt, TRUCK_PHYSICS_STEP);
		dt -= step;
		glm::vec3 oldTractorRear = tractorRear, oldTrailerRear = trailerRear;
		glm::mat4 oldTractorModel = tractorModel, oldTrailerModel = truckModel;
		float oldTractorYaw = tractorYaw, oldTrailerYaw = trailerYaw, oldSteer = steeringAngle;
		float target = steeringInput * TRUCK_STEER_LIMIT;
		float rate = steeringInput == 0 ? TRUCK_STEER_RETURN_SPEED : TRUCK_STEER_SPEED;
		steeringAngle += glm::clamp(target - steeringAngle, -rate * step, rate * step);
		float speed = throttle * TRUCK_DRIVE_SPEED;
		float angle = glm::radians(tractorYaw);
		float requestedTurn = speed * glm::tan(glm::radians(steeringAngle))
			/ ((TRACTOR_FRONT_Z - TRACTOR_REAR_Z) * truckScale);
		float length = (TRUCK_HITCH_Z - TRAILER_AXLE_Z) * truckScale;
		float hitchOffset = (TRUCK_HITCH_Z - TRACTOR_REAR_Z) * truckScale;
		float trailerAngle = glm::radians(trailerYaw);
		float joint = angle - trailerAngle;
		// 연결부가 뻑뻑할수록 상대 회전은 줄어들고 앞뒤 차체가 함께 도는 성향이 커집니다.
		// 뒤 차축의 횡방향 속도 0 조건을 유지하면서 앞 차체의 실제 회전율도 함께 계산합니다.
		auto rates = [&](float relative, float& frontRate, float& backRate) {
			float denominator = length - hitchOffset * glm::cos(relative);
			float lockedRate = speed * glm::sin(relative) / denominator;
			frontRate = lockedRate + TRUCK_JOINT_MOBILITY * (requestedTurn - lockedRate);
			backRate = (speed * glm::sin(relative) + hitchOffset * frontRate * glm::cos(relative)) / length;
		};
		float turn, trailerTurn;
		rates(joint, turn, trailerTurn);
		float halfJoint = joint + (turn - trailerTurn) * step * .5f;
		rates(halfJoint, turn, trailerTurn);
		float limit = glm::radians(TRUCK_ARTICULATION_LIMIT);
		float jointRate = glm::clamp(turn - trailerTurn, (-limit - joint) / step, (limit - joint) / step);
		// 최대 연결각에서는 상대 회전만 멈춥니다. 전후진과 차량 전체의 곡선 주행은 계속됩니다.
		turn = (speed * glm::sin(halfJoint) + length * jointRate)
			/ (length - hitchOffset * glm::cos(halfJoint));
		trailerTurn = turn - jointRate;
		float middle = angle + turn * step * .5f;
		tractorRear += glm::vec3(glm::sin(middle), 0, glm::cos(middle)) * speed * step;
		tractorYaw += glm::degrees(turn * step);
		tractorModel = glm::translate(glm::mat4(1), tractorRear)
			* glm::rotate(glm::mat4(1), glm::radians(tractorYaw), glm::vec3(0, 1, 0))
			* glm::scale(glm::mat4(1), glm::vec3(truckScale))
			* glm::translate(glm::mat4(1), glm::vec3(0, 0, -TRACTOR_REAR_Z));
		if (speed != 0) {
			glm::vec3 hitch = glm::vec3(tractorModel * glm::vec4(0, 0, TRUCK_HITCH_Z, 1));
			trailerAngle += trailerTurn * step;
			trailerYaw = glm::degrees(trailerAngle);
			trailerRear = hitch - glm::vec3(glm::sin(trailerAngle), 0, glm::cos(trailerAngle)) * length;
			truckModel = glm::translate(glm::mat4(1), hitch)
				* glm::rotate(glm::mat4(1), glm::radians(trailerYaw), glm::vec3(0, 1, 0))
				* glm::scale(glm::mat4(1), glm::vec3(truckScale))
				* glm::translate(glm::mat4(1), glm::vec3(0, 0, -TRUCK_HITCH_Z));
		}
		// 바닥 경계만 이동을 막습니다. 연결각 제한은 위의 회전율 계산에서 처리합니다.
		if (!TruckInsideFloor()) {
			tractorRear = oldTractorRear; trailerRear = oldTrailerRear;
			tractorYaw = oldTractorYaw; trailerYaw = oldTrailerYaw;
			tractorModel = oldTractorModel; truckModel = oldTrailerModel;
			if (!TruckInsideFloor()) steeringAngle = oldSteer;
		}
	}
}

glm::mat4 TruckPartTransform(const TruckPart& part)
{
	glm::mat4 model = part.tractor ? tractorModel : (part.wing ? TruckWingTransform() : truckModel);
	if (part.frontWheel >= 0) {
		glm::vec3 pivot(part.frontWheel == 1 ? FRONT_WHEEL_X : -FRONT_WHEEL_X, FRONT_WHEEL_Y, TRACTOR_FRONT_Z);
		model = model * glm::translate(glm::mat4(1), pivot)
			* glm::rotate(glm::mat4(1), glm::radians(steeringAngle), glm::vec3(0, 1, 0))
			* glm::translate(glm::mat4(1), -pivot);
	}
	return model;
}

// 중심점뿐 아니라 각 부품의 회전된 경계 상자 전체를 바닥 안에 유지합니다.
bool TruckInsideFloor()
{
	for (const TruckPart& part : truckParts) {
		glm::mat4 model = TruckPartTransform(part);
		for (int corner = 0; corner < 8; ++corner) {
			glm::vec3 local((corner & 1) ? part.maximum.x : part.minimum.x,
				(corner & 2) ? part.maximum.y : part.minimum.y,
				(corner & 4) ? part.maximum.z : part.minimum.z);
			glm::vec3 world = glm::vec3(model * glm::vec4(local, 1));
			if (glm::abs(world.x) > FLOOR_WIDTH * .5f - TRUCK_BOUNDARY_MARGIN
				|| glm::abs(world.z) > FLOOR_DEPTH * .5f - TRUCK_BOUNDARY_MARGIN) return false;
		}
	}
	return true;
}

void UpdateTruckMotion(GLFWwindow* window, float dt)
{
	float throttle = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ? 1.0f : 0.0f)
		- (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ? 1.0f : 0.0f);
	float steer = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ? 1.0f : 0.0f)
		- (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ? 1.0f : 0.0f);
	AdvanceTruckMotion(throttle, steer, dt);
}

// 지붕 반대쪽 모서리의 길이 방향 축(Z)을 중심으로 옆판과 지붕에 같은 회전을 적용한다.
glm::mat4 TruckWingTransform()
{
	glm::vec3 hinge(TRUCK_HINGE_X, TRUCK_HINGE_Y, 0);
	return truckModel * glm::translate(glm::mat4(1), hinge)
		* glm::rotate(glm::mat4(1), glm::radians(truckOpenAngle), glm::vec3(0, 0, 1))
		* glm::translate(glm::mat4(1), -hinge);
}

// 바퀴 최하단은 Y=0에 놓고 주차선은 바닥보다 살짝 높여 겹침을 방지한다.
void DrawTruck()
{
	glm::vec3 parkingSize = truckSize + glm::vec3(PARKING_MARGIN * 2, 0, PARKING_MARGIN * 2);
	for (glm::vec3 parkingCenter : { truckCenter, secondParkingCenter }) {
	for (int side = -1; side <= 1; side += 2) {
		glm::vec3 longLine = parkingCenter + glm::vec3(0, PARKING_LINE_HEIGHT * .5f, side * parkingSize.z * .5f);
		glm::vec3 endLine = parkingCenter + glm::vec3(side * parkingSize.x * .5f, PARKING_LINE_HEIGHT * .5f, 0);
		DrawBox(glm::translate(glm::mat4(1), longLine),
			glm::vec3(parkingSize.x + PARKING_LINE_WIDTH, PARKING_LINE_HEIGHT, PARKING_LINE_WIDTH), glm::vec3(1));
		DrawBox(glm::translate(glm::mat4(1), endLine),
			glm::vec3(PARKING_LINE_WIDTH, PARKING_LINE_HEIGHT, parkingSize.z + PARKING_LINE_WIDTH), glm::vec3(1));
	}
	}
	// 원본 하부 프레임 위에 얇은 적재 바닥을 놓아 내부가 열린 상태에서도 바닥이 보인다.
	DrawBox(glm::translate(truckModel, glm::vec3(.161608f, 5.40f, -36.30f)),
		glm::vec3(11.75f, .16f, 41.5f), glm::vec3(.30f));
	glBindVertexArray(truckVao);
	for (const TruckPart& part : truckParts) {
		glm::mat4 model = TruckPartTransform(part);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(colorLocation, 1, glm::value_ptr(part.color));
		glDrawArrays(GL_TRIANGLES, part.first, part.count);
	}
	glBindVertexArray(0);
}

glm::mat4 BeltTransform(float distance)
{
	float arcLength = BELT_CORNER_RADIUS * glm::half_pi<float>();
	glm::vec3 position = glm::vec3(BELT_START_X, BELT_HEIGHT, BELT_START_Z) + sceneOffset;
	float angle = 0;
	if (distance <= BELT_FIRST_LENGTH)position.z -= distance;
	else if (distance < BELT_FIRST_LENGTH + arcLength) {
		angle = (distance - BELT_FIRST_LENGTH) / BELT_CORNER_RADIUS;
		position.x += BELT_CORNER_RADIUS * (1 - glm::cos(angle));
		position.z -= BELT_FIRST_LENGTH + BELT_CORNER_RADIUS * glm::sin(angle);
	}
	else {
		angle = glm::half_pi<float>();
		position.x += BELT_CORNER_RADIUS + distance - BELT_FIRST_LENGTH - arcLength;
		position.z -= BELT_FIRST_LENGTH + BELT_CORNER_RADIUS;
	}
	return glm::rotate(glm::translate(glm::mat4(1), position), -angle, glm::vec3(0, 1, 0));
}

void DrawBox(const glm::mat4& model, glm::vec3 size, glm::vec3 color)
{
	glm::mat4 transform = glm::scale(model, size);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(transform));
	glUniform3fv(colorLocation, 1, glm::value_ptr(color));
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 0, 36); glBindVertexArray(0);
}

void DrawScene()
{
	glClearColor(.70f, .82f, .94f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glUseProgram(programID);
	glm::vec3 eye = cameraPosition, target = cameraTarget;
	if (truckView) {
		glm::vec3 front = glm::vec3(tractorModel * glm::vec4(0, 0, TRACTOR_FRONT_Z, 1));
		glm::vec3 rear = glm::vec3(truckModel * glm::vec4(0, 0, CARGO_MIN_Z, 1));
		target = (front + rear) * .5f + glm::vec3(0, TRUCK_HEIGHT * .35f, 0);
		glm::mat4 heading = glm::rotate(glm::mat4(1), glm::radians(tractorYaw), glm::vec3(0, 1, 0));
		// 트럭 뒤쪽 위에서 전진 방향을 바라보며, 앞 차체의 방향을 따라갑니다.
		eye = target + glm::vec3(heading * glm::vec4(truckViewOffset, 0));
	}
	if (firstPerson) {
		glm::mat4 box(1);
		if (BoxViewTransform(viewBoxNumber, box)) {
			eye = glm::vec3(box * glm::vec4(0, BOX_EYE_HEIGHT, -BOX_EYE_FORWARD, 1));
			glm::vec3 forward = glm::normalize(glm::vec3(box * glm::vec4(0, 0, -1, 0)));
			target = eye + forward;
		}
	}
	glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0, 1, 0));
	glm::mat4 projection = (truckView || firstPerson || perspective) ? glm::perspective(glm::radians(PERSPECTIVE_FOVY), PERSPECTIVE_ASPECT, PERSPECTIVE_NEAR, PERSPECTIVE_FAR)
		: glm::ortho(-viewRange * PERSPECTIVE_ASPECT, viewRange * PERSPECTIVE_ASPECT, -viewRange, viewRange, PERSPECTIVE_NEAR, PERSPECTIVE_FAR);
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view)); glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
	glUniform3fv(eyeLocation, 1, glm::value_ptr(eye));
	glm::vec3 cargoLights[3];
	for (int i = 0; i < 3; ++i)
		cargoLights[i] = glm::vec3(TruckWingTransform() * glm::vec4(.16f, 16.15f,
			glm::mix(CARGO_MIN_Z, CARGO_MAX_Z, (i + .5f) / 3.0f), 1));
	glUniform3fv(cargoLightsLocation, 3, glm::value_ptr(cargoLights[0]));
	// 트럭을 이동·회전해도 내부 판정이 따라가도록 매 프레임 갱신한다.
	glm::mat4 worldToTruck = glm::inverse(truckModel);
	glUniformMatrix4fv(worldToTruckLocation, 1, GL_FALSE, glm::value_ptr(worldToTruck));
	DrawBox(glm::translate(glm::mat4(1), glm::vec3(0, -FLOOR_HEIGHT * .5f, 0)), glm::vec3(FLOOR_WIDTH, FLOOR_HEIGHT, FLOOR_DEPTH), glm::vec3(.32f, .34f, .32f));
	DrawTruck();
	glm::mat4 identity(1); glm::vec3 beltColor(.22f, .24f, .27f);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(identity)); glUniform3fv(colorLocation, 1, glm::value_ptr(beltColor));
	glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES, 36, surfaceCount); glBindVertexArray(0);
	for (int i = 0; i < BELT_SEGMENTS; ++i) {
		float d0 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * i / BELT_SEGMENTS;
		float d1 = -BELT_END_MARGIN + (pathLength + 2 * BELT_END_MARGIN) * (i + 1) / BELT_SEGMENTS;
		for (int side = -1; side <= 1; side += 2) {
			glm::vec3 a = glm::vec3(BeltTransform(d0) * glm::vec4(side * (BELT_WIDTH + RAIL_WIDTH) * .5f, RAIL_HEIGHT * .5f, 0, 1));
			glm::vec3 b = glm::vec3(BeltTransform(d1) * glm::vec4(side * (BELT_WIDTH + RAIL_WIDTH) * .5f, RAIL_HEIGHT * .5f, 0, 1));
			glm::vec3 delta = b - a;
			glm::mat4 rail = glm::rotate(glm::translate(glm::mat4(1), (a + b) * .5f), glm::atan(-delta.x, -delta.z), glm::vec3(0, 1, 0));
			DrawBox(rail, glm::vec3(RAIL_WIDTH, RAIL_HEIGHT, glm::length(delta) + .002f), glm::vec3(.85f, .70f, .12f));
		}
	}
	for (int i = 0; i < LEG_PAIRS; ++i) {
		glm::mat4 frame = BeltTransform(pathLength * i / (LEG_PAIRS - 1));
		for (int side = -1; side <= 1; side += 2)
			DrawBox(glm::translate(frame, glm::vec3(side * (BELT_WIDTH - LEG_WIDTH) * .5f, -BELT_HEIGHT * .5f, 0)), glm::vec3(LEG_WIDTH, BELT_HEIGHT, LEG_WIDTH), glm::vec3(.15f));
	}
	for (int i = 0; i < STRIPE_COUNT; ++i) {
		float distance = glm::mod(beltDistance + pathLength * i / STRIPE_COUNT, pathLength);
		DrawBox(glm::translate(BeltTransform(distance), glm::vec3(0, STRIPE_HEIGHT * .5f, 0)), glm::vec3(BELT_WIDTH * .97f, STRIPE_HEIGHT, STRIPE_LENGTH), glm::vec3(.50f, .52f, .55f));
	}
	DrawCargo();
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int mods)
{
	if (action != GLFW_PRESS && action != GLFW_REPEAT)return;
	if (key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL)ScrollCallback(window, 0, 1);
	if (key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS)ScrollCallback(window, 0, -1);
	if (action != GLFW_PRESS)return;
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)glfwSetWindowShouldClose(window, true);
	if (key == GLFW_KEY_V) { firstPerson = !firstPerson; truckView = false; dragging = false; }
	if (key == GLFW_KEY_T) { truckView = !truckView; firstPerson = false; dragging = false; }
	if (key == GLFW_KEY_N) {
		int direction = (mods & GLFW_MOD_SHIFT) ? -1 : 1;
		viewBoxNumber = (viewBoxNumber + direction + BOX_COUNT) % BOX_COUNT;
		cout << "View box: " << viewBoxNumber + 1 << endl;
	}
	if (key == GLFW_KEY_B)moving = !moving;
	if (key == GLFW_KEY_O) {
		if (loadingNumber >= 0) closeAfterLoading = !closeAfterLoading;
		else truckOpen = !truckOpen;
	}
	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5) {
		int number = key - GLFW_KEY_1;
		BeltCargo& box = beltCargo[number];
		if (box.state == CargoState::Loaded) {
			box.state = CargoState::WaitingSpawn;
			box.requested = false;
		}
		else if (box.state == CargoState::Belt && truckOpen && truckOpenAngle > .01f && !closeAfterLoading) {
			// 대기 중 같은 번호를 반복 입력해도 중복 등록하지 않는다.
			box.requested = true;
		}
	}
	if (key == GLFW_KEY_P)perspective = (mods & GLFW_MOD_SHIFT) == 0;
	if (key == GLFW_KEY_R) {
		ResetTruckMotion();
		firstPerson = false; truckView = false; viewBoxNumber = 0;
		truckViewOffset = glm::vec3(0, TRUCK_VIEW_HEIGHT, -TRUCK_VIEW_FORWARD);
		truckOpen = false; truckOpenAngle = 0;
		ResetCargo();
		beltDistance = 0; moving = false; perspective = true; dragging = false; viewRange = VIEW_RANGE;
		cameraPosition = glm::vec3(CAMERA_X, CAMERA_Y, CAMERA_Z) + sceneOffset; cameraTarget = glm::vec3(CAMERA_TARGET_X, CAMERA_TARGET_Y, CAMERA_TARGET_Z) + sceneOffset;
	}
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
	if (firstPerson) return;
	if (truckView) {
		float radius = glm::clamp(glm::length(truckViewOffset) - (float)yoffset * ZOOM_STEP, CAMERA_RADIUS_MIN, CAMERA_RADIUS_MAX);
		truckViewOffset = glm::normalize(truckViewOffset) * radius;
		return;
	}
	if (perspective) {
		glm::vec3 offset = cameraPosition - cameraTarget;
		float radius = glm::clamp(glm::length(offset) - (float)yoffset * ZOOM_STEP, CAMERA_RADIUS_MIN, CAMERA_RADIUS_MAX);
		cameraPosition = cameraTarget + glm::normalize(offset) * radius;
	}
	else viewRange = glm::clamp(viewRange - (float)yoffset * ZOOM_STEP, VIEW_RANGE_MIN, VIEW_RANGE_MAX);
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (firstPerson || button != GLFW_MOUSE_BUTTON_LEFT)return;
	dragging = action == GLFW_PRESS;
	if (dragging)glfwGetCursorPos(window, &previousMouseX, &previousMouseY);
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (firstPerson || !dragging)return;
	int width, height; glfwGetWindowSize(window, &width, &height); if (width <= 0 || height <= 0)return;
	// 트럭 시점에서는 차량 기준의 orbit 오프셋만 변경하여 주행 중에도 시점을 유지합니다.
	glm::vec3 offset = truckView ? truckViewOffset : cameraPosition - cameraTarget;
	float radius = glm::length(offset);
	float yaw = glm::atan(offset.x, offset.z), pitch = glm::asin(glm::clamp(offset.y / radius, -1.0f, 1.0f));
	yaw -= glm::radians(ORBIT_DEGREES) * (float)(x - previousMouseX) / width;
	pitch += glm::radians(ORBIT_DEGREES) * (float)(y - previousMouseY) / height;
	previousMouseX = x; previousMouseY = y;
	pitch = glm::clamp(pitch, -glm::radians(ORBIT_PITCH_LIMIT), glm::radians(ORBIT_PITCH_LIMIT));
	offset = radius * glm::vec3(glm::cos(pitch) * glm::sin(yaw), glm::sin(pitch), glm::cos(pitch) * glm::cos(yaw));
	if (truckView) truckViewOffset = offset;
	else cameraPosition = cameraTarget + offset;
}


string ReadFile(const char* name)
{
	ifstream file(name);
	if (!file) { cerr << "Cannot open shader: " << name << endl; return ""; }
	stringstream stream; stream << file.rdbuf(); return stream.str();
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
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0 && colorLocation >= 0;
}





