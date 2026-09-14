//프로젝트속성->링커->명령줄에다음과같이3개의라이브러리추가
//-> opengl32.lib glew32.lib glfw3.lib

#include <gl/glew.h>
#include <gl/glfw3.h>
#include <iostream>

int main() {
	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}
	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//--- 윈도우생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우생성실패!" << std::endl;
		glfwTerminate();
		return -1;
	}
	//--- 컨텍스트설정
	glfwMakeContextCurrent(window);
	//--- GLEW 초기화
	glewExperimental = GL_TRUE; // 최신 기능 사용
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}
	//--- 뷰포트설정
	glViewport(0, 0, 800, 600);
	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {
		// 입력처리
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		// 화면지우기(파란색)
		glClearColor(0.0f, 0.0f, 1.0f, 1.0f); // RGBA (파랑)
		glClear(GL_COLOR_BUFFER_BIT);
		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}