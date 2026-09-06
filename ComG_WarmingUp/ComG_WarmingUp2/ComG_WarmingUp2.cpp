#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <windows.h>

using namespace std;

#define LINE_COUNT 10

const unsigned short COLOR_DEFAULT = 7;
const unsigned short COLOR_SKY_BLUE = 11;
const unsigned short COLOR_MAGENTA = 13;

void setTextColor(unsigned short color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

struct ProgramState {
    vector<char> activeCommands;        // 활성화 된 명령어 순서대로 저장 (하나를 끄면 역연산이 아니라 남은 명령어를 다시 순서대로 재연산)
    string replaceTarget;
    string replaceText;
    string searchText;
    size_t jInputCount = 0;
};

bool readFile(const string& fileName, vector<string>& lines) {
    ifstream inputFile(fileName);

    if (!inputFile.is_open()) {
        return false;
    }

    string line;
    while (lines.size() < LINE_COUNT && getline(inputFile, line)) {     // 최대 10줄을 한 줄 단위로 string에 저장
        lines.push_back(line);
    }

    return true;
}

bool isActive(const ProgramState& state, char command) {
    return find(state.activeCommands.begin(), state.activeCommands.end(), command)
        != state.activeCommands.end();
}

void removeCommand(ProgramState& state, char command) {
    auto commandPosition = find(
        state.activeCommands.begin(), state.activeCommands.end(), command);

    if (commandPosition != state.activeCommands.end()) {
        state.activeCommands.erase(commandPosition);
    }
}

bool isDigit(char character) {
    return character >= '0' && character <= '9';
}

bool isUppercase(char character) {
    return character >= 'A' && character <= 'Z';
}

char changeCase(char character) {
    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 'a';
    }
    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 'A';
    }
    return character;
}

char toLowercase(char character) {
    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 'a';
    }
    return character;
}

string toLowercase(const string& text) {
    string result = text;

    for (char& character : result) {
        character = toLowercase(character);
    }

    return result;
}

bool isWordDelimiter(char character, bool starIsDelimiter) {        // 공백(e가 켜져 있다면 *)으로 단어 구분
    return character == ' ' || (starIsDelimiter && character == '*');
}

void changeAllCases(vector<string>& lines) {
    for (string& line : lines) {
        for (char& character : line) {
            character = changeCase(character);
        }
    }
}

void reverseLines(vector<string>& lines) {          // 문장 뒤집기
    for (string& line : lines) {
        reverse(line.begin(), line.end());
    }
}

void replaceSpacesWithStars(vector<string>& lines) {
    for (string& line : lines) {
        replace(line.begin(), line.end(), ' ', '*');
    }
}

void reverseWords(vector<string>& lines, bool starIsDelimiter) {        // 단어 뒤집기
    for (string& line : lines) {
        size_t wordStart = 0;

        while (wordStart < line.size()) {
            while (wordStart < line.size()
                && isWordDelimiter(line[wordStart], starIsDelimiter)) {
                ++wordStart;
            }

            size_t wordEnd = wordStart;
            while (wordEnd < line.size()
                && !isWordDelimiter(line[wordEnd], starIsDelimiter)) {
                ++wordEnd;
            }

            reverse(line.begin() + wordStart, line.begin() + wordEnd);
            wordStart = wordEnd;
        }
    }
}

void replaceAllStrings(
    vector<string>& lines,
    const string& target,
    const string& replacement) {
    for (string& line : lines) {
        size_t position = 0;

        while ((position = line.find(target, position)) != string::npos) {
            line.replace(position, target.size(), replacement);
            position += replacement.size();
        }
    }
}

vector<string> splitAfterNumbers(const vector<string>& lines) {
    vector<string> result;

    for (const string& line : lines) {
        string sentence;

        for (size_t i = 0; i < line.size(); ++i) {
            sentence += line[i];

            bool numberEndsHere = isDigit(line[i])
                && (i + 1 == line.size() || !isDigit(line[i + 1]));

            if (numberEndsHere) {
                result.push_back(sentence);
                sentence.clear();
            }
        }

        if (!sentence.empty() || line.empty()) {
            result.push_back(sentence);
        }
    }

    return result;
}

void rotateSentences(vector<string>& lines, size_t jInputCount) {
    if (lines.empty()) {
        return;
    }

    size_t moveCount = jInputCount % lines.size();

    if (moveCount != 0) {
        rotate(lines.begin(), lines.end() - moveCount, lines.end());
    }
}

vector<string> makeFinalLines(      // 최종 출력 문장
    const vector<string>& originalLines,
    const ProgramState& state) {
    vector<string> lines = originalLines;
    bool starIsDelimiter = isActive(state, 'e');

    for (char command : state.activeCommands) {
        switch (command) {
        case 'a':
            changeAllCases(lines);
            break;
        case 'd':
            reverseLines(lines);
            break;
        case 'e':
            replaceSpacesWithStars(lines);
            break;
        case 'f':
            reverseWords(lines, starIsDelimiter);
            break;
        case 'g':
            replaceAllStrings(lines, state.replaceTarget, state.replaceText);
            break;
        case 'h':
            lines = splitAfterNumbers(lines);
            break;
        }
    }

    rotateSentences(lines, state.jInputCount);
    return lines;
}

size_t countWords(const string& line, bool starIsDelimiter) {
    size_t wordCount = 0;
    bool readingWord = false;

    for (char character : line) {
        if (isWordDelimiter(character, starIsDelimiter)) {
            readingWord = false;
        }
        else if (!readingWord) {
            ++wordCount;
            readingWord = true;
        }
    }

    return wordCount;
}

size_t markUppercaseWords(
    const string& line,
    bool starIsDelimiter,
    vector<int>& colors) {
    size_t uppercaseWordCount = 0;
    size_t wordStart = 0;

    while (wordStart < line.size()) {
        while (wordStart < line.size()
            && isWordDelimiter(line[wordStart], starIsDelimiter)) {
            ++wordStart;
        }

        size_t wordEnd = wordStart;
        while (wordEnd < line.size()
            && !isWordDelimiter(line[wordEnd], starIsDelimiter)) {
            ++wordEnd;
        }

        if (wordStart < wordEnd && isUppercase(line[wordStart])) {
            ++uppercaseWordCount;
            fill(colors.begin() + wordStart, colors.begin() + wordEnd, 1);
        }

        wordStart = wordEnd;
    }

    return uppercaseWordCount;
}

size_t markSearchResults(
    const string& line,
    const string& searchText,
    vector<int>& colors) {
    string lowercaseLine = toLowercase(line);
    string lowercaseSearchText = toLowercase(searchText);
    size_t matchCount = 0;
    size_t position = 0;

    while ((position = lowercaseLine.find(lowercaseSearchText, position))
        != string::npos) {
        ++matchCount;
        fill(
            colors.begin() + position,
            colors.begin() + position + searchText.size(),
            2);
        position += searchText.size();
    }

    return matchCount;
}

void printColoredLine(const string& line, const vector<int>& colors) {
    int currentColor = 0;

    for (size_t i = 0; i < line.size(); ++i) {
        if (colors[i] != currentColor) {
            if (colors[i] == 1) {
                setTextColor(COLOR_SKY_BLUE);
            }
            else if (colors[i] == 2) {
                setTextColor(COLOR_MAGENTA);
            }
            else {
                setTextColor(COLOR_DEFAULT);
            }

            currentColor = colors[i];
        }

        cout << line[i];
    }

    if (currentColor != 0) {
        setTextColor(COLOR_DEFAULT);
    }
}

void printActiveCommands(
    const ProgramState& state,
    size_t currentSentenceCount) {
    cout << "\n[현재 적용된 명령어] ";

    if (state.activeCommands.empty()) {
        cout << "없음";
    }
    else {
        for (size_t i = 0; i < state.activeCommands.size(); ++i) {
            if (i != 0) {
                cout << " ";
            }

            char command = state.activeCommands[i];
            cout << command;

            if (command == 'j') {
                size_t moveCount = currentSentenceCount == 0
                    ? 0
                    : state.jInputCount % currentSentenceCount;
                cout << moveCount;
            }
        }
    }

    cout << "\n";
}

void drawScreen(
    const vector<string>& originalLines,
    const ProgramState& state,
    const string& message) {
    system("cls");

    vector<string> lines = makeFinalLines(originalLines, state);
    bool starIsDelimiter = isActive(state, 'e');
    bool showWordCount = isActive(state, 'b');
    bool showUppercaseWords = isActive(state, 'c');
    bool showSearchResults = isActive(state, 'i');
    size_t totalUppercaseWordCount = 0;
    size_t totalSearchResultCount = 0;

    cout << "[문장 출력]\n";

    for (const string& line : lines) {
        vector<int> colors(line.size(), 0);

        if (showUppercaseWords) {
            totalUppercaseWordCount += markUppercaseWords(
                line, starIsDelimiter, colors);
        }
        if (showSearchResults) {
            totalSearchResultCount += markSearchResults(
                line, state.searchText, colors);
        }

        printColoredLine(line, colors);

        if (showWordCount) {
            cout << " [단어 수: " << countWords(line, starIsDelimiter) << "]";
        }

        cout << "\n";
    }

    if (showUppercaseWords) {
        cout << "\n대문자로 시작하는 단어 수: "
            << totalUppercaseWordCount << "\n";
    }
    if (showSearchResults) {
        cout << "검색 문자열 \"" << state.searchText << "\"의 개수: "
            << totalSearchResultCount << "\n";
    }

    printActiveCommands(state, lines.size());

    if (!message.empty()) {
        cout << message << "\n";
    }
}

bool activateCommand(ProgramState& state, char command, string& message) {
    if (command == 'g') {
        cout << "찾을 문자열 입력: ";
        getline(cin, state.replaceTarget);

        if (state.replaceTarget.empty()) {
            message = "찾을 문자열은 비워 둘 수 없습니다.";
            return false;
        }

        cout << "바꿀 문자열 입력(빈 문자열 입력 시 삭제): ";
        getline(cin, state.replaceText);
    }
    else if (command == 'i') {
        cout << "검색할 문자열 입력: ";
        getline(cin, state.searchText);

        if (state.searchText.empty()) {
            message = "검색할 문자열은 비워 둘 수 없습니다.";
            return false;
        }
    }

    state.activeCommands.push_back(command);
    return true;
}

void processCommand(ProgramState& state, char command, string& message) {
    if (command == 'j') {
        ++state.jInputCount;

        if (!isActive(state, 'j')) {
            state.activeCommands.push_back('j');
        }
        return;
    }

    if (command < 'a' || command > 'i') {
        message = "알 수 없는 명령어입니다.";
        return;
    }

    if (isActive(state, command)) {
        removeCommand(state, command);
    }
    else {
        activateCommand(state, command, message);
    }
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    string fileName;
    vector<string> originalLines;       // 읽은 문장 저장
    ProgramState state;
    string message;

    cout << "파일 이름 입력: ";
    getline(cin, fileName);

    if (!readFile(fileName, originalLines)) {
        cerr << "파일을 열 수 없습니다: " << fileName << "\n";
        return 1;
    }

    while (true) {
        drawScreen(originalLines, state, message);
        message.clear();

        cout << "명령어 입력(a~j, q: 종료): ";

        string commandInput;
        getline(cin, commandInput);

        if (commandInput.empty()) {
            message = "명령어를 입력해주세요.";
            continue;
        }

        char command = commandInput[0];

        if (command == 'q') {
            break;
        }

        processCommand(state, command, message);
    }

    return 0;
}
