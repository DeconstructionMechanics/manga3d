#pragma once

#include "../global.hpp"
#include "../Color.hpp"



namespace Raster{
    class Camera;
}

/*
heap data inside
*/
class Raster::Camera{
private:
    Camera(const Camera& other);
    Camera& operator=(const Camera& other);
public:
    enum class Projection{
        ORTHO,
        PERSP,
        FISHEYE
    };

    Projection projection_type;
    Raster::Color bg_color;
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
    inline void alloc_buff(){
        if(z_buff){
            throw Manga3DException("Raster::Camera::alloc_buff(): memory leak z_buff");
        }
        z_buff = new float[w * h];

        if(top_buff){
            throw Manga3DException("Raster::Camera::alloc_buff(): memory leak top_buff");
        }
        if(w <= 0 || h <= 0){
            throw Manga3DException("Raster::Camera::alloc_buff(): illegal w,h");
        }
        top_buff = new float[w * h * (int)bg_color.image_color];
    }
    inline void delete_buff(){
        if(z_buff){
            delete[] z_buff;
        }
        if(top_buff){
            delete[] top_buff;
        }
        clear_buff();
    }
    inline void clear_buff(){
        z_buff = nullptr;
        top_buff = nullptr;
    }
    Camera(Raster::Color bg_color, int w, int h): bg_color(bg_color), w(w), h(h){
        clear_buff();
        alloc_buff();
    }
    ~Camera(){
        delete_buff();
    }

    void init_buffs(){ //bg_color[1] => green => BLACKWHITE
        int wh = w * h;
        float* z_buff_t = z_buff;
        float* top_buff_t = top_buff;
        for(int i = 0;i < wh;i++){
            *z_buff_t = -MAX_F;
            z_buff_t++;
            color_assign(bg_color, top_buff_t);
            top_buff_t += (int)bg_color.image_color;
        }
    }

    inline float* get_buff(Eigen::Vector3f ind, float* buff, int channel) const{
        if(top_buff == NULL || ind[2] > 0){
            return nullptr;
        }
        return get_buff((int)ind[0], (int)ind[1], buff, channel);
    }
    inline float* get_buff(int x, int y, float* buff, int channel) const{
        if(x < 0 || x >= this->w || y < 0 || y >= this->h){
            return nullptr;
        }
        return get_buff_trust(x, y, buff, channel);
    }
    inline float* get_buff_trust(int x, int y, float* buff, int channel) const{
        int index = x + y * this->w;
        if(channel > 1){
            index *= channel;
        }
        return &(buff[index]);
    }
    inline float* get_top_buff(Eigen::Vector3f ind) const{
        return get_buff((int)ind[0], (int)ind[1], this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_top_buff(int x, int y) const{
        return get_buff(x, y, this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_top_buff_trust(int x, int y) const{
        return get_buff_trust(x, y, this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_z_buff(Eigen::Vector3f ind) const{
        return get_buff((int)ind[0], (int)ind[1], this->z_buff, 1);
    }
    inline float* get_z_buff(int x, int y) const{
        return get_buff(x, y, this->z_buff, 1);
    }
    inline float* get_z_buff_trust(int x, int y) const{
        return get_buff_trust(x, y, this->z_buff, 1);
    }

private:
    std::optional<Eigen::Matrix4f> movecamera_matrix_cache; //position
    std::optional<Eigen::Matrix4f> rotatecamera_matrix_cache; //lookat_g
    std::optional<Eigen::Matrix4f> upcamera_matrix_cache; //up_t
    std::optional<Eigen::Matrix4f> putcamera_matrix_cache; //movecamera_matrix_cache rotatecamera_matrix_cache upcamera_matrix_cache

    std::optional<Eigen::Matrix4f> ortho_matrix_cache; //w h fov
    std::optional<Eigen::Matrix4f> ortho_cache; //putcamera_matrix_cache ortho_matrix_cache
    std::optional<Eigen::Matrix4f> persp_matrix_cache; //near far
    std::optional<Eigen::Matrix4f> persp_cache; //putcamera_matrix_cache persp_matrix_cache ortho_matrix_cache
    std::optional<Eigen::Matrix4f> fisheyeviewport_matrix_cache; //w h fov

public:
    void config(Projection projection_type, Raster::Color bg_color, int w, int h, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat_g, float up_t = 0, float near = 0.0001, float far = 10000){
        this->projection_type = projection_type;
        if(this->bg_color.image_color != bg_color.image_color || this->w != w || this->h != h){
            delete_buff();
            this->bg_color = bg_color;
            this->w = w;
            this->h = h;
            alloc_buff();
        }
        else if(this->bg_color != bg_color){
            this->bg_color = bg_color;
        }

        //update putcamera_matrix_cache
        bool putcamera_changed = false;
        if(!movecamera_matrix_cache || position != this->position){
            this->position = position;
            Eigen::Matrix4f Move;
            Move << 1, 0, 0, -position.x(),
                0, 1, 0, -position.y(),
                0, 0, 1, -position.z(),
                0, 0, 0, 1;
            movecamera_matrix_cache = Move;
            putcamera_changed = true;
        }
        lookat_g = lookat_g.normalized();
        if(!rotatecamera_matrix_cache || lookat_g != this->lookat_g){
            this->lookat_g = lookat_g;
            Eigen::Matrix4f Rotate;
            float A = std::sqrt(lookat_g.x() * lookat_g.x() + lookat_g.z() * lookat_g.z());
            if(A == 0){
                Rotate << 1, 0, 0, 0,
                    0, 0, lookat_g.y(), 0,
                    0, -lookat_g.y(), 0, 0,
                    0, 0, 0, 1;
            }
            else{
                float A_inv = 1 / A;
                Rotate << (-lookat_g.z() * A_inv), 0, (lookat_g.x() * A_inv), 0,
                    (-lookat_g.x() * lookat_g.y() * A_inv), A, (-lookat_g.z() * lookat_g.y() * A_inv), 0,
                    (-lookat_g.x()), (-lookat_g.y()), (-lookat_g.z()), 0,
                    0, 0, 0, 1;
            }
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

        //update ortho_matrix_cache
        bool ortho_changed = false;
        if(!ortho_matrix_cache || w != this->w || h != this->h || !equal(fovY, this->fovY)){
            this->w = w;
            this->h = h;
            this->fovY = fovY;
            Eigen::Matrix4f ViewPort;
            float half_w = w * 0.5;
            float half_h = h * 0.5;
            float scale = half_w / std::tan(fovY * 0.5);
            ViewPort << scale, 0, 0, half_w,
                0, -scale, 0, half_h,
                0, 0, 1, 0,
                0, 0, 0, 1;
            ortho_matrix_cache = ViewPort;
            ortho_changed = true;
        }
        if(!ortho_cache || putcamera_changed || ortho_changed){
            ortho_cache = ortho_matrix_cache.value() * putcamera_matrix_cache.value();
        }

        //update persp_matrix_cache
        bool persp_changed = false;
        if(!persp_matrix_cache || !equal(near, this->near) || !equal(far, this->far)){
            this->near = near;
            this->far = far;
            this->w = w;
            this->h = h;
            this->fovY = fovY;
            Eigen::Matrix4f Proj;
            float half_w = w * 0.5;
            float half_h = h * 0.5;
            float scale = half_w / std::tan(fovY * 0.5);
            Proj << 1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, (near + far), (near * far),
                0, 0, -1, 0;
            persp_matrix_cache = Proj;
            persp_changed = true;
        }
        if(!persp_cache || putcamera_changed || persp_changed || ortho_changed){
            persp_cache = ortho_matrix_cache.value() * persp_matrix_cache.value() * putcamera_matrix_cache.value();
        }

        //update fisheyeviewport_matrix_cache
        if(!fisheyeviewport_matrix_cache || w != this->w || h != this->h || !equal(fovY, this->fovY)){
            this->w = w;
            this->h = h;
            this->fovY = fovY;
            Eigen::Matrix4f Scale;
            float p = fovY * 0.5;
            float half_w = w * 0.5;
            float half_h = h * 0.5;
            float scale = half_h / std::sin(p);
            Scale << (scale), 0, 0, half_w,
                0, (-scale), 0, half_h,
                0, 0, 1, 0,
                0, 0, 0, 1;
            fisheyeviewport_matrix_cache = Scale;
        }

    }
    inline void config(float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g){
        config(this->projection_type, this->bg_color, this->w, this->h, this->fovY, position, lookat_g);
    }

    void projection(Eigen::Vector3f& point_position) const{
        Eigen::Vector4f point_position_h = point_position.homogeneous();
        switch(this->projection_type){
        case Projection::ORTHO:
            try{
                point_position_h = ortho_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): ortho_cache empty", e);
            }
            break;
        case Projection::PERSP:
            try{
                point_position_h = persp_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): persp_cache empty", e);
            }
            break;

        default: // FISHEYE
            try{
                point_position_h = putcamera_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): putcamera_matrix_cache empty", e);
            }
            float dist_i = 1 / point_position_h.hnormalized().norm();
            point_position_h[0] *= dist_i;
            point_position_h[1] *= dist_i;
            try{
                point_position_h = fisheyeviewport_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): fisheyeviewport_matrix_cache empty", e);
            }
            break;
        }
        point_position = point_position_h.hnormalized();
    }

};
