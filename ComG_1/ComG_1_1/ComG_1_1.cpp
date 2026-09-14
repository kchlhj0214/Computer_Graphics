//프로젝트속성->링커->명령줄에다음과같이3개의라이브러리추가
//-> opengl32.lib glew32.lib glfw3.lib

#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>
#include <random>

using namespace std;

void InputProcess(GLFWwindow* window);
void DrawScene();
void SetRandomColor();
void UpdateTimer();

//--- 난수 생성기와 RGB 값의 분포
random_device rd;
mt19937 generator(rd());
uniform_real_distribution<float> distribution(0.0f, 1.0f);

//--- 타이머 상태
bool timerRunning = false;
double lastChangeTime = 0.0;
const double timerInterval = 1.0; // 1초 간격

int main() {
	//--- GLFW 초기화
	if (!glfwInit()) {
		cerr << "GLFW 초기화 실패!" << endl;
		return -1;
	}
	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);		// GLFW_OPENGL_COMPAT_PROFILE
	//--- 윈도우생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		cerr << "윈도우생성실패!" << endl;
		glfwTerminate();
		return -1;
	}
	//--- 컨텍스트설정
	glfwMakeContextCurrent(window);
	//--- GLEW 초기화
	glewExperimental = GL_TRUE; // 최신 기능 사용
	if (glewInit() != GLEW_OK) {
		cerr << "GLEW 초기화 실패!" << endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}
	//--- 뷰포트설정
	glViewport(0, 0, 800, 600);
	//--- 초기 배경색: 흰색 (한 번만 설정)
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {
		InputProcess(window);
		UpdateTimer();
		// 화면지우기
		DrawScene();
		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}


//--- 키보드입력처리함수
void InputProcess(GLFWwindow* window)
{
	// 이전 키 상태를 보관하여 길게 눌러도 한 번만 처리
	static bool previousA = false;
	static bool previousT = false;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
		glClearColor(0.0f, 1.0f, 1.0f, 1.0f); // RGBA
	}
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // RGBA
	}
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
		glClearColor(1.0f, 1.0f, 0.0f, 1.0f); // 노랑색
	}
	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		glClearColor(0.4f, 0.4f, 0.4f, 1.0f); // 회색
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // RGBA
	}

	bool currentA = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
	bool currentT = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;

	// a: 새로 누른 순간에 랜덤색으로 변경
	if (currentA && !previousA) {
		SetRandomColor();
	}

	// t: 타이머 시작, 실행 중이면 그대로 유지
	if (currentT && !previousT && !timerRunning) {
		timerRunning = true;
		lastChangeTime = glfwGetTime();
	}

	// s: 타이머 정지, 현재 색 유지
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		timerRunning = false;
	}

	previousA = currentA;
	previousT = currentT;
}

//--- 랜덤 배경색 설정 함수
void SetRandomColor()
{
	float r = distribution(generator);
	float g = distribution(generator);
	float b = distribution(generator);
	glClearColor(r, g, b, 1.0f);
}

//--- 타이머 처리 함수
void UpdateTimer()
{
	if (!timerRunning) {
		return;
	}

	double currentTime = glfwGetTime();
	if (currentTime - lastChangeTime >= timerInterval) {
		SetRandomColor();
		lastChangeTime = currentTime;
	}
}
//--- 렌더링함수
void DrawScene()
{
	// 마지막으로 지정한 배경색 적용
	glClear(GL_COLOR_BUFFER_BIT);

}
