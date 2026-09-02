#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

#define N 4

// 1차원 인덱스 접근 함수 (r행 c열)
double& at(vector<double>& mat, int r, int c) {
    return mat[r * N + c];
}

double at(const vector<double>& mat, int r, int c) {
    return mat[r * N + c];
}

void randomizeMatrix(vector<double>& mat) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dis(0, 9);

    for (int i = 0; i < N * N; ++i) {
        mat[i] = dis(gen);
    }
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

void printSingleMatrix(const string& title, const vector<double>& mat) {
    cout << "[" << title << "]\n";
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            cout << setw(5) << at(mat, r, c);
        }
        cout << "\n";
    }
    cout << "\n";
}

void drawUI(const vector<double>& mat1, const vector<double>& mat2, bool eToggled, bool fToggled) {
    system("cls");
    cout << "현재 행렬 상태\n";
    printTwoMatrices(mat1, mat2);

    if (eToggled) {
        cout << "[상태 안내] 'e' (행 최솟값 차감) 토글 중입니다. 해제하려면 'e'를 입력하세요.\n";
    }
    else if (fToggled) {
        cout << "[상태 안내] 'f' (열 최댓값 가산) 토글 중입니다. 해제하려면 'f'를 입력하세요.\n";
    }
    cout << "\n\n";
}

// --- 여인수 전개(Cofactor Expansion) 기반 행렬식 계산 ---
double getDeterminant(const vector<double>& mat, int n) {
    vector<vector<double>> a(n, vector<double>(n));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            a[r][c] = mat[r * n + c];
        }
    }

    double det = 1.0;
    int swapCount = 0; // 행 교환 횟수 기록 (행 교환 시 행렬식 부호가 반전됨)

    for (int i = 0; i < n; ++i) {
        // 1. 부분 피보팅(Partial Pivoting): i번째 열에서 절대값이 가장 큰 행을 찾음
        int pivotRow = i;
        for (int j = i + 1; j < n; ++j) {
            if (abs(a[j][i]) > abs(a[pivotRow][i])) {
                pivotRow = j;
            }
        }

        // 피벗 원소가 0에 가까우면(선형 의존 관계) 행렬식은 0
        if (abs(a[pivotRow][i]) < 1e-9) {
            return 0.0;
        }

        // 2. 피벗 행과 현재 행 교환
        if (pivotRow != i) {
            swap(a[i], a[pivotRow]);
            swapCount++; // 행 교환 시 부호 변경 카운트
        }

        // 3. i번째 열의 하단 원소들을 0으로 소거 (상삼각행렬화)
        for (int j = i + 1; j < n; ++j) {
            double factor = a[j][i] / a[i][i];
            for (int k = i; k < n; ++k) {
                a[j][k] -= factor * a[i][k];
            }
        }

        // 대각선 성분을 누적 곱
        det *= a[i][i];
    }

    // 4. 행 교환 횟수가 홀수이면 부호를 반전 (-1 곱함)
    if (swapCount % 2 != 0) {
        det = -det;
    }

    return det;
}

// N x N 행렬의 전치행렬 구하기
vector<double> transposeMatrix(const vector<double>& mat) {
    vector<double> result(N * N);
    for (int r = 0; r < N; ++r) {
        for (int c = 0; c < N; ++c) {
            at(result, c, r) = at(mat, r, c);
        }
    }
    return result;
}

// 각 행의 최솟값을 찾아 해당 행의 모든 원소에서 차감
vector<double> subtractRowMin(const vector<double>& mat) {
    vector<double> result = mat;
    for (int r = 0; r < N; ++r) {
        double minVal = at(result, r, 0);
        for (int c = 1; c < N; ++c) {
            minVal = min(minVal, at(result, r, c));
        }
        for (int c = 0; c < N; ++c) {
            at(result, r, c) -= minVal;
        }
    }
    return result;
}

// 각 열의 최댓값을 찾아 해당 열의 모든 원소에 가산
vector<double> addColMax(const vector<double>& mat) {
    vector<double> result = mat;
    for (int c = 0; c < N; ++c) {
        double maxVal = at(result, 0, c);
        for (int r = 1; r < N; ++r) {
            maxVal = max(maxVal, at(result, r, c));
        }
        for (int r = 0; r < N; ++r) {
            at(result, r, c) += maxVal;
        }
    }
    return result;
}

int main() {
    vector<double> mat1(N * N);
    vector<double> mat2(N * N);

    randomizeMatrix(mat1);
    randomizeMatrix(mat2);

    char command = ' ';

    // 토글 상태 변수
    bool eToggled = false;
    bool fToggled = false;

    // 원본 백업용
    vector<double> origMat1 = mat1;
    vector<double> origMat2 = mat2;

    // 초기 화면 그리기
    drawUI(mat1, mat2, eToggled, fToggled);

    while (true) {
        cout << "명령어 입력: ";
        cin >> command;

        if (command == 'q') break;

        // 토글 활성화 시 다른 명령어 제한
        if (eToggled && command != 'e') {
            cout << ">> [경고] 'e' 토글 상태입니다. 다른 명령을 실행하려면 'e'를 먼저 입력하세요.\n\n";
            continue;
        }
        if (fToggled && command != 'f') {
            cout << ">> [경고] 'f' 토글 상태입니다. 다른 명령을 실행하려면 'f'를 먼저 입력하세요.\n\n";
            continue;
        }

        // 명령어 수용 시 매번 화면을 초기화하고 상단 UI를 새로 그린 후, 그 아래에 연산 결과를 출력
        drawUI(mat1, mat2, eToggled, fToggled);

        switch (command) {
        case 'a': { // 덧셈
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) result[i] = mat1[i] + mat2[i];
            printSingleMatrix("A + B 연산 결과", result);
            break;
        }
        case 'd': { // 뺄셈
            vector<double> result(N * N);
            for (int i = 0; i < N * N; ++i) result[i] = mat1[i] - mat2[i];
            printSingleMatrix("A - B 연산 결과", result);
            break;
        }
        case 'm': { // 곱셈
            vector<double> result(N * N, 0.0);
            for (int r = 0; r < N; ++r) {
                for (int c = 0; c < N; ++c) {
                    for (int k = 0; k < N; ++k) {
                        at(result, r, c) += at(mat1, r, k) * at(mat2, k, c);
                    }
                }
            }
            printSingleMatrix("A * B 연산 결과", result);
            break;
        }
        case 'r': { // 행렬식
            double det1 = getDeterminant(mat1, N);
            double det2 = getDeterminant(mat2, N);
            cout << "[행렬식 값]\n";
            cout << "det(A) = " << det1 << "\n";
            cout << "det(B) = " << det2 << "\n\n";
            break;
        }
        case 't': { // 전치행렬 및 전치행렬의 행렬식
            vector<double> trans1 = transposeMatrix(mat1);
            vector<double> trans2 = transposeMatrix(mat2);
            printSingleMatrix("A의 전치행렬 (A^T)", trans1);
            printSingleMatrix("B의 전치행렬 (B^T)", trans2);

            cout << "[전치행렬의 행렬식 값]\n";
            cout << "det(A^T) = " << getDeterminant(trans1, N) << "\n";
            cout << "det(B^T) = " << getDeterminant(trans2, N) << "\n\n";
            break;
        }
        case 'e': { // 행 최솟값 차감 토글
            if (!eToggled) {
                origMat1 = mat1;
                origMat2 = mat2;
                mat1 = subtractRowMin(mat1);
                mat2 = subtractRowMin(mat2);
                eToggled = true;
            }
            else {
                mat1 = origMat1;
                mat2 = origMat2;
                eToggled = false;
            }
            // 변형된 행렬 상태로 상단 UI 다시 업데이트
            drawUI(mat1, mat2, eToggled, fToggled);
            break;
        }
        case 'f': { // 열 최댓값 가산 토글
            if (!fToggled) {
                origMat1 = mat1;
                origMat2 = mat2;
                mat1 = addColMax(mat1);
                mat2 = addColMax(mat2);
                fToggled = true;
            }
            else {
                mat1 = origMat1;
                mat2 = origMat2;
                fToggled = false;
            }
            // 변형된 행렬 상태로 상단 UI 다시 업데이트
            drawUI(mat1, mat2, eToggled, fToggled);
            break;
        }
        case '+': { // 원본 행렬 모든 항 +1 (0~9 모듈러)
            for (int i = 0; i < N * N; ++i) {
                mat1[i] = static_cast<int>(mat1[i] + 1) % 10;
                mat2[i] = static_cast<int>(mat2[i] + 1) % 10;
            }
            drawUI(mat1, mat2, eToggled, fToggled);
            cout << ">> 모든 항에 +1 연산이 적용되었습니다.\n\n";
            break;
        }
        case '-': { // 원본 행렬 모든 항 -1 (0~9 모듈러)
            for (int i = 0; i < N * N; ++i) {
                mat1[i] = static_cast<int>(mat1[i] - 1 + 10) % 10;
                mat2[i] = static_cast<int>(mat2[i] - 1 + 10) % 10;
            }
            drawUI(mat1, mat2, eToggled, fToggled);
            cout << ">> 모든 항에 -1 연산이 적용되었습니다.\n\n";
            break;
        }
        case 's': // 행렬 랜덤 재생성
            randomizeMatrix(mat1);
            randomizeMatrix(mat2);
            eToggled = false;
            fToggled = false;
            drawUI(mat1, mat2, eToggled, fToggled);
            cout << ">> 행렬이 새로 생성되었습니다.\n\n";
            break;

        default:
            cout << ">> 알 수 없는 명령어입니다.\n\n";
            break;
        }
    }

    return 0;
}