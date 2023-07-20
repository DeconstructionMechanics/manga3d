#pragma once

#include "global.hpp"
#include "OBJ.hpp"

namespace BVH{
    using namespace Eigen;
    class BBox{
    public:
        Vector3f low_bound;
        Vector3f high_bound;
    };

    class Node{
    public:
        BBox bbox;
        Node* left;
        Node* right;
        Obj::Triangle* triangle;
        /*pointer allocated on the heap*/
        Node(std::vector<Obj::Triangle*> triangles){
            if(triangles.size() == 1){
                triangle = triangles[0];
                bbox.low_bound = min(triangle->A->position,triangle->B->position,triangle->C->position);
                bbox.high_bound = max(triangle->A->position,triangle->B->position,triangle->C->position);
                left = nullptr;
                right = nullptr;
            }
            else{
                float min_x = MAX,max_x = MIN,min_y = MAX,max_y = MIN,min_z = MAX,max_z = MIN;
                for(const Obj::Triangle* t : triangles){
                    Eigen::Vector3f A_position = t->A->position;
                    minmaxize(min_x,max_x,A_position.x());
                    minmaxize(min_y,max_y,A_position.y());
                    minmaxize(min_z,max_z,A_position.z());
                }
                float x_diff = max_x - min_x;
                float y_diff = max_y - min_y;
                float z_diff = max_z - min_z;
                if(x_diff > y_diff && x_diff > z_diff){
                    std::sort(triangles.begin(),triangles.end(),[](Obj::Triangle* a,Obj::Triangle* b){
                        return a->A->position.x() < b->A->position.x();
                    });
                }
                else if(y_diff > x_diff && y_diff > z_diff){
                    std::sort(triangles.begin(),triangles.end(),[](Obj::Triangle* a,Obj::Triangle* b){
                        return a->A->position.y() < b->A->position.y();
                    });
                }
                else if(z_diff > x_diff && z_diff > y_diff){
                    std::sort(triangles.begin(),triangles.end(),[](Obj::Triangle* a,Obj::Triangle* b){
                        return a->A->position.y() < b->A->position.y();
                    });
                }
                std::vector<Obj::Triangle*>::iterator middle = triangles.begin() + triangles.size() / 2;
                std::vector<Obj::Triangle*> first_half(triangles.begin(), middle);
                std::vector<Obj::Triangle*> second_half(middle, triangles.end());

                left = new Node(first_half);
                right = new Node(second_half);
                bbox.low_bound = min(left->bbox.low_bound,right->bbox.low_bound);
                bbox.high_bound = max(left->bbox.high_bound,right->bbox.high_bound);
                triangle = nullptr;
            }
        }
        ~Node(){
            delete left;
            delete right;
            left = nullptr;
            right = nullptr;
        }

        Obj::Triangle* triangle_insert(Obj::Ray ray){
            return NULL;
        }

    };
}
