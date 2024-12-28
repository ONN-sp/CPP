#include "rapidjson/reader.h"
#include <fstream>
#include <iostream>
#include <chrono>

using namespace rapidjson;

struct MyHandler : public BaseReaderHandler<UTF8<>, MyHandler> {
    bool Null() { return true; }
    bool Bool(bool b) {  return true; }
    bool Int(int i) { return true; }
    bool Uint(unsigned u) { return true; }
    bool Int64(int64_t i) { return true; }
    bool Uint64(uint64_t u) {return true;}
    bool Double(double d) { return true;; }
    bool String(const char* str, SizeType length, bool copy) { 
        return true;
    }
    bool StartObject() { return true;}
    bool Key(const char* str, SizeType length, bool copy) { 
       return true;
    }
    bool EndObject(SizeType memberCount) { return true; }
    bool StartArray() { return true; }
    bool EndArray(SizeType elementCount) {return true; }
};

void testRapidJSON(const std::string& filename, int testNums) {
    std::ifstream file(filename);
    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    Reader reader;
	MyHandler handler;
	StringStream ss(json.c_str());
	auto start = std::chrono::high_resolution_clock::now();
	for(int i=0;i<testNums;i++)
		reader.Parse(ss, handler);
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