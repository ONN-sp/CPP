#include "json/json.h"
#include <fstream>
#include <iostream>
#include <chrono>

void testRapidJSON(const std::string& filename, int testNums) {
    std::ifstream file(filename);
	if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	Json::Reader reader;
    Json::Value root;
	auto start = std::chrono::high_resolution_clock::now();
	for(int i=0;i<testNums;i++)
		reader.parse(json, root);
	auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
	std::cout << "RapidJSON parse time: " << duration.count() << " seconds" << std::endl;
}

int main() {
	int testNums = 1000;
    const std::string filename = "sample.json";
	testRapidJSON(filename, testNums);
	return 0;
}