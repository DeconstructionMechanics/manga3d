#pragma once

#include "global.hpp"
#include "OBJ.hpp"

namespace Raster{
    class Light;
    class PointLight;
    class Camera;
    class Rasterizer;
    enum class ImageColor{
        FULLCOLOR,
        BLACKWHITE
    };
}

class Raster::Light{
public:
    float I;
    Eigen::Vector3f position;
};

class Raster::PointLight: public Raster::Light{

};

class Raster::Camera{
public:
    enum class Projection{
        ORTHO,
        PERSP,
        FISHEYE
    };
    Projection projection_type;
    Raster::ImageColor image_color;
    int w;
    int h;
    float fovY;
    Eigen::Vector3f position;
    Eigen::Vector3f lookat_g; // must be normalized()
    std::optional<float> up_t; // counterclockwise z rotation

    float* top_buff;

    Camera(Raster::ImageColor image_color, int w, int h): image_color(image_color), w(w), h(h){
        if(image_color == Raster::ImageColor::FULLCOLOR){
            top_buff = new float[w * h * 3];
        }
        else{
            top_buff = new float[w * h];
        }
    }
    ~Camera(){
        delete[] top_buff;
    }

    void ortho_projection(Eigen::Vector3f& point_position){
        point_position -= this->position;
        float A = std::sqrt(lookat_g.x() * lookat_g.x() + lookat_g.z() * lookat_g.z());
        Eigen::Matrix3f lookat_rotate;
        lookat_rotate << (-lookat_g.z() / A), 0, (lookat_g.x() / A),
            (-lookat_g.x() * lookat_g.y() / A), A, (-lookat_g.z() * lookat_g.y() / A),
            (-lookat_g.x()), (-lookat_g.y()), (-lookat_g.z());
        point_position = lookat_rotate * point_position;
        if(up_t.has_value()){
            float c = std::cos(up_t.value());
            float s = std::sin(up_t.value());
            float x = point_position.x();
            float y = point_position.y();
            point_position[0] = c * x - s * y;
            point_position[1] = s * x + c * y;
        }
    }
    void projection(Eigen::Vector3f& point_position){
        switch(this->projection_type){
        case Projection::ORTHO:
            ortho_projection(point_position);
            if(PI - fovY > 0){
                point_position /= PI - fovY;
            }
            break;
        case Projection::PERSP:
            /* code */
            break;

        default: // FISHEYE
            break;
        }

    }

};

class Raster::Rasterizer{
public:

    std::vector<Obj::Vertex*> vertices;
    std::vector<Obj::Edge*> edges;
    std::vector<Obj::Triangle*> triangles;

    std::vector<Raster::Light> lights;
    Raster::Camera camera;

    Rasterizer(Raster::ImageColor image_color, int w, int h): camera(image_color, w, h){}
    Rasterizer(const std::string obj_path, Raster::ImageColor image_color, int w, int h): camera(image_color, w, h){
        obj_loader(vertices, edges, triangles, obj_path);
    }
    ~Rasterizer(){
        obj_deletor(vertices, edges, triangles);
    }

};
