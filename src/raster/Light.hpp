#pragma once

#include "../global.hpp"
#include "../obj/OBJ.hpp"
#include "Camera.hpp"


namespace Raster{
    class Light;
    class PointLight;
    class SunLight;
}


class Raster::Light{
public:
    float I;
    Raster::Camera camera;
    Light(float I): I(I), camera(Raster::Color(0), 1, 1){}
    virtual void cast_shadow(std::vector<Obj::ObjSet*>& obj_set){}
    virtual inline Eigen::Vector3f& get_l(Eigen::Vector3f& obj_position){}
};

class Raster::PointLight: public Raster::Light{
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set){}
    inline Eigen::Vector3f& get_l(Eigen::Vector3f& obj_position){}
};

class Raster::SunLight: public Raster::Light{
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set){}
    inline Eigen::Vector3f& get_l(Eigen::Vector3f& obj_position){}
};

