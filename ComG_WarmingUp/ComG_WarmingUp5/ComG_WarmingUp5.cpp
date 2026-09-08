#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <conio.h>
#include <windows.h>

#pragma execution_character_set("utf-8")
#pragma comment(lib, "user32.lib")

using namespace std;

#define INITIAL_BOARD_SIZE 30
#define MIN_BOARD_SIZE 10
#define MAX_BOARD_SIZE 40

const unsigned short COLOR_DEFAULT = 7;
const unsigned short COLOR_FIRST = 12;
const unsigned short COLOR_SECOND = 11;
const unsigned short COLOR_OVERLAP = 14;

struct RectData {
    long long left = 0;
    long long top = 0;
    long long right = 0;
    long long bottom = 0;
};

enum class InputResult {
    Success,
    Reset,
    Quit
};

long long rectangleWidth(const RectData& rectangle) {
    return rectangle.right - rectangle.left + 1;
}

long long rectangleHeight(const RectData& rectangle) {
    return rectangle.bottom - rectangle.top + 1;
}

long long rectangleArea(const RectData& rectangle) {
    return rectangleWidth(rectangle) * rectangleHeight(rectangle);
}

int positiveModulo(long long value, int divisor) {
    int result = static_cast<int>(value % divisor);
    return result < 0 ? result + divisor : result;
}

bool axisContains(long long start, long long length, int coordinate, int boardSize) {
    if (length >= boardSize) return true;

    int wrappedStart = positiveModulo(start, boardSize);
    int offset = (coordinate - wrappedStart + boardSize) % boardSize;
    return offset < length;
}

bool containsCell(const RectData& rectangle, int x, int y, int boardSize) {
    return axisContains(rectangle.left, rectangleWidth(rectangle), x, boardSize)
        && axisContains(rectangle.top, rectangleHeight(rectangle), y, boardSize);
}

void setTextColor(unsigned short color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void printBoard(int boardSize, const RectData& first, const RectData& second) {
    for (int y = 0; y < boardSize; ++y) {
        for (int x = 0; x < boardSize; ++x) {
            bool inFirst = containsCell(first, x, y, boardSize);
            bool inSecond = containsCell(second, x, y, boardSize);

            if (inFirst && inSecond) {
                setTextColor(COLOR_OVERLAP);
                cout << '#';
            }
            else if (inFirst) {
                setTextColor(COLOR_FIRST);
                cout << 'O';
            }
            else if (inSecond) {
                setTextColor(COLOR_SECOND);
                cout << 'X';
            }
            else {
                setTextColor(COLOR_DEFAULT);
                cout << '.';
            }
            cout << ' ';
        }
        setTextColor(COLOR_DEFAULT);
        cout << "\n";
    }
}

void drawScreen(int boardSize, const RectData& first,
    const RectData& second, const string& message) {
    system("cls");
    printBoard(boardSize, first, second);
    setTextColor(COLOR_DEFAULT);

    cout << "\n보드 크기: " << boardSize << " x " << boardSize << "\n";
    if (!message.empty()) cout << message << "\n";
    cout << "명령 입력: ";
}

bool validInitialRectangle(const RectData& rectangle, int boardSize) {
    return rectangle.left >= 0 && rectangle.top >= 0
        && rectangle.right >= rectangle.left
        && rectangle.bottom >= rectangle.top
        && rectangle.right < boardSize
        && rectangle.bottom < boardSize;
}

InputResult readRectangle(const string& name, int boardSize, RectData& rectangle) {
    while (true) {
        system("cls");
        cout << name << " 사각형 좌표 입력(left top right bottom)\n";
        cout << "좌표 범위: 0~" << boardSize - 1 << " (r: 다시 시작, q: 종료)\n";
        cout << "입력: ";

        string line;
        getline(cin, line);
        if (line == "r" || line == "R") return InputResult::Reset;
        if (line == "q" || line == "Q") return InputResult::Quit;

        istringstream input(line);
        string extra;
        RectData candidate;
        if ((input >> candidate.left >> candidate.top
            >> candidate.right >> candidate.bottom)
            && !(input >> extra)
            && validInitialRectangle(candidate, boardSize)) {
            rectangle = candidate;
            return InputResult::Success;
        }

        cout << "잘못된 좌표입니다. Enter를 누르면 다시 입력합니다.";
        getline(cin, line);
    }
}

InputResult setupRectangles(RectData& first, RectData& second) {
    while (true) {
        InputResult result = readRectangle("첫 번째", INITIAL_BOARD_SIZE, first);
        if (result == InputResult::Quit) return result;
        if (result == InputResult::Reset) continue;

        result = readRectangle("두 번째", INITIAL_BOARD_SIZE, second);
        if (result == InputResult::Quit) return result;
        if (result == InputResult::Reset) continue;
        return InputResult::Success;
    }
}

void moveRectangle(RectData& rectangle, int x, int y) {
    rectangle.left += x;
    rectangle.right += x;
    rectangle.top += y;
    rectangle.bottom += y;
}

bool resizeRectangle(RectData& rectangle, int xChange,
    int yChange, int boardSize) {
    long long newWidth = rectangleWidth(rectangle) + xChange;
    long long newHeight = rectangleHeight(rectangle) + yChange;

    if (newWidth < 1 || newHeight < 1
        || newWidth > boardSize || newHeight > boardSize) {
        return false;
    }

    rectangle.right += xChange;
    rectangle.bottom += yChange;
    return true;
}

bool canShrinkBoard(int newSize, const RectData& first, const RectData& second) {
    return rectangleWidth(first) <= newSize
        && rectangleHeight(first) <= newSize
        && rectangleWidth(second) <= newSize
        && rectangleHeight(second) <= newSize;
}

bool handleFirstResize(int key, RectData& rectangle,
    int boardSize, string& message) {
    int command = 0;
    bool reverse = false;

    if (key >= '1' && key <= '4') {
        command = key - '0';
        reverse = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    }
    else if (key == '!') { command = 1; reverse = true; }
    else if (key == '@') { command = 2; reverse = true; }
    else if (key == '#') { command = 3; reverse = true; }
    else if (key == '$') { command = 4; reverse = true; }
    else return false;

    int direction = reverse ? -1 : 1;
    int xChange = 0;
    int yChange = 0;

    if (command == 1) xChange = yChange = direction;
    else if (command == 2) xChange = direction;
    else if (command == 3) yChange = direction;
    else {
        xChange = direction;
        yChange = -direction;
    }

    if (!resizeRectangle(rectangle, xChange, yChange, boardSize)) {
        message = "첫 번째 사각형은 1과 현재 보드 크기 사이로만 변경할 수 있습니다.";
    }
    return true;
}

bool handleSecondResize(int key, RectData& rectangle,
    int boardSize, string& message) {
    char lowerKey = static_cast<char>(key);
    bool reverse = lowerKey >= 'A' && lowerKey <= 'Z';
    if (reverse) lowerKey = lowerKey - 'A' + 'a';
    if (lowerKey != 'z' && lowerKey != 'x'
        && lowerKey != 'c' && lowerKey != 'v') return false;

    int direction = reverse ? -1 : 1;
    int xChange = 0;
    int yChange = 0;

    if (lowerKey == 'z') xChange = yChange = direction;
    else if (lowerKey == 'x') xChange = direction;
    else if (lowerKey == 'c') yChange = direction;
    else {
        xChange = direction;
        yChange = -direction;
    }

    if (!resizeRectangle(rectangle, xChange, yChange, boardSize)) {
        message = "두 번째 사각형은 1과 현재 보드 크기 사이로만 변경할 수 있습니다.";
    }
    return true;
}

bool handleArrowKey(int arrowKey, RectData& rectangle) {
    if (arrowKey == 72) moveRectangle(rectangle, 0, -1);
    else if (arrowKey == 80) moveRectangle(rectangle, 0, 1);
    else if (arrowKey == 75) moveRectangle(rectangle, -1, 0);
    else if (arrowKey == 77) moveRectangle(rectangle, 1, 0);
    else return false;
    return true;
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    while (true) {
        RectData first;
        RectData second;
        if (setupRectangles(first, second) == InputResult::Quit) break;

        int boardSize = INITIAL_BOARD_SIZE;
        string message;
        bool resetRequested = false;

        while (!resetRequested) {
            drawScreen(boardSize, first, second, message);
            message.clear();

            int key = _getch();
            if (key == 0 || key == 224) {
                if (!handleArrowKey(_getch(), second)) {
                    message = "지원하지 않는 특수 키입니다.";
                }
                continue;
            }

            if (key == 'q' || key == 'Q') return 0;
            if (key == 'r' || key == 'R') {
                resetRequested = true;
                continue;
            }
            if (key == '+') {
                if (boardSize == MAX_BOARD_SIZE) {
                    message = "보드는 더 이상 확대할 수 없습니다.";
                }
                else ++boardSize;
                continue;
            }
            if (key == '-') {
                int newSize = boardSize - 1;
                if (boardSize == MIN_BOARD_SIZE) {
                    message = "보드는 더 이상 축소할 수 없습니다.";
                }
                else if (!canShrinkBoard(newSize, first, second)) {
                    message = "사각형보다 작아지므로 보드를 축소할 수 없습니다.";
                }
                else boardSize = newSize;
                continue;
            }
            if (key == 'b' || key == 'B') {
                message = "첫 번째 사각형 면적: " + to_string(rectangleArea(first))
                    + "    두 번째 사각형 면적: " + to_string(rectangleArea(second));
                continue;
            }

            if (key == 'w' || key == 'W') moveRectangle(first, 0, -1);
            else if (key == 's' || key == 'S') moveRectangle(first, 0, 1);
            else if (key == 'a' || key == 'A') moveRectangle(first, -1, 0);
            else if (key == 'd' || key == 'D') moveRectangle(first, 1, 0);
            else if (handleFirstResize(key, first, boardSize, message)) { }
            else if (handleSecondResize(key, second, boardSize, message)) { }
            else message = "알 수 없는 명령입니다.";
        }
    }

    return 0;
}
 //명령어 정리
 //wasd            >> 첫번째 사각형 이동
 //상하좌우 화살표 >> 두번째 사각형 이동
 //1234 !@#$       >> 첫번째 사각형 확대 및 축소
 //zxcv ZXCV       >> 두번째 사각형 확대 및 축소
 //+ -             >> 보드판 확대 및 축소
 //b r q           >> 사각형 면적 출력, 리셋, 종료