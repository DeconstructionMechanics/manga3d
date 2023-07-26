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
    inline void alloc_buff(){
        if(z_buff){
            throw "Raster::Camera::alloc_buff(): memory leak z_buff";
        }
        z_buff = new float[w * h];

        if(top_buff){
            throw "Raster::Camera::alloc_buff(): memory leak top_buff";
        }
        if(w <= 0 || h <= 0){
            throw "Raster::Camera::alloc_buff(): illegal w,h";
        }
        if(image_color == Raster::ImageColor::FULLCOLOR){
            top_buff = new float[w * h * 3];
        }
        else{
            top_buff = new float[w * h];
        }
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
    Camera(): image_color(Raster::ImageColor::BLACKWHITE), w(1), h(1){
        clear_buff();
        alloc_buff();
    }
    Camera(Raster::ImageColor image_color, int w, int h): image_color(image_color), w(w), h(h){
        clear_buff();
        alloc_buff();
    }
    ~Camera(){
        delete_buff();
    }

    void init_buffs(Eigen::Vector3f bg_color){ //bg_color[1] => green => BLACKWHITE
        int wh = w * h;
        for(int i = 0;i < wh;i++){
            z_buff[i] = MAX_F;
            if(this->image_color == Raster::ImageColor::FULLCOLOR){
                top_buff[3 * i] = bg_color[0];
                top_buff[3 * i + 1] = bg_color[1];
                top_buff[3 * i + 2] = bg_color[2];
            }
            else{
                top_buff[i] = bg_color[1];
            }
        }
    }

    inline float* get_buff(Eigen::Vector3f ind, float* buff) const{
        if(top_buff == NULL || ind[2] > 0){
            return nullptr;
        }
        return get_buff((int)ind[0], (int)ind[1], buff);
    }
    inline float* get_buff(int x, int y, float* buff) const{
        if(x < 0 || x >= this->w || y < 0 || y >= this->w){
            return nullptr;
        }
        return get_buff_trust(x, y, buff);
    }
    inline float* get_buff_trust(int x, int y, float* buff) const{
        int index = x + y * this->w;
        if(this->image_color == Raster::ImageColor::FULLCOLOR){
            index *= 3;
        }
        return &(buff[index]);
    }
    inline float* get_top_buff(Eigen::Vector3f ind) const{
        return get_buff((int)ind[0], (int)ind[1], this->top_buff);
    }
    inline float* get_top_buff(int x, int y) const{
        return get_buff(x, y, this->top_buff);
    }
    inline float* get_top_buff_trust(int x, int y) const{
        return get_buff_trust(x, y, this->top_buff);
    }
    inline float* get_z_buff(Eigen::Vector3f ind) const{
        return get_buff((int)ind[0], (int)ind[1], this->z_buff);
    }
    inline float* get_z_buff(int x, int y) const{
        return get_buff(x, y, this->z_buff);
    }
    inline float* get_z_buff_trust(int x, int y) const{
        return get_buff_trust(x, y, this->z_buff);
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
            Move << 1, 0, 0, -position.x(),
                0, 1, 0, -position.y(),
                0, 0, 1, -position.z(),
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

        //update ortho_matrix_cache
        bool ortho_changed = false;
        if(!ortho_matrix_cache || w != this->w || h != this->h || !equal(fovY, this->fovY)){
            this->w = w;
            this->h = h;
            this->fovY = fovY;
            Eigen::Matrix4f ViewPort;
            float half_w = w / 2.0;
            float half_h = h / 2.0;
            float scale = half_w / std::tan(fovY / 2);
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
            float half_w = w / 2.0;
            float half_h = h / 2.0;
            float scale = half_w / std::tan(fovY / 2);
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
            float p = fovY / 2;
            float v = p * w / h;
            float half_w = w / 2.0;
            float half_h = h / 2.0;
            Scale << (half_w / std::sin(v)), 0, 0, half_w,
                0, (-half_h / std::sin(p)), 0, half_h,
                0, 0, 1, 0,
                0, 0, 0, 1;
            fisheyeviewport_matrix_cache = Scale;
        }

    }
    inline void config(float fovY, Eigen::Vector3f position, Eigen::Vector3f lookat_g){
        config(this->projection_type, this->image_color, this->w, this->h, this->fovY, position, lookat_g);
    }

    void projection(Eigen::Vector3f& point_position) const{
        Eigen::Vector4f point_position_h = point_position.homogeneous();
        switch(this->projection_type){
        case Projection::ORTHO:
            try{
                point_position_h = ortho_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw "Raster::Camera::projection(): ortho_cache empty";
            }
            break;
        case Projection::PERSP:
            try{
                point_position_h = persp_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw "Raster::Camera::projection(): persp_cache empty";
            }
            break;

        default: // FISHEYE
            try{
                point_position_h = putcamera_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw "Raster::Camera::projection(): putcamera_matrix_cache empty";
            }
            float dist_i = 1 / point_position_h.hnormalized().norm();
            point_position_h[0] *= dist_i;
            point_position_h[1] *= dist_i;
            try{
                point_position_h = fisheyeviewport_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw "Raster::Camera::projection(): fisheyeviewport_matrix_cache empty";
            }
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

    Eigen::Vector3f bg_color;

    inline void load_obj(const std::string obj_path){
        obj_loader(vertices, edges, triangles, obj_path);
    }
    inline void delete_obj(){
        obj_deletor(vertices, edges, triangles);;
    }
    Rasterizer(Raster::ImageColor image_color, int w, int h): camera(image_color, w, h){}
    Rasterizer(const std::string obj_path, Eigen::Vector3f bg_color = Eigen::Vector3f(1, 1, 1)): bg_color(bg_color){
        load_obj(obj_path);
    }
    Rasterizer(const std::string obj_path, Raster::ImageColor image_color, int w, int h): camera(image_color, w, h){
        load_obj(obj_path);
    }
    ~Rasterizer(){
        delete_obj();
    }

    void project_vertices(bool verbose = false){
        int i = 0;
        for(Obj::Vertex* vertex : this->vertices){
            camera.projection(vertex->position);
            if(verbose){
                i++;
                if(i % 100 == 0){
                    print_progress(i, this->vertices.size(), "Project vertex");
                }
            }
        }
        if(verbose){
            std::cout << "End project vertex" << std::endl;
        }
    }

    void paint_line_simple(Eigen::Vector3f a, Eigen::Vector3f b, Eigen::Vector3f color = Eigen::Vector3f(0, 0, 0)){
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
                    pixel_ptr[0] = color[1];
                }
                else{
                    pixel_ptr[0] = color[0];
                    pixel_ptr[1] = color[1];
                    pixel_ptr[2] = color[2];
                }
            }
        }
    }
    void paint_frame_simple(bool verbose = false){
        camera.init_buffs(bg_color);
        int i = 0;
        project_vertices(verbose);
        for(Obj::Edge* edge : this->edges){
            paint_line_simple(edge->start->position, edge->end->position);
            if(verbose){
                i++;
                if(i % 100 == 0){
                    print_progress(i, this->edges.size(), "Paint pixels");
                }
            }
        }
        if(verbose){
            std::cout << "End paint_frame_simple()" << std::endl;
        }
    }
    void paint_outline_simple(Eigen::Vector3f color = Eigen::Vector3f(0, 0, 0),bool verbose = false){
        camera.init_buffs(bg_color);
        int i = 0;
        project_vertices(verbose);
        for(Obj::Triangle* triangle : this->triangles){
            triangle->calculate_normal();
        }
        for(Obj::Triangle* triangle : this->triangles){
            if(triangle->normal.value().z() < 0){
                continue;
            }
            if(triangle->A->position[2] > 0 && triangle->B->position[2] > 0 && triangle->C->position[2] > 0){
                continue;
            }
            float l, r, u, d;
            l = min(triangle->A->position[0], triangle->B->position[0], triangle->C->position[0]) - 1;
            r = max(triangle->A->position[0], triangle->B->position[0], triangle->C->position[0]) + 1;
            u = min(triangle->A->position[1], triangle->B->position[1], triangle->C->position[1]) - 1;
            d = max(triangle->A->position[1], triangle->B->position[1], triangle->C->position[1]) + 1;
            maximize(l, 0);
            minimize(r, camera.w - 0.9);
            maximize(u, 0);
            minimize(d, camera.h - 0.9);
            for(int x = l;x < r;x++){
                for(int y = u;y < d;y++){
                    if(triangle->is_inside_triangle(x, y)){
                        Eigen::Vector3f bc_coord = triangle->get_barycentric_coordinate(x, y);
                        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->position[2], triangle->B->position[2], triangle->C->position[2]));
                        if(z > 0){
                            continue;
                        }
                        float* z_p = camera.get_z_buff_trust(x, y);
                        if(z > *z_p){
                            *z_p = z;

                            bool outline = false;
                            float max_dist_boundary = 1.5;
                            float max_crease_boundary = 1;
                            float crease_angle = 2;
                            truify(outline,((triangle->AB->is_boundary() || triangle->AB->is_silhouette()) && triangle->AB->point_distance_2d(x,y) < max_dist_boundary));
                            truify(outline,((triangle->AB->is_crease(crease_angle)) && triangle->AB->point_distance_2d(x,y) < max_crease_boundary));
                            truify(outline,((triangle->BC->is_boundary() || triangle->BC->is_silhouette()) && triangle->BC->point_distance_2d(x,y) < max_dist_boundary));
                            truify(outline,((triangle->BC->is_crease(crease_angle)) && triangle->BC->point_distance_2d(x,y) < max_crease_boundary));
                            truify(outline,((triangle->CA->is_boundary() || triangle->CA->is_silhouette()) && triangle->CA->point_distance_2d(x,y) < max_dist_boundary));
                            truify(outline,((triangle->CA->is_crease(crease_angle)) && triangle->CA->point_distance_2d(x,y) < max_crease_boundary));
                            
                            Eigen::Vector3f this_color;
                            if(outline){
                                this_color = color;
                            }
                            else{
                                this_color = bg_color;
                            }
                            float* top_p = camera.get_top_buff_trust(x, y);
                            if(camera.image_color == Raster::ImageColor::BLACKWHITE){
                                *top_p = this_color[1];
                            }
                            else{
                                top_p[0] = this_color[0];
                                top_p[1] = this_color[1];
                                top_p[2] = this_color[2];
                            }
                        }
                    }
                }
            }
        }
    }
};
