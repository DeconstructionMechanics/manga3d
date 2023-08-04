#pragma once

#include "../global.hpp"
#include "../Color.hpp"
#include "Camera.hpp"
#include "Light.hpp"
#include "ShaderLight.hpp"


namespace Raster{
    class CameraView;
}

/*
heap data inside
*/
class Raster::CameraView: public Raster::Camera{
public:
    CameraView(Raster::Color bg_color, int w, int h): Camera(bg_color, w, h){}

    enum class PaintOpt{
        PHONESHADING,
        MANGASHADING
    };
    void paint(const PaintOpt opt, const std::vector<Obj::ObjSet*>& obj_set, const std::vector<Raster::Light*>& lights, const Raster::Color& fill_color, const float shadow_bias, const bool pcf, const bool paint_back, const bool verbose){
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
                        if(!triangle->is_inside_triangle(x, y)){
                            continue;
                        }
                        Eigen::Vector3f bc_coord = triangle->get_barycentric_coordinate(x, y);
                        float* z_p = this->get_z_buff_trust(x, y);
                        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
                        if(z > 0 || z < *z_p){
                            continue;
                        }
                        *z_p = z;

                        Raster::Color texture_color = get_texture_color(fill_color, obj, triangle, bc_coord);
                        Raster::Color result_color = phone_shader(triangle,lights,fill_color,shadow_bias,pcf,bc_coord,texture_color);
                        float* top_p = this->get_top_buff_trust(x, y);
                        color_assign(result_color, top_p);
                    }
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
    }
};
