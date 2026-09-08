#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <iomanip>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <windows.h>

#pragma execution_character_set("utf-8")

using namespace std;

struct Vertex {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int lineNumber = 0;
    bool valid = false;
};

struct Texture {
    double s = 0.0;
    double t = 0.0;
    int lineNumber = 0;
    bool valid = false;
};

struct FaceIndex {
    int vertex = 0;
    int texture = 0;
    bool hasTexture = false;
};

struct Face {
    array<FaceIndex, 3> indices;
    int number = 0;
    int lineNumber = 0;
    bool valid = true;
};

struct ParseError {
    int lineNumber = 0;
    string message;
};

struct ModelData {
    vector<Vertex> vertices;
    vector<Texture> textures;
    vector<Face> faces;
    vector<ParseError> errors;
    bool hasDuplicateVertex = false;
};

string trim(const string& text) {
    size_t first = 0;
    while (first < text.size()
        && isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }

    size_t last = text.size();
    while (last > first
        && isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }
    return text.substr(first, last - first);
}

void addError(ModelData& model, int lineNumber, const string& message) {
    model.errors.push_back({ lineNumber, message });
}

bool hasOnlyNumberCharacters(const string& text, bool allowSlash) {
    for (unsigned char character : text) {
        if (isdigit(character) || isspace(character)
            || character == '+' || character == '-'
            || character == '.' || character == 'e' || character == 'E') {
            continue;
        }
        if (allowSlash && character == '/') continue;
        return false;
    }
    return true;
}

bool sameVertex(const Vertex& first, const Vertex& second) {
    return first.x == second.x
        && first.y == second.y
        && first.z == second.z;
}

template <size_t N>
bool parseCoordinates(const string& values, array<double, N>& coordinates) {
    istringstream input(values);
    string token;
    for (double& coordinate : coordinates) {
        if (!(input >> token)) return false;
        istringstream number(token);
        if (!(number >> coordinate) || !number.eof() || !isfinite(coordinate)) {
            return false;
        }
    }
    return !(input >> token);
}

bool isDegenerateTriangle(const Vertex& a, const Vertex& b, const Vertex& c) {
    double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
    double vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
    double uLength = hypot(ux, uy, uz);
    double vLength = hypot(vx, vy, vz);
    if (uLength == 0.0 || vLength == 0.0) return true;

    ux /= uLength; uy /= uLength; uz /= uLength;
    vx /= vLength; vy /= vLength; vz /= vLength;
    double crossLength = hypot(uy * vz - uz * vy,
        uz * vx - ux * vz, ux * vy - uy * vx);
    constexpr double angularTolerance = 1e-12;
    return crossLength <= angularTolerance;
}

bool parsePositiveIndex(const string& text, int& value) {
    if (text.empty()) return false;

    size_t usedLength = 0;
    try {
        long long parsed = stoll(text, &usedLength);
        if (usedLength != text.size() || parsed < 1
            || parsed > 2147483647LL) {
            return false;
        }
        value = static_cast<int>(parsed);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool parseFaceIndex(const string& token, FaceIndex& result) {
    size_t slash = token.find('/');
    if (slash == string::npos) {
        result.hasTexture = false;
        return parsePositiveIndex(token, result.vertex);
    }

    if (token.find('/', slash + 1) != string::npos) return false;

    string vertexText = token.substr(0, slash);
    string textureText = token.substr(slash + 1);
    result.hasTexture = true;
    return parsePositiveIndex(vertexText, result.vertex)
        && parsePositiveIndex(textureText, result.texture);
}

void parseVertexLine(const string& values, int lineNumber, ModelData& model) {
    model.vertices.emplace_back();
    Vertex& vertex = model.vertices.back();
    vertex.lineNumber = lineNumber;
    if (!hasOnlyNumberCharacters(values, false)) {
        addError(model, lineNumber, "정점 데이터에 허용되지 않는 문자가 있습니다.");
        return;
    }

    array<double, 3> coordinates;
    if (!parseCoordinates(values, coordinates)) {
        addError(model, lineNumber, "v에는 x, y, z 세 개의 숫자가 필요합니다.");
        return;
    }
    vertex.x = coordinates[0];
    vertex.y = coordinates[1];
    vertex.z = coordinates[2];
    if (!isfinite(vertex.x) || !isfinite(vertex.y) || !isfinite(vertex.z)
        || vertex.x < -1.0 || vertex.x > 1.0
        || vertex.y < -1.0 || vertex.y > 1.0
        || vertex.z < -1.0 || vertex.z > 1.0) {
        addError(model, lineNumber, "정점 좌표는 모두 -1.0 이상 1.0 이하여야 합니다.");
        return;
    }

    for (const Vertex& saved : model.vertices) {
        if (saved.valid && sameVertex(saved, vertex)) {
            model.hasDuplicateVertex = true;
            addError(model, lineNumber,
                "정점 좌표가 " + to_string(saved.lineNumber) + "번 줄과 중복됩니다.");
            break;
        }
    }
    vertex.valid = true;
}

void parseTextureLine(const string& values, int lineNumber, ModelData& model) {
    model.textures.emplace_back();
    Texture& texture = model.textures.back();
    texture.lineNumber = lineNumber;
    if (!hasOnlyNumberCharacters(values, false)) {
        addError(model, lineNumber, "텍스처 데이터에 허용되지 않는 문자가 있습니다.");
        return;
    }

    array<double, 2> coordinates;
    if (!parseCoordinates(values, coordinates)) {
        addError(model, lineNumber, "vt에는 s, t 두 개의 숫자가 필요합니다.");
        return;
    }
    texture.s = coordinates[0];
    texture.t = coordinates[1];
    if (!isfinite(texture.s) || !isfinite(texture.t)
        || texture.s < 0.0 || texture.s > 1.0
        || texture.t < 0.0 || texture.t > 1.0) {
        addError(model, lineNumber, "텍스처 좌표는 모두 0.0 이상 1.0 이하여야 합니다.");
        return;
    }

    texture.valid = true;
}

void parseFaceLine(const string& values, int lineNumber,
    int faceNumber, ModelData& model) {
    if (!hasOnlyNumberCharacters(values, true)) {
        addError(model, lineNumber, "면 데이터에 허용되지 않는 문자가 있습니다.");
        return;
    }

    istringstream input(values);
    vector<string> tokens;
    string token;
    while (input >> token) tokens.push_back(token);

    if (tokens.size() != 3) {
        addError(model, lineNumber, "삼각형 면은 정확히 세 개의 꼭짓점이 필요합니다.");
        return;
    }

    Face face;
    face.number = faceNumber;
    face.lineNumber = lineNumber;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (!parseFaceIndex(tokens[i], face.indices[i])) {
            addError(model, lineNumber,
                "면 인덱스 '" + tokens[i] + "'의 형식이 잘못되었습니다.");
            return;
        }
    }

    bool usesTexture = face.indices[0].hasTexture;
    for (const FaceIndex& index : face.indices) {
        if (index.hasTexture != usesTexture) {
            addError(model, lineNumber,
                "한 면에서 정점 전용 형식과 정점/텍스처 형식을 섞을 수 없습니다.");
            return;
        }
    }
    model.faces.push_back(face);
}

void parseDataFile(istream& input, ModelData& model) {
    string line;
    int lineNumber = 0;
    int faceNumber = 0;

    while (getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.size() >= 3
            && static_cast<unsigned char>(line[0]) == 0xEF
            && static_cast<unsigned char>(line[1]) == 0xBB
            && static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);
        }

        size_t comment = line.find('#');
        if (comment != string::npos) line.erase(comment);
        line = trim(line);
        if (line.empty()) continue;

        istringstream lineInput(line);
        string command;
        lineInput >> command;
        string values;
        getline(lineInput, values);
        values = trim(values);

        if (command == "v") {
            parseVertexLine(values, lineNumber, model);
        }
        else if (command == "vt") {
            parseTextureLine(values, lineNumber, model);
        }
        else if (command == "f") {
            ++faceNumber;
            parseFaceLine(values, lineNumber, faceNumber, model);
        }
        else {
            addError(model, lineNumber,
                "허용되지 않는 데이터 종류 '" + command + "'입니다.");
        }
    }
}

void validateFaces(ModelData& model) {
    for (Face& face : model.faces) {
        for (const FaceIndex& index : face.indices) {
            if (index.vertex > static_cast<int>(model.vertices.size())) {
                addError(model, face.lineNumber,
                    "정점 인덱스 값 " + to_string(index.vertex) + "이(가) 범위를 벗어났습니다.");
                face.valid = false;
            }
            else if (!model.vertices[index.vertex - 1].valid) {
                addError(model, face.lineNumber,
                    "정점 인덱스 값 " + to_string(index.vertex) + "이(가) 잘못된 정점 데이터를 참조합니다.");
                face.valid = false;
            }
            if (index.hasTexture
                && index.texture > static_cast<int>(model.textures.size())) {
                addError(model, face.lineNumber,
                    "텍스처 인덱스 값 " + to_string(index.texture) + "이(가) 범위를 벗어났습니다.");
                face.valid = false;
            }
            else if (index.hasTexture && !model.textures[index.texture - 1].valid) {
                addError(model, face.lineNumber,
                    "텍스처 인덱스 값 " + to_string(index.texture) + "이(가) 잘못된 텍스처 데이터를 참조합니다.");
                face.valid = false;
            }
        }

        int first = face.indices[0].vertex;
        int second = face.indices[1].vertex;
        int third = face.indices[2].vertex;
        if (first == second || first == third || second == third) {
            addError(model, face.lineNumber,
                "한 삼각형에서 같은 정점 인덱스를 두 번 이상 사용할 수 없습니다.");
            face.valid = false;
        }

        if (!face.valid) continue;
        const Vertex& v1 = model.vertices[first - 1];
        const Vertex& v2 = model.vertices[second - 1];
        const Vertex& v3 = model.vertices[third - 1];
        if (sameVertex(v1, v2) || sameVertex(v1, v3) || sameVertex(v2, v3)) {
            addError(model, face.lineNumber,
                "서로 다른 꼭짓점 인덱스가 동일한 정점 좌표를 가리킵니다.");
            face.valid = false;
        }
        else if (isDegenerateTriangle(v1, v2, v3)) {
            addError(model, face.lineNumber,
                "세 정점이 일직선 위에 있거나 거의 일직선이어서 삼각형을 구성할 수 없습니다.");
            face.valid = false;
        }
    }
}

void writeVertex(ostream& output, const Vertex& vertex) {
    output << "(" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")";
}

void writeTexture(ostream& output, const Texture& texture) {
    output << "(" << texture.s << ", " << texture.t << ")";
}

void writeReport(ostream& output, const string& inputName,
    const ModelData& model) {
    output << fixed << setprecision(6);
    output << "Input file: " << inputName << "\n";
    output << "Vertex count: " << model.vertices.size() << "\n";
    output << "Texture count: " << model.textures.size() << "\n";
    output << "Parsed face count: " << model.faces.size() << "\n\n";

    for (const Face& face : model.faces) {
        if (!face.valid) continue;

        output << "Face " << face.number << " ("
            << face.indices[0].vertex << ", "
            << face.indices[1].vertex << ", "
            << face.indices[2].vertex << "):\n";
        output << "vertex ";
        for (const FaceIndex& index : face.indices) {
            writeVertex(output, model.vertices[index.vertex - 1]);
            output << " ";
        }
        output << "\n";

        if (face.indices[0].hasTexture) {
            output << "texture ";
            for (const FaceIndex& index : face.indices) {
                writeTexture(output, model.textures[index.texture - 1]);
                output << " ";
            }
            output << "\n";
        }
        else {
            output << "texture: none\n";
        }
        output << "\n";
    }

    if (!model.hasDuplicateVertex) {
        output << "No duplicate vertex value\n";
    }
    else {
        output << "Duplicate vertex value found\n";
    }

    if (model.errors.empty()) {
        output << "No errors\n";
    }
    else {
        output << "\nErrors: " << model.errors.size() << "\n";
        for (const ParseError& error : model.errors) {
            output << "Line " << error.lineNumber << ": " << error.message << "\n";
        }
    }
}

filesystem::path utf8Path(const string& text) {
    return filesystem::path(u8string(text.begin(), text.end()));
}

string findInputPath(const string& fileName) {
    const array<string, 4> candidates = {
        fileName,
        "ComG_WarmingUp6/" + fileName,
        "../../" + fileName,
        "../../../ComG_WarmingUp6/" + fileName
    };

    for (const string& path : candidates) {
        ifstream test(utf8Path(path));
        if (test.is_open()) return path;
    }
    return "";
}

string parentPath(const string& path) {
    size_t separator = path.find_last_of("/\\");
    if (separator == string::npos) return "";
    return path.substr(0, separator + 1);
}

bool processFile(const string& inputName) {
    string inputPath = findInputPath(inputName);
    if (inputPath.empty()) {
        cout << "[ERROR] " << inputName << " 파일을 찾을 수 없습니다.\n";
        return false;
    }

    ifstream input(utf8Path(inputPath));
    if (!input.is_open()) {
        cout << "[ERROR] 입력 파일을 열 수 없습니다.\n";
        return false;
    }
    string sourceText;
    string sourceLine;
    while (getline(input, sourceLine)) {
        sourceText += sourceLine;
        if (!input.eof()) sourceText += '\n';
    }
    if (input.bad()) {
        cout << "[ERROR] 입력 파일을 읽는 중 오류가 발생했습니다.\n";
        return false;
    }
    input.close();
    // Keep comments and blank lines; omit only the invisible UTF-8 BOM for display.
    size_t displayStart = sourceText.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
    cout << "\n불러온 데이터: " << inputPath << "\n";
    cout << "---------- 원본 데이터 시작 ----------\n";
    cout << sourceText.substr(displayStart);
    if (sourceText.size() > displayStart && sourceText.back() != '\n') cout << '\n';
    cout << "---------- 원본 데이터 끝 ----------\n\n";

    istringstream source(sourceText);
    ModelData model;
    parseDataFile(source, model);
    validateFaces(model);

    if (!model.errors.empty()) {
        writeReport(cout, inputName, model);
        cout << "검사 실패: 오류를 수정한 뒤 다시 실행하세요. 결과는 저장하지 않습니다.\n";
        return false;
    }

    cout << "검사 통과. 저장할 파일명 또는 경로를 입력하세요: ";
    string resultName;
    if (!getline(cin, resultName) || (resultName = trim(resultName)).empty()) {
        cout << "[ERROR] 저장할 파일명이 입력되지 않았습니다.\n";
        return false;
    }
    string resultPath = resultName.find_first_of("/\\") == string::npos
        ? parentPath(inputPath) + resultName : resultName;
    const auto outputPath = utf8Path(resultPath);
    error_code pathError;
    if (filesystem::equivalent(utf8Path(inputPath), outputPath, pathError)) {
        cout << "[ERROR] 입력 파일과 같은 파일에 결과를 저장할 수 없습니다.\n";
        return false;
    }
    ofstream result(outputPath, ios::binary);
    if (!result.is_open()) {
        cout << "[ERROR] " << resultPath << " 파일을 만들 수 없습니다.\n";
        return false;
    }

    result << "\xEF\xBB\xBF";
    writeReport(result, inputName, model);
    result.close();
    if (!result) {
        cout << "[ERROR] 결과 파일을 저장하는 중 오류가 발생했습니다.\n";
        return false;
    }

    cout << "결과 저장 위치: " << resultPath << "\n\n";
    cout << "저장된 결과:\n";
    writeReport(cout, inputName, model);
    return true;
}

int main() {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    cout << "읽을 데이터 파일명 또는 경로를 입력하세요: ";
    string inputName;
    if (!getline(cin, inputName) || (inputName = trim(inputName)).empty()) {
        cout << "[ERROR] 읽을 파일명이 입력되지 않았습니다.\n";
        return 1;
    }
    return processFile(inputName) ? 0 : 1;
}
