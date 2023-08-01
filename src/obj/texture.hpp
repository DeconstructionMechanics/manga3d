#pragma once

#include "../global.hpp"

namespace Tex{
    class Texture{
    public:
        int height;
        int width;
        float* tex;
        cv::Mat image;

        Texture(const std::string& text_path){
            image = cv::imread(text_path, cv::IMREAD_COLOR);
            if(image.empty()){
                throw Manga3DException("Tex: texture image is not opened, " + text_path);
            }
            cv::cvtColor(image, image, cv::COLOR_BGR2RGB);
            image.convertTo(image, CV_32FC3, 1.0 / 255.0);
            this->height = image.rows;
            this->width = image.cols;
            this->tex = image.ptr<float>(0);
        }


        Eigen::Vector3f bilinear_sampling(float u, float v){
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
            float* xy = tex + ((x - 1) + (y - 1) * width) * 3;
            float* x1y = xy + 3;
            float* xy1 = xy + width * 3;
            float* x1y1 = xy1 + 3;
            float left_w = u + 0.5 - x;
            float low_w = v + 0.5 - y;
            Eigen::Vector3f up(left_w * xy1[0] + (1 - left_w) * x1y1[0], left_w * xy1[1] + (1 - left_w) * x1y1[1], left_w * xy1[2] + (1 - left_w) * x1y1[2]);
            Eigen::Vector3f low(left_w * xy[0] + (1 - left_w) * x1y[0], left_w * xy[1] + (1 - left_w) * x1y[1], left_w * xy[2] + (1 - left_w) * x1y[2]);
            return low_w * low + (1 - low_w) * up;
        }
    };
}
