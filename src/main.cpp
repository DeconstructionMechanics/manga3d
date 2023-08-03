#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

#include "global.hpp"
#include "./raster/Rasterizer.hpp"


void show_image(Raster::Camera& camera, std::optional<std::string> filename = std::nullopt){
    cv::Mat image;
    cv::Mat image_2c;
    std::vector<cv::Mat> channels2;
    std::vector<cv::Mat> channels4;
    switch(camera.bg_color.image_color){
    case Raster::Color::ImageColor::FULLCOLORALPHA:
        image = cv::Mat(camera.h, camera.w, CV_32FC4, camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC4);
        break;
    case Raster::Color::ImageColor::FULLCOLOR:
        image = cv::Mat(camera.h, camera.w, CV_32FC3, camera.top_buff);
        image *= 256;
        image.convertTo(image, CV_8UC3);
        break;
    case Raster::Color::ImageColor::BLACKWHITEALPHA:
        image_2c = cv::Mat(camera.h, camera.w, CV_32FC2, camera.top_buff);
        cv::split(image_2c, channels2);
        channels4 = { channels2[0],channels2[0].clone(),channels2[0].clone(),channels2[1] };
        cv::merge(channels4, image);
        image *= 256;
        image.convertTo(image, CV_8UC4);
        break;
    default:
        image = cv::Mat(camera.h, camera.w, CV_32FC1, camera.top_buff);
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
    rasterizer.add_light(Raster::Rasterizer::LightType::POINTLIGHT,20,Raster::Color(1,1,1,1),1024,PI / 2,Eigen::Vector3f(2,4,4));
    rasterizer.add_light(Raster::Rasterizer::LightType::SUNLIGHT,2,Raster::Color(1,1,1,1),1024,PI / 1.1,Eigen::Vector3f(1,0,0));
    rasterizer.shadow_bake(true);

    Eigen::Vector3f position(1, 0, 5);
    Eigen::Vector3f lookat(-1, 0, -5);
    rasterizer.config_camera(Raster::Camera::Projection::PERSP, bg_color, 900, 600, PI / 2, position, lookat);

    std::cout << "rendering" << std::endl;
    // rasterizer.paint_frame_simple(line_color,true);
    rasterizer.paint_shadow(fill_color,0.09,true);

    std::cout << std::endl << "showing image" << std::endl;
    show_image(rasterizer.camera,"monkey");//, "monkeyframe");


    return 0;
}
