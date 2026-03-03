# 编译器和编译选项
CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

# 包含的库：多线程和 MySQL 客户端
LIBS = -lpthread -lmysqlclient

# 所有的源码文件 (自动扫描当前目录下的所有 .cpp)
SOURCES = $(wildcard *.cpp)
# 将 .cpp 对应的文件名换成 .o
OBJS = $(SOURCES:.cpp=.o)

# 最终生成的可执行文件名
TARGET = server

# 默认规则
$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

# 编译每个 .cpp 文件生成 .o
%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

# 清理编译产生的垃圾
clean:
	rm -rf $(OBJS) $(TARGET)