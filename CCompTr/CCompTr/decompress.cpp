#include <vector>
#include <thread>
#include <iostream>
#include "decompress.hpp"
#include "util.hpp"
#define dictionaryCount 15

extern bool isLetter[256];
std::vector<std::vector<unsigned char>> byteDictionaries[dictionaryCount];

std::vector<std::vector<unsigned char>> CreateDictionary(std::vector<unsigned char> vs,int begin,int end) {
	std::vector<std::vector<unsigned char>> result;
	int p = begin;
	while (p < end) {
		int length = vs[p++];
		std::vector<unsigned char> part(vs.begin() + p, vs.begin() + p + length);
		result.push_back(part);
		p += length;
	}
	return result;
}

void CreateDictionaries(std::vector<unsigned char> dict) {
	int pointer = 0;
    int counter = 0;
	while (pointer < dict.size()) {
		int b1 = dict[pointer++];
		int	b2 = dict[pointer++];
		int length = (b1 << 8) + b2;
        byteDictionaries[counter++] = CreateDictionary(dict,pointer,pointer+length);
		pointer += length;
	}
}

void decode(std::vector<unsigned char> stream,unsigned int start,unsigned int end,std::vector<char>& result){
    bool prevIsAlpha = false;
    bool isAlpha = false;
    short val,dictIndex,code,length;
    unsigned int p = start;
    while (p < end) {
        val = stream[p++];
        dictIndex = (val & 240) >> 4;
        code = val & 15;
        if (dictIndex == dictionaryCount) { //Not found in dict
            length = code;
            if (code == 8)  //len > 8
                length = stream[p++];
            isAlpha = length > 0 && isLetter[stream[p]];
            if (prevIsAlpha && isAlpha)
                result.push_back(32);
            result.insert(result.end(), stream.begin()+p, stream.begin()+p+length);
            p += length;
        }
        else {
            if (code & 8) //code > 8
                code = ((code & 7) << 8) + stream[p++];
            auto& word = byteDictionaries[dictIndex][code];
            isAlpha = word.size() > 0 && isLetter[word[0]];
            if (prevIsAlpha && isAlpha)
                result.push_back(32);
            result.insert(result.end(), word.begin(), word.end());
        }
        prevIsAlpha = isAlpha;
    }
}

void Decompress(std::string fileName,unsigned char coreCount) {
    unsigned int p = 64*4; //first 4 byte is decompressed size,63 unsigned ints are flags

    std::string root = getRoot(fileName);
	std::vector<unsigned char> dict = readAllBytes(combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "dict"));
    auto streamPath = combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "stream");

    auto header = readBytes(streamPath,0,p);

    
    unsigned int decompressedSize = *(unsigned int *)&header[0];
        
	FillIsLetter();
	CreateDictionaries(dict);
    std::vector<unsigned int> flags;
    flags.reserve(64);
    flags.push_back(p);
    for(int i=1;i<64;i++)
        flags.push_back(*(unsigned int *)&header[i*4]+p);
    flags.push_back(flags[63]);

    unsigned int step = 64 / coreCount;
    
    std::vector<std::vector<char>> results;
    for(int i=0;i<64;i+=step){
        std::vector<char> vector;
        vector.reserve(decompressedSize/coreCount);
        results.push_back(vector);
    }

    std::vector<std::thread> threads;
    for(int i=0;i<64;i+=step){
        std::vector<unsigned char> stream = readBytes(streamPath, flags[i], flags[i+step]);
        threads.push_back(std::thread(decode,stream, 0, stream.size(),std::ref(results[i/step])));
    }
    for(auto &th:threads)th.join();
    
    bool first = false;
    bool second = false;
    for(int i=0;i<64;i+=step){
        appendBytes(combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "output"), results[i/step],i);
        if(i+step<64){
            first = isLetter[(unsigned char)results[i/step][results[i/step].size()-1]];
            second = i+step==64?0:results[(i+step)/step].size()==0?0:isLetter[(unsigned char)results[(i+step)/step][0]];
            if(first && second)
                appendByte(combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "output"), 32);
        }
    }
}
