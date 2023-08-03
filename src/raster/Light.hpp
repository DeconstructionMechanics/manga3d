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
private:
    Light(const Light& other);
    Light& operator=(const Light& other);
public:
    int index;
    float I;
    Raster::Camera camera;
    Light(float I): I(I), camera(Raster::Color(0), 1, 1){}
    virtual void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){}
    virtual void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose = false){}
    virtual inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){ return Eigen::Vector3f(); }
};

class Raster::PointLight: public Raster::Light{
public:
    PointLight(float I): Light(I){}
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){
        Eigen::Vector3f lookat = -position;
        this->camera.config(Raster::Camera::Projection::PERSP, bg_color, w, w, fovY, position, lookat);
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose = false){
        this->camera.paint_simple(obj_set, camera.bg_color, camera.bg_color, 1, Raster::Camera::PaintSimpleOpt::SHADOW, false, verbose);
    }
    inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){
        return (this->camera.position - obj_position).normalized();
    }
};

class Raster::SunLight: public Raster::Light{
public:
    SunLight(float I): Light(I){}
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){
        Eigen::Vector3f true_position = -position * 10;
        this->camera.config(Raster::Camera::Projection::ORTHO, bg_color, w, w, fovY, true_position, position);
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose = false){
        camera.paint_simple(obj_set, camera.bg_color, camera.bg_color, 1, Raster::Camera::PaintSimpleOpt::SHADOW, false, verbose);
    }
    inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){
        return (-this->camera.lookat_g);
    }
};

