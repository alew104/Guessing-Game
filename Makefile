# Makefile for Writing Make Files Example

# *****************************************************
# Variables to control Makefile operation

CXX = g++
CXXFLAGS = -std=c++11

# ****************************************************
# Targets needed to bring the executable up to date

all: hw3_client hw3_server

hw3_client: hw3_client.cpp
	$(CXX) $(CXXFLAGS) hw3_client.cpp -o hw3_client

hw3_server: hw3_server.cpp
	$(CXX) $(CXXFLAGS) hw3_server.cpp -o hw3_server -lpthread
