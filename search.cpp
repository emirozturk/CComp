#include <vector>
#include <thread>
#include <iostream>
#include "search.hpp"
#include "util.hpp"
#define dictionaryCount 15

char indexes[256]{ 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //0
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //20
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //40
                    14, 14, 14, 14, 14,  2, 10,  7,  9,  9,  4, 11,  5, 10, 13, 13,  8,  6, 12, 12, //60
                    11, 13, 12,  0,  1, 13, 13,  3, 13, 13, 13, 14, 14, 14, 14, 14, 14,  2, 10,  7, //80
                     9,  9,  4, 11,  5, 10, 13, 13,  8,  6, 12, 12, 11, 13, 12,  0,  1, 13, 13,  3, //100
                    13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //120
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //140
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //160
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //180
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //200
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //220
                    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14 };               //240

struct searchStruct{
    int dictIndex;
    unsigned short code;
    std::vector<char> phrase;
};

searchStruct GetCode(std::vector<unsigned char> dict,std::string word) {
    int pointer = 0;
    std::vector<char> result;
    std::vector<std::vector<char>> dicts;
    while (pointer < dict.size()) {
        int b1 = dict[pointer++];
        int b2 = dict[pointer++];
        int length = (b1 << 8) + b2;
        std::vector<char> part(dict.begin() + pointer, dict.begin() + pointer + length);
        pointer += length;
        dicts.push_back(part);
    }
    
    int p = 0;
    int length = 0;
    int dictIndex = indexes[word[0]];
    unsigned short code=0;
    while(p<dicts[dictIndex].size()){
        length = dicts[dictIndex][p++];
        if(length==word.size())
        {
            std::string phrase(dicts[dictIndex].begin()+p,dicts[dictIndex].begin()+p+length);
            if(phrase == word)
                return searchStruct{.dictIndex=dictIndex,.code=code,.phrase=result};
        }
        p+=length;
        code++;
    }
    result.insert(result.end(),word.begin(),word.end());
    searchStruct x;
    x.dictIndex = dictionaryCount;
    x.phrase = result;
    return x;
}

void SearchWord(std::vector<unsigned char> stream,searchStruct& ss, unsigned int start,unsigned int end,unsigned int& count){
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
            if(dictIndex==ss.dictIndex && std::equal(ss.phrase.begin(), ss.phrase.end(), stream.begin()+p))
               count++;
            p += length;
        }
        else {
            if (code & 8) //code > 8
                code = ((code & 7) << 8) + stream[p++];
            if(dictIndex==ss.dictIndex && code == ss.code)
                count++;
        }
    }
}

unsigned int Search(std::string fileName,int coreCount,std::string word){
    std::string root = getRoot(fileName);
    std::vector<unsigned char> dict = readAllBytes(combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "dict"));
    auto streamPath = combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "stream");
    
    //unsigned int decompressedSize = *(unsigned int *)&stream[0];
    
    
    auto ss = GetCode(dict, word);
    unsigned int p = 64*4; //first 4 byte is decompressed size,63 unsigned ints are flags

    auto header = readBytes(streamPath,0,p);

    std::vector<unsigned int> flags;
    flags.reserve(64);
    flags.push_back(p);
    for(int i=1;i<64;i++)
        flags.push_back(*(unsigned int *)&header[i*4]+p);
    flags.push_back(flags[63]);

    unsigned int step = 64 / coreCount;
    
    std::vector<unsigned int> results;
    for(int i=0;i<64;i+=step)
        results.push_back(0);

    std::vector<std::thread> threads;
    for(int i=0;i<64;i+=step)
    {
        auto stream = readBytes(streamPath, flags[i], flags[i+step]);
        threads.push_back(std::thread(SearchWord,stream,std::ref(ss), 0, stream.size(),std::ref(results[i/step])));
    }
    for(auto &th:threads)th.join();
    
    unsigned int total = 0;
    for(int i=0;i<results.size();i++)
        total+=results[i];
    return total;
}
