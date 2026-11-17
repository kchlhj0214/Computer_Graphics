#pragma once
// Generated from snow_accumulation.json. Change that sheet, then regenerate.
namespace snow {
struct Stage { const char* id; float minimumDepth, maximumDepth; };
static const Stage kStages[3] = {
    {"light",0.03000000f,0.08000000f},
    {"medium",0.12000000f,0.24000000f},
    {"deep",0.30000000f,0.50000000f}
};
static const float kDefaultWidth=4.00000000f,kDefaultDepth=4.00000000f;
static const float kDefaultInset=0.00200000f;
static const unsigned kDefaultSegments=32;
static const float kExamplePyramidHeight=1.60000000f;
}
