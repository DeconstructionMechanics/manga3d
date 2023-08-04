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
    std::function<float(Eigen::Vector3f& point_position)> get_distance;

    SMShader(std::function<float(Eigen::Vector3f& point_position)> get_distance): Shader(){
        this->do_outline = false;
        this->get_distance = get_distance;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        if(!get_distance){
            throw Manga3DException("Raster::SMShader::shade(), get_distance function pointer lost");
        }
        Eigen::Vector3f point = triangle->get_position_from_barycentric(bc_coord);
        float z = -get_distance(point);
        if(z < *z_p){
            return;
        }
        *z_p = z;
        if(verbose){
            Eigen::Vector3f origin(0,0,0);
            color_assign(fill_color * (-z / (get_distance(origin) * 3)), top_p);
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
    virtual void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat){
        throw Manga3DException("Raster::Light::config() is called, thus not doing anything.");
    }
    virtual float get_distance(Eigen::Vector3f& point_position){
        throw Manga3DException("Raster::Light::get_distance() is called, thus not doing anything.");
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
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat){
        Eigen::Vector3f lookat_ = -position;
        this->camera.config(Raster::Camera::Projection::PERSP, bg_color, w, w, fovY, position, lookat_);
    }
    float get_distance(Eigen::Vector3f& point_position){
        return (this->camera.position - point_position).norm();
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose){
        SMShader smshader(std::bind(&get_distance, this, std::placeholders::_1));
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
    void config(Raster::Color& bg_color, int w, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat){
        Eigen::Vector3f position_ = -lookat * 100;
        this->camera.config(Raster::Camera::Projection::ORTHO, bg_color, w, w, fovY, position_, lookat);
    }
    float get_distance(Eigen::Vector3f& point_position){
        return point_position.dot(this->camera.lookat_g);
    }
    void cast_shadow(std::vector<Obj::ObjSet*>& obj_set, bool verbose){
        SMShader smshader(std::bind(&get_distance, this, std::placeholders::_1));
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

