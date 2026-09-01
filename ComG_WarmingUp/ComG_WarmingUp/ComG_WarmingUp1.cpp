#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

#define N 4

void randomizeMatrix(vector<double>& mat) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dis(0, 9);

    for (int i = 0; i < N * N; ++i) {
        mat[i] = dis(gen);
    }
}

// 1차원 인덱스 접근 함수 (r행 c열)
double& at(vector<double>& mat, int r, int c) {
    return mat[r * N + c];
}

double at(const vector<double>& mat, int r, int c) {
    return mat[r * N + c];
}

void printTwoMatrices(const vector<double>& mat1, const vector<double>& mat2) {
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            cout << setw(5) << at(mat1, r, c);
        }
        cout << "   |";
        for (int c = 0; c < N; ++c) {
            cout << setw(5) << at(mat2, r, c);
        }
        cout << "\n";
    }
    cout << "\n";
}

// 단일 결과 행렬 출력 함수
void printSingleMatrix(const string& title, const vector<double>& mat) {
    cout << "===== [" << title << "] =====\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            cout << setw(5) << at(mat, r, c);
        }
        cout << "\n";
    }
    cout << "\n";
}

// 화면을 지우고 현재 상태를 다시 그리는 함수
void clearAndDrawUI(const vector<double>& mat1, const vector<double>& mat2) {
    system("cls"); // Windows 콘솔 화면 지우기
    printTwoMatrices(mat1, mat2);
}

int main() {
    // 1. N x N 크기 1차원 벡터 생성 및 초기화
    vector<double> mat1(N * N);
    vector<double> mat2(N * N);

    // 2. 기본적으로 랜덤한 값의 행렬 2개 생성
    randomizeMatrix(mat1);
    randomizeMatrix(mat2);

    char command = ' ';

    while (command != 'q') {
        // 화면 지우기 후 현재 생성된 2개의 기본 행렬 출력
        clearAndDrawUI(mat1, mat2);

        cout << "명령어를 입력하세요: ";
        cin >> command;

        if (command == 'q') break;

        // 화면 지우고 결과 표시 준비
        clearAndDrawUI(mat1, mat2);

        // 명령어 처리 예시
        switch (command) {
        case 'a': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 'd': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] - mat2[i];
            }
            printSingleMatrix("A - B 결과", result);
            break;
        }
        case 'm': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case '+': {
            vector<double> result1(N * N);
            vector<double> result2(N * N);
            for (int i = 0; i < N * N; ++i) {
                result1[i] = mat1[i] + 1;
                result2[i] = mat2[i] + 1;
            }
            printSingleMatrix("A + 1 결과", result1);
            printSingleMatrix("B + 1 결과", result2);

            for (int i = 0; i < N * N; ++i) {
                mat1[i] = result1[i];
                mat2[i] = result2[i];
            }
            break;
        }
        case '-': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 'r': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 't': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 'e': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 'f': {
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) {
                result[i] = mat1[i] + mat2[i];
            }
            printSingleMatrix("A + B 결과", result);
            break;
        }
        case 's':
            randomizeMatrix(mat1);
            randomizeMatrix(mat2);
            cout << ">> 행렬이 새로 생성되었습니다.\n";
            break;

        default:
            cout << ">> 알 수 없는 명령어입니다.\n";
            break;
        }

        cout << "\n다음 명령어를 입력하려면 아무 키나 누른 후 Enter를 입력하세요...";
        string pause;
        cin >> pause;
    }
}