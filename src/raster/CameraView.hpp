#pragma once

#include "../global.hpp"
#include "../Color.hpp"
#include "Camera.hpp"
#include "Light.hpp"


namespace Raster{
    class CameraView;
}

/*
heap data inside
*/
class Raster::CameraView: public Raster::Camera{
public:
    CameraView(Raster::Color bg_color, int w, int h): Camera(bg_color, w, h){}

    void paint_shadow(std::vector<Obj::ObjSet*>& obj_set, std::vector<Raster::Light*>& lights, const Raster::Color& fill_color, float shadow_bias, bool pcf, bool paint_back, bool verbose){
        this->init_buffs();
        int i = 0;
        project_vertices(obj_set, verbose);

        for(Obj::ObjSet* obj : obj_set){
            i = 0;
            for(Obj::Triangle* triangle : obj->triangles){
                triangle->calculate_normal();
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->triangles.size()){
                        print_progress(i, obj->triangles.size(), "Triangle normal calculation");
                    }
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
        if(verbose){
            std::cout << "Triangle normal calculated" << std::endl;
        }
        for(Obj::ObjSet* obj : obj_set){
            i = 1;
            for(Obj::Triangle* triangle : obj->triangles){
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->triangles.size()){
                        print_progress(i, obj->triangles.size(), "Triangle rasterizing");
                    }
                }
                if(!paint_back && triangle->normal.value().z() < 0){
                    continue;
                }
                if(triangle->A->projected_position[2] > 0 && triangle->B->projected_position[2] > 0 && triangle->C->projected_position[2] > 0){
                    continue;
                }
                float l, r, u, d;
                l = min(triangle->A->projected_position[0], triangle->B->projected_position[0], triangle->C->projected_position[0]) - 1;
                r = max(triangle->A->projected_position[0], triangle->B->projected_position[0], triangle->C->projected_position[0]) + 1;
                u = min(triangle->A->projected_position[1], triangle->B->projected_position[1], triangle->C->projected_position[1]) - 1;
                d = max(triangle->A->projected_position[1], triangle->B->projected_position[1], triangle->C->projected_position[1]) + 1;
                maximize(l, 0);
                minimize(r, this->w - 0.9);
                maximize(u, 0);
                minimize(d, this->h - 0.9);
                if(l > r || u > d){
                    continue;
                }
                for(int y = u;y < d;y++){
                    for(int x = l;x < r;x++){
                        if(triangle->is_inside_triangle(x, y)){
                            Eigen::Vector3f bc_coord = triangle->get_barycentric_coordinate(x, y);
                            float* z_p = this->get_z_buff_trust(x, y);

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
                            Raster::Color light_sum(this->bg_color.image_color,0.1,1);
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
                                    light_color *= max(0,normal.dot(light->get_l(point)));
                                    light_sum += light_color;
                                }
                            }
                            Raster::Color texture_color = get_texture_color(fill_color,obj,triangle,bc_coord);
                            float* top_p = this->get_top_buff_trust(x, y);
                            color_assign(texture_color * light_sum, top_p);
                        }
                    }
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
    }
};
