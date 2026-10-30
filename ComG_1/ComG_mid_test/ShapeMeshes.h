#pragma once
#include "TankScene.h"

// MakeMesh(종류,원둘레분할,구위도분할): 중심 원점, 전체 크기 1인 삼각형 정점 목록.
// 크기·위치·색상·회전은 공통 셰이더 uniform으로 처리하므로 도형별 셰이더는 없습니다.
inline std::vector<glm::vec3> MakeMesh(ShapeKind kind, int slices, int stacks)
{
    if (slices<3 || stacks<2)
        throw std::runtime_error("Mesh needs at least 3 slices and 2 stacks");
    std::vector<glm::vec3> vertices;

    // triangle(a,b,c): 삼각형 하나를 버퍼에 추가하는 지역 함수입니다.
    auto triangle=[&](glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        vertices.push_back(a);
        vertices.push_back(b);
        vertices.push_back(c);
    };

    if (kind==ShapeKind::Box || kind==ShapeKind::Pyramid)
    {
        glm::vec3 p[]={
            {-.5f,-.5f,-.5f}, {.5f,-.5f,-.5f}, {.5f,.5f,-.5f}, {-.5f,.5f,-.5f},
            {-.5f,-.5f,.5f}, {.5f,-.5f,.5f}, {.5f,.5f,.5f}, {-.5f,.5f,.5f}
        };
        if (kind==ShapeKind::Box)
        {
            int indices[]={0,2,1,0,3,2,4,5,6,4,6,7,0,4,7,0,7,3,
                1,2,6,1,6,5,3,7,6,3,6,2,0,1,5,0,5,4};
            for (int index : indices)
                vertices.push_back(p[index]);
        }
        else
        {
            glm::vec3 top(0,.5f,0);
            triangle(p[0],top,p[1]);
            triangle(p[1],top,p[5]);
            triangle(p[5],top,p[4]);
            triangle(p[4],top,p[0]);
            triangle(p[0],p[1],p[5]);
            triangle(p[0],p[5],p[4]);
        }
    }
    else if (kind==ShapeKind::Sphere)
    {
        // spherePoint: 위도/경도에서 반지름 0.5인 구의 정점을 계산합니다.
        auto spherePoint=[](float latitude, float longitude)
        {
            return .5f*glm::vec3(glm::cos(latitude)*glm::cos(longitude),
                glm::sin(latitude),glm::cos(latitude)*glm::sin(longitude));
        };
        for (int y=0; y<stacks; ++y)
        {
            float v0=-glm::half_pi<float>()+glm::pi<float>()*y/stacks;
            float v1=-glm::half_pi<float>()+glm::pi<float>()*(y+1)/stacks;
            for (int x=0; x<slices; ++x)
            {
                float u0=glm::two_pi<float>()*x/slices;
                float u1=glm::two_pi<float>()*(x+1)/slices;
                glm::vec3 a=spherePoint(v0,u0), b=spherePoint(v0,u1);
                glm::vec3 c=spherePoint(v1,u0), d=spherePoint(v1,u1);
                if (y>0)
                    triangle(a,c,b);
                if (y<stacks-1)
                    triangle(b,c,d);
            }
        }
    }
    else
    {
        for (int i=0; i<slices; ++i)
        {
            float a=glm::two_pi<float>()*i/slices;
            float b=glm::two_pi<float>()*(i+1)/slices;
            glm::vec3 p(.5f*glm::cos(a),-.5f,.5f*glm::sin(a));
            glm::vec3 q(.5f*glm::cos(b),-.5f,.5f*glm::sin(b));
            triangle({0,-.5f,0},p,q);
            if (kind==ShapeKind::Cone)
                triangle(p,{0,.5f,0},q);
            else
            {
                glm::vec3 r(p.x,.5f,p.z), s(q.x,.5f,q.z);
                triangle(p,r,q);
                triangle(q,r,s);
                triangle({0,.5f,0},s,r);
            }
        }
    }
    return vertices;
}
