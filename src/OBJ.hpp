#pragma once

#include <optional>
#include <fstream>
#include <string>

#include "global.hpp"
#include "texture.hpp"



namespace ObjFile{
    struct v{
        float x, y, z;
    };
    struct vt{
        float u, v;
    };
    struct vn{
        float x, y, z;
    };
    struct f{
        int n_v;
        int n_p;
        std::vector<int> index;
    };
}



namespace Obj{
    using namespace Eigen;

    class Vertex;
    class Edge;
    class Triangle;
    class ObjSet;

    class Vertex{
    public:
        Vector3f position;
        std::vector<Edge*> as_start;
        std::vector<Edge*> as_end;
    };

    class Edge{
    public:
        Vertex* start;
        Vertex* end;
        Triangle* triangle;
        Edge* reverse;

        Edge(): start(nullptr), end(nullptr), triangle(nullptr), reverse(nullptr){}

        float point_distance_2d(int x, int y) const{
            float Y = start->position[1] - end->position[1];
            float X = end->position[0] - start->position[0];
            return (std::abs(Y * x + X * y + start->position[0] * end->position[1] - end->position[0] * start->position[1]) / std::sqrt(X * X + Y * Y));
        }

        bool is_boundary() const{
            return reverse == NULL;
        }
        bool is_crease(float angle) const;
        bool is_silhouette() const;
    };

    class Triangle{
    public:
        Vertex* A;
        std::optional<Vector2f> A_texture_uv;
        std::optional<Vector3f> A_normal;
        Vertex* B;
        std::optional<Vector2f> B_texture_uv;
        std::optional<Vector3f> B_normal;
        Vertex* C;
        std::optional<Vector2f> C_texture_uv;
        std::optional<Vector3f> C_normal;
        Edge* AB;
        Edge* BC;
        Edge* CA;
        std::optional<Vector3f> normal;

        Triangle(): A(nullptr), B(nullptr), C(nullptr), AB(nullptr), BC(nullptr), CA(nullptr){};

        inline void calculate_normal(){
            this->normal = (this->B->position - this->A->position).cross(this->A->position - this->C->position).normalized();
        }
        inline bool is_inside_triangle(int x, int y) const{
            float ax, ay, bx, by, cx, cy;
            ax = this->A->position[0] - x;
            ay = this->A->position[1] - y;
            bx = this->B->position[0] - x;
            by = this->B->position[1] - y;
            cx = this->C->position[0] - x;
            cy = this->C->position[1] - y;
            float v0, v1, v2;
            v0 = ax * by - ay * bx;
            v1 = bx * cy - by * cx;
            v2 = cx * ay - cy * ax;
            return v0 < 0 && v1 < 0 && v2 < 0;
        }
        Vector3f get_barycentric_coordinate(int x, int y) const{
            float alpha, beta, gama;
            alpha = (-(x - this->B->position[0]) * (this->C->position[1] - this->B->position[1]) + (y - this->B->position[1]) * (this->C->position[0] - this->B->position[0])) / (-(this->A->position[0] - this->B->position[0]) * (this->C->position[1] - this->B->position[1]) + (this->A->position[1] - this->B->position[1]) * (this->C->position[0] - this->B->position[0]));
            beta = (-(x - this->C->position[0]) * (this->A->position[1] - this->C->position[1]) + (y - this->C->position[1]) * (this->A->position[0] - this->C->position[0])) / (-(this->B->position[0] - this->C->position[0]) * (this->A->position[1] - this->C->position[1]) + (this->B->position[1] - this->C->position[1]) * (this->A->position[0] - this->C->position[0]));
            gama = 1 - alpha - beta;
            return Vector3f(alpha, beta, gama);
        }
    };

    bool Edge::is_crease(float angle) const{
        if(reverse == NULL){
            return false;
        }
        try{
            float cost = this->triangle->normal.value().dot(this->reverse->triangle->normal.value());
            return cost < std::cos(angle);
        }
        catch(const std::bad_optional_access& e){
            throw Manga3DException("triangle->normal is not calculated", e);
        }
    }
    bool Edge::is_silhouette() const{
        try{
            if(this->triangle->normal.value()[2] > 0 && this->reverse->triangle->normal.value()[2] < 0){
                return true;
            }
            return false;
        }
        catch(const std::bad_optional_access& e){
            throw Manga3DException("triangle->normal is not calculated", e);
        }
    }


    /*
    heap pointer inside member
    */
    class ObjSet{
    public:
        std::vector<Obj::Vertex*> vertices;
        std::vector<Obj::Edge*> edges;
        std::vector<Obj::Triangle*> triangles;
        Texture texture;

        /*
        vertex_list,edge_list,triangle_list have elements allocated on the heap
        use `clear_heap()` to delete them
        */
        ObjSet(const std::string obj_path){
            std::vector<ObjFile::v> raw_v;
            std::vector<ObjFile::vt> raw_vt;
            std::vector<ObjFile::vn> raw_vn;
            std::vector<ObjFile::f> raw_f;

            std::ifstream file(obj_path);
            if(file.is_open()){
                std::string line;
                while(std::getline(file, line)){
                    if(line.substr(0, 2) == "v "){
                        ObjFile::v _v;
                        sscanf(line.c_str(), "v %f %f %f", &_v.x, &_v.y, &_v.z);
                        raw_v.push_back(_v);
                    }
                    else if(line.substr(0, 3) == "vt "){
                        ObjFile::vt _vt;
                        sscanf(line.c_str(), "vt %f %f", &_vt.u, &_vt.v);
                        raw_vt.push_back(_vt);
                    }
                    else if(line.substr(0, 3) == "vn "){
                        ObjFile::vn _vn;
                        sscanf(line.c_str(), "vn %f %f %f", &_vn.x, &_vn.y, &_vn.z);
                        raw_vn.push_back(_vn);
                    }
                    else if(line.substr(0, 2) == "f "){
                        ObjFile::f _f;
                        _f.n_p = 1;
                        _f.n_v = 1;
                        std::vector<char> buffer;
                        int i = 2;
                        while(line.c_str()[i] != '\0'){
                            if(line.c_str()[i] == '/'){
                                _f.n_p++;
                                std::string str(buffer.begin(), buffer.end());
                                buffer.clear();
                                _f.index.push_back(std::stoi(str));
                            }
                            else if(line.c_str()[i] == ' '){
                                _f.n_p = 1;
                                _f.n_v++;
                                std::string str(buffer.begin(), buffer.end());
                                buffer.clear();
                                _f.index.push_back(std::stoi(str));
                            }
                            else{
                                buffer.push_back(line.c_str()[i]);
                            }
                            i++;
                        }
                        std::string str(buffer.begin(), buffer.end());
                        buffer.clear();
                        _f.index.push_back(std::stoi(str));
                        raw_f.push_back(_f);
                    }
                }
                file.close(); // Close the OBJ file



                for(const ObjFile::v& _v : raw_v){
                    Obj::Vertex* vertex = new Obj::Vertex();
                    vertex->position = Eigen::Vector3f(_v.x, _v.y, _v.z);
                    this->vertices.push_back(vertex);
                }

                for(const ObjFile::f& _f : raw_f){
                    for(int i = 0;i < _f.n_v - 2;i++){
                        Obj::Triangle* triangle = new Obj::Triangle();
                        this->triangles.push_back(triangle);

                        triangle->A = this->vertices[_f.index[0] - 1];
                        if(_f.index[1] <= raw_vt.size()){
                            triangle->A_texture_uv = Eigen::Vector2f(raw_vt[_f.index[1] - 1].u, raw_vt[_f.index[1] - 1].v);
                        }
                        if(_f.index[2] <= raw_vn.size()){
                            triangle->A_normal = Eigen::Vector3f(raw_vn[_f.index[2] - 1].x, raw_vn[_f.index[2] - 1].y, raw_vn[_f.index[2] - 1].z);
                        }
                        triangle->B = this->vertices[_f.index[(i + 1) * _f.n_p] - 1];
                        if(_f.index[(i + 1) * _f.n_p + 1] <= raw_vt.size()){
                            triangle->B_texture_uv = Eigen::Vector2f(raw_vt[_f.index[(i + 1) * _f.n_p + 1] - 1].u, raw_vt[_f.index[(i + 1) * _f.n_p + 1] - 1].v);
                        }
                        if(_f.index[(i + 1) * _f.n_p + 2] <= raw_vn.size()){
                            triangle->B_normal = Eigen::Vector3f(raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].x, raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].y, raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].z);
                        }
                        triangle->C = this->vertices[_f.index[(i + 2) * _f.n_p] - 1];
                        if(_f.index[(i + 2) * _f.n_p + 1] <= raw_vt.size()){
                            triangle->C_texture_uv = Eigen::Vector2f(raw_vt[_f.index[(i + 2) * _f.n_p + 1] - 1].u, raw_vt[_f.index[(i + 2) * _f.n_p + 1] - 1].v);
                        }
                        if(_f.index[(i + 2) * _f.n_p + 2] <= raw_vn.size()){
                            triangle->C_normal = Eigen::Vector3f(raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].x, raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].y, raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].z);
                        }

                        triangle->AB = new Obj::Edge();
                        this->edges.push_back(triangle->AB);
                        triangle->AB->start = triangle->A;
                        triangle->A->as_start.push_back(triangle->AB);
                        triangle->AB->end = triangle->B;
                        triangle->B->as_end.push_back(triangle->AB);
                        triangle->AB->triangle = triangle;
                        triangle->BC = new Obj::Edge();
                        this->edges.push_back(triangle->BC);
                        triangle->BC->start = triangle->B;
                        triangle->B->as_start.push_back(triangle->BC);
                        triangle->BC->end = triangle->C;
                        triangle->C->as_end.push_back(triangle->BC);
                        triangle->BC->triangle = triangle;
                        triangle->CA = new Obj::Edge();
                        this->edges.push_back(triangle->CA);
                        triangle->CA->start = triangle->C;
                        triangle->C->as_start.push_back(triangle->CA);
                        triangle->CA->end = triangle->A;
                        triangle->A->as_end.push_back(triangle->CA);
                        triangle->CA->triangle = triangle;
                    }
                }

                for(Obj::Edge* edge : this->edges){
                    for(Obj::Edge* reverse : edge->start->as_end){
                        if(reverse->start == edge->end){
                            edge->reverse = reverse;
                            break;
                        }
                    }
                }
            }
            else{
                throw Manga3DException("Obj: .obj file is not opened, " + obj_path);
            }
        }

        void clear_heap(){
            for(int i = 0;i < this->vertices.size();i++){
                if(this->vertices[i]){
                    delete this->vertices[i];
                    this->vertices[i] = nullptr;
                }
            }
            this->vertices.clear();
            for(int i = 0;i < this->edges.size();i++){
                if(this->edges[i]){
                    delete this->edges[i];
                    this->edges[i] = nullptr;
                }
            }
            this->edges.clear();
            for(int i = 0;i < this->triangles.size();i++){
                if(this->triangles[i]){
                    delete this->triangles[i];
                    this->triangles[i] = nullptr;
                }
            }
            this->triangles.clear();
        }
    };

    class Ray{
    public:
        enum class Express{
            DIRECTION,
            ENDPOINT
        };
        Vector3f origin;
        std::optional<Vector3f> direction;
        std::optional<float> t;
        std::optional<Vector3f> end;
        Ray(Express express, Vector3f& origin, Vector3f& b): origin(origin){
            if(express == Express::DIRECTION){
                direction = b;
            }
            else{
                end = b;
            }
        }
        void ENDPOINT_to_DIRECTION(){
            if(end.has_value() && !direction.has_value()){
                Eigen::Vector3f direction_temp = end.value() - origin;
                direction = direction_temp.normalized();
                t = direction_temp.norm();
            }
        }
    };
}







