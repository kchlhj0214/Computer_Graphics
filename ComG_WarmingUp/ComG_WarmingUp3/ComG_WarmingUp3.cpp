#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <windows.h>

#pragma execution_character_set("utf-8")

using namespace std;

#define MAX_POINTS 10

struct Point {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct PointSlot {
    Point point;
    bool occupied = false;
};

struct PointList {
    vector<PointSlot> slots = vector<PointSlot>(MAX_POINTS);
    vector<int> logicalOrder;
    int pointCount = 0;
};

struct PointPair {
    int firstRoom;
    int secondRoom;
    double distance;
};

int nextRoom(int room) {
    return (room + 1) % MAX_POINTS;
}

int previousRoom(int room) {
    return (room - 1 + MAX_POINTS) % MAX_POINTS;
}

double squaredOriginDistance(const Point& point) {
    double x = point.x;
    double y = point.y;
    double z = point.z;
    return x * x + y * y + z * z;
}

double originDistance(const Point& point) {
    return sqrt(squaredOriginDistance(point));
}

double squaredPointDistance(const Point& first, const Point& second) {
    double x = static_cast<double>(first.x) - second.x;
    double y = static_cast<double>(first.y) - second.y;
    double z = static_cast<double>(first.z) - second.z;
    return x * x + y * y + z * z;
}

bool addTop(PointList& list, const Point& point) {
    if (list.pointCount == MAX_POINTS) return false;

    int room = 0;
    if (!list.logicalOrder.empty()) {
        room = nextRoom(list.logicalOrder.back());
        while (list.slots[room].occupied) {
            room = nextRoom(room);
        }
    }

    list.slots[room] = { point, true };
    list.logicalOrder.push_back(room);
    ++list.pointCount;
    return true;
}

bool removeTop(PointList& list) {
    if (list.pointCount == 0) return false;

    int room = list.logicalOrder.back();
    list.slots[room].occupied = false;
    list.logicalOrder.pop_back();
    --list.pointCount;
    return true;
}

bool addBottom(PointList& list, const Point& point) {
    if (list.pointCount == MAX_POINTS) return false;

    if (list.slots[0].occupied) {
        int emptyRoom = 1;
        while (list.slots[emptyRoom].occupied) {
            ++emptyRoom;
        }

        for (int room = emptyRoom; room > 0; --room) {
            list.slots[room] = list.slots[room - 1];
        }

        for (int& room : list.logicalOrder) {
            if (room < emptyRoom) {
                ++room;
            }
        }
    }

    list.slots[0] = { point, true };
    list.logicalOrder.insert(list.logicalOrder.begin(), 0);
    ++list.pointCount;
    return true;
}

bool removeBottom(PointList& list) {
    if (list.pointCount == 0) return false;

    int room = list.logicalOrder.front();
    list.slots[room].occupied = false;
    list.logicalOrder.erase(list.logicalOrder.begin());
    --list.pointCount;
    return true;
}

void moveDown(PointList& list) {
    vector<PointSlot> moved(MAX_POINTS);
    for (int room = 0; room < MAX_POINTS; ++room) {
        moved[previousRoom(room)] = list.slots[room];
    }
    list.slots = moved;

    for (int& room : list.logicalOrder) {
        room = previousRoom(room);
    }
}

void clearList(PointList& list) {
    list.slots.assign(MAX_POINTS, PointSlot{});
    list.logicalOrder.clear();
    list.pointCount = 0;
}

vector<PointSlot> makeDisplaySlots(const PointList& list, bool sorted) {
    if (!sorted) return list.slots;

    vector<Point> points;
    for (auto room = list.logicalOrder.rbegin();
        room != list.logicalOrder.rend(); ++room) {
        points.push_back(list.slots[*room].point);
    }

    stable_sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
        return squaredOriginDistance(a) < squaredOriginDistance(b);
    });

    vector<PointSlot> display(MAX_POINTS);
    for (size_t i = 0; i < points.size(); ++i) {
        display[i] = { points[i], true };
    }
    return display;
}

void printList(const vector<PointSlot>& slots, bool sorted) {
    for (int room = MAX_POINTS - 1; room >= 0; --room) {
        cout << room;
        if (slots[room].occupied) {
            const Point& p = slots[room].point;
            cout << " " << p.x << " " << p.y << " " << p.z;
            if (sorted) {
                cout << "    거리: " << fixed << setprecision(6)
                    << originDistance(p) << defaultfloat;
            }
        }
        cout << "\n";
    }
}

void appendPair(ostringstream& output, const vector<PointSlot>& slots,
    const PointPair& pair) {
    const Point& a = slots[pair.firstRoom].point;
    const Point& b = slots[pair.secondRoom].point;
    output << pair.firstRoom << "번 (" << a.x << ", " << a.y << ", " << a.z
        << ") <-> " << pair.secondRoom << "번 (" << b.x << ", " << b.y
        << ", " << b.z << ")    거리: " << pair.distance << "\n";
}

string makePairResult(const vector<PointSlot>& slots) {
    vector<PointPair> closest;
    vector<PointPair> farthest;
    double minimum = 0.0;
    double maximum = 0.0;
    bool firstPair = true;

    for (int a = 0; a < MAX_POINTS; ++a) {
        if (!slots[a].occupied) continue;
        for (int b = a + 1; b < MAX_POINTS; ++b) {
            if (!slots[b].occupied) continue;

            double squared = squaredPointDistance(slots[a].point, slots[b].point);
            PointPair pair{ a, b, sqrt(squared) };

            if (firstPair || squared < minimum) {
                minimum = squared;
                closest.assign(1, pair);
            }
            else if (squared == minimum) closest.push_back(pair);

            if (firstPair || squared > maximum) {
                maximum = squared;
                farthest.assign(1, pair);
            }
            else if (squared == maximum) farthest.push_back(pair);
            firstPair = false;
        }
    }

    ostringstream output;
    output << fixed << setprecision(6) << "[가장 가까운 점 조합]\n";
    for (const PointPair& pair : closest) appendPair(output, slots, pair);
    output << "\n[가장 먼 점 조합]\n";
    for (const PointPair& pair : farthest) appendPair(output, slots, pair);
    return output.str();
}

void drawScreen(const PointList& list, bool sorted, const string& output) {
    system("cls");
    vector<PointSlot> display = makeDisplaySlots(list, sorted);
    printList(display, sorted);
    if (!output.empty()) cout << "\n" << output;
    cout << "\n명령어 입력: ";
}

bool readPoint(istringstream& input, Point& point) {
    string extra;
    if (!(input >> point.x >> point.y >> point.z)) return false;
    return !(input >> extra);
}

bool changesList(char command) {
    return command == '+' || command == '-' || command == 'e'
        || command == 'd' || command == 'b' || command == 'c';
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    PointList list;
    bool sorted = false;
    string output;

    while (true) {
        drawScreen(list, sorted, output);

        string line;
        getline(cin, line);
        output.clear();

        istringstream input(line);
        char command;
        if (!(input >> command)) {
            output = "명령어를 입력해주세요.\n";
            continue;
        }
        if (command == 'q') break;

        if (sorted && changesList(command)) {
            output = "f 정렬을 먼저 해제해야 리스트를 변경할 수 있습니다.\n";
            continue;
        }

        switch (command) {
        case '+': {
            Point point;
            if (!readPoint(input, point)) output = "사용법: + x y z\n";
            else if (!addTop(list, point)) output = "리스트가 가득 찼습니다.\n";
            break;
        }
        case '-':
            if (!removeTop(list)) output = "리스트가 비어 있습니다.\n";
            break;
        case 'e': {
            Point point;
            if (!readPoint(input, point)) output = "사용법: e x y z\n";
            else if (!addBottom(list, point)) output = "리스트가 가득 찼습니다.\n";
            break;
        }
        case 'd':
            if (!removeBottom(list)) output = "리스트가 비어 있습니다.\n";
            break;
        case 'a':
            output = "저장된 점의 개수: " + to_string(list.pointCount) + "\n";
            break;
        case 'b':
            moveDown(list);
            break;
        case 'c':
            clearList(list);
            break;
        case 'f':
            sorted = !sorted;
            break;
        case 'g':
            if (list.pointCount < 2) {
                output = "거리 비교를 위해 점이 두 개 이상 필요합니다.\n";
            }
            else {
                output = makePairResult(makeDisplaySlots(list, sorted));
            }
            break;
        default:
            output = "알 수 없는 명령어입니다.\n";
            break;
        }
    }
    return 0;
}
