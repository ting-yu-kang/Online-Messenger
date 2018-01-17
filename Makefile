TARGET=client server
TARGET1=ex_client ex_server
CLIENT_OBJS=./src/client.o ./src/interface.o ./src/stdafx.o ./src/AES.o
SERVER_OBJS=./src/server.o ./src/stdafx.o ./src/AES.o
BIN=./bin
SRC=./src
GCC=g++
RM=rm -f
IP=127.0.0.1

all: $(TARGET)
	$(RM) $(SRC)/*.o
runc:
	$(BIN)/client $(IP)

runs:
	$(BIN)/server

clean:
	$(RM) $(SRC)/*~
	$(RM) $(SRC)/*.o
	$(RM) $(BIN)/*

client:$(CLIENT_OBJS)
	@echo "making: " $@
	$(GCC) -o $(BIN)/$@ $(CLIENT_OBJS)

server:$(SERVER_OBJS)
	@echo "making: " $@
	$(GCC) -o $(BIN)/$@ $(SERVER_OBJS)

%.o:%.c
	@echo "making: " $@
	@$(GCC) -c $< -o $@
