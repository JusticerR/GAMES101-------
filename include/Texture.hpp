//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <algorithm>
#include <map>
#include <string>
#include <opencv2/opencv.hpp>
class Texture{
private:
    cv::Mat image_data;

public:
    Texture(const std::string& name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        // 1. 强制限幅在 [0.001, 0.999] 之间，绝对防止计算出等于 width/height 的索引
        if (u < 0) u = 0; if (u > 1) u = 1;
        if (v < 0) v = 0; if (v > 1) v = 1;

        // 2. 转换为整数坐标并确保不越界 (减去1是为了防止索引等于宽/高)
        auto u_img = u * (width - 1);
        auto v_img = (1 - v) * (height - 1);
        
        // 3. 直接读取
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

};
#endif //RASTERIZER_TEXTURE_H
