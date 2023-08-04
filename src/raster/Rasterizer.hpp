#pragma once

#include "../global.hpp"
#include "../obj/OBJ.hpp"
#include "../Color.hpp"
#include "Light.hpp"
#include "Camera.hpp"
#include "ShaderAdv.hpp"

namespace Raster{
    class Rasterizer;
}


/*
heap pointer inside member
*/
class Raster::Rasterizer{
public:

    std::vector<Obj::ObjSet*> obj_set;

    std::vector<Raster::Light*> lights;
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
        for(Raster::Light* light : this->lights){
            if(light){
                delete light;
            }
        }
        this->lights.clear();
    }

    inline void config_camera(Raster::Camera::Projection projection_type, Raster::Color bg_color, int w, int h, float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g, float up_t = 0, float near = DEFAULT_NEAR, float far = DEFAULT_FAR){
        this->camera.config(projection_type, bg_color, w, h, fovY, position, lookat_g, up_t, near, far);
    }
    inline void config_camera(float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g){
        this->camera.config(this->camera.projection_type, this->camera.bg_color, this->camera.w, this->camera.h, fovY, position, lookat_g, 0, this->camera.near, this->camera.far);
    }

    enum class LightType{
        POINTLIGHT,
        SUNLIGHT
    };
    inline void add_light(LightType light_type, float I, Raster::Color light_color, int sm_resolution, float sm_fov, Eigen::Vector3f position_lookat){
        Raster::Light* light;
        if(light_type == LightType::POINTLIGHT){
            light = new PointLight(I);
        }
        else if(light_type == LightType::SUNLIGHT){
            light = new SunLight(I);
        }
        else{
            return;
        }
        light->index = this->lights.size();
        light->config(light_color, sm_resolution, sm_fov, position_lookat, position_lookat);
        this->lights.push_back(light);
    }

    void shadow_bake(bool verbose = false){
        for(Raster::Light* light : this->lights){
            light->cast_shadow(obj_set, verbose);
        }
        if(verbose){
            std::cout << "End shadow_bake()" << std::endl;
        }
    }

    inline void paint_frame_simple(Raster::Color color, bool verbose = false){
        camera.paint_frame_simple(this->obj_set, color, verbose);
        if(verbose){
            std::cout << "End paint_frame_simple()" << std::endl;
        }
    }
    inline void paint_outline_simple(Raster::Color line_color, Raster::Color fill_color, int thickness = 2, float crease_angle = 1, int crease_thickness = 1, bool paint_back = false, bool verbose = false){
        Raster::OutlineShader outline_shader(thickness, crease_angle, crease_thickness, line_color);
        camera.paint(outline_shader, this->obj_set, fill_color, paint_back, verbose);
        if(verbose){
            std::cout << "End paint_outline_simple()" << std::endl;
        }
    }
    inline void paint_texture_simple(Raster::Color fill_color, bool paint_back = false, bool verbose = false){
        Raster::TextureShader texture_shader;
        camera.paint(texture_shader, this->obj_set, fill_color, paint_back, verbose);
        if(verbose){
            std::cout << "End paint_texture_simple()" << std::endl;
        }
    }
    inline void paint_phoneshading(const Raster::Color fill_color, float shadow_bias = 0.05, bool pcf = false, bool paint_back = false, bool verbose = false){
        Raster::Color line_color(fill_color.image_color,0,1);
        DiscreteShader discrete_shader(lights, shadow_bias, pcf);
        discrete_shader.set_outline(2,1,1,line_color);
        camera.paint(discrete_shader, this->obj_set, fill_color, paint_back, verbose);
        if(verbose){
            std::cout << "End paint_phoneshading()" << std::endl;
        }
    }

    void fxaa(){
        for(int y = 0; y < this->camera.h;y += 2){
            for(int x = 0;x < this->camera.w;x += 2){
                Raster::Color xy(this->camera.bg_color.image_color,this->camera.get_top_buff(x,y));
                Raster::Color x1y(this->camera.bg_color.image_color,this->camera.get_top_buff(x+1,y));
                Raster::Color xy1(this->camera.bg_color.image_color,this->camera.get_top_buff(x,y+1));
                Raster::Color x1y1(this->camera.bg_color.image_color,this->camera.get_top_buff(x+1,y+1));
                bool xy_change;
                bool x1y_change;
                bool xy1_change;
                bool x1y1_change;
                
            }
        }
    }
};
