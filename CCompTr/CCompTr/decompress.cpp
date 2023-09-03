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

/*
ç - 231 c3 a7 | 195 167 ->1 *
ö - 246 c3 b6 | 195 182 ->2 *
ü - 252 c3 bc | 195 188 ->3 *
ı - 305 c4 b1 | 196 177 ->4 *
ş - 351 c5 9f | 197 159 ->5 *

Ç - 199 c3 87 | 195 135 ->6 *
Ö - 214 c3 96 | 195 150 ->7 *
Ü - 220 c3 9c | 195 156 ->8 *
İ - 304 c4 b0 | 196 176 ->9 *
Ş - 350 c5 9e | 197 158 ->10*
*/

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
            if(word[0] >0 && word[0]<11){
                std::vector<char> replacement(2);
                if(word[0]==21)replacement = {(char)195, (char)167};
                if(word[0]==22)replacement = {(char)195, (char)182};
                if(word[0]==23)replacement = {(char)195, (char)188};
                if(word[0]==24)replacement = {(char)196, (char)177};
                if(word[0]==25)replacement = {(char)197, (char)159};
                if(word[0]==26)replacement = {(char)195, (char)135};
                if(word[0]==27)replacement = {(char)195, (char)150};
                if(word[0]==28)replacement = {(char)195, (char)156};
                if(word[0]==29)replacement = {(char)196, (char)176};
                if(word[0]==30)replacement = {(char)197, (char)158};
                word.erase(word.begin());
                word.insert(word.begin(), replacement.begin(), replacement.end());

            }
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
