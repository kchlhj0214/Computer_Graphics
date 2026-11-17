#pragma once
// A height-field snow blanket for flat, sloping, pyramid and custom surfaces.
// No graphics, JSON, or math-library dependency. C++11 or later.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>
#include "SnowAccumulationStages.h"

namespace snow {
struct Vec3 { float x, y, z; };
struct Vertex { Vec3 position, normal; float u, v; };
struct Mesh { std::vector<Vertex> vertices; std::vector<std::uint32_t> indices; };
struct Settings {
    float width = kDefaultWidth, depth = kDefaultDepth;
    unsigned segments = kDefaultSegments;
    float undersideInset = kDefaultInset;
};
using HeightSampler = std::function<float(float x, float z)>;
inline Vec3 Normalized(Vec3 a) {
    const float length = std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);
    return {a.x/length,a.y/length,a.z/length};
}
inline float PyramidHeight(float x, float z, float width, float depth,
                           float height, float baseY = 0.0f) {
    if (!(width>0 && depth>0 && height>=0)) throw std::invalid_argument("Invalid pyramid size");
    return baseY+height*std::max(0.0f,1.0f-std::max(std::fabs(2*x/width),std::fabs(2*z/depth)));
}
inline float Sample(const HeightSampler& surface, float x, float z) {
    const float y=surface(x,z);
    if (!std::isfinite(y)) throw std::invalid_argument("Surface height must be finite");
    return y;
}
inline float Mix(float a,float b,float t) { return a+(b-a)*t; }
// amount: 0=no snow, 1=light, 2=medium, 3=deep; fractions blend the stages.
// All positive amounts use exactly the same vertex/index layout for the same Settings.
inline Mesh Build(const Settings& s, const HeightSampler& surface, float amount) {
    if (!surface || !std::isfinite(s.width) || !std::isfinite(s.depth) ||
        !std::isfinite(s.undersideInset) || !std::isfinite(amount) ||
        s.width<=0 || s.depth<=0 || s.segments<2 || s.segments>512 || s.undersideInset<0)
        throw std::invalid_argument("Invalid snow settings");
    Mesh mesh;
    if (amount<=0) return mesh;
    amount=std::min(3.0f,amount);
    const int upper=static_cast<int>(std::ceil(amount)), lower=upper-1;
    const float t=amount-lower;
    const float low=Mix(lower==0?0:kStages[lower-1].minimumDepth,kStages[upper-1].minimumDepth,t);
    const float high=Mix(lower==0?0:kStages[lower-1].maximumDepth,kStages[upper-1].maximumDepth,t);
    const unsigned count=s.segments+1, grid=count*count;
    std::vector<float> base(grid),top(grid);
    const float dx=s.width/s.segments,dz=s.depth/s.segments;
    const auto index=[&](unsigned x,unsigned z) { return z*count+x; };
    const auto X=[&](unsigned i) { return -.5f*s.width+i*dx; };
    const auto Z=[&](unsigned j) { return -.5f*s.depth+j*dz; };
    for (unsigned j=0;j<count;++j) for (unsigned i=0;i<count;++i)
        base[index(i,j)]=Sample(surface,X(i),Z(j));
    // Periodic, deterministic relief stays aligned between all three stages.
    const float pi=3.14159265358979323846f;
    for (unsigned j=0;j<count;++j) for (unsigned i=0;i<count;++i) {
        const float x=X(i),z=Z(j),u=float(i)/s.segments,v=float(j)/s.segments;
        // Estimate the supporting surface slope at boundary vertices as well.
        const float sx=(Sample(surface,x+dx,z)-Sample(surface,x-dx,z))/(2*dx);
        const float sz=(Sample(surface,x,z+dz)-Sample(surface,x,z-dz))/(2*dz);
        const float slope=1/std::sqrt(1+sx*sx+sz*sz);
        const float ripple=.5f+.22f*std::cos(2*pi*u)*std::sin(2*pi*v)
                            +.18f*std::sin(4*pi*u)*std::cos(2*pi*v)
                            +.10f*std::cos(2*pi*(u+v));
        const auto a=index(i,j);
        top[a]=base[a]+Mix(low,high,ripple)*(.45f+.55f*slope);
    }
    const auto heightNormal=[&](const std::vector<float>& h,unsigned i,unsigned j) {
        const unsigned i0=i?i-1:0,i1=std::min(i+1,s.segments);
        const unsigned j0=j?j-1:0,j1=std::min(j+1,s.segments);
        const float sx=(h[index(i1,j)]-h[index(i0,j)])/((i1-i0)*dx);
        const float sz=(h[index(i,j1)]-h[index(i,j0)])/((j1-j0)*dz);
        return Normalized({-sx,1,-sz});
    };
    mesh.vertices.reserve(2*grid+16*s.segments);
    for (int layer=0;layer<2;++layer) for(unsigned j=0;j<count;++j) for(unsigned i=0;i<count;++i) {
        Vec3 n=heightNormal(layer==0?top:base,i,j);
        if(layer==1)n={-n.x,-n.y,-n.z};
        const float y=layer==0?top[index(i,j)]:base[index(i,j)]-s.undersideInset;
        mesh.vertices.push_back({{X(i),y,Z(j)},n,float(i)/s.segments,1-float(j)/s.segments});
    }
    const auto triangle=[&](unsigned a,unsigned b,unsigned c) {
        mesh.indices.push_back(a);mesh.indices.push_back(b);mesh.indices.push_back(c);
    };
    for(unsigned j=0;j<s.segments;++j)for(unsigned i=0;i<s.segments;++i) {
        const unsigned a=index(i,j),b=index(i+1,j),c=index(i,j+1),d=index(i+1,j+1);
        // Split along the pyramid ridges; layout depends on grid position, not snow amount.
        const bool diagonal=(X(i)+dx*.5f)*(Z(j)+dz*.5f)>=0;
        if(diagonal){triangle(a,d,b);triangle(a,c,d);triangle(a+grid,b+grid,d+grid);triangle(a+grid,d+grid,c+grid);}
        else {triangle(a,c,b);triangle(b,c,d);triangle(a+grid,b+grid,c+grid);triangle(b+grid,d+grid,c+grid);}
    }
    std::vector<unsigned> ring;
    for(unsigned i=0;i<s.segments;++i)ring.push_back(index(i,0));
    for(unsigned j=0;j<s.segments;++j)ring.push_back(index(s.segments,j));
    for(unsigned i=s.segments;i>0;--i)ring.push_back(index(i,s.segments));
    for(unsigned j=s.segments;j>0;--j)ring.push_back(index(0,j));
    for(unsigned k=0;k<ring.size();++k) {
        const unsigned a=ring[k],b=ring[(k+1)%ring.size()],start=unsigned(mesh.vertices.size());
        const Vec3 p=mesh.vertices[a].position,q=mesh.vertices[b].position;
        const Vec3 side=Normalized({q.z-p.z,0,p.x-q.x});
        for(unsigned v : {a,b,a+grid,b+grid}) {
            Vertex copy=mesh.vertices[v];copy.normal=side;mesh.vertices.push_back(copy);
        }
        triangle(start,start+1,start+2);triangle(start+1,start+3,start+2);
    }
    return mesh;
}
inline void WriteOBJ(const Mesh& mesh,const std::string& filename,
                     const std::string& materialFile="snow_accumulation.mtl") {
    std::ofstream out(filename);
    if(!out)throw std::runtime_error("Cannot write snow OBJ");
    out<<"# Surface-conforming snow blanket; Y up; v/vt/vn triangles\nmtllib "<<materialFile<<"\no snow_cover\n"<<std::fixed<<std::setprecision(8);
    for(const auto& v:mesh.vertices)out<<"v "<<v.position.x<<' '<<v.position.y<<' '<<v.position.z<<'\n';
    for(const auto& v:mesh.vertices)out<<"vt "<<v.u<<' '<<v.v<<'\n';
    for(const auto& v:mesh.vertices)out<<"vn "<<v.normal.x<<' '<<v.normal.y<<' '<<v.normal.z<<'\n';
    out<<"usemtl snow\ns 1\n";
    for(std::size_t i=0;i<mesh.indices.size();i+=3) {
        out<<'f';for(int k=0;k<3;++k){const auto n=mesh.indices[i+k]+1;out<<' '<<n<<'/'<<n<<'/'<<n;}out<<'\n';
    }
    if(!out)throw std::runtime_error("Snow OBJ write failed");
}
} // namespace snow
