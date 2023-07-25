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
private:
    float fovY;
    Eigen::Vector3f position;
    Eigen::Vector3f lookat_g; // must be normalized()
    std::optional<float> up_t; // counterclockwise z rotation
    float near;
    float far;

    float* z_buff;
public:
    float* top_buff;

    /*
    buffers are allocated on the heap
    use `delete_buff()` to delete them
    */
    void alloc_buff(){
        if(image_color == Raster::ImageColor::FULLCOLOR){
            top_buff = new float[w * h * 3];
        }
        else{
            top_buff = new float[w * h];
        }
        z_buff = new float[w * h];
    }
    void delete_buff(){
        delete[] top_buff;
        delete[] z_buff;
    }
    Camera(Raster::ImageColor image_color, int w, int h): image_color(image_color), w(w), h(h){
        alloc_buff();
    }
    ~Camera(){
        delete_buff();
    }

    float* get_top_buff(Eigen::Vector3f ind){
        if(top_buff == NULL){
            return nullptr;
        }
        if(ind[0] < 0 || ind[0] >= this->w || ind[1] < 0 || ind[1] >= this->h || ind[2] > 0){
            return nullptr;
        }
        int index = (int)ind[0] + (int)ind[1] * this->w;
        if(this->image_color == Raster::ImageColor::FULLCOLOR){
            index *= 3;
        }
        return &(top_buff[index]);
    }

private:
    std::optional<Eigen::Matrix4f> movecamera_matrix_cache; //position
    std::optional<Eigen::Matrix4f> rotatecamera_matrix_cache; //lookat_g
    std::optional<Eigen::Matrix4f> upcamera_matrix_cache; //up_t
    std::optional<Eigen::Matrix4f> putcamera_matrix_cache; //movecamera_matrix_cache rotatecamera_matrix_cache upcamera_matrix_cache

    std::optional<Eigen::Matrix4f> persp_matrix_cache; //near far

    std::optional<Eigen::Matrix4f> viewport_matrix_cache; //w h
    std::optional<Eigen::Matrix4f> scaleviewport_matrix_cache; //near fov viewport_matrix_cache
    std::optional<Eigen::Matrix4f> fisheyeviewport_matrix_cache; //fov viewport_matrix_cache

    std::optional<Eigen::Matrix4f> ortho_cache; //putcamera_matrix_cache scaleviewport_matrix_cache
    std::optional<Eigen::Matrix4f> persp_cache; //putcamera_matrix_cache persp_matrix_cache scaleviewport_matrix_cache

public:
    void config(Projection projection_type, Raster::ImageColor image_color, int w, int h, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat_g, float up_t = 0, float near = 0.0001, float far = 10000){
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
        if(!putcamera_matrix_cache.has_value() || putcamera_changed){
            putcamera_matrix_cache = rotatecamera_matrix_cache.value() * movecamera_matrix_cache.value();
            if(this->up_t){
                putcamera_matrix_cache = upcamera_matrix_cache.value() * putcamera_matrix_cache.value();
            }
            putcamera_changed = true;
        }

        //update persp_matrix_cache
        bool persp_changed = false;
        if(!persp_matrix_cache || (!equal(near, this->near) || !equal(far, this->far))){
            this->near = near;
            this->far = far;
            Eigen::Matrix4f Proj;
            Proj << near, 0, 0, 0,
                0, near, 0, 0,
                0, 0, (near + far), (near * far),
                0, 0, -1, 0;
            persp_matrix_cache = Proj;
            persp_changed = true;
        }

        //update viewport_matrix_cache
        bool viewport_changed = false;
        bool scale_changed = false;
        if(!viewport_matrix_cache || w != this->w || h != this->h){
            this->w = w;
            this->h = h;
            Eigen::Matrix4f ViewPort;
            float half_w = w / 2.0;
            float half_h = h / 2.0;
            ViewPort << half_w, 0, 0, half_w,
                0, -half_h, 0, half_h,
                0, 0, 1, 0,
                0, 0, 0, 1;
            viewport_matrix_cache = ViewPort;
            viewport_changed = true;
        }
        float p = fovY / 2;
        float v = p * w / h;
        if(!scaleviewport_matrix_cache || viewport_changed || !equal(fovY, this->fovY) || !equal(near, this->near)){
            this->fovY = fovY;
            this->near = near;
            Eigen::Matrix4f Scale;
            Scale << (1 / (near * std::tan(p))), 0, 0, 0,
                0, (1 / (near * std::tan(v))), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;
            scaleviewport_matrix_cache = viewport_matrix_cache.value() * Scale;
            scale_changed = true;
        }
        if(!fisheyeviewport_matrix_cache || viewport_changed || !equal(fovY, this->fovY)){
            this->fovY = fovY;
            Eigen::Matrix4f Scale;
            Scale << (1 / std::sin(p)), 0, 0, 0,
                0, (1 / std::sin(v)), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1;
            scaleviewport_matrix_cache = viewport_matrix_cache.value() * Scale;
        }

        //update main matrices
        if(!ortho_cache || putcamera_changed || scale_changed){
            ortho_cache = scaleviewport_matrix_cache.value() * putcamera_matrix_cache.value();
        }
        if(!persp_cache || putcamera_changed || persp_changed || scale_changed){
            persp_cache = scaleviewport_matrix_cache.value() * persp_matrix_cache.value() * putcamera_matrix_cache.value();
        }
    }
    void config(float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g){
        config(this->projection_type, this->image_color, this->w, this->h, this->fovY, position, lookat_g);
    }

    void projection(Eigen::Vector3f& point_position){
        Eigen::Vector4f point_position_h = point_position.homogeneous();
        switch(this->projection_type){
        case Projection::ORTHO:
            point_position_h = ortho_cache.value() * point_position_h;
            break;
        case Projection::PERSP:
            point_position_h = persp_cache.value() * point_position_h;
            break;

        default: // FISHEYE
            point_position_h = putcamera_matrix_cache.value() * point_position_h;
            float dist_i = 1 / point_position_h.hnormalized().norm();
            point_position_h[0] *= dist_i;
            point_position_h[1] *= dist_i;
            point_position_h = fisheyeviewport_matrix_cache.value() * point_position_h;
            break;
        }
        point_position = point_position_h.hnormalized();
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

    void paint_line_simple(Eigen::Vector3f a, Eigen::Vector3f b){
        int dim = 0;
        if(std::abs(a[1] - b[1]) > std::abs(a[0] - b[0])){
            dim = 1;
        };
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
        for(;leftp[dim] < rightp[dim];leftp += i){
            float* pixel_ptr = this->camera.get_top_buff(leftp);
            if(pixel_ptr){
                if(this->camera.image_color == Raster::ImageColor::BLACKWHITE){
                    pixel_ptr[0] = 1;
                }
                else{
                    pixel_ptr[0] = 1;
                    pixel_ptr[1] = 1;
                    pixel_ptr[2] = 1;
                }
            }
        }
    }
    void paint_frame_simple(bool verbose = false){
        int i = 0;
        for(Obj::Edge* edge : this->edges){
            Eigen::Vector3f start = edge->start->position;
            Eigen::Vector3f end = edge->end->position;
            camera.projection(start);
            camera.projection(end);
            paint_line_simple(start, end);
            if(verbose){
                i++;
                if(i % 10 == 0){
                    std::cout << i << "/" << this->edges.size() << " paint_frame_simple()%\r";
                    std::cout.flush();
                }
            }
        }
    }
};
