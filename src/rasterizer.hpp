#pragma once

#include "global.hpp"
#include "OBJ.hpp"

namespace Raster{
    class Color;
    class Light;
    class PointLight;
    class Camera;
    class Rasterizer;
}



/*
heap data inside
*/
class Raster::Color{
public:
    enum class ImageColor{
        FULLCOLORALPHA = 4,
        FULLCOLOR = 3,
        BLACKWHITEALPHA = 2,
        BLACKWHITE = 1
    };
    std::string imgcolor_2_string(ImageColor color) const{
        switch(color){
        case ImageColor::FULLCOLORALPHA:
            return "FULLCOLORALPHA";
        case ImageColor::FULLCOLOR:
            return "FULLCOLOR";
        case ImageColor::BLACKWHITEALPHA:
            return "BLACKWHITEALPHA";
        default:
            return "BLACKWHITEALPHA";
        }
    }
    ImageColor image_color;
    float* color;
    /*
    color is allocated on the heap,
    using ~Color() to delete
    */
    Color(float r, float g, float b, float alpha):image_color(ImageColor::FULLCOLORALPHA){
        color = new float[4];
        color[0] = b;
        color[1] = g;
        color[2] = r;
        color[3] = alpha;
    }
    Color(float r, float g, float b):image_color(ImageColor::FULLCOLOR){
        color = new float[3];
        color[0] = b;
        color[1] = g;
        color[2] = r;
    }
    Color(float g, float alpha):image_color(ImageColor::BLACKWHITEALPHA){
        color = new float[2];
        color[0] = g;
        color[1] = alpha;
    }
    Color(float g): image_color(ImageColor::BLACKWHITE){
        color = new float[1];
        color[0] = g;
    }
    Color(ImageColor image_color, float g, float alpha): image_color(image_color){
        color = new float[(int)image_color];
        for(int i = 0;i < (int)image_color - 1;i++){
            color[i] = g;
        }
        int last = (int)image_color - 1;
        if(last % 2 == 1){
            color[last] = alpha;
        }
        else{
            color[last] = g;
        }
    }
    ~Color(){
        if(color){
            delete[] color;
            color = nullptr;
        }
    }
    Color(const Color& other){
        this->image_color = other.image_color;
        this->color = new float[(int)other.image_color];
        for(int i = 0;i < (int)image_color;i++){
            this->color[i] = other.color[i];
        }
    }
    inline Color& operator=(const Color& other){
        if(this->color){
            delete[] this->color;
            this->color = nullptr;
        }
        this->image_color = other.image_color;
        this->color = new float[(int)other.image_color];
        for(int i = 0;i < (int)image_color;i++){
            this->color[i] = other.color[i];
        }
    }

    inline void color_assign_fullcoloralpha(float* buff) const{
        if(this->image_color != ImageColor::FULLCOLORALPHA){
            throw Manga3DException("Color assignment unmatch, expect FULLCOLORALPHA, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
        buff[2] = color[2];
        buff[3] = color[3];
    }
    inline void color_assign_fullcolor(float* buff) const{
        if(this->image_color != ImageColor::FULLCOLOR){
            throw Manga3DException("Color assignment unmatch, expect FULLCOLOR, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
        buff[2] = color[2];
    }
    inline void color_assign_blackwhitealpha(float* buff) const{
        if(this->image_color != ImageColor::BLACKWHITEALPHA){
            throw Manga3DException("Color assignment unmatch, expect BLACKWHITEALPHA, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
    }
    inline void color_assign_blackwhite(float* buff) const{
        if(this->image_color != ImageColor::BLACKWHITE){
            throw Manga3DException("Color assignment unmatch, expect BLACKWHITE, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
    }
    inline friend void color_assign(Color& color, float* ptr){
        switch(color.image_color){
        case ImageColor::FULLCOLORALPHA:
            color.color_assign_fullcoloralpha(ptr);
            break;
        case ImageColor::FULLCOLOR:
            color.color_assign_fullcolor(ptr);
            break;
        case ImageColor::BLACKWHITEALPHA:
            color.color_assign_blackwhitealpha(ptr);
            break;
        default:
            color.color_assign_blackwhite(ptr);
            break;
        }
    }

    inline bool operator!=(Color& compare_color){
        if(this->image_color == compare_color.image_color){
            for(int i = 0;i < (int)this->image_color;i++){
                if(this->color[i] != compare_color.color[i]){
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};




class Raster::Light{
public:
    float I;
    Eigen::Vector3f position;
};

class Raster::PointLight: public Raster::Light{

};





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
        if(x < 0 || x >= this->w || y < 0 || y >= this->w){
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
                throw Manga3DException("Raster::Camera::projection(): ortho_cache empty");
            }
            break;
        case Projection::PERSP:
            try{
                point_position_h = persp_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): persp_cache empty");
            }
            break;

        default: // FISHEYE
            try{
                point_position_h = putcamera_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): putcamera_matrix_cache empty");
            }
            float dist_i = 1 / point_position_h.hnormalized().norm();
            point_position_h[0] *= dist_i;
            point_position_h[1] *= dist_i;
            try{
                point_position_h = fisheyeviewport_matrix_cache.value() * point_position_h;
            }
            catch(const std::bad_optional_access& e){
                throw Manga3DException("Raster::Camera::projection(): fisheyeviewport_matrix_cache empty");
            }
            break;
        }
        point_position = point_position_h.hnormalized();
    }

};





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
    inline void load_obj(const std::string obj_path){
        obj_set.push_back(new Obj::ObjSet(obj_path));
    }
    Rasterizer(Raster::Color bg_color = Raster::Color(0, 0)): camera(bg_color, 1, 1){}
    Rasterizer(const std::string obj_path, Raster::Color bg_color = Raster::Color(0, 0)): camera(bg_color, 1, 1){
        load_obj(obj_path);
    }
    ~Rasterizer(){
        for(Obj::ObjSet* obj : this->obj_set){
            if(obj){
                delete obj;
            }
        }
        this->obj_set.clear();
    }

    void project_vertices(bool verbose = false){
        for(Obj::ObjSet* obj : this->obj_set){
            int i = 0;
            for(Obj::Vertex* vertex : obj->vertices){
                camera.projection(vertex->position);
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


    void paint_line_simple(const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Raster::Color& color){
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
        for(;leftp[dim] < rightp[dim] + 0.5;leftp += i){
            for(int j = 0;j < 4;j++){
                Eigen::Vector3f position = leftp;
                if(j & 1){
                    position[0]++;
                }
                if(j >> 1){
                    position[1]++;
                }
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
        paint_line_simple(edge->start->position, edge->end->position, color);
    }
    void paint_frame_simple(bool verbose = false){
        camera.init_buffs();
        int i = 0;
        project_vertices(verbose);

        for(Obj::ObjSet* obj : this->obj_set){
            i = 0;
            for(Obj::Edge* edge : obj->edges){
                paint_line_simple(edge->start->position, edge->end->position, Raster::Color(this->camera.bg_color.image_color, 0, 1));
                if(verbose){
                    i++;
                    if(i % 100 == 0 || i == obj->edges.size()){
                        print_progress(i, obj->edges.size(), "Paint pixels");
                    }
                }
                if(verbose){
                    std::cout << std::endl;
                }
            }
        }
        if(verbose){
            std::cout << "End paint_frame_simple()" << std::endl;
        }
    }


    // todo change color def
    void paint_outline_simple(const Raster::Color color, Raster::Color fill_color, float crease_angle = 1, bool verbose = false){
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
                for(int y = u;y < d;y++){
                    for(int x = l;x < r;x++){
                        if(triangle->is_inside_triangle(x, y)){
                            Eigen::Vector3f bc_coord = triangle->get_barycentric_coordinate(x, y);
                            float z = bc_coord.dot(Eigen::Vector3f(triangle->A->position[2], triangle->B->position[2], triangle->C->position[2]));
                            if(z > 0){
                                continue;
                            }
                            float* z_p = camera.get_z_buff_trust(x, y);
                            if(no_less_than(z, *z_p)){
                                *z_p = z;

                                float* top_p = camera.get_top_buff_trust(x, y);
                                color_assign(fill_color, top_p);
                            }
                        }
                    }
                }
                bool outline_AB = false;
                bool outline_BC = false;
                bool outline_CA = false;
                truify(outline_AB, (triangle->AB->is_boundary() || triangle->AB->is_silhouette()));
                truify(outline_BC, (triangle->BC->is_boundary() || triangle->BC->is_silhouette()));
                truify(outline_CA, (triangle->CA->is_boundary() || triangle->CA->is_silhouette()));
                truify(outline_AB, triangle->AB->is_crease(crease_angle));
                truify(outline_BC, triangle->BC->is_crease(crease_angle));
                truify(outline_CA, triangle->CA->is_crease(crease_angle));
                if(outline_AB){
                    paint_line_simple(triangle->AB, color);
                }
                if(outline_BC){
                    paint_line_simple(triangle->BC, color);
                }
                if(outline_CA){
                    paint_line_simple(triangle->CA, color);
                }
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
        if(verbose){
            std::cout << "End paint_outline_simple()" << std::endl;
        }
    }
};
