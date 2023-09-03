#include <vector>
#include <array>
#include <thread>
#include "secondPass.hpp"
#include "wordPointer.hpp"
#include "util.hpp"
#include "robin_map.h"
#define dictionaryCount 15

extern std::vector<WordPointer> words;
extern std::array<tsl::robin_map<std::string, unsigned short>,dictionaryCount> splitDictionaries;
extern unsigned long fileSize;
extern std::vector<unsigned char> fileBytes;

std::vector<unsigned char> longAsBytes(unsigned long value){
    std::vector<unsigned char> result;
    result.push_back(value & 0xFF);
    result.push_back((value >> 8) & 0xFF);
    result.push_back((value >> 16) & 0xFF);
    result.push_back((value >> 24) & 0xFF);
    return result;
}

void Encode(const std::vector<unsigned char>& fileBytes,
            std::vector<unsigned char>& result,unsigned int start,unsigned int end) {
	std::string word;
	
    unsigned int index;
    result.reserve((end-start)*4);
    
    //n. sözlükteki indexler indexesn için kelimelerin içinden indis'inci olan okunuyor.
    //If dictionaries don't contain this word, word length (len) is obtained.
    //If len < 8 : 240+len is written in 1 byte and then the word is written to output.
    //If len > 8: 240+8+len is written in 2 bytes and the word is written to output after that.
    //If dictionaries conatin word 
    //If codeword <8: First four bits of 1 byte is encoded as dictionary index, 1 bit is encoded as 0 and codeword is encoded into last 3 bits.
    //If codeword >8  First four bits of 2 bytes is encoded as dictionary index, 1 bit is encoded as 1 (to read one more byte at decompression) and 12 bit is used for codeword.
	for (int i = start; i < end; i++) {
		WordPointer& pointer = words[i];
		word = std::string(fileBytes.begin() + pointer.start, fileBytes.begin() + pointer.start +pointer.length);
        index = pointer.index;
		if (splitDictionaries[index].contains(word)) {
            int code = splitDictionaries[index][word];
			if (code < 8)
                result.push_back((index << 4) + code);
			else {
				short val = (index << 12) + 2048 + code;
                char *p = (char *)&val;
                result.push_back(*(p+1));
                result.push_back(*p);
			}
		}
		else {
			unsigned char length = word.length();
			if (word.length() < 8) {
                result.push_back(240+length);
                result.insert(result.end(), word.begin(),word.end());
			}
			else {
                result.push_back(248);
                result.push_back(length);
                result.insert(result.end(), word.begin(),word.end());
			}
		}
	}
}

void secondPass(std::string fileName) {
    std::string outFile = combinePath(combinePath(getRoot(fileName), getFileNameWithoutExtension(fileName)), "stream");
    
	std::vector<char> output;
	std::array<std::vector<unsigned char>,64> results;
    std::array<unsigned int,64> sectionFlags{};

    unsigned int sectionSize = (unsigned int)words.size() / 64;
    std::vector<std::thread> threads;
    threads.reserve(63);

    for (int i = 0; i < 63; i++)
        threads.emplace_back(Encode,std::ref(fileBytes),std::ref(results[i]),i*sectionSize,(i+1)*sectionSize);
    threads.emplace_back(Encode,std::ref(fileBytes),std::ref(results[63]),63*sectionSize,words.size());
    for(auto &th:threads){th.join();}
    
    unsigned int outputSize = 0;
    for(int i=0;i<64;i++){
        outputSize+=results[i].size();
        sectionFlags[i]=outputSize;
    }
    unsigned int resultSize = 0;
    for(int i=0;i<64;i++)
        resultSize+=results[i].size();
    sectionFlags[62]=resultSize;

    appendBytes(outFile,longAsBytes(fileSize),0);
    for(int i=0;i<63;i++)
        appendBytes(outFile, longAsBytes(sectionFlags[i]), 1);

    for(auto & result : results)
        appendBytes(outFile, result, 1);
}
