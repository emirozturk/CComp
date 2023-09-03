#ifndef util_hpp
#define util_hpp
#include <string>
#include <vector>

void FillIsLetter();
std::string getRoot(const std::string& fileName);
void createDirectory(const std::string& path);
std::string combinePath(const std::string& p1, const std::string& p2);
size_t getFileSize(std::ifstream& file);
std::string getFileNameWithoutExtension(const std::string& fileName);
std::vector<char> getBytes(unsigned short val);
std::vector<unsigned char> readAllBytes(const std::string& fileName);
std::vector<unsigned char> readBytes(const std::string& fileName,int from,int to);
void writeAllBytes(const std::string& fileName, const std::vector<char>& outputStream);
void appendBytes(const std::string& fileName, const std::vector<char>& outputStream,int index);
void appendBytes(const std::string& fileName, const std::vector<unsigned char>& outputStream,int index);
void appendByte(const std::string& fileName, unsigned char byte);
#endif /* util_hpp */
