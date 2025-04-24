# 编译器和编译选项
CC = gcc
CFLAGS = -pthread -Wall
LDFLAGS = -lm  # 链接数学库

# 可执行文件的目标
CLIENT = client
SERVER = server

# 源代码文件
CLIENT_SRC = client.c
SERVER_SRC = server.c

# 对应的目标对象文件
CLIENT_OBJ = client.o
SERVER_OBJ = server.o

# 默认目标
all: $(CLIENT) $(SERVER)

# 客户端目标
$(CLIENT): $(CLIENT_OBJ)
	$(CC) -o $(CLIENT) $(CLIENT_OBJ) -pthread $(LDFLAGS)

# 服务器目标
$(SERVER): $(SERVER_OBJ)
	$(CC) -o $(SERVER) $(SERVER_OBJ) -pthread $(LDFLAGS)

# 编译客户端对象文件
$(CLIENT_OBJ): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

# 编译服务器对象文件
$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

# 清理目标
clean:
	rm -f $(CLIENT) $(SERVER) $(CLIENT_OBJ) $(SERVER_OBJ)

# 伪目标
.PHONY: all clean

