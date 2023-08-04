#pragma once

#include "../global.hpp"
#include "../Color.hpp"

namespace Raster{
    class Shader;
}

class Raster::Shader{
public:
    bool do_outline;
    std::optional<int> thickness;
    std::optional<float> crease_angle;
    std::optional<int> crease_thickness;
    std::optional<Raster::Color> line_color;

    virtual void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        throw Manga3DException("Raster::Shader shade() is called, thus not doing anything.");
    }

    virtual void post_shade(float* top_buff){}
};

