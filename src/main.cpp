#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "global.hpp"
#include "OBJ.hpp"
#include "BVH.hpp"
#include "rasterizer.hpp"


void show_image(Raster::Rasterizer& raster, std::optional<std::string> filename){
    cv::Mat image(raster.camera.h, raster.camera.w, CV_32FC3, raster.camera.top_buff.data());
    image *= 256;
    image.convertTo(image, CV_8UC3);
    cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    int key = 0;
    if(filename.has_value()){
#if defined(_WIN32) || defined(_WIN64)
        std::string path = ".\\output\\";
#else
        std::string path = "./output/";
#endif
        std::string suffix = ".png";
        cv::imwrite(path + filename.value() + suffix, image);
    }
    while(key != 27){
        cv::imshow("image", image);
        key = cv::waitKey(0);
    }
}

int main(){
    Raster::Rasterizer raster;
    int wi = 80;
    int hi = 60;

    raster.camera.w = wi;
    raster.camera.h = hi;
    for(int h = 0;h < hi;h++){
        for(int w = 0;w < wi;w++){
            raster.camera.top_buff.push_back(Eigen::Vector3f(w / (float)wi, h / (float)hi, 0));
        }
    }
    show_image(raster, "eg");




    return 0;
}
