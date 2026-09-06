#include <iostream>
#include <random>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <windows.h>

#pragma execution_character_set("utf-8")

using namespace std;

#define MIN_BOARD_WIDTH 3
#define MAX_BOARD_WIDTH 6
#define MIN_BOARD_HEIGHT 3
#define MAX_BOARD_HEIGHT 6
#define MAX_ATTEMPTS 5

const unsigned short COLOR_DEFAULT = 7;
const unsigned short COLOR_MATCHED = 10;
const char JOKER = '@';

struct Card {
    char value = '*';
    bool matched = false;
};

struct Position {
    int row = -1;
    int column = -1;
};

struct GameState {
    int width = 0;
    int height = 0;
    int attemptsRemaining = MAX_ATTEMPTS;
    int score = 0;
    bool finished = false;
    vector<vector<Card>> board;
    vector<Position> temporaryOpenCards;
    string message;
};

void setTextColor(unsigned short color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

char toUppercase(char value) {
    return value >= 'a' && value <= 'z' ? value - 'a' + 'A' : value;
}

char toLowercase(char value) {
    return value >= 'A' && value <= 'Z' ? value - 'A' + 'a' : value;
}

bool samePosition(const Position& first, const Position& second) {
    return first.row == second.row && first.column == second.column;
}

bool isTemporarilyOpen(const GameState& game, int row, int column) {
    for (const Position& position : game.temporaryOpenCards) {
        if (position.row == row && position.column == column) return true;
    }
    return false;
}

void initializeGame(GameState& game, int width, int height) {
    game = GameState{};
    game.width = width;
    game.height = height;
    game.board.assign(height, vector<Card>(width));

    int cellCount = width * height;
    vector<char> values;
    for (int i = 0; i < cellCount / 2; ++i) {
        char value = static_cast<char>('a' + i);
        values.push_back(value);
        values.push_back(value);
    }
    if (cellCount % 2 != 0) values.push_back(JOKER);

    static random_device randomDevice;
    static mt19937 generator(randomDevice());
    shuffle(values.begin(), values.end(), generator);

    int index = 0;
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            game.board[row][column].value = values[index++];
        }
    }
}

void printBoard(const GameState& game, bool revealAll = false) {
    cout << "   ";
    for (int column = 0; column < game.width; ++column) {
        cout << static_cast<char>('a' + column) << " ";
    }
    cout << "\n";

    for (int row = 0; row < game.height; ++row) {
        cout << row + 1 << "  ";
        for (int column = 0; column < game.width; ++column) {
            const Card& card = game.board[row][column];
            if (card.matched) {
                setTextColor(COLOR_MATCHED);
                cout << toUppercase(card.value);
                setTextColor(COLOR_DEFAULT);
            }
            else if (revealAll || isTemporarilyOpen(game, row, column)) {
                cout << card.value;
            }
            else {
                cout << '*';
            }
            cout << " ";
        }
        cout << "\n";
    }
}

void drawScreen(const GameState& game) {
    system("cls");
    printBoard(game);
    cout << "\n남은 기회: " << game.attemptsRemaining
        << "    현재 점수: " << game.score << "\n";
    if (!game.message.empty()) cout << "\n" << game.message << "\n";

    if (game.finished) {
        cout << "\n최종 점수: " << game.score
            << "\nr: 다시 시작, q: 종료\n";
    }
    else {
        cout << "\n두 칸 선택(a1 1b), r: 리셋, h: 힌트, q: 종료\n";
    }
    cout << "입력: ";
}

bool readBoardSize(int& width, int& height) {
    while (true) {
        system("cls");
        cout << "보드의 가로와 세로 크기 입력(" << MIN_BOARD_WIDTH << "~"
            << MAX_BOARD_WIDTH << "): ";

        string line;
        getline(cin, line);
        if (line == "q") return false;

        istringstream input(line);
        string extra;
        if ((input >> width >> height) && !(input >> extra)
            && width >= MIN_BOARD_WIDTH && width <= MAX_BOARD_WIDTH
            && height >= MIN_BOARD_HEIGHT && height <= MAX_BOARD_HEIGHT) {
            return true;
        }

        cout << "가로와 세로는 각각 지정된 범위의 정수여야 합니다.\n"
            << "계속하려면 Enter를 누르세요.";
        getline(cin, line);
    }
}

bool parsePosition(const string& text, int width, int height, Position& position) {
    if (text.size() != 2) return false;

    char first = toLowercase(text[0]);
    char second = toLowercase(text[1]);
    char column;
    char row;

    if (first >= 'a' && first <= 'f' && second >= '1' && second <= '6') {
        column = first;
        row = second;
    }
    else if (second >= 'a' && second <= 'f' && first >= '1' && first <= '6') {
        column = second;
        row = first;
    }
    else return false;

    position = { row - '1', column - 'a' };
    return position.row < height && position.column < width;
}

bool allNormalCardsMatched(const GameState& game) {
    for (const vector<Card>& row : game.board) {
        for (const Card& card : row) {
            if (card.value != JOKER && !card.matched) return false;
        }
    }
    return true;
}

void finishGameIfNeeded(GameState& game) {
    if (allNormalCardsMatched(game)) {
        for (vector<Card>& row : game.board) {
            for (Card& card : row) {
                if (card.value == JOKER) card.matched = true;
            }
        }
        game.score += game.attemptsRemaining;
        game.finished = true;
        game.message = "모든 짝을 맞혔습니다. 남은 기회가 보너스 점수로 추가됐습니다.";
    }
    else if (game.attemptsRemaining == 0) {
        game.finished = true;
        game.message = "기회를 모두 사용했습니다.";
    }
}

void matchWithJoker(GameState& game, const Position& first, const Position& second) {
    Position joker = game.board[first.row][first.column].value == JOKER
        ? first : second;
    Position normal = samePosition(joker, first) ? second : first;
    char target = game.board[normal.row][normal.column].value;

    game.board[joker.row][joker.column].matched = true;
    for (vector<Card>& row : game.board) {
        for (Card& card : row) {
            if (card.value == target) card.matched = true;
        }
    }
    ++game.score;
    game.message = "조커가 같은 문자의 다른 카드까지 열었습니다.";
}

void selectCards(GameState& game, const Position& first, const Position& second) {
    Card& firstCard = game.board[first.row][first.column];
    Card& secondCard = game.board[second.row][second.column];
    --game.attemptsRemaining;

    if (firstCard.value == JOKER || secondCard.value == JOKER) {
        matchWithJoker(game, first, second);
    }
    else if (firstCard.value == secondCard.value) {
        firstCard.matched = true;
        secondCard.matched = true;
        ++game.score;
        game.message = "짝을 맞혔습니다.";
    }
    else {
        game.temporaryOpenCards = { first, second };
        game.message = "서로 다른 카드입니다. 다음 입력 후 다시 가려집니다.";
    }
    finishGameIfNeeded(game);
}

void showHint(const GameState& game) {
    system("cls");
    printBoard(game, true);
    cout << "\n힌트: 모든 카드를 3초 동안 공개합니다.\n";
    Sleep(3000);
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    while (true) {
        int width;
        int height;
        if (!readBoardSize(width, height)) break;

        GameState game;
        initializeGame(game, width, height);
        bool resetRequested = false;

        while (!resetRequested) {
            drawScreen(game);

            string commandLine;
            getline(cin, commandLine);
            game.temporaryOpenCards.clear();
            game.message.clear();

            if (game.finished) {
                if (commandLine == "q") return 0;
                if (commandLine == "r") resetRequested = true;
                else game.message = "게임 종료 후에는 r 또는 q만 입력할 수 있습니다.";
                continue;
            }

            if (commandLine == "q") return 0;
            if (commandLine == "r") {
                resetRequested = true;
                continue;
            }
            if (commandLine == "h") {
                showHint(game);
                continue;
            }

            istringstream input(commandLine);
            string firstText, secondText, extra;
            Position first, second;
            if (!(input >> firstText >> secondText) || (input >> extra)
                || !parsePosition(firstText, width, height, first)
                || !parsePosition(secondText, width, height, second)) {
                game.message = "두 좌표를 올바르게 입력해주세요. 예: a1 1b";
                continue;
            }
            if (samePosition(first, second)) {
                game.message = "같은 칸을 두 번 선택할 수 없습니다.";
                continue;
            }
            if (game.board[first.row][first.column].matched
                || game.board[second.row][second.column].matched) {
                game.message = "이미 맞춘 카드는 선택할 수 없습니다.";
                continue;
            }

            selectCards(game, first, second);
        }
    }
    return 0;
}
