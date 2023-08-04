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

    float fovY;
    Eigen::Vector3f position;
    Eigen::Vector3f lookat_g; // must be normalized()
    std::optional<float> up_t; // counterclockwise z rotation
    float near;
    float far;

    float* z_buff;
    float* top_buff;

    /*
    buffers are allocated on the heap
    use `delete_buff()` to delete them
    */
    inline void alloc_buff(){
        if(w <= 0 || h <= 0){
            throw Manga3DException("Raster::Camera::alloc_buff(): illegal w,h");
        }

        if(z_buff){
            throw Manga3DException("Raster::Camera::alloc_buff(): memory leak z_buff");
        }
        z_buff = new float[w * h];

        if(top_buff){
            throw Manga3DException("Raster::Camera::alloc_buff(): memory leak top_buff");
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

    template<typename T>
    inline T* get_buff(Eigen::Vector3f ind, T* buff, int channel) const{
        if(top_buff == NULL || ind[2] > 0){
            return nullptr;
        }
        return get_buff<T>((int)ind[0], (int)ind[1], buff, channel);
    }
    template<typename T>
    inline T* get_buff(int x, int y, T* buff, int channel) const{
        if(x < 0 || x >= this->w || y < 0 || y >= this->h){
            return nullptr;
        }
        return get_buff_trust<T>(x, y, buff, channel);
    }
    template<typename T>
    inline T* get_buff_trust(int x, int y, T* buff, int channel) const{
        int index = x + y * this->w;
        if(channel > 1){
            index *= channel;
        }
        return &(buff[index]);
    }
    inline float* get_top_buff(Eigen::Vector3f ind) const{
        return get_buff<float>((int)ind[0], (int)ind[1], this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_top_buff(int x, int y) const{
        return get_buff<float>(x, y, this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_top_buff_trust(int x, int y) const{
        return get_buff_trust<float>(x, y, this->top_buff, (int)this->bg_color.image_color);
    }
    inline float* get_z_buff(Eigen::Vector3f ind) const{
        return get_buff<float>((int)ind[0], (int)ind[1], this->z_buff, 1);
    }
    inline float* get_z_buff(int x, int y) const{
        return get_buff<float>(x, y, this->z_buff, 1);
    }
    inline float* get_z_buff_trust(int x, int y) const{
        return get_buff_trust<float>(x, y, this->z_buff, 1);
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
    void config(Projection projection_type, Raster::Color& bg_color, int w, int h, float fovY, Eigen::Vector3f& position, Eigen::Vector3f& lookat_g, float up_t = 0, float near = DEFAULT_NEAR, float far = DEFAULT_FAR){
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








    void project_vertices(const std::vector<Obj::ObjSet*>& obj_set, const bool verbose){
        for(Obj::ObjSet* obj : obj_set){
            int i = 0;
            for(Obj::Vertex* vertex : obj->vertices){
                vertex->projected_position = vertex->position;
                this->projection(vertex->projected_position);
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


    void paint_line_simple(const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Raster::Color& color, const int thickness){
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
                float* pixel_ptr = this->get_top_buff(position);
                float* z_ptr = this->get_z_buff(position);
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
        paint_line_simple(edge->start->projected_position, edge->end->projected_position, color, 2);
    }
    void paint_frame_simple(std::vector<Obj::ObjSet*>& obj_set, Raster::Color color, bool verbose){
        this->init_buffs();
        int i = 0;
        project_vertices(obj_set, verbose);

        for(Obj::ObjSet* obj : obj_set){
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

    Raster::Color get_texture_color(const Raster::Color& default_color, const Obj::ObjSet* obj, const Obj::Triangle* triangle, const Eigen::Vector3f& bc_coord){
        Raster::Color color = default_color;
        if(obj->texture.has_value()){
            float u, v;
            Eigen::Vector2f uv = triangle->get_uv_from_barycentric(bc_coord);
            u = uv[0];
            v = uv[1];
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
        return color;
    }

    /*opt = outline, texture, shadow */
    enum class PaintSimpleOpt{
        OUTLINE,
        TEXTURE,
        SHADOW
    };
    void paint_simple(std::vector<Obj::ObjSet*>& obj_set, const Raster::Color& line_color, Raster::Color& fill_color, float crease_angle, PaintSimpleOpt opt, bool paint_back, bool do_outline, bool verbose){
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
                        if(opt == PaintSimpleOpt::SHADOW){
                            Eigen::Vector3f point = triangle->get_position_from_barycentric(bc_coord);
                            float z = -(point - this->position).norm();
                            if(z < *z_p){
                                continue;
                            }
                            *z_p = z;
                            if(verbose){
                                float* top_p = this->get_top_buff_trust(x, y);
                                color_assign(fill_color * (-z / (this->position.norm() * 3)), top_p);
                            }
                            continue;
                        }

                        float z = bc_coord.dot(Eigen::Vector3f(triangle->A->projected_position[2], triangle->B->projected_position[2], triangle->C->projected_position[2]));
                        if(z > 0 || z < *z_p){
                            continue;
                        }
                        *z_p = z;
                        if(opt == PaintSimpleOpt::OUTLINE){
                            float* top_p = this->get_top_buff_trust(x, y);
                            color_assign(fill_color, top_p);
                        }
                        else if(opt == PaintSimpleOpt::TEXTURE){
                            Raster::Color color = get_texture_color(fill_color, obj, triangle, bc_coord);
                            float* top_p = this->get_top_buff_trust(x, y);
                            color_assign(color, top_p);
                        }
                    }
                }

                if(opt == PaintSimpleOpt::OUTLINE || do_outline){
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
            }
            if(verbose){
                std::cout << std::endl;
            }
        }
    }

};
