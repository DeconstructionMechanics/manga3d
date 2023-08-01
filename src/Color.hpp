#pragma once

#include "global.hpp"


namespace Raster{
    class Color;
}

/*
heap data inside
*/
class Raster::Color{
public:
    enum class ImageColor{
        FULLCOLORALPHA = 4,
        FULLCOLOR = 3,
        BLACKWHITEALPHA = 2,
        BLACKWHITE = 1
    };
    std::string imgcolor_2_string(ImageColor color) const{
        switch(color){
        case ImageColor::FULLCOLORALPHA:
            return "FULLCOLORALPHA";
        case ImageColor::FULLCOLOR:
            return "FULLCOLOR";
        case ImageColor::BLACKWHITEALPHA:
            return "BLACKWHITEALPHA";
        default:
            return "BLACKWHITEALPHA";
        }
    }
    ImageColor image_color;
    float* color;
    /*
    color is allocated on the heap,
    using ~Color() to delete
    */
    Color(float r, float g, float b, float alpha):image_color(ImageColor::FULLCOLORALPHA){
        color = new float[4];
        color[0] = b;
        color[1] = g;
        color[2] = r;
        color[3] = alpha;
    }
    Color(float r, float g, float b):image_color(ImageColor::FULLCOLOR){
        color = new float[3];
        color[0] = b;
        color[1] = g;
        color[2] = r;
    }
    Color(float g, float alpha):image_color(ImageColor::BLACKWHITEALPHA){
        color = new float[2];
        color[0] = g;
        color[1] = alpha;
    }
    Color(float g): image_color(ImageColor::BLACKWHITE){
        color = new float[1];
        color[0] = g;
    }
    Color(ImageColor image_color, float g, float alpha): image_color(image_color){
        color = new float[(int)image_color];
        for(int i = 0;i < (int)image_color - 1;i++){
            color[i] = g;
        }
        int last = (int)image_color - 1;
        if(last % 2 == 1){
            color[last] = alpha;
        }
        else{
            color[last] = g;
        }
    }
    ~Color(){
        if(color){
            delete[] color;
            color = nullptr;
        }
    }
    Color(const Color& other){
        this->image_color = other.image_color;
        this->color = new float[(int)other.image_color];
        for(int i = 0;i < (int)image_color;i++){
            this->color[i] = other.color[i];
        }
    }
    inline Color& operator=(const Color& other){
        if(this->color){
            delete[] this->color;
            this->color = nullptr;
        }
        this->image_color = other.image_color;
        this->color = new float[(int)other.image_color];
        for(int i = 0;i < (int)image_color;i++){
            this->color[i] = other.color[i];
        }
        return *this;
    }

    inline void color_assign_fullcoloralpha(float* buff) const{
        if(this->image_color != ImageColor::FULLCOLORALPHA){
            throw Manga3DException("Color assignment unmatch, expect FULLCOLORALPHA, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
        buff[2] = color[2];
        buff[3] = color[3];
    }
    inline void color_assign_fullcolor(float* buff) const{
        if(this->image_color != ImageColor::FULLCOLOR){
            throw Manga3DException("Color assignment unmatch, expect FULLCOLOR, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
        buff[2] = color[2];
    }
    inline void color_assign_blackwhitealpha(float* buff) const{
        if(this->image_color != ImageColor::BLACKWHITEALPHA){
            throw Manga3DException("Color assignment unmatch, expect BLACKWHITEALPHA, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
        buff[1] = color[1];
    }
    inline void color_assign_blackwhite(float* buff) const{
        if(this->image_color != ImageColor::BLACKWHITE){
            throw Manga3DException("Color assignment unmatch, expect BLACKWHITE, get " + imgcolor_2_string(image_color));
        }
        buff[0] = color[0];
    }
    inline friend void color_assign(const Color& color, float* ptr){
        switch(color.image_color){
        case ImageColor::FULLCOLORALPHA:
            color.color_assign_fullcoloralpha(ptr);
            break;
        case ImageColor::FULLCOLOR:
            color.color_assign_fullcolor(ptr);
            break;
        case ImageColor::BLACKWHITEALPHA:
            color.color_assign_blackwhitealpha(ptr);
            break;
        default:
            color.color_assign_blackwhite(ptr);
            break;
        }
    }

    inline bool operator!=(Color& compare_color){
        if(this->image_color == compare_color.image_color){
            for(int i = 0;i < (int)this->image_color;i++){
                if(this->color[i] != compare_color.color[i]){
                    return false;
                }
            }
            return true;
        }
        return false;
    }
};

