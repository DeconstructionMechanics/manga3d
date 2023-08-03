#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "global.hpp"
#include "./raster/Rasterizer.hpp"


void show_image(Raster::Rasterizer& raster, std::optional<std::string> filename = std::nullopt){
    cv::Mat image;
    cv::Mat image_2c;
    std::vector<cv::Mat> channels2;
    std::vector<cv::Mat> channels4;
    switch(raster.camera.bg_color.image_color){
    case Raster::Color::ImageColor::FULLCOLORALPHA:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC4, raster.camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC4);
        break;
    case Raster::Color::ImageColor::FULLCOLOR:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC3, raster.camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC3);
        break;
    case Raster::Color::ImageColor::BLACKWHITEALPHA:
        image_2c = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC2, raster.camera.top_buff);
        cv::split(image_2c, channels2);
        channels4 = { channels2[0],channels2[0].clone(),channels2[0].clone(),channels2[1] };
        cv::merge(channels4, image);
        image *= 256;
        image.convertTo(image, CV_8UC4);
        break;
    default:
        image = cv::Mat(raster.camera.h, raster.camera.w, CV_32FC1, raster.camera.top_buff);
        image *= 256;
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
    
    Raster::Color bg_color(1,1,1, 0);
    Raster::Color line_color(0,0,0, 1);
    Raster::Color fill_color(1,1,1, 1);

    Raster::Rasterizer rasterizer(".\\model\\monkey\\monkey.obj", ".\\model\\monkey\\color.png");
    // Raster::Rasterizer rasterizer(".\\model\\cow\\spot_triangulated.obj", ".\\model\\cow\\spot_texture.png");
    std::cout << "load complete" << std::endl;
    rasterizer.add_light(Raster::Rasterizer::LightType::POINTLIGHT,1,Raster::Color(1,1,1,1),1024,PI / 2,Eigen::Vector3f(1,4,4));
    // rasterizer.add_light(Raster::Rasterizer::LightType::SUNLIGHT,1,Raster::Color(1,1,1,1),1024,PI / 2,Eigen::Vector3f(0,-1,0));
    rasterizer.shadow_bake();

    Eigen::Vector3f position(0.001, 0, 5);
    Eigen::Vector3f lookat(0, 0,-1);
    rasterizer.config_camera(Raster::Camera::Projection::PERSP, bg_color, 900, 600, PI / 2, position, lookat);

    std::cout << "rendering" << std::endl;
    // rasterizer.paint_frame_simple(line_color,true);
    rasterizer.paint_shadow(fill_color);

    std::cout << std::endl << "showing image" << std::endl;
    show_image(rasterizer, "monkey_shadow");//, "monkeyframe");


    return 0;
}
