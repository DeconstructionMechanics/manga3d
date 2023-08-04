#pragma once

#include "../global.hpp"
#include "../Color.hpp"
#include "Light.hpp"


Raster::Color phone_shader(const Obj::Triangle* triangle, const std::vector<Raster::Light*>& lights, const Raster::Color& fill_color, const float shadow_bias, const bool pcf, const Eigen::Vector3f bc_coord, const Raster::Color& texture_color){
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
            throw Manga3DException("Raster::CameraView::paint_shadow(): triangle normal lost", e);
        }
    }
    normal.normalize();
    point += normal * shadow_bias;
    Raster::Color light_sum(fill_color.image_color, 0.1, 1);
    for(Raster::Light* light : lights){
        bool shadowed = false;
        float light_dist = -(point - light->camera.position).norm();
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
    return light_sum;//texture_color * light_sum;
}


