#pragma once

#include <eigen3/Eigen/Eigen>
#include <vector>
#include <limits>
#include <cmath>

#define PI 3.14159265359f

#define EPSILON 0.000001
#define MIN_F 1.17549435082228750796873653722224568e-38F
#define MAX_F 3.40282346638528859811704183484516925e+38F

inline void minimize(float& min, const float a){
    if(a < min){
        min = a;
    }
}

inline void maximize(float& max, const float a){
    if(a > max){
        max = a;
    }
}

inline void minmaxize(float& min, float& max, const float a){
    if(a < min){
        min = a;
    }
    else if(a > max){
        max = a;
    }
}

inline const float min(const float a, const float b){
    if(a < b){
        return a;
    }
    return b;
}
inline const float min(const float a, const float b, const float c){
    if(a < b && a < c){
        return a;
    }
    if(b < a && b < c){
        return b;
    }
    return c;
}

inline const float max(const float a, const float b){
    if(a > b){
        return a;
    }
    return b;
}
inline const float max(const float a, const float b, const float c){
    if(a > b && a > c){
        return a;
    }
    if(b > a && b > c){
        return b;
    }
    return c;
}

inline const Eigen::Vector3f min(const Eigen::Vector3f a, const Eigen::Vector3f b){
    return Eigen::Vector3f(min(a.x(), b.x()), min(a.y(), b.y()), min(a.z(), b.z()));
}
inline const Eigen::Vector3f min(const Eigen::Vector3f a, const Eigen::Vector3f b, const Eigen::Vector3f c){
    return Eigen::Vector3f(min(a.x(), b.x(), c.x()), min(a.y(), b.y(), c.y()), min(a.z(), b.z(), c.z()));
}

inline const Eigen::Vector3f max(const Eigen::Vector3f a, const Eigen::Vector3f b){
    return Eigen::Vector3f(max(a.x(), b.x()), max(a.y(), b.y()), max(a.z(), b.z()));
}
inline const Eigen::Vector3f max(const Eigen::Vector3f a, const Eigen::Vector3f b, const Eigen::Vector3f c){
    return Eigen::Vector3f(max(a.x(), b.x(), c.x()), max(a.y(), b.y(), c.y()), max(a.z(), b.z(), c.z()));
}


inline const bool equal(const float a, const float b){
    return ((0 < a - b && a - b < EPSILON) || (0 < b - a && b - a < EPSILON));
}
inline const bool iszero(const float a){
    return ((0 < a && a < EPSILON) || (0 > a && -a < EPSILON));
}
inline const bool no_less_than(const float a, const float b){
    return (a > b - EPSILON);
}
inline const bool no_more_than(const float a, const float b){
    return (a < b + EPSILON);
}

inline void truify(bool& to_change, bool condition){
    if(condition){
        to_change = true;
    }
}
inline void falsify(bool& to_change, bool condition){
    if(!condition){
        to_change = false;
    }
}

inline void print_progress(const int i, const int total, const std::string message){
    std::cout.flush();
    std::cout << message << ": " << i << "/" << total << "\r";
}







class Manga3DException: public std::exception{
    std::string message;
public:
    Manga3DException(const std::string& message):message(message){}
    Manga3DException(const std::string& message, const std::exception& e){
        this->message = message + ": " + e.what();
    }
    virtual const char* what() const noexcept override{
        return message.c_str();
    }

};

