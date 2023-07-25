#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "global.hpp"
#include "OBJ.hpp"
#include "BVH.hpp"
#include "rasterizer.hpp"


void show_image(Raster::Rasterizer& raster, std::optional<std::string> filename = std::nullopt){
    cv::Mat image;
    if(raster.camera.image_color == Raster::ImageColor::FULLCOLOR){
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC3, raster.camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC3);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
    }
    else{
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC1, raster.camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC1);
    }
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
    // while(key != 27){
    //     cv::imshow("image", image);
    //     key = cv::waitKey(0);
    // }
    cv::imshow("image", image);
    cv::waitKey(0);
}

int main(){
    Raster::Rasterizer rasterizer(".\\model\\cow\\spot_triangulated.obj",Raster::ImageColor::BLACKWHITE,500,400);
    std::cout << "load complete" << std::endl;
    Eigen::Vector3f position(0,0,1);
    Eigen::Vector3f direction(0,0,-1);
    rasterizer.camera.config(Raster::Camera::Projection::PERSP,Raster::ImageColor::BLACKWHITE,500,400,PI/2,position,direction);
    std::cout << "rendering" << std::endl;
    rasterizer.paint_frame_simple();
    std::cout << std::endl << "showing image" << std::endl;
    show_image(rasterizer);
    return 0;
}
