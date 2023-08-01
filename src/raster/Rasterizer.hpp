#pragma once

#include "../global.hpp"
#include "../obj/OBJ.hpp"
#include "../Color.hpp"
#include "Camera.hpp"
#include "Light.hpp"

namespace Raster{
    class Rasterizer;
}


/*
heap pointer inside member
*/
class Raster::Rasterizer{
public:

    std::vector<Obj::ObjSet*> obj_set;

    std::vector<Raster::Light> lights;
    Raster::Camera camera;


    /*
    new Obj::ObjSet is allocated on the heap
        use `~ObjSet()` to delete them
    */
    inline void load_obj(const std::string obj_path, const std::string tex_path = ""){
        obj_set.push_back(new Obj::ObjSet(obj_path, tex_path));
    }
    Rasterizer(Raster::Color bg_color = Raster::Color(0, 0)): camera(bg_color, 1, 1){}
    Rasterizer(const std::string obj_path, const std::string tex_path = "", Raster::Color bg_color = Raster::Color(0, 0)): camera(bg_color, 1, 1){
        load_obj(obj_path, tex_path);
    }
    ~Rasterizer(){
        for(Obj::ObjSet* obj : this->obj_set){
            if(obj){
                obj->clear_heap();
                delete obj;
            }
        }
        this->obj_set.clear();
    }

    void project_vertices(bool verbose = false){
        for(Obj::ObjSet* obj : this->obj_set){
            int i = 0;
            for(Obj::Vertex* vertex : obj->vertices){
                vertex->projected_position = vertex->position;
                camera.projection(vertex->projected_position);
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->vertices.size()){
                        print_progress(i, obj->vertices.size(), "Project vertex");
                    }
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
        if(verbose){
            std::cout << "End project vertex" << std::endl;
        }
    }


    void paint_line_simple(const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Raster::Color& color, const int thickness = 2){
        int dim = 0;
        if(std::abs(a[1] - b[1]) > std::abs(a[0] - b[0])){
            dim = 1;
        };
        if(equal(a[dim], b[dim])){
            return;
        }
        Eigen::Vector3f leftp;
        Eigen::Vector3f rightp;
        if(a[dim] < b[dim]){
            leftp = a;
            rightp = b;
        }
        else{
            leftp = b;
            rightp = a;
        }
        Eigen::Vector3f i = (rightp - leftp).normalized();
        float x2y2 = 1 - i[2] * i[2];
        if(x2y2 == 0){
            return;
        }
        i /= std::sqrt(x2y2);
        for(;leftp[dim] < rightp[dim] + 0.5;leftp += i){
            std::vector<Eigen::Vector3f> positions;
            positions.push_back(leftp);
            if(thickness > 1){
                positions.push_back(Eigen::Vector3f(leftp[0] + 1, leftp[1], leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0], leftp[1] + 1, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] + 1, leftp[1] + 1, leftp[2]));
            }
            if(thickness > 2){
                positions.push_back(Eigen::Vector3f(leftp[0] - 1, leftp[1], leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] - 1, leftp[1] + 1, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0], leftp[1] - 1, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] + 1, leftp[1] - 1, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] + 2, leftp[1], leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] + 2, leftp[1] + 1, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0], leftp[1] + 2, leftp[2]));
                positions.push_back(Eigen::Vector3f(leftp[0] + 1, leftp[1] + 2, leftp[2]));
            }
            for(Eigen::Vector3f& position : positions){
                float* pixel_ptr = this->camera.get_top_buff(position);
                float* z_ptr = this->camera.get_z_buff(position);
                if(pixel_ptr && z_ptr){
                    if(no_less_than(leftp[2], *z_ptr)){
                        *z_ptr = leftp[2];
                        color_assign(color, pixel_ptr);
                    }
                }
            }
        }
    }
    inline void paint_line_simple(const Obj::Edge* edge, const Raster::Color& color){
        paint_line_simple(edge->start->projected_position, edge->end->projected_position, color);
    }
    void paint_frame_simple(Raster::Color color, bool verbose = false){
        camera.init_buffs();
        int i = 0;
        project_vertices(verbose);

        for(Obj::ObjSet* obj : this->obj_set){
            i = 0;
            for(Obj::Edge* edge : obj->edges){
                paint_line_simple(edge, color);
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->edges.size()){
                        print_progress(i, obj->edges.size(), "Paint edge");
                    }
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
        if(verbose){
            std::cout << "End paint_frame_simple()" << std::endl;
        }
    }

    /*opt = outline, texture, shader*/
    void paint_simple(const Raster::Color& line_color, Raster::Color& fill_color, float crease_angle = 1, std::string opt = "outline", bool verbose = false){
        camera.init_buffs();
        int i = 0;
        project_vertices(verbose);

        for(Obj::ObjSet* obj : this->obj_set){
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
        for(Obj::ObjSet* obj : this->obj_set){
            i = 1;
            for(Obj::Triangle* triangle : obj->triangles){
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->triangles.size()){
                        print_progress(i, obj->triangles.size(), "Triangle rasterizing");
                    }
                }
                // if(triangle->normal.value().z() < 0){
                //     continue;
                // }
                if(triangle->A->projected_position[2] > 0 && triangle->B->projected_position[2] > 0 && triangle->C->projected_position[2] > 0){
                    continue;
                }
                float l, r, u, d;
                l = min(triangle->A->projected_position[0], triangle->B->projected_position[0], triangle->C->projected_position[0]) - 1;
                r = max(triangle->A->projected_position[0], triangle->B->projected_position[0], triangle->C->projected_position[0]) + 1;
                u = min(triangle->A->projected_position[1], triangle->B->projected_position[1], triangle->C->projected_position[1]) - 1;
                d = max(triangle->A->projected_position[1], triangle->B->projected_position[1], triangle->C->projected_position[1]) + 1;
                maximize(l, 0);
                minimize(r, camera.w - 0.9);
                maximize(u, 0);
                minimize(d, camera.h - 0.9);
                if(l > r || u > d){
                    continue;
                }
                for(int y = u;y < d;y++){
                    for(int x = l;x < r;x++){
                        if(triangle->is_inside_triangle(x, y)){
                            Eigen::Vector3f bc_coord = triangle->get_barycentric_coordinate(x, y);
                            float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
                            if(z > 0){
                                continue;
                            }
                            float* z_p = camera.get_z_buff_trust(x, y);
                            if(no_less_than(z, *z_p)){
                                *z_p = z;
                                if(opt == "outline"){
                                    float* top_p = camera.get_top_buff_trust(x, y);
                                    color_assign(fill_color, top_p);
                                }
                                else if(opt == "texture"){
                                    Raster::Color color = fill_color;
                                    if(obj->texture.has_value()){
                                        float u, v;
                                        try{
                                            u = bc_coord.dot(Eigen::Vector3f(triangle->A_texture_uv.value()[0], triangle->B_texture_uv.value()[0], triangle->C_texture_uv.value()[0]));
                                            v = bc_coord.dot(Eigen::Vector3f(triangle->A_texture_uv.value()[1], triangle->B_texture_uv.value()[1], triangle->C_texture_uv.value()[1]));
                                        }
                                        catch(const std::bad_optional_access& e){
                                            throw Manga3DException("Raster::Rasterizer::paint_simple(): triangle uv lost", e);
                                        }
                                        Eigen::Vector3f tex_color = obj->texture.value().bilinear_sampling(u, v);
                                        switch(color.image_color){
                                        case Raster::Color::ImageColor::FULLCOLORALPHA:
                                            color = Raster::Color(tex_color[0], tex_color[1], tex_color[2], 1);
                                            break;
                                        case Raster::Color::ImageColor::FULLCOLOR:
                                            color = Raster::Color(tex_color[0], tex_color[1], tex_color[2]);
                                            break;
                                        case Raster::Color::ImageColor::BLACKWHITEALPHA:
                                            color = Raster::Color(tex_color[1], 1);
                                            break;
                                        default:
                                            color = Raster::Color(tex_color[1]);
                                            break;
                                        }
                                    }
                                    float* top_p = camera.get_top_buff_trust(x, y);
                                    color_assign(color, top_p);
                                }
                            }
                        }
                    }
                }
                bool outline_AB = false;
                bool outline_BC = false;
                bool outline_CA = false;
                outline_AB |= (triangle->AB->is_boundary() || triangle->AB->is_silhouette());
                outline_BC |= (triangle->BC->is_boundary() || triangle->BC->is_silhouette());
                outline_CA |= (triangle->CA->is_boundary() || triangle->CA->is_silhouette());
                outline_AB |= triangle->AB->is_crease(crease_angle);
                outline_BC |= triangle->BC->is_crease(crease_angle);
                outline_CA |= triangle->CA->is_crease(crease_angle);
                if(outline_AB){
                    paint_line_simple(triangle->AB, line_color);
                }
                if(outline_BC){
                    paint_line_simple(triangle->BC, line_color);
                }
                if(outline_CA){
                    paint_line_simple(triangle->CA, line_color);
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
    }


    inline void paint_outline_simple(const Raster::Color& line_color, Raster::Color fill_color, float crease_angle = 1, bool verbose = false){
        paint_simple(line_color, fill_color, crease_angle, "outline", verbose);
        if(verbose){
            std::cout << "End paint_outline_simple()" << std::endl;
        }
    }
};
