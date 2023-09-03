#include "compress.hpp"
#include "wordPointer.hpp"
#include "firstPass.hpp"
#include "secondPass.hpp"
#include "util.hpp"
#include "robin_map.h"
std::vector<WordPointer> words;
std::array<tsl::robin_map<std::string, unsigned short>,dictionaryCount> splitDictionaries;
std::vector<unsigned char> fileBytes;
unsigned long fileSize;

void Compress(std::string fileName,int coreCount,int mode) {
	std::string root = getRoot(fileName);
	createDirectory(combinePath(root, getFileNameWithoutExtension(fileName)));

	std::vector<char> dictionaryStream = firstPass(fileName,coreCount,mode);
	writeAllBytes(combinePath(combinePath(root, getFileNameWithoutExtension(fileName)), "dict"), dictionaryStream);
	secondPass(fileName);
}
