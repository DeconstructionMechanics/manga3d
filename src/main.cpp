#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "global.hpp"
#include "OBJ.hpp"
#include "BVH.hpp"
#include "rasterizer.hpp"


void show_image(Raster::Rasterizer& raster, std::optional<std::string> filename = std::nullopt){
    cv::Mat image;
    switch(raster.camera.bg_color.image_color){
    case Raster::Color::ImageColor::FULLCOLORALPHA:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC4, raster.camera.top_buff);
        image.convertTo(image, CV_8UC4);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        break;
    case Raster::Color::ImageColor::FULLCOLOR:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC3, raster.camera.top_buff);
        image.convertTo(image, CV_8UC3);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);
        break;
    case Raster::Color::ImageColor::BLACKWHITEALPHA:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC2, raster.camera.top_buff);
        image.convertTo(image, CV_8UC2);
        break;
    default:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC1, raster.camera.top_buff);
        image.convertTo(image, CV_8UC1);
        break;
    }
    if(filename.has_value()){
#if defined(_WIN32) || defined(_WIN64)
        std::string path = ".\\output\\";
#else
        std::string path = "./output/";
#endif
        std::string suffix = ".png";
        cv::imwrite(path + filename.value() + suffix, image);
    }
    cv::imshow("image", image);
    cv::waitKey(0);
}

int main(){
    Raster::Color bg_color(1);
    Raster::Color line_color(0);
    Raster::Color fill_color(1);
    
    Raster::Rasterizer rasterizer(".\\model\\cow\\spot_triangulated.obj");
    std::cout << "load complete" << std::endl;

    Eigen::Vector3f position(1.5, 0, 0);
    Eigen::Vector3f lookat(-1, 0, 0);
    rasterizer.camera.config(Raster::Camera::Projection::PERSP, bg_color, 600, 400, PI / 2, position, lookat);

    std::cout << "rendering" << std::endl;
    // rasterizer.paint_frame_simple(true);
    rasterizer.paint_outline_simple(line_color,fill_color, 1, true);

    std::cout << std::endl << "showing image" << std::endl;
    show_image(rasterizer,"outline1");//, "outline");4


    return 0;
}
