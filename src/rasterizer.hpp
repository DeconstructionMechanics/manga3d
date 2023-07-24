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

    void alloc_buff(){
        if(image_color == Raster::ImageColor::FULLCOLOR){
            top_buff = new float[w * h * 3];
        }
        else{
            top_buff = new float[w * h];
        }
    }
    void delete_buff(){
        delete[] top_buff;
    }
    Camera(Raster::ImageColor image_color, int w, int h): image_color(image_color), w(w), h(h){
        alloc_buff();
    }
    ~Camera(){
        delete_buff();
    }

    std::optional<Eigen::Matrix4f> movecamera_matrix_cache;
    std::optional<Eigen::Matrix4f> rotatecamera_matrix_cache;
    std::optional<Eigen::Matrix4f> upcamera_matrix_cache;
    std::optional<Eigen::Matrix4f> putcamera_matrix_cache;

    std::optional<Eigen::Matrix4f> persp_matrix_cache;

    void config(Projection projection_type, Raster::ImageColor image_color, int w, int h, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat_g, float up_t){
        this->projection_type = projection_type;
        if(this->image_color != image_color || this->w != w || this->h != h){
            delete_buff();
            this->image_color = image_color;
            this->w = w;
            this->h = h;
            alloc_buff();
        }

        //update putcamera_matrix_cache
        bool putcamera_changed = false;
        if(!movecamera_matrix_cache || position != this->position){
            this->position = position;
            Eigen::Matrix4f Move;
            Move << 1, 0, 0, position.x(),
                0, 1, 0, position.y(),
                0, 0, 1, position.z(),
                0, 0, 0, 1;
            movecamera_matrix_cache = Move;
            putcamera_changed = true;
        }
        if(!rotatecamera_matrix_cache || lookat_g != this->lookat_g){
            this->lookat_g = lookat_g;
            Eigen::Matrix4f Rotate;
            float A = std::sqrt(lookat_g.x() * lookat_g.x() + lookat_g.z() * lookat_g.z());
            Rotate << (-lookat_g.z() / A), 0, (lookat_g.x() / A), 0,
                (-lookat_g.x() * lookat_g.y() / A), A, (-lookat_g.z() * lookat_g.y() / A), 0,
                (-lookat_g.x()), (-lookat_g.y()), (-lookat_g.z()), 0,
                0, 0, 0, 1;
            rotatecamera_matrix_cache = Rotate;
            putcamera_changed = true;
        }
        if((!this->up_t && !iszero(up_t)) || (this->up_t && !equal(up_t, this->up_t.value()))){
            this->up_t = up_t;
            Eigen::Matrix4f RotUp;
            RotUp << std::cos(up_t), -std::sin(up_t), 0, 0,
                std::sin(up_t), std::cos(up_t), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;
            upcamera_matrix_cache = RotUp;
            putcamera_changed = true;
        }
        else if(this->up_t && iszero(up_t)){
            this->up_t.reset();
            this->upcamera_matrix_cache.reset();
            putcamera_changed = true;
        }
        if(!putcamera_matrix_cache.has_value() || putcamera_changed == true){
            putcamera_matrix_cache = rotatecamera_matrix_cache.value() * movecamera_matrix_cache.value();
            if(this->up_t){
                putcamera_matrix_cache = upcamera_matrix_cache.value() * putcamera_matrix_cache.value();
            }
        }

    }
    void config(float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g){

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
            Eigen::Matrix3f up_rotate;
            up_rotate << std::cos(up_t.value()), -std::sin(up_t.value()), 0,
                std::sin(up_t.value()), std::cos(up_t.value()), 0,
                0, 0, 1;
            point_position = up_rotate * point_position;
        }
    }
    void persp_projection(Eigen::Vector3f& point_position){
        ortho_projection(point_position);
        float zNear = 0.001;
        float zFar = 10000;
        Eigen::Matrix4f persp_m;
        persp_m << zNear, 0, 0, 0,
            0, zNear, 0, 0,
            0, 0, (zNear + zFar), (-zNear * zFar),
            0, 0, 1, 0;
        point_position = (persp_m * point_position.homogeneous()).hnormalized();

        // float half_height = zNear * tan(fovY / 2);
        // float half_width = w / h * half_height;
        // zNear = -zNear;
        // zFar = -zFar;

        // Eigen::Matrix4f ortho;
        // ortho << 1 / half_width, 0, 0, -half_width,
        //     0, 1 / half_height, 0, -half_height,
        //     0, 0, 2 / (zNear - zFar), -(zNear + zFar) / 2,
        //     0, 0, 0, 1;
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
