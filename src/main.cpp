#include <iostream>
#include <vector>
#include <functional>
#include <opencv2/opencv.hpp>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"
//用顶点插值三角形中间的法向量是phong shading逐像素）用顶点算颜色插值中间的颜色是 gouraud shading 逐顶点着色 一个面算一个法向量算一次颜色是逐面flat shading
Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1,0,0,-eye_pos[0],
                 0,1,0,-eye_pos[1],
                 0,0,1,-eye_pos[2],
                 0,0,0,1;

    view = translate*view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
                0, 1, 0, 0,
                -sin(angle), 0, cos(angle), 0,
                0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,
              0, 2.5, 0, 0,
              0, 0, 2.5, 0,
              0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1;

    return translate * rotation * scale;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
   Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
    float rad = eye_fov / 180.0f * static_cast<float>(MY_PI);
    float t = std::tan(rad / 2.0f) * std::abs(zNear);
    float r = t * aspect_ratio;
    float l = -r;
    float b = -t;
    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.
Eigen::Matrix4f persp2ortho;
    persp2ortho << zNear, 0,     0,             0,
                   0,     zNear, 0,             0,
                   0,     0,     zNear + zFar, -zNear * zFar,
                   0,     0,     1,             0;

    Eigen::Matrix4f ortho;
    ortho << -2.0f / (r - l), 0,                0,               -(r + l) / (r - l),
             0,              -2.0f / (t - b),   0,               -(t + b) / (t - b),
             0,              0,                2.0f / (zNear - zFar), -(zNear + zFar) / (zNear - zFar),
             0,              0,                0,               1;

    projection = ortho * persp2ortho;
    return projection;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload& payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

static Eigen::Vector3f reflect(const Eigen::Vector3f& vec, const Eigen::Vector3f& axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    // 关键改动：直接从纹理中获取 kd，注意除以 255 归一化到 [0,1]
    Eigen::Vector3f kd = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y()) / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // ...以下逻辑与 phong_fragment_shader 完全一致...
        Eigen::Vector3f l = (light.position - point).normalized();
        Eigen::Vector3f v = (eye_pos - point).normalized();
        Eigen::Vector3f h = (l + v).normalized();
        float r2 = (light.position - point).squaredNorm();

        Eigen::Vector3f diffuse = kd.cwiseProduct(light.intensity / r2) * std::max(0.0f, normal.dot(l));
        Eigen::Vector3f specular = ks.cwiseProduct(light.intensity / r2) * std::pow(std::max(0.0f, normal.dot(h)), p);

        result_color += (diffuse + specular);
    }
    result_color += ka.cwiseProduct(amb_light_intensity);

    return result_color * 255.f;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload& payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f result_color = {0, 0, 0};
    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        for (auto& light : lights)
    {
        // 1. 各个基础向量的准备
        Eigen::Vector3f l = (light.position - point).normalized(); // 光源方向
        Eigen::Vector3f v = (eye_pos - point).normalized();       // 视角方向
        Eigen::Vector3f h = (l + v).normalized();                 // 半程向量
        
        // 2. 能量衰减 (距离越远越暗)
        float r2 = (light.position - point).squaredNorm();        
        Eigen::Vector3f light_intensity = light.intensity / r2;

        // 3. 漫反射 Diffuse
        // kd 是物体颜色，max(0, n·l) 确保背面不亮
        Eigen::Vector3f diffuse = kd.cwiseProduct(light_intensity) * std::max(0.0f, normal.dot(l));

        // 4. 高光 Specular
        // ks 是高光系数，p 是反射指数
        Eigen::Vector3f specular = ks.cwiseProduct(light_intensity) * std::pow(std::max(0.0f, normal.dot(h)), p);

        result_color += (diffuse + specular);
    }
    // 5. 环境光 Ambient (不管多少灯，环境光通常只算一次)
    result_color += ka.cwiseProduct(amb_light_intensity);

   for (auto& light : lights)
    {
        // 1. 各个基础向量的准备
        Eigen::Vector3f l = (light.position - point).normalized(); // 光源方向
        Eigen::Vector3f v = (eye_pos - point).normalized();       // 视角方向
        Eigen::Vector3f h = (l + v).normalized();                 // 半程向量
        
        // 2. 能量衰减 (距离越远越暗)
        float r2 = (light.position - point).squaredNorm();        
        Eigen::Vector3f light_intensity = light.intensity / r2;

        // 3. 漫反射 Diffuse
        // kd 是物体颜色，max(0, n·l) 确保背面不亮
        Eigen::Vector3f diffuse = kd.cwiseProduct(light_intensity) * std::max(0.0f, normal.dot(l));

        // 4. 高光 Specular
        // ks 是高光系数，p 是反射指数
        Eigen::Vector3f specular = ks.cwiseProduct(light_intensity) * std::pow(std::max(0.0f, normal.dot(h)), p);

        result_color += (diffuse + specular);
    }
    // 5. 环境光 Ambient (不管多少灯，环境光通常只算一次)
    result_color += ka.cwiseProduct(amb_light_intensity);

    return result_color * 255.f;
    }

   
}

Eigen::Vector3f flat_fragment_shader(const fragment_shader_payload& payload)
{
    // 使用当前像素（即三角形面）的法向量直接映射为 RGB 颜色
    // normal 已经在 rasterizer.cpp 中被设置为该三角形的面法线
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    float kh = 0.2, kn = 0.1;
    
    // TODO: Implement displacement mapping here
    float x = normal.x();
    float y = normal.y();
    float z = normal.z();
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = (float)payload.texture->width;
    float h = (float)payload.texture->height;

    // 1. 构造 TBN 矩阵
    Eigen::Vector3f t(x * y / std::sqrt(x * x + z * z), std::sqrt(x * x + z * z), z * y / std::sqrt(x * x + z * z));
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN << t, b, normal;

    // 2. 计算高度梯度 dU, dV
    float dU = kh * kn * (payload.texture->getColor(u + 1.0f / w, v).norm() - payload.texture->getColor(u, v).norm());
    float dV = kh * kn * (payload.texture->getColor(u, v + 1.0f / h).norm() - payload.texture->getColor(u, v).norm());

    // 3. 位移贴图特有：真实移动像素点在 3D 空间的位置 (沿原始法线移动)
    point = point + (kn * normal * payload.texture->getColor(u, v).norm());

    // 4. 更新法线方向 (用于后续光照计算)
    Eigen::Vector3f ln(-dU, -dV, 1.0f);
    normal = (TBN * ln).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto& light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular* 
        // components are. Then, accumulate that result on the *result_color* object.
        Eigen::Vector3f light_dir = (light.position - point).normalized();
        Eigen::Vector3f view_dir = (eye_pos - point).normalized();
        Eigen::Vector3f half_vec = (light_dir + view_dir).normalized();

        float r2 = (light.position - point).squaredNorm();
        Eigen::Vector3f intensity = light.intensity / r2;

        // 漫反射与高光计算 (使用更新后的 point 和 normal)
        Eigen::Vector3f diffuse = kd.cwiseProduct(intensity) * std::max(0.0f, normal.dot(light_dir));
        Eigen::Vector3f specular = ks.cwiseProduct(intensity) * std::pow(std::max(0.0f, normal.dot(half_vec)), p);

        result_color += (diffuse + specular);
    }
    // 加上环境光
    result_color += ka.cwiseProduct(amb_light_intensity);


    return result_color * 255.f;
}


Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload& payload)
{
    
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color; 
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;


    float kh = 0.2, kn = 0.1;

    // TODO: Implement bump mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Normal n = normalize(TBN * ln)
    float x = normal.x();
    float y = normal.y();
    float z = normal.z();

    // 1. 获取 UV 坐标和纹理尺寸
    float u = payload.tex_coords.x();
    float v = payload.tex_coords.y();
    float w = (float)payload.texture->width;
    float h = (float)payload.texture->height;

    // 2. 构造 TBN 矩阵
    // 注意：t 向量的构造需要考虑分母为 0 的情况，这里简化实现或使用作业提供的公式
    Eigen::Vector3f t(x * y / std::sqrt(x * x + z * z), std::sqrt(x * x + z * z), z * y / std::sqrt(x * x + z * z));
    Eigen::Vector3f b = normal.cross(t);
    Eigen::Matrix3f TBN;
    TBN << t, b, normal;

    // 3. 计算高度梯度 dU, dV (注意：h(u,v) 是纹理颜色的模长)
    float dU = kh * kn * (payload.texture->getColor(u + 1.0f / w, v).norm() - payload.texture->getColor(u, v).norm());
    float dV = kh * kn * (payload.texture->getColor(u, v + 1.0f / h).norm() - payload.texture->getColor(u, v).norm());

    // 4. 构建局部法线并变换回世界空间
    Eigen::Vector3f ln(-dU, -dV, 1.0f);
    normal = (TBN * ln).normalized();

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = normal;

    return result_color * 255.f;
}

int main(int argc, const char** argv)
{
    std::vector<Triangle*> TriangleList;

    float angle = 140.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    std::string obj_path = "models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile("models/spot/spot_triangulated_good.obj");
    if (!loadout) {
        std::cerr << "Error: Could not find model file at 'models/spot/spot_triangulated_good.obj'. "
                  << "Please ensure you are running the executable from the project root directory." << std::endl;
        return -1;
    }
    for(auto mesh:Loader.LoadedMeshes)
    {
        for(int i=0;i<mesh.Vertices.size();i+=3)
        {
            Triangle* t = new Triangle();
            for(int j=0;j<3;j++)
            {
                t->setVertex(j,Vector4f(mesh.Vertices[i+j].Position.X,mesh.Vertices[i+j].Position.Y,mesh.Vertices[i+j].Position.Z,1.0));
                t->setNormal(j,Vector3f(mesh.Vertices[i+j].Normal.X,mesh.Vertices[i+j].Normal.Y,mesh.Vertices[i+j].Normal.Z));
                t->setTexCoord(j,Vector2f(mesh.Vertices[i+j].TextureCoordinate.X, mesh.Vertices[i+j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(700, 700);

    auto texture_path = "hmap.jpg";
    r.set_texture(Texture(obj_path + texture_path));

    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = phong_fragment_shader;

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc == 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = displacement_fragment_shader;
        }
        else if (argc == 3 && std::string(argv[2]) == "flat")
        {
            std::cout << "Rasterizing using the flat shading\n";
            active_shader = flat_fragment_shader;
            r.set_flat_shading(true);
        }
    }

    Eigen::Vector3f eye_pos = {0,0,10};

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    while(key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45.0, 1, 0.1, 50));

        //r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        r.draw(TriangleList);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);
        key = cv::waitKey(10);

        if (key == 'a' )
        {
            angle -= 0.1;
        }
        else if (key == 'd')
        {
            angle += 0.1;
        }

    }
    return 0;
}
