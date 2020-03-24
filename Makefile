MAKE = make
TARGET = my_code
SOURCE = main.cpp

default: 
	mpicxx -std=c++11 -o $(TARGET) $(SOURCE)