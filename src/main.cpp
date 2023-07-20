#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "OBJ.hpp"
#include "BVH.hpp"
#include "rasterizer.hpp"


int main(){
    Raster::Rasterizer raster;
    int wi = 80;
    int hi = 60;
    int key = 0;

    raster.camera.w = wi;
    raster.camera.h = hi;
    for(int h = 0;h < hi;h++){
        for(int w = 0;w < wi;w++){
            raster.camera.top_buff.push_back(Eigen::Vector3f(w / (float)wi,h / (float)hi,0));
        }
    }
    
    auto a = raster.camera.top_buff.data();
    
    
    while (key != 27){
        cv::Mat image(raster.camera.h, raster.camera.w, CV_32FC3, raster.camera.top_buff.data());
        image *= 256;
        image.convertTo(image, CV_8UC3);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        cv::imshow("image", image);
        key = cv::waitKey(0);
        std::cout << image << std::endl;
    }
    

    return 0;
}
