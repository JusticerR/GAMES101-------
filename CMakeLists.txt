cmake_minimum_required(VERSION 3.10)
project(GAMES101Rasterizer)

# 1. 强制指定 C++ 与现代标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options("/utf-8")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# 2. 外部依赖项：利用系统的环境变量自动寻找
# (要求使用者通过包管理器或环境变量自行安装 OpenCV 和 Eigen3)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

# 3. 收集所有源代码文件 (实现文件)
set(SOURCES
    src/main.cpp
    src/rasterizer.cpp
    src/Texture.cpp
    src/Triangle.cpp
)

# 4. 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 5. 指定头文件包括路径
# PUBLIC 意味着如果这个工程被别的工程引用，头文件路径也能传递
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include    # 项目内部的头文件
    ${EIGEN3_INCLUDE_DIRS}                 # 系统中 Eigen3 头文件
    ${OpenCV_INCLUDE_DIRS}                 # 系统中 OpenCV 头文件
)

# 6. 链接外部依赖库 (仅需要链接 OpenCV，因为 Eigen 是纯头文件库)
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${OpenCV_LIBS}
)