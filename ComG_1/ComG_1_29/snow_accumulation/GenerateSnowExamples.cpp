// Rebuild the examples using the same mesh generator used at runtime.
#include "SnowAccumulation.h"
#include <iostream>
int main() {
 try {
    snow::Settings settings;
    const snow::HeightSampler plane=[](float,float){return 0.0f;};
    const snow::HeightSampler pyramid=[&](float x,float z){return snow::PyramidHeight(x,z,settings.width,settings.depth,snow::kExamplePyramidHeight);};
    snow::WriteOBJ(snow::Build(settings,plane,1.00000000f),"snow_plane_1.obj");
    snow::WriteOBJ(snow::Build(settings,plane,2.00000000f),"snow_plane_2.obj");
    snow::WriteOBJ(snow::Build(settings,plane,3.00000000f),"snow_plane_3.obj");
    snow::WriteOBJ(snow::Build(settings,pyramid,1.00000000f),"snow_pyramid_1.obj");
    snow::WriteOBJ(snow::Build(settings,pyramid,2.00000000f),"snow_pyramid_2.obj");
    snow::WriteOBJ(snow::Build(settings,pyramid,3.00000000f),"snow_pyramid_3.obj");
    std::cout << "Created six surface-conforming snow models.\n";
    return 0;
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
}
