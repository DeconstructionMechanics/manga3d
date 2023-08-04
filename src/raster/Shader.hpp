#pragma once

#include "../global.hpp"
#include "../Color.hpp"


class Shader{
    bool do_outline;

    virtual void shade(const Obj::Triangle* triangle,
        const Raster::Color& fill_color,
        const Eigen::Vector3f bc_coord,
        const float* z_p,
        const float* top_p,
        const bool verbose){}
}

