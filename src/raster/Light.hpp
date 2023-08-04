#pragma once

#include "../global.hpp"
#include "../obj/OBJ.hpp"
#include "Camera.hpp"


namespace Raster{
    class Light;
    class PointLight;
    class SunLight;

    class SMShader;
}


class Raster::SMShader: public Raster::Shader{
public:
    Eigen::Vector3f light_position;

    SMShader(Eigen::Vector3f light_position): Shader(){
        this->do_outline = false;
        this->light_position = light_position;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        Eigen::Vector3f point = triangle->get_position_from_barycentric(bc_coord);
        float z = -(point - this->light_position).norm();
        if(z < *z_p){
            return;
        }
        *z_p = z;
        if(verbose){
            color_assign(fill_color * (-z / (light_position.norm() * 3)), top_p);
        }
    }
};


class Raster::Light{
private:
    Light(const Light& other);
    Light& operator=(const Light& other);
public:
    int index;
    float I;
    Raster::Camera camera;
    Light(float I): I(I), camera(Raster::Color(0), 1, 1){}
    virtual void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){
        throw Manga3DException("Raster::Light::config() is called, thus not doing anything.");
    }
    virtual void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose){
        throw Manga3DException("Raster::Light::cast_shadow() is called, thus not doing anything.");
    }
    virtual inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){
        throw Manga3DException("Raster::Light::get_l() is called, thus not doing anything.");
    }
    virtual inline float get_I(float distance){
        throw Manga3DException("Raster::Light::get_I() is called, thus not doing anything.");
    }
};

class Raster::PointLight: public Raster::Light{
public:
    PointLight(float I): Light(I){}
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){
        Eigen::Vector3f lookat = -position;
        this->camera.config(Raster::Camera::Projection::PERSP, bg_color, w, w, fovY, position, lookat);
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose){
        SMShader smshader(this->camera.position);
        this->camera.paint(smshader, obj_set, camera.bg_color, true, verbose);
        if(verbose){
            std::cout << "Raster::PointLight::cast_shadow() complete" << std::endl;
        }
    }
    inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){
        return (this->camera.position - obj_position).normalized();
    }
    inline float get_I(float distance){ return this->I / (distance * distance); }
};

class Raster::SunLight: public Raster::Light{
public:
    SunLight(float I): Light(I){}
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position){
        Eigen::Vector3f true_position = -position;
        this->camera.config(Raster::Camera::Projection::ORTHO, bg_color, w, w, fovY, true_position, position);
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose){
        SMShader smshader(this->camera.position);
        this->camera.paint(smshader, obj_set, camera.bg_color, true, verbose);
        if(verbose){
            std::cout << "Raster::SunLight::cast_shadow() complete" << std::endl;
        }
    }
    inline Eigen::Vector3f get_l(Eigen::Vector3f& obj_position){
        return (-this->camera.lookat_g);
    }
    inline float get_I(float distance){ return this->I; }
};

