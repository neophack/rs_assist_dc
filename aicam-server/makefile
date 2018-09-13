INC_PATH = -I ./inc
	#-I ./local/mysql/include/mysql
#LIB_PATH = -L ./local/mysql/lib/mysql
SRC_PATH = ./src
OBJ_PATH = ./obj
all:
	g++ -g -std=c++11 -c $(SRC_PATH)/tcp_server.cpp -o $(OBJ_PATH)/tcp_server.o $(INC_PATH) 
	g++ -g -c $(SRC_PATH)/unpack.cpp -o $(OBJ_PATH)/unpack.o $(INC_PATH) 
	g++ -g -pthread  $(OBJ_PATH)/tcp_server.o $(OBJ_PATH)/unpack.o $(SRC_PATH)/main.cpp $(INC_PATH) $(LIB_PATH) -lmysqlclient -o collect_goods_server
