
# 清除构建目录
rm -r build

# 创建并进入build目录
mkdir -p build && cd build

# 生成构建系统
cmake ..

# 编译项目
make

# 运行程序
./lwcli_example