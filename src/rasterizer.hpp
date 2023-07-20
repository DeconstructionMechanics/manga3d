#pragma once

#include "global.hpp"
#include "OBJ.hpp"

namespace Raster{
    class Light;
    class Camera;
    class Rasterizer;
}

class Raster::Light{

};

class Raster::Camera{
public:
    enum class Projection{
        ORTHOGRAPH,
        PERSPECTIVE,
        FISHEYE
    };
    Projection Projection;
    int w;
    int h;
    float fovY;

    std::vector<Eigen::Vector3f> top_buff;

};

class Raster::Rasterizer{
public:
    std::vector<Obj::Vertex*> vertices;
    std::vector<Obj::Edge*> edges;
    std::vector<Obj::Triangle*> triangles;

    std::vector<Raster::Light> lights;
    Raster::Camera camera;

};
