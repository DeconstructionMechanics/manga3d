#pragma once

#include "../global.hpp"

namespace Tex{
    class Texture{
    public:
        int height;
        int width;
        int channel;
        float* tex;
    private:
        cv::Mat image;
    public:
        /*used to initialize color texture*/
        Texture(const std::string& text_path){
            image = cv::imread(text_path, cv::IMREAD_COLOR);
            if(image.empty()){
                throw Manga3DException("Tex: texture image is not opened, " + text_path);
            }
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
            image.convertTo(image, CV_32FC3, 1.0 / 255.0);
            this->height = image.rows;
            this->width = image.cols;
            this->channel = 3;
            this->tex = image.ptr<float>(0);
        }


        Eigen::Vector3f bilinear_sampling(float u, float v) const{
            v = 1 - v;
            u *= width;
            v *= height;
            int x = u + 0.5;
            int y = v + 0.5;
            if(x >= width){
                x = width - 1;
            }
            else if(x <= 0){
                x = 1;
            }
            if(y >= height){
                y = height - 1;
            }
            else if(y <= 0){
                y = 1;
            }
            float* xy = tex + ((x - 1) + (y - 1) * width) * channel;
            float* x1y = xy + channel;
            float* xy1 = xy + width * channel;
            float* x1y1 = xy1 + channel;
            float left_w = u + 0.5 - x;
            float low_w = v + 0.5 - y;
            Eigen::Vector3f up((1 - left_w) * xy1[0] + left_w * x1y1[0], (1 - left_w) * xy1[1] + left_w * x1y1[1], (1 - left_w) * xy1[2] + left_w * x1y1[2]);
            Eigen::Vector3f low((1 - left_w) * xy[0] + left_w * x1y[0], (1 - left_w) * xy[1] + left_w * x1y[1], (1 - left_w) * xy[2] + left_w * x1y[2]);
            return (1 - low_w) * low + low_w * up;
        }
    };
}
