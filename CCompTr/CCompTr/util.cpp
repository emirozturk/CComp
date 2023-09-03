#include "util.hpp"
#ifdef __APPLE__
#include <filesystem>
#elif __linux__
#include <experimental/filesystem>
#endif
#include <fstream>
#include <iostream>

bool isLetter[256];

/*
ç - 231 c3 a7 | 195 167 ->128 *
ö - 246 c3 b6 | 195 182 ->129 *
ü - 252 c3 bc | 195 188 ->130 *
ı - 305 c4 b1 | 196 177 ->131 *
ş - 351 c5 9f | 197 159 ->132 *

Ç - 199 c3 87 | 195 135 ->133 *
Ö - 214 c3 96 | 195 150 ->134 *
Ü - 220 c3 9c | 195 156 ->135 *
İ - 304 c4 b0 | 196 176 ->136 *
Ş - 350 c5 9e | 197 158 ->137 *
*/

void FillIsLetter() {
    for(int i=0;i<256;i++)isLetter[i]=false;
    for(int i=48;i<58;i++)
        isLetter[i]=true;
	for (int i = 65; i <= 90; i++)
		isLetter[i] = true;
	for (int i = 97; i <= 122; i++)
		isLetter[i] = true;
    for(int i=128;i<138;i++)isLetter[i]=true;//for turkish  çöüış and ÇÖÜİŞ chars
}

std::vector<char> getBytes(unsigned short val) {
    char *p = (char *)&val;
    std::vector<char> result{*(p+1),*p};
	return result;
}

std::string getRoot(const std::string& fileName) {
	std::string directory;
	char sep = '/';
#ifdef _WIN32
	sep = '\\';
#endif
	const size_t last_slash_idx = fileName.rfind(sep);
	if (std::string::npos != last_slash_idx)
		directory = fileName.substr(0, last_slash_idx);
	return directory;
}
std::string getFileNameWithoutExtension(const std::string& fileName) {
	std::string directory;
	char sep = '/';
#ifdef _WIN32
	sep = '\\';
#endif
	const size_t last_slash_idx = fileName.rfind(sep);
	const size_t last_dot_idx = fileName.rfind('.');
	if (std::string::npos != last_slash_idx)
		directory = fileName.substr(last_slash_idx + 1, last_dot_idx - last_slash_idx - 1);
	return directory;
}
void createDirectory(const std::string& path) {
#ifdef __linux__
	std::experimental::filesystem::create_directory(path);
#else
	std::__fs::filesystem::create_directory(path);
#endif
}
std::string combinePath(const std::string& p1, const std::string& p2) {
	char sep = '/';
	std::string tmp = p1;

#ifdef _WIN32
	sep = '\\';
#endif

	if (p1[p1.length()] != sep) {
		tmp += sep;
		return(tmp + p2);
	}
	else
		return(p1 + p2);
}

size_t getFileSize(std::ifstream& file) {
	file.seekg(0, file.end);
	size_t length = file.tellg();
	file.seekg(0, file.beg);
	return length;
}

std::vector<unsigned char> readAllBytes(const std::string& fileName) {
	std::ifstream infile(fileName,std::ios::binary);
	size_t fileSize = getFileSize(infile);

	std::vector<unsigned char> inputStream;
	inputStream.resize(fileSize);
	infile.read((char *)&inputStream[0], fileSize);
	infile.close();

	return inputStream;
}

std::vector<unsigned char> readBytes(const std::string& fileName,int from,int to) {
    std::ifstream infile(fileName,std::ios::binary);
    std::vector<unsigned char> inputStream;
    inputStream.resize(to-from);
    infile.seekg(from);
    infile.read((char *)&inputStream[0], to-from);
    infile.close();

    return inputStream;
}

void writeAllBytes(const std::string& fileName, const std::vector<char>& outputStream) {
	std::ofstream outfile(fileName, std::ios::binary | std::ios::out);
	outfile.write(&outputStream[0], outputStream.size());
	outfile.flush();
	outfile.close();
}
void appendBytes(const std::string& fileName, const std::vector<char>& outputStream,int index){
    std::ofstream outfile;
    if(index==0)outfile = std::ofstream(fileName, std::ios::binary |std::ios::out);
    else outfile = std::ofstream(fileName, std::ios::binary |std::ios::app);
    outfile.write(&outputStream[0], outputStream.size());
    outfile.flush();
    outfile.close();
}
void appendBytes(const std::string& fileName, const std::vector<unsigned char>& outputStream,int index){
    std::ofstream outfile;
    if(index==0)outfile = std::ofstream(fileName, std::ios::binary |std::ios::out);
    else outfile = std::ofstream(fileName, std::ios::binary |std::ios::app);
    outfile.write((char *)&outputStream[0], outputStream.size());
    outfile.flush();
    outfile.close();
}
void appendByte(const std::string& fileName,  unsigned char byte){
    std::ofstream outfile(fileName, std::ios::binary |std::ios::app);
    outfile << byte;
    outfile.flush();
    outfile.close();
}
