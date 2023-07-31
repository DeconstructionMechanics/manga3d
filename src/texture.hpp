#pragma once

#include "global.hpp"

namespace Tex{
    class Texture{
    public:
        Texture(std::string text_path){
            cv::Mat image = cv::imread(text_path, cv::IMREAD_UNCHANGED);
            if(image.empty()){
                throw Manga3DException("Tex: texture image is not opened, " + text_path);
            }
        }
    };
}
