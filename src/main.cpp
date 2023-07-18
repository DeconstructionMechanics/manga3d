#include <iostream>
#include <vector>

#include "global.hpp"
#include "OBJ.hpp"
#include "BVH.hpp"

int main(){
    std::vector<Obj::Vertex*> vertices;
    std::vector<Obj::Edge*> edges;
    std::vector<Obj::Triangle*> triangles;
    std::string path = ".\\model\\cow\\spot_triangulated.obj";
    obj_loader(vertices,edges,triangles,path);
    std::cout << vertices.size() << " " << edges.size() << " " << triangles.size() << std::endl;
    BVH::Node* bvh_node = new BVH::Node(triangles);
    std::cout << bvh_node->bbox.low_bound << std::endl << bvh_node->bbox.high_bound << std::endl;
    obj_deletor(vertices,edges,triangles);
    
}
