#pragma once

#include "../global.hpp"
#include "../Color.hpp"
#include "Shader.hpp"
#include "Light.hpp"

namespace Raster{
    class OutlineShader;
    class TextureShader;
    class PhoneShader;
    class DiscreteShader;
}

class Raster::OutlineShader: public Raster::Shader{
public:
    OutlineShader(int thickness, float crease_angle, int crease_thickness, Raster::Color& line_color): Shader(){
        this->do_outline = true;
        this->thickness = thickness;
        this->crease_angle = crease_angle;
        this->crease_thickness = crease_thickness;
        this->line_color = line_color;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
        if(z > 0 || z < *z_p){
            return;
        }
        *z_p = z;
        color_assign(fill_color, top_p);
    }
};

class Raster::TextureShader: public Raster::Shader{
public:
    TextureShader(): Shader(){
        this->do_outline = false;
    }

    void set_outline(int thickness, float crease_angle, int crease_thickness, Raster::Color& line_color){
        this->do_outline = true;
        this->thickness = thickness;
        this->crease_angle = crease_angle;
        this->crease_thickness = crease_thickness;
        this->line_color = line_color;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
        if(z > 0 || z < *z_p){
            return;
        }
        *z_p = z;
        Raster::Color color = get_texture_color(fill_color, obj, triangle, bc_coord);
        color_assign(color, top_p);
    }
};

Raster::Color light_reach(
    const std::vector<Raster::Light*>& lights,
    const Obj::Triangle* triangle,
    const Raster::Color& fill_color,
    const Eigen::Vector3f bc_coord,
    const float shadow_bias,
    const bool pcf){

    Eigen::Vector3f point = triangle->get_position_from_barycentric(bc_coord);
    Eigen::Vector3f normal;
    if(triangle->is_smooth){
        normal = triangle->get_normal_from_barycentric(bc_coord);
    }
    else{
        try{
            normal = triangle->normal.value();
        }
        catch(const std::bad_optional_access& e){
            throw Manga3DException("Raster::PhoneShader::shade(): triangle normal lost", e);
        }
    }
    normal.normalize();
    point += normal * shadow_bias;
    Raster::Color light_sum(fill_color.image_color, 0.1, 1);
    for(Raster::Light* light : lights){
        bool shadowed = false;
        float light_dist = -light->get_distance(point);
        Eigen::Vector3f projected_point = point;
        light->camera.projection(projected_point);
        if(!pcf){
            float* light_z = light->camera.get_z_buff(projected_point);
            if(light_z && *light_z > light_dist){
                shadowed = true;
            }
        }
        else{
            projected_point[0] -= 1;
            projected_point[1] -= 1;
            int cnt = 0;
            for(int i = 0;i < 3;i++){
                for(int j = 0;j < 3;j++){
                    float* light_z = light->camera.get_z_buff(Eigen::Vector3f(projected_point[0] + i, projected_point[1] + j, projected_point[2]));
                    if(light_z && *light_z > light_dist){
                        cnt += 1;
                    }
                }
            }
            if(cnt > 4){
                shadowed = true;
            }
        }
        if(!shadowed){
            Raster::Color light_color = (light->camera.bg_color * (light->get_I(light_dist)));
            light_color *= max(0, normal.dot(light->get_l(point)));
            light_sum += light_color;
        }
    }
    return light_sum;
}

class Raster::PhoneShader: public Raster::Shader{
public:
    std::vector<Raster::Light*>& lights;
    float shadow_bias;
    bool pcf;

    PhoneShader(std::vector<Raster::Light*>& lights, const float shadow_bias, const bool pcf): Shader(), lights(lights){
        this->do_outline = false;
        this->shadow_bias = shadow_bias;
        this->pcf = pcf;
    }

    void set_outline(int thickness, float crease_angle, int crease_thickness, Raster::Color& line_color){
        this->do_outline = true;
        this->thickness = thickness;
        this->crease_angle = crease_angle;
        this->crease_thickness = crease_thickness;
        this->line_color = line_color;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
        if(z > 0 || z < *z_p){
            return;
        }
        *z_p = z;

        Raster::Color texture_color = get_texture_color(fill_color, obj, triangle, bc_coord);
        Raster::Color result_color = light_reach(lights, triangle, fill_color, bc_coord, this->shadow_bias, this->pcf);
        color_assign(result_color, top_p);
    }
};

class Raster::DiscreteShader: public Raster::Shader{
public:
    std::vector<Raster::Light*>& lights;
    float shadow_bias;
    bool pcf;

    DiscreteShader(std::vector<Raster::Light*>& lights, const float shadow_bias, const bool pcf): Shader(), lights(lights){
        this->do_outline = false;
        this->shadow_bias = shadow_bias;
        this->pcf = pcf;
    }

    void set_outline(int thickness, float crease_angle, int crease_thickness, Raster::Color& line_color){
        this->do_outline = true;
        this->thickness = thickness;
        this->crease_angle = crease_angle;
        this->crease_thickness = crease_thickness;
        this->line_color = line_color;
    }

    void shade(const Obj::ObjSet* obj,
        const Obj::Triangle* triangle,
        const Eigen::Vector3f bc_coord,
        const Raster::Color& fill_color,
        float* z_p,
        float* top_p,
        const bool verbose){

        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
        if(z > 0 || z < *z_p){
            return;
        }
        *z_p = z;

        Raster::Color texture_color = get_texture_color(fill_color, obj, triangle, bc_coord);
        Raster::Color result_color = light_reach(lights, triangle, fill_color, bc_coord, this->shadow_bias, this->pcf);
        for(int i = 0;i < (int)result_color.image_color;i++){
            if(result_color.color[i] < 0.3){
                result_color.color[i] = 0.3;
            }
            else{
                result_color.color[i] = 1;
            }
        }
        color_assign(result_color * texture_color, top_p);
    }
};

