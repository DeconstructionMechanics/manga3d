#pragma once

#include <optional>
#include <fstream>
#include <string>

#include "global.hpp"

namespace Obj{
    using namespace Eigen;

    class Vertex;
    class Edge;
    class Triangle;

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
        Ray(Express express,Vector3f& origin,Vector3f& b) : origin(origin) {
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

/*
vertex_list,edge_list,triangle_list have elements allocated on the heap
use `obj_deletor()` to delete them
*/
void obj_loader(std::vector<Obj::Vertex*>& vertex_list, std::vector<Obj::Edge*>& edge_list, std::vector<Obj::Triangle*>& triangle_list, const std::string obj_path){
    using namespace ObjFile;

    std::vector<v> raw_v;
    std::vector<vt> raw_vt;
    std::vector<vn> raw_vn;
    std::vector<f> raw_f;

    std::ifstream file(obj_path);
    if(file.is_open()){
        std::string line;
        while(std::getline(file, line)){
            if(line.substr(0, 2) == "v "){
                v _v;
                sscanf(line.c_str(), "v %f %f %f", &_v.x, &_v.y, &_v.z);
                raw_v.push_back(_v);
            }
            else if(line.substr(0, 3) == "vt "){
                vt _vt;
                sscanf(line.c_str(), "vt %f %f", &_vt.u, &_vt.v);
                raw_vt.push_back(_vt);
            }
            else if(line.substr(0, 3) == "vn "){
                vn _vn;
                sscanf(line.c_str(), "vn %f %f %f", &_vn.x, &_vn.y, &_vn.z);
                raw_vn.push_back(_vn);
            }
            else if(line.substr(0, 2) == "f "){
                f _f;
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

        for(const v& _v : raw_v){
            Obj::Vertex* vertex = new Obj::Vertex();
            vertex->position = Eigen::Vector3f(_v.x, _v.y, _v.z);
            vertex_list.push_back(vertex);
        }

        for(const f& _f : raw_f){
            for(int i = 0;i < _f.n_v - 2;i++){
                Obj::Triangle* triangle = new Obj::Triangle();
                triangle_list.push_back(triangle);

                triangle->A = vertex_list[_f.index[0] - 1];
                if(_f.index[1] <= raw_vt.size()){
                    triangle->A_texture_uv = Eigen::Vector2f(raw_vt[_f.index[1] - 1].u, raw_vt[_f.index[1] - 1].v);
                }
                if(_f.index[2] <= raw_vn.size()){
                    triangle->A_normal = Eigen::Vector3f(raw_vn[_f.index[2] - 1].x, raw_vn[_f.index[2] - 1].y, raw_vn[_f.index[2] - 1].z);
                }
                triangle->B = vertex_list[_f.index[(i + 1) * _f.n_p] - 1];
                if(_f.index[(i + 1) * _f.n_p + 1] <= raw_vt.size()){
                    triangle->B_texture_uv = Eigen::Vector2f(raw_vt[_f.index[(i + 1) * _f.n_p + 1] - 1].u, raw_vt[_f.index[(i + 1) * _f.n_p + 1] - 1].v);
                }
                if(_f.index[(i + 1) * _f.n_p + 2] <= raw_vn.size()){
                    triangle->B_normal = Eigen::Vector3f(raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].x, raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].y, raw_vn[_f.index[(i + 1) * _f.n_p + 2] - 1].z);
                }
                triangle->C = vertex_list[_f.index[(i + 2) * _f.n_p] - 1];
                if(_f.index[(i + 2) * _f.n_p + 1] <= raw_vt.size()){
                    triangle->C_texture_uv = Eigen::Vector2f(raw_vt[_f.index[(i + 2) * _f.n_p + 1] - 1].u, raw_vt[_f.index[(i + 2) * _f.n_p + 1] - 1].v);
                }
                if(_f.index[(i + 2) * _f.n_p + 2] <= raw_vn.size()){
                    triangle->C_normal = Eigen::Vector3f(raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].x, raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].y, raw_vn[_f.index[(i + 2) * _f.n_p + 2] - 1].z);
                }

                triangle->AB = new Obj::Edge();
                edge_list.push_back(triangle->AB);
                triangle->AB->start = triangle->A;
                triangle->A->as_start.push_back(triangle->AB);
                triangle->AB->end = triangle->B;
                triangle->B->as_end.push_back(triangle->AB);
                triangle->AB->triangle = triangle;
                triangle->BC = new Obj::Edge();
                edge_list.push_back(triangle->BC);
                triangle->BC->start = triangle->B;
                triangle->B->as_start.push_back(triangle->BC);
                triangle->BC->end = triangle->C;
                triangle->C->as_end.push_back(triangle->BC);
                triangle->BC->triangle = triangle;
                triangle->CA = new Obj::Edge();
                edge_list.push_back(triangle->CA);
                triangle->CA->start = triangle->C;
                triangle->C->as_start.push_back(triangle->CA);
                triangle->CA->end = triangle->A;
                triangle->A->as_end.push_back(triangle->CA);
                triangle->CA->triangle = triangle;
            }
        }

        for(Obj::Edge* edge : edge_list){
            for(Obj::Edge* reverse : edge->start->as_end){
                if(reverse->start == edge->end){
                    edge->reverse = reverse;
                    break;
                }
            }
        }
    } // otherwise, file is not opened
}

void obj_deletor(std::vector<Obj::Vertex*>& vertex_list, std::vector<Obj::Edge*>& edge_list, std::vector<Obj::Triangle*>& triangle_list){
    for(int i = 0;i < vertex_list.size();i++){
        delete vertex_list[i];
    }
    vertex_list.clear();
    for(int i = 0;i < edge_list.size();i++){
        delete edge_list[i];
    }
    edge_list.clear();
    for(int i = 0;i < triangle_list.size();i++){
        delete triangle_list[i];
    }
    triangle_list.clear();
}

