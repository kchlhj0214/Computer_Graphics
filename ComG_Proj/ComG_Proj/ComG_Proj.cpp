#define NOMINMAX
#include <Windows.h>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <GL/glm/glm.hpp>
#include <GL/glm/ext.hpp>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
using namespace std;

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define ARENA_RADIUS 17.1f
#define ARENA_SEGMENTS 128
#define PLAYER_WIDTH 0.8f
#define PLAYER_HEIGHT 1.6f
#define PLAYER_DEPTH 0.8f
#define CHARACTER_SPEED_MULTIPLIER 2.0f
#define PLAYER_SPEED (22.0f / 3.6f / 3.0f * CHARACTER_SPEED_MULTIPLIER)
#define GUARD_SPEED (14.0f / 3.6f / 3.0f * CHARACTER_SPEED_MULTIPLIER)
#define EYE_HEIGHT 1.45f
#define CAMERA_DISTANCE 5.2f
#define CAMERA_ELEVATION 27.0f
#define MAX_MOUSE_SENSITIVITY 0.003f
#define ROUND_COUNT 3
#define BRIDGE_WIDTH (5.0f / 3.0f)
#define BRIDGE_OUTSIDE (7.0f / 3.0f)
#define BRIDGE_INSIDE (5.0f / 3.0f)
#define SPAWN_INTERVAL 0.24f
#define ENTRY_SPEED (6.0f / 3.0f * CHARACTER_SPEED_MULTIPLIER)
#define ENEMY_SPEED (10.0f / 3.6f / 3.0f * CHARACTER_SPEED_MULTIPLIER)
#define ENEMY_SPACING (PLAYER_DEPTH + 0.7f / 3.0f)
#define ENEMY_HEALTH 60
#define PLAYER_HEALTH 100
#define MAX_ENERGY 40
#define MELEE_REACH (3.4f / 3.0f + PLAYER_WIDTH * .5f)
#define RANGED_REACH (15.0f / 3.0f)
#define PARRY_RADIUS (4.6f / 3.0f)
#define REACH_TOLERANCE .001f
#define BULLET_RADIUS (1.4f / 6.0f)
#define BULLET_SPEED (8.0f / 3.0f)
#define BEAM_DURATION 1.5f

enum class Screen { Start, Playing, Paused, Settings, Cleared, Defeated };
enum class Surface : GLint { Flat, Lit, Arena };
enum class EnemyType { Melee, Ranged };
enum class EnemyState { Entering, Waiting, Tracking, Searching, Wandering, Windup, Recovering };

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
};

struct Mesh
{
	GLint first;
	GLsizei count;
};

struct UiRectangle
{
	float x, y, width, height;
};

struct MenuLayout
{
	float scale;
	UiRectangle panel;
	UiRectangle startButton;
	UiRectangle settingsButton;
	UiRectangle sensitivitySlider;
	UiRectangle backButton;
};

struct Enemy
{
	EnemyType type;
	glm::vec3 position;
	glm::vec3 entryDirection;
	glm::vec3 facing{0, 0, -1};
	float joinRadius = ARENA_RADIUS - BRIDGE_INSIDE;
	glm::vec3 lastSeen{0};
	glm::vec3 wanderTarget{0};
	EnemyState state = EnemyState::Entering;
	float health = ENEMY_HEALTH;
	float actionAt = 0;
};

struct Bullet
{
	glm::vec3 position;
	glm::vec3 direction;
};

struct Beam
{
	glm::vec3 direction{0, 0, -1};
	float startedAt = 0;
	float expiresAt = 0;
	float width = 0;
	float damage = 0;
	int nextHit = 0;
};

const glm::vec3 worldUp(0, 1, 0);
const glm::vec3 entries[] = {{0, 0, -1}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}};
Screen screen = Screen::Start;
Screen settingsReturn = Screen::Start;
int sensitivitySliderPercent = 50;
bool draggingSensitivity = false;
glm::vec3 playerPosition(0);
float playerYaw = 0, firstPersonPitch = 0;
float orbitYaw = 0, orbitElevation = 0;
bool firstPerson = true, mousePositionReady = false;
double previousMouseX = 0, previousMouseY = 0;
int playerHealth = PLAYER_HEALTH, energy = 0;
int currentRound = 1, spawnedEnemies = 0;
float gameTime = 0, nextSpawnAt = 0;
float moveUnlockAt = 0, defenseUnlockAt = 0, parryUntil = 0;
float parryInvulnerableUntil = 0, hitInvulnerableUntil = 0;
bool parrySucceeded = false, parryStunned = false, blocking = false;
vector<Enemy> enemies;
vector<Bullet> bullets;
Beam beam;
mt19937 wanderRandom(0);

GLuint programID = 0, vao = 0, vbo = 0;
GLint modelLocation, viewLocation, projectionLocation, colorLocation, surfaceLocation;
Mesh playerMesh{}, floorMesh{}, rectangleMesh{};

string ReadFile(const filesystem::path& path);
bool InitShader();
void InitGeometry();
void UpdatePlayer(GLFWwindow* window, float dt);
void UpdateRound(GLFWwindow* window, float dt);
void SetScreen(GLFWwindow* window, Screen next);
void DrawScene(GLFWwindow* window, int width, int height);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseCallback(GLFWwindow* window, int button, int action, int mods);
void CursorCallback(GLFWwindow* window, double x, double y);
void FocusCallback(GLFWwindow* window, int focused);

int main()
{
	if (!glfwInit()) return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Patience Has A Capacity", nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK || !InitShader()) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	glfwSwapInterval(1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, CursorCallback);
	glfwSetWindowFocusCallback(window, FocusCallback);
	InitGeometry();

	double previousTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		double now = glfwGetTime();
		float dt = (float)glm::min(now - previousTime, 0.1);
		previousTime = now;

		if (screen == Screen::Playing) UpdateRound(window, dt);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		if (width > 0 && height > 0) {
			glViewport(0, 0, width, height);
			DrawScene(window, width, height);
			glfwSwapBuffers(window);
		}
		else {
			glfwWaitEventsTimeout(0.1);
		}
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(programID);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void InitGeometry()
{
	const glm::vec3 corners[] = {
		{-.5f, -.5f, -.5f}, {.5f, -.5f, -.5f}, {.5f, .5f, -.5f}, {-.5f, .5f, -.5f},
		{-.5f, -.5f, .5f}, {.5f, -.5f, .5f}, {.5f, .5f, .5f}, {-.5f, .5f, .5f}
	};
	const int indices[] = {
		0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 4, 7, 0, 7, 3,
		1, 2, 6, 1, 6, 5, 3, 7, 6, 3, 6, 2, 0, 1, 5, 0, 5, 4
	};
	const glm::vec3 normals[] = {
		{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}
	};
	vector<Vertex> vertices;
	playerMesh = {0, 36};
	for (int i = 0; i < playerMesh.count; ++i) {
		vertices.push_back({corners[indices[i]], normals[i / 6]});
	}

	floorMesh = {(GLint)vertices.size(), ARENA_SEGMENTS * 3};
	for (int i = 0; i < ARENA_SEGMENTS; ++i) {
		float a = glm::two_pi<float>() * i / ARENA_SEGMENTS;
		float b = glm::two_pi<float>() * (i + 1) / ARENA_SEGMENTS;
		vertices.push_back({glm::vec3(0), worldUp});
		vertices.push_back({glm::vec3(glm::cos(a), 0, -glm::sin(a)), worldUp});
		vertices.push_back({glm::vec3(glm::cos(b), 0, -glm::sin(b)), worldUp});
	}

	rectangleMesh = {(GLint)vertices.size(), 6};
	const glm::vec3 rectangle[] = {
		{-.5f, -.5f, 0}, {.5f, -.5f, 0}, {.5f, .5f, 0},
		{-.5f, -.5f, 0}, {.5f, .5f, 0}, {-.5f, .5f, 0}
	};
	for (glm::vec3 position : rectangle) {
		vertices.push_back({position, glm::vec3(0, 0, 1)});
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

glm::vec3 ForwardDirection(float yaw)
{
	return glm::vec3(glm::sin(yaw), 0, -glm::cos(yaw));
}

void UpdatePlayer(GLFWwindow* window, float dt)
{
	glm::vec3 forward = ForwardDirection(playerYaw);
	glm::vec3 right = glm::cross(forward, worldUp);
	glm::vec3 movement(0);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += forward;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= forward;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += right;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= right;
	if (movement == glm::vec3(0)) return;

	float speed = blocking ? GUARD_SPEED : PLAYER_SPEED;
	playerPosition += glm::normalize(movement) * speed * dt;
	// 캐릭터가 어느 방향을 바라봐도 발밑 영역 전체가 경기장 안에 있도록 여유를 둔다.
	float limit = ARENA_RADIUS - glm::length(glm::vec2(PLAYER_WIDTH, PLAYER_DEPTH)) * .5f;
	float distance = glm::length(playerPosition);
	if (distance > limit) {
		playerPosition *= limit / distance;
	}
}

int RoundEnemyCount()
{
	return (currentRound + 1) * 4;
}

void StartRound(int round)
{
	currentRound = round;
	spawnedEnemies = 0;
	nextSpawnAt = gameTime + .45f;
	enemies.clear();
	bullets.clear();
	beam = {};
}

void StartGame(int round)
{
	playerPosition = glm::vec3(0);
	playerHealth = PLAYER_HEALTH;
	energy = 0;
	gameTime = 0;
	moveUnlockAt = defenseUnlockAt = parryUntil = 0;
	parryInvulnerableUntil = hitInvulnerableUntil = 0;
	parrySucceeded = parryStunned = blocking = false;
	wanderRandom.seed(0);
	StartRound(round);
}

void SpawnEnemies()
{
	while (spawnedEnemies < RoundEnemyCount() && gameTime >= nextSpawnAt) {
		glm::vec3 outward = entries[spawnedEnemies % 4];
		int entrySlot = spawnedEnemies / 4;
		EnemyType type = entrySlot == 1 ? EnemyType::Ranged : EnemyType::Melee;
		Enemy enemy{type, outward * (ARENA_RADIUS + BRIDGE_OUTSIDE - PLAYER_DEPTH * .5f), outward, -outward};
		// 먼저 도착한 적은 경기장 안쪽까지 이동해 다음 적이 다리를 건널 공간을 남긴다.
		enemy.joinRadius -= (currentRound - entrySlot) * ENEMY_SPACING;
		enemies.push_back(enemy);
		++spawnedEnemies;
		nextSpawnAt += SPAWN_INTERVAL;
	}
}

bool MoveToward(glm::vec3& position, glm::vec3 target, float step)
{
	glm::vec3 offset = target - position;
	float distance = glm::length(offset);
	if (distance <= step) {
		position = target;
		return true;
	}
	position += offset * (step / distance);
	return false;
}

bool Faces(glm::vec3 forward, glm::vec3 toward, float cosine)
{
	return glm::dot(forward, toward) >= glm::length(toward) * cosine;
}

bool TryParry(glm::vec3 toward, Enemy* attacker)
{
	if (gameTime >= parryUntil || gameTime < hitInvulnerableUntil) return false;
	if (!Faces(ForwardDirection(playerYaw), toward, glm::cos(glm::quarter_pi<float>()))) return false;

	energy = glm::min(MAX_ENERGY, energy + 10);
	if (!parrySucceeded) {
		parrySucceeded = true;
		parryInvulnerableUntil = gameTime + .5f;
	}
	if (attacker && !parryStunned) {
		parryStunned = true;
		attacker->actionAt = gameTime + 1.5f;
	}
	return true;
}

void ResolveHit(glm::vec3 toward, bool moving)
{
	if (gameTime < glm::max(parryInvulnerableUntil, hitInvulnerableUntil)) return;
	float guardCosine = moving ? 0.0f : glm::cos(glm::quarter_pi<float>());
	if (blocking && Faces(ForwardDirection(playerYaw), toward, guardCosine)) return;

	playerHealth = glm::max(0, playerHealth - 10);
	hitInvulnerableUntil = gameTime + .5f;
}

void ChooseWanderTarget(Enemy& enemy)
{
	uniform_real_distribution<float> unit(0, 1);
	for (int attempt = 0; attempt < 8; ++attempt) {
		glm::vec3 direction = ForwardDirection(unit(wanderRandom) * glm::two_pi<float>());
		glm::vec3 target = enemy.lastSeen + direction * (glm::sqrt(unit(wanderRandom)) * (5.0f / 3.0f));
		if (glm::length(target) <= ARENA_RADIUS - PLAYER_WIDTH) {
			enemy.wanderTarget = target;
			return;
		}
	}
	enemy.wanderTarget = enemy.position;
	enemy.actionAt = gameTime + .25f;
}

void UpdateEnemy(Enemy& enemy, float dt, bool moving)
{
	if (enemy.health <= 0) return;
	if (enemy.state == EnemyState::Entering) {
		glm::vec3 join = enemy.entryDirection * enemy.joinRadius;
		if (MoveToward(enemy.position, join, ENTRY_SPEED * dt)) {
			enemy.state = EnemyState::Waiting;
		}
		return;
	}

	glm::vec3 towardPlayer = playerPosition - enemy.position;
	float distance = glm::length(towardPlayer);
	if (enemy.state == EnemyState::Windup) {
		if (gameTime < enemy.actionAt) return;
		enemy.state = EnemyState::Recovering;
		enemy.actionAt = gameTime + .8f;
		if (distance <= MELEE_REACH + REACH_TOLERANCE
			&& Faces(enemy.facing, towardPlayer, glm::cos(glm::quarter_pi<float>()))) {
			glm::vec3 towardEnemy = distance > 0 ? -towardPlayer : -enemy.facing;
			if (distance > PARRY_RADIUS + REACH_TOLERANCE || !TryParry(towardEnemy, &enemy)) {
				ResolveHit(towardEnemy, moving);
			}
		}
		return;
	}
	if (enemy.state == EnemyState::Recovering) {
		if (gameTime < enemy.actionAt) return;
		enemy.state = EnemyState::Tracking;
	}

	bool melee = enemy.type == EnemyType::Melee;
	float detection = (melee ? 20.0f : 24.0f) / 3.0f;
	if (distance <= detection) {
		enemy.lastSeen = playerPosition;
		enemy.state = EnemyState::Tracking;
		if (distance > 0) enemy.facing = towardPlayer / distance;
		float reach = melee ? MELEE_REACH : RANGED_REACH;
		if (distance > reach + REACH_TOLERANCE) {
			MoveToward(enemy.position, playerPosition, glm::min(ENEMY_SPEED * dt, distance - reach));
		}
		else if (melee) {
			enemy.state = EnemyState::Windup;
			enemy.actionAt = gameTime + .6f;
		}
		else if (gameTime >= enemy.actionAt) {
			bullets.push_back({enemy.position, enemy.facing});
			enemy.actionAt = gameTime + 3.0f;
		}
		return;
	}

	if (enemy.state == EnemyState::Tracking) enemy.state = EnemyState::Searching;
	if (enemy.state == EnemyState::Searching) {
		if (MoveToward(enemy.position, enemy.lastSeen, ENEMY_SPEED * dt)) {
			enemy.state = EnemyState::Wandering;
			enemy.actionAt = gameTime;
			ChooseWanderTarget(enemy);
		}
	}
	else if (enemy.state == EnemyState::Wandering && gameTime >= enemy.actionAt) {
		if (MoveToward(enemy.position, enemy.wanderTarget, ENEMY_SPEED * .6f * dt)) {
			ChooseWanderTarget(enemy);
		}
	}
}

float BeamLength()
{
	float along = glm::dot(playerPosition, beam.direction);
	float remaining = ARENA_RADIUS * ARENA_RADIUS - glm::dot(playerPosition, playerPosition);
	return glm::sqrt(along * along + remaining) - along;
}

float BeamWidth(float distance)
{
	return beam.width * glm::mix(.35f, 1.0f, glm::clamp(distance / .8f, 0.0f, 1.0f));
}

bool BeamContains(glm::vec3 position, float radius)
{
	glm::vec3 offset = position - playerPosition;
	float along = glm::dot(offset, beam.direction);
	float across = glm::abs(glm::dot(offset, glm::cross(beam.direction, worldUp)));
	return along >= -radius && along <= BeamLength() + radius
		&& across <= BeamWidth(along) * .5f + radius;
}

void DamageEnemiesInBeam()
{
	for (Enemy& enemy : enemies) {
		if (enemy.health > 0 && BeamContains(enemy.position, PLAYER_WIDTH * .5f)) {
			enemy.health -= beam.damage;
			if (enemy.health <= 0) {
				playerHealth = glm::min(PLAYER_HEALTH, playerHealth + 5);
			}
		}
	}
}

void StartBeam()
{
	if (energy == 0 || gameTime < defenseUnlockAt || gameTime < beam.expiresAt || blocking) return;

	beam = {ForwardDirection(playerYaw), gameTime, gameTime + BEAM_DURATION,
		energy >= 30 ? 1.4f : .7f, energy * .5f, 1};
	energy = 0;
	DamageEnemiesInBeam();
}

void UpdateBeam()
{
	while (beam.nextHit < 3 && gameTime < beam.expiresAt
		&& gameTime >= beam.startedAt + beam.nextHit * .5f) {
		DamageEnemiesInBeam();
		++beam.nextHit;
	}
}

float DistanceToPath(glm::vec3 point, glm::vec3 start, glm::vec3 direction, float length)
{
	float along = glm::clamp(glm::dot(point - start, direction), 0.0f, length);
	return glm::length(point - (start + direction * along));
}

void UpdateBullets(float dt, bool moving)
{
	float step = BULLET_SPEED * dt;
	erase_if(bullets, [&](Bullet& bullet) {
		glm::vec3 end = bullet.position + bullet.direction * step;
		if (gameTime < beam.expiresAt) {
			if (BeamContains(bullet.position, BULLET_RADIUS) || BeamContains(end, BULLET_RADIUS)) return true;
		}

		float closest = DistanceToPath(playerPosition, bullet.position, bullet.direction, step);
		if (closest <= PARRY_RADIUS + BULLET_RADIUS && TryParry(-bullet.direction, nullptr)) return true;
		if (closest <= PLAYER_WIDTH * .5f + BULLET_RADIUS) {
			ResolveHit(-bullet.direction, moving);
			return true;
		}
		bullet.position = end;
		return glm::length(end) >= ARENA_RADIUS;
	});
}

Screen UpdateCombat(float dt, bool moving)
{
	SpawnEnemies();
	UpdateBeam();
	for (Enemy& enemy : enemies) {
		UpdateEnemy(enemy, dt, moving);
	}
	UpdateBullets(dt, moving);
	erase_if(enemies, [](const Enemy& enemy) { return enemy.health <= 0; });

	if (playerHealth == 0) return Screen::Defeated;
	if (spawnedEnemies == RoundEnemyCount() && enemies.empty()) {
		if (currentRound == ROUND_COUNT) return Screen::Cleared;
		StartRound(currentRound + 1);
	}
	return Screen::Playing;
}

void UpdateRound(GLFWwindow* window, float dt)
{
	gameTime += dt;
	blocking = gameTime >= defenseUnlockAt && gameTime >= beam.expiresAt
		&& glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
	glm::vec3 previousPosition = playerPosition;
	if (gameTime >= moveUnlockAt) UpdatePlayer(window, dt);

	Screen result = UpdateCombat(dt, playerPosition != previousPosition);
	if (result != Screen::Playing) SetScreen(window, result);
}

void StartParry()
{
	if (gameTime < defenseUnlockAt || gameTime < beam.expiresAt) return;

	const float windows[] = {.34f, .26f, .18f, .13f};
	parryUntil = gameTime + windows[glm::min(energy / 10, 3)];
	moveUnlockAt = gameTime + .5f;
	defenseUnlockAt = gameTime + 1.0f;
	parrySucceeded = parryStunned = blocking = false;
}

glm::mat4 CameraView()
{
	if (firstPerson) {
		glm::vec3 eye = playerPosition + worldUp * EYE_HEIGHT;
		glm::vec3 direction = ForwardDirection(playerYaw) * glm::cos(firstPersonPitch)
			+ worldUp * glm::sin(firstPersonPitch);
		return glm::lookAt(eye, eye + direction, worldUp);
	}

	glm::vec3 target = playerPosition + worldUp * PLAYER_HEIGHT * .62f;
	float elevation = glm::radians(CAMERA_ELEVATION) + orbitElevation;
	glm::vec3 offset = (
		-ForwardDirection(playerYaw + orbitYaw) * glm::cos(elevation)
		+ worldUp * glm::sin(elevation)
	) * CAMERA_DISTANCE;
	return glm::lookAt(target + offset, target, worldUp);
}

void SetViewProjection(const glm::mat4& view, const glm::mat4& projection)
{
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));
}

void DrawMesh(const Mesh& mesh, const glm::mat4& model, glm::vec4 color, Surface surface)
{
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
	glUniform4fv(colorLocation, 1, glm::value_ptr(color));
	glUniform1i(surfaceLocation, (GLint)surface);
	glDrawArrays(GL_TRIANGLES, mesh.first, mesh.count);
}

void DrawBox(glm::vec3 position, glm::vec3 size, glm::vec4 color, glm::vec3 forward, Surface surface)
{
	glm::mat4 model = glm::translate(glm::mat4(1), position);
	model = glm::rotate(model, -glm::atan(forward.x, -forward.z), worldUp);
	DrawMesh(playerMesh, glm::scale(model, size), color, surface);
}

void DrawRoundObjects()
{
	for (glm::vec3 outward : entries) {
		float center = ARENA_RADIUS + (BRIDGE_OUTSIDE - BRIDGE_INSIDE) * .5f;
		glm::vec3 position = outward * center - worldUp * .115f;
		DrawBox(position, glm::vec3(BRIDGE_WIDTH, .2f, BRIDGE_OUTSIDE + BRIDGE_INSIDE),
			glm::vec4(.48f, .40f, .30f, 1), outward, Surface::Lit);
	}

	for (const Enemy& enemy : enemies) {
		glm::vec4 color = enemy.type == EnemyType::Melee
			? glm::vec4(.81f, .34f, .23f, 1)
			: glm::vec4(.63f, .39f, .83f, 1);
		if (enemy.state == EnemyState::Windup) {
			color = glm::vec4(.98f, .70f, .22f, 1);
			glm::vec3 center = enemy.position + enemy.facing * MELEE_REACH * .5f + worldUp * .018f;
			DrawBox(center, glm::vec3(.34f, .023f, MELEE_REACH), color, enemy.facing, Surface::Flat);
		}
		DrawBox(enemy.position + worldUp * PLAYER_HEIGHT * .5f,
			glm::vec3(PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_DEPTH), color, enemy.facing, Surface::Lit);
	}

	for (const Bullet& bullet : bullets) {
		DrawBox(bullet.position + worldUp * .68f, glm::vec3(BULLET_RADIUS * 2),
			glm::vec4(.79f, .59f, 1, 1), bullet.direction, Surface::Flat);
	}

	if (gameTime < beam.expiresAt) {
		glm::vec4 color(.37f, .92f, .96f, 1);
		float length = BeamLength();
		float start = 0;
		for (float end : {.2f, .4f, .6f, .8f, length}) {
			end = glm::min(end, length);
			if (end <= start) break;
			float middle = (start + end) * .5f;
			DrawBox(playerPosition + beam.direction * middle + worldUp * .68f,
				glm::vec3(BeamWidth(middle), .12f, end - start), color, beam.direction, Surface::Flat);
			start = end;
		}
	}
}

void DrawArena(float aspect)
{
	glEnable(GL_DEPTH_TEST);
	SetViewProjection(CameraView(), glm::perspective(glm::radians(60.0f), aspect, .05f, 80.0f));

	glm::mat4 floor = glm::scale(glm::mat4(1), glm::vec3(ARENA_RADIUS, 1, ARENA_RADIUS));
	DrawMesh(floorMesh, floor, glm::vec4(.29f, .36f, .40f, 1), Surface::Arena);
	DrawRoundObjects();

	if (!firstPerson) {
		glm::mat4 player = glm::translate(glm::mat4(1), playerPosition + worldUp * PLAYER_HEIGHT * .5f);
		player = glm::rotate(player, -playerYaw, worldUp);
		DrawMesh(playerMesh, glm::scale(player, glm::vec3(PLAYER_WIDTH, PLAYER_HEIGHT, PLAYER_DEPTH)),
			glm::vec4(.21f, .59f, .66f, 1), Surface::Lit);

		glm::mat4 front = glm::translate(player, glm::vec3(0, .35f, -PLAYER_DEPTH * .5f - .002f));
		front = glm::scale(front, glm::vec3(.48f, .18f, 1));
		DrawMesh(rectangleMesh, front, glm::vec4(.77f, .97f, .99f, 1), Surface::Flat);
	}
}

MenuLayout GetMenuLayout(int width, int height)
{
	float scale = glm::min(1.0f, glm::min(width / 440.0f, height / 260.0f));
	float x = width * .5f, y = height * .5f;
	return {
		scale,
		{x, y, 360 * scale, 196 * scale},
		{x, y, 176 * scale, 56 * scale},
		{x, y + 62 * scale, 176 * scale, 42 * scale},
		{x, y, 232 * scale, 32 * scale},
		{x, y + 62 * scale, 132 * scale, 42 * scale}
	};
}

bool IsInside(const UiRectangle& rectangle, double x, double y)
{
	return glm::abs(x - rectangle.x) <= rectangle.width * .5f
		&& glm::abs(y - rectangle.y) <= rectangle.height * .5f;
}

void SetSensitivityFromCursor(const UiRectangle& slider, double x)
{
	if (slider.width <= 0) return;
	float left = slider.x - slider.width * .5f;
	float progress = glm::clamp(((float)x - left) / slider.width, 0.0f, 1.0f);
	sensitivitySliderPercent = (int)glm::round(progress * 100.0f);
}

float MouseSensitivityScale()
{
	// 슬라이더의 왼쪽, 가운데, 오른쪽을 최대 감도의 1%, 10%, 100%에 대응시킨다.
	return glm::pow(100.0f, sensitivitySliderPercent / 100.0f - 1.0f);
}

bool IsArenaVisible()
{
	if (screen == Screen::Settings) return settingsReturn == Screen::Paused;
	return screen != Screen::Start;
}

void DrawRectangle(const UiRectangle& rectangle, glm::vec4 color)
{
	glm::mat4 model = glm::translate(glm::mat4(1), glm::vec3(rectangle.x, rectangle.y, 0));
	model = glm::scale(model, glm::vec3(rectangle.width, rectangle.height, 1));
	DrawMesh(rectangleMesh, model, color, Surface::Flat);
}

const unsigned char* LetterRows(char letter)
{
	static const unsigned char rows[][7] = {
		{24, 25, 2, 4, 8, 19, 3},    // 퍼센트 기호
		{14, 17, 19, 21, 25, 17, 14}, // 숫자 0
		{4, 12, 4, 4, 4, 4, 14},    // 숫자 1
		{14, 17, 1, 2, 4, 8, 31},    // 숫자 2
		{30, 1, 1, 14, 1, 1, 30},    // 숫자 3
		{2, 6, 10, 18, 31, 2, 2},    // 숫자 4
		{31, 16, 16, 30, 1, 1, 30},  // 숫자 5
		{14, 16, 16, 30, 17, 17, 14}, // 숫자 6
		{31, 1, 2, 4, 8, 8, 8},      // 숫자 7
		{14, 17, 17, 14, 17, 17, 14}, // 숫자 8
		{14, 17, 17, 15, 1, 1, 14},   // 숫자 9
		{14, 17, 17, 31, 17, 17, 17}, // 영문 대문자 A
		{30, 17, 17, 30, 17, 17, 30}, // 영문 대문자 B
		{14, 17, 16, 16, 16, 17, 14}, // 영문 대문자 C
		{30, 17, 17, 17, 17, 17, 30}, // 영문 대문자 D
		{31, 16, 16, 30, 16, 16, 31}, // 영문 대문자 E
		{31, 16, 16, 30, 16, 16, 16}, // 영문 대문자 F
		{14, 17, 16, 23, 17, 17, 15}, // 영문 대문자 G
		{17, 17, 17, 31, 17, 17, 17}, // 영문 대문자 H
		{14, 4, 4, 4, 4, 4, 14},     // 영문 대문자 I
		{17, 18, 20, 24, 20, 18, 17}, // 영문 대문자 K
		{16, 16, 16, 16, 16, 16, 31}, // 영문 대문자 L
		{17, 27, 21, 21, 17, 17, 17}, // 영문 대문자 M
		{17, 25, 25, 21, 19, 19, 17}, // 영문 대문자 N
		{14, 17, 17, 17, 17, 17, 14}, // 영문 대문자 O
		{30, 17, 17, 30, 16, 16, 16}, // 영문 대문자 P
		{30, 17, 17, 30, 20, 18, 17}, // 영문 대문자 R
		{15, 16, 16, 14, 1, 1, 30},   // 영문 대문자 S
		{31, 4, 4, 4, 4, 4, 4},      // 영문 대문자 T
		{17, 17, 17, 17, 17, 17, 14}, // 영문 대문자 U
		{17, 17, 17, 17, 17, 10, 4},  // 영문 대문자 V
		{17, 17, 10, 4, 4, 4, 4}     // 영문 대문자 Y
	};
	size_t index = string_view("%0123456789ABCDEFGHIKLMNOPRSTUVY").find(letter);
	return index != string_view::npos ? rows[index] : nullptr;
}

void DrawLabel(string_view label, float x, float y, float size)
{
	float left = x - ((float)label.size() * 6 - 1) * size * .5f;
	float top = y - 3.5f * size;
	for (char letter : label) {
		const unsigned char* rows = LetterRows(letter);
		if (rows) {
			for (int row = 0; row < 7; ++row) {
				for (int column = 0; column < 5; ++column) {
					if (rows[row] & (1 << (4 - column))) {
						DrawRectangle({left + (column + .5f) * size, top + (row + .5f) * size, size, size},
							glm::vec4(.94f, .97f, .98f, 1));
					}
				}
			}
		}
		left += 6 * size;
	}
}

void DrawRoundHud()
{
	int remaining = RoundEnemyCount() - spawnedEnemies + (int)enemies.size();
	DrawRectangle({99, 52, 174, 80}, glm::vec4(.055f, .085f, .11f, .82f));
	DrawLabel("ROUND " + to_string(currentRound), 59, 29, 1.6f);
	DrawLabel("LEFT " + to_string(remaining), 142, 29, 1.6f);

	DrawLabel("HP", 52, 51, 1.35f);
	DrawLabel("ENERGY", 52, 72, 1.35f);
	const glm::vec4 background(.20f, .25f, .29f, 1);
	DrawRectangle({126, 51, 86, 9}, background);
	DrawRectangle({126, 72, 86, 9}, background);
	float healthWidth = 86.0f * playerHealth / PLAYER_HEALTH;
	float energyWidth = 86.0f * energy / MAX_ENERGY;
	if (healthWidth > 0) {
		DrawRectangle({83 + healthWidth * .5f, 51, healthWidth, 9}, glm::vec4(.36f, .72f, .43f, 1));
	}
	if (energyWidth > 0) {
		DrawRectangle({83 + energyWidth * .5f, 72, energyWidth, 9}, glm::vec4(.34f, .76f, .84f, 1));
	}
}

void DrawMenuButton(const UiRectangle& button, string_view label, float textSize, double x, double y)
{
	glm::vec4 color = IsInside(button, x, y)
		? glm::vec4(.19f, .50f, .55f, 1)
		: glm::vec4(.14f, .39f, .44f, 1);
	DrawRectangle(button, color);
	DrawLabel(label, button.x, button.y, textSize);
}

void DrawSettings(const MenuLayout& menu, double x, double y)
{
	DrawRectangle(menu.panel, glm::vec4(.12f, .17f, .22f, 1));
	DrawLabel("MOUSE SENSITIVITY", menu.panel.x, menu.panel.y - 57 * menu.scale, 2.15f * menu.scale);
	int actualPercent = (int)glm::round(MouseSensitivityScale() * 100.0f);
	DrawLabel(to_string(actualPercent) + "%", menu.panel.x, menu.panel.y - 28 * menu.scale, 1.9f * menu.scale);

	const UiRectangle& slider = menu.sensitivitySlider;
	float progress = sensitivitySliderPercent / 100.0f;
	float left = slider.x - slider.width * .5f;
	float fillWidth = slider.width * progress;
	DrawRectangle({slider.x, slider.y, slider.width, 8 * menu.scale}, glm::vec4(.24f, .29f, .33f, 1));
	if (fillWidth > 0) {
		DrawRectangle({left + fillWidth * .5f, slider.y, fillWidth, 8 * menu.scale}, glm::vec4(.25f, .57f, .64f, 1));
	}
	DrawRectangle({left + fillWidth, slider.y, 12 * menu.scale, 24 * menu.scale}, glm::vec4(.82f, .93f, .95f, 1));
	DrawMenuButton(menu.backButton, "BACK", 2.0f * menu.scale, x, y);
}

void DrawInterface(GLFWwindow* window)
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if (width <= 0 || height <= 0) return;

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	SetViewProjection(glm::mat4(1), glm::ortho(0.0f, (float)width, (float)height, 0.0f));

	MenuLayout menu = GetMenuLayout(width, height);
	double x = 0, y = 0;
	if (screen != Screen::Playing) glfwGetCursorPos(window, &x, &y);
	if (IsArenaVisible()) {
		DrawRoundHud();
		if (screen != Screen::Playing) {
			DrawRectangle({menu.panel.x, menu.panel.y, (float)width, (float)height},
				glm::vec4(.025f, .04f, .06f, .65f));
		}
	}

	if (screen == Screen::Start) {
		DrawRectangle(menu.panel, glm::vec4(.12f, .17f, .22f, 1));
		DrawMenuButton(menu.startButton, "START", 2.6f * menu.scale, x, y);
		DrawMenuButton(menu.settingsButton, "SETTINGS", 2.0f * menu.scale, x, y);
	}
	else if (screen == Screen::Settings) {
		DrawSettings(menu, x, y);
	}
	else if (screen != Screen::Playing) {
		string_view label = "PAUSED";
		if (screen == Screen::Cleared) label = "CLEAR";
		else if (screen == Screen::Defeated) label = "DEFEAT";
		DrawLabel(label, menu.panel.x, menu.panel.y, 3.2f * menu.scale);
		if (screen == Screen::Paused) DrawMenuButton(menu.settingsButton, "SETTINGS", 2.0f * menu.scale, x, y);
	}
	glDisable(GL_BLEND);
}

void DrawScene(GLFWwindow* window, int width, int height)
{
	glClearColor(.055f, .085f, .11f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(programID);
	glBindVertexArray(vao);
	if (IsArenaVisible()) DrawArena((float)width / height);
	DrawInterface(window);
	glBindVertexArray(0);
}

bool IsAltHeld(GLFWwindow* window)
{
	return glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
}

void ResetOrbit()
{
	orbitYaw = 0;
	orbitElevation = 0;
}

void SetScreen(GLFWwindow* window, Screen next)
{
	screen = next;
	ResetOrbit();
	draggingSensitivity = false;
	mousePositionReady = false;
	glfwSetInputMode(window, GLFW_CURSOR, screen == Screen::Playing ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void KeyCallback(GLFWwindow* window, int key, int, int action, int)
{
	if (action == GLFW_RELEASE && (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)) {
		if (!IsAltHeld(window)) ResetOrbit();
	}
	if (action != GLFW_PRESS) return;
	if (screen == Screen::Settings) {
		if (key == GLFW_KEY_ESCAPE) SetScreen(window, settingsReturn);
		return;
	}

	if (key >= GLFW_KEY_1 && key <= GLFW_KEY_3) {
		StartGame(key - GLFW_KEY_1 + 1);
		SetScreen(window, Screen::Playing);
		return;
	}
	if (key == GLFW_KEY_R && screen != Screen::Start) {
		StartGame(1);
		SetScreen(window, Screen::Playing);
		return;
	}

	if (key == GLFW_KEY_ESCAPE) {
		if (screen == Screen::Playing) SetScreen(window, Screen::Paused);
		else if (screen == Screen::Paused) SetScreen(window, Screen::Playing);
	}
	if (key == GLFW_KEY_V && screen == Screen::Playing) {
		firstPerson = !firstPerson;
		ResetOrbit();
	}
}

void HandleMenuClick(GLFWwindow* window, const MenuLayout& menu, double x, double y)
{
	if (screen == Screen::Settings) {
		if (IsInside(menu.backButton, x, y)) {
			SetScreen(window, settingsReturn);
		}
		else {
			UiRectangle hitArea = menu.sensitivitySlider;
			hitArea.width += 12 * menu.scale;
			if (IsInside(hitArea, x, y)) {
				draggingSensitivity = true;
				SetSensitivityFromCursor(menu.sensitivitySlider, x);
			}
		}
		return;
	}
	if ((screen == Screen::Start || screen == Screen::Paused) && IsInside(menu.settingsButton, x, y)) {
		settingsReturn = screen;
		SetScreen(window, Screen::Settings);
	}
	else if (screen == Screen::Start && IsInside(menu.startButton, x, y)) {
		StartGame(1);
		SetScreen(window, Screen::Playing);
	}
}

void MouseCallback(GLFWwindow* window, int button, int action, int)
{
	if (screen == Screen::Playing) {
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) blocking = false;
		if (action != GLFW_PRESS) return;
		if (button == GLFW_MOUSE_BUTTON_RIGHT) StartParry();
		else if (button == GLFW_MOUSE_BUTTON_LEFT) StartBeam();
		return;
	}
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;
	if (action == GLFW_RELEASE) draggingSensitivity = false;
	if (action != GLFW_PRESS) return;

	int width, height;
	double x, y;
	glfwGetWindowSize(window, &width, &height);
	glfwGetCursorPos(window, &x, &y);
	HandleMenuClick(window, GetMenuLayout(width, height), x, y);
}

void CursorCallback(GLFWwindow* window, double x, double y)
{
	if (screen == Screen::Settings && draggingSensitivity) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		SetSensitivityFromCursor(GetMenuLayout(width, height).sensitivitySlider, x);
	}

	float deltaX = (float)(x - previousMouseX);
	float deltaY = (float)(y - previousMouseY);
	previousMouseX = x;
	previousMouseY = y;
	if (!mousePositionReady) {
		mousePositionReady = true;
		return;
	}
	if (screen != Screen::Playing) return;

	float sensitivity = MAX_MOUSE_SENSITIVITY * MouseSensitivityScale();
	if (!firstPerson && IsAltHeld(window)) {
		orbitYaw -= deltaX * sensitivity;
		orbitElevation = glm::clamp(orbitElevation - deltaY * sensitivity,
			glm::radians(10.0f - CAMERA_ELEVATION), glm::radians(78.0f - CAMERA_ELEVATION));
		return;
	}

	playerYaw += deltaX * sensitivity;
	if (firstPerson) {
		firstPersonPitch = glm::clamp(firstPersonPitch - deltaY * sensitivity,
			glm::radians(-85.0f), glm::radians(85.0f));
	}
}

void FocusCallback(GLFWwindow*, int focused)
{
	mousePositionReady = false;
	if (!focused) {
		draggingSensitivity = false;
		ResetOrbit();
	}
}

string ReadFile(const filesystem::path& path)
{
	ifstream file(path);
	if (!file) {
		cerr << "Cannot open shader: " << path << endl;
		return "";
	}
	stringstream stream;
	stream << file.rdbuf();
	return stream.str();
}

bool InitShader()
{
	wchar_t executablePath[MAX_PATH];
	DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
	filesystem::path directory;
	if (pathLength > 0 && pathLength < MAX_PATH) {
		directory = filesystem::path(executablePath).parent_path();
	}
	string sources[] = {ReadFile(directory / "vertex.glsl"), ReadFile(directory / "fragment.glsl")};
	if (sources[0].empty() || sources[1].empty()) return false;

	GLuint shaders[] = {glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};
	for (int i = 0; i < 2; ++i) {
		const char* code = sources[i].c_str();
		glShaderSource(shaders[i], 1, &code, nullptr);
		glCompileShader(shaders[i]);
		GLint ok;
		glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
		if (!ok) {
			char log[1024];
			glGetShaderInfoLog(shaders[i], sizeof(log), nullptr, log);
			cerr << log << endl;
			glDeleteShader(shaders[0]);
			glDeleteShader(shaders[1]);
			return false;
		}
	}

	programID = glCreateProgram();
	glAttachShader(programID, shaders[0]);
	glAttachShader(programID, shaders[1]);
	glLinkProgram(programID);
	glDeleteShader(shaders[0]);
	glDeleteShader(shaders[1]);

	GLint ok;
	glGetProgramiv(programID, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024];
		glGetProgramInfoLog(programID, sizeof(log), nullptr, log);
		cerr << log << endl;
		glDeleteProgram(programID);
		return false;
	}

	modelLocation = glGetUniformLocation(programID, "modelTransform");
	viewLocation = glGetUniformLocation(programID, "viewTransform");
	projectionLocation = glGetUniformLocation(programID, "projectionTransform");
	colorLocation = glGetUniformLocation(programID, "uColor");
	surfaceLocation = glGetUniformLocation(programID, "uSurface");
	return modelLocation >= 0 && viewLocation >= 0 && projectionLocation >= 0
		&& colorLocation >= 0 && surfaceLocation >= 0;
}
