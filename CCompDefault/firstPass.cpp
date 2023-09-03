#include <array>
#include <thread>
#include "firstPass.hpp"
#include "wordPointer.hpp"
#include "util.hpp"
#include "robin_map.h"
#include "StartEnd.hpp"
#include <iostream>

#define dictionaryCount 15


                      // 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19
char dictIndexes[256]{  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, //0
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

extern bool isLetter[256];  //Spaceless word model needs space check. To speed up this check isLetter array is filled at the beginning.
extern std::vector<WordPointer> words;  //Word length and start index values. Filled in first pass 
extern std::array<tsl::robin_map<std::string, unsigned short>,dictionaryCount> splitDictionaries;//15 dictionaries. 2048 words each
extern unsigned long fileSize;

extern std::vector<unsigned char> fileBytes;

//To sort key values in map
template <typename T1, typename T2>
struct less_second {
    typedef std::pair<T1, T2> type;
    bool operator ()(type const& a, type const& b) const {
        return a.second > b.second;
    }
};

//robin map is used to obtain fast lookup. Each element added into a vector and sorted by frequency (Biggest to smallest). Then 2048 of them are taken.
void sortMapbyValue(const tsl::robin_map<std::string, unsigned int>& map,std::array<std::string,2048>& result) {
    std::vector<std::pair<std::string,int>> robinList(map.begin(),map.end());
    std::sort(robinList.begin(), robinList.end(), less_second<std::string, int>());
    size_t size = robinList.size()>2048?2048:robinList.size();
    for (int i = 0; i < size; i++)
        result[i]=robinList[i].first;
}

//Dictionaries are converted into streams to write into dict file. 
void createDictionaryStream(const std::array<std::string,2048>& array,std::vector<char>& result) {
    int length = 0;
    for(int i=0;i<2048;i++)
        length+=array[i].size();
    result.reserve(length);
    for (int i = 0; i < array.size(); i++) {
        result.push_back(array[i].size());
        result.insert(result.end(), array[i].begin(), array[i].end());
    }
}

void createSplitDictsAndStreams(tsl::robin_map<std::string, unsigned int>& WordFrequencies,
                                std::array<std::string,2048>array,
                                tsl::robin_map<std::string, unsigned short>& splitDictionary,
                                std::vector<char>& partialStream){
    sortMapbyValue(WordFrequencies,array);
    for (int j = 0; j < array.size(); j++)
        splitDictionary[array[j]] = j;
    
    createDictionaryStream(array,partialStream);
}

void countWords(const std::vector<unsigned char>& inputBytes,StartEnd startEnd,unsigned int& counter){
    unsigned int Pointer = startEnd.start;
    unsigned int Start = startEnd.start;
    unsigned int Length = startEnd.end;
    while (Pointer < Length)
    {
        Start = Pointer;
        if (Pointer < Length && isLetter[inputBytes[Pointer]])
            while (Pointer < Length && isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                Pointer++;
        else
        {
            if (inputBytes[Pointer] != ' ')
                while (Pointer < Length && !isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                    Pointer++;
            else
            {
                Pointer++;
                if (Pointer < Length && isLetter[inputBytes[Pointer]])
                {
                    Start = Pointer;
                    while (Pointer < Length && isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                        Pointer++;
                }
                else
                    while (Pointer < Length && !isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                        Pointer++;
            }
        }
        counter++;
    }
    if (Length-Start > 0)
        counter++;
}

void extractWords(const std::vector<unsigned char>& inputBytes,StartEnd startEnd,int coreCount,unsigned int& wordCount, std::array<tsl::robin_map<std::string, unsigned int>,dictionaryCount>& result,unsigned int reservationSize,int mode){
    unsigned int Pointer = startEnd.start;
    unsigned int Start = startEnd.start;
    unsigned int counter = wordCount;
    unsigned int Length = startEnd.end;
    std::string word;

    WordPointer wordPointer;

    for (int j = 0; j < dictionaryCount-1; j++){
        result[j] = tsl::robin_map<std::string, unsigned int>();
        result[j].reserve(reservationSize);
    }

    while (Pointer < Length)
    {
        Start = Pointer;
        if (Pointer < Length && isLetter[inputBytes[Pointer]])
            while (Pointer < Length && isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                Pointer++;
        else
        {
            if (inputBytes[Pointer] != ' ')
                while (Pointer < Length && !isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                    Pointer++;
            else
            {
                Pointer++;
                if (Pointer < Length && isLetter[inputBytes[Pointer]])
                {
                    Start = Pointer;
                    while (Pointer < Length && isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                        Pointer++;
                }
                else
                    while (Pointer < Length && !isLetter[inputBytes[Pointer]] && Pointer-Start<250)
                        Pointer++;
            }
        }
        wordPointer.start = Start;
        wordPointer.length = Pointer-Start;
        word = std::string(inputBytes.begin() + Start, inputBytes.begin() + Pointer);
        int index = dictIndexes[(unsigned char)word[0]];
        wordPointer.index = index;

        
        if(Pointer<((float)Length)*((float)mode/100)){
            unsigned int value = result[index][word];
            result[index][word] = value + 1;
        }

        words[counter++] = wordPointer;
    }
    if (word.size() > 0)
    {
        int index = dictIndexes[(unsigned char)word[0]];

        unsigned int value = result[index][word];
        result[index][word] = value + 1;

        wordPointer.start = Pointer;
        wordPointer.length = Length-Pointer;
        wordPointer.index = index;
        words[counter++] = wordPointer;
    }
}

std::vector<char> firstPass(const std::string fileName,int coreCount,int mode) {
    fileBytes = readAllBytes(fileName);
    FillIsLetter(); //Isletter array is created. Numbers and [a-zA-Z] values are 1 others are 0

    fileSize = fileBytes.size();
    size_t Length = fileSize;
    
    
    unsigned int sectionSize = Length/coreCount;
        
    //This part is for correct decompression. 
    //Threads splits input text into blocks. If a block ends with a part of a word, at decompression state these are interpreted as two words. 
    //That means at decompression, there will be a space character between the word's split parts. To prevent this, block sizes are extended to include whole word.
    //startEnd structure is used for keeping start and end indexes of blocks
    std::vector<StartEnd> startsandEnds;
        
    unsigned int start = 0;
    unsigned int end = sectionSize;
    for(unsigned int i=0;i<coreCount-1;i++){
        while(isLetter[fileBytes[end]])end++;
        while(!isLetter[fileBytes[end]])end++;
        startsandEnds.push_back(StartEnd{.start=start,.end=end});
        start = end;
        end = end+sectionSize;
    }
    startsandEnds.push_back(StartEnd{.start=start,.end=(unsigned int)Length});

    //Number of CoreCount threads are generated. File is split into CoreCount threads. Each thread obtains frequencies and extracts words.
    
    std::vector<std::thread> countingThreads;
    std::vector<unsigned int> sums;
    sums.resize(coreCount+1);
    for(int i=0;i<coreCount;i++)
        countingThreads.push_back(std::thread(countWords,std::ref(fileBytes),startsandEnds[i],std::ref(sums[i+1])));

    for(auto &th:countingThreads)th.join();
    
    for(int i=2;i<coreCount+1;i++)
        sums[i]+=sums[i-1];
    
    long wordCount = sums[coreCount];
    words.resize(wordCount);
    
    //15 wordFrequency arrays for 15 different dictionaries. It is for speeding up the lookup operation by reducing element count.
    std::array<tsl::robin_map<std::string, unsigned int>,dictionaryCount> WordFrequencies;

    unsigned int reservationSize = wordCount*12/sqrt(fileSize);
    //Reservation is for performance. It is statistically obtained as inputsize/6000 and could be changed to increase performance in different situations.
    for (int i = 0; i < dictionaryCount; i++) {
        WordFrequencies[i] = tsl::robin_map<std::string,unsigned int>();
        WordFrequencies[i].reserve(reservationSize);
    }
    
    std::array<tsl::robin_map<std::string, unsigned int>,dictionaryCount> results[coreCount];
        
    std::vector<std::thread> threadList;
    for(int i=0;i<coreCount;i++)
        threadList.push_back(std::thread(extractWords,std::ref(fileBytes),startsandEnds[i],coreCount,std::ref(sums[i]), std::ref(results[i]),reservationSize,mode));
    
    for(auto &th:threadList)
        th.join();
    
    //After obtaining words, words and their frequencies are concatenated like reducing after mapping
    for(int i=0;i<coreCount;i++)
        for(int j=0;j<dictionaryCount;j++){
            std::vector<std::pair<std::string,unsigned int>> robinList(results[i][j].begin(),results[i][j].end());
            for(auto &pair:robinList)
                WordFrequencies[j][pair.first]+=pair.second;
        }
    
    //Each dictionary is generated with a different thread.
    std::vector<char> dictionaryStream;
    std::array<std::vector<char>,dictionaryCount> partialStreams;
    for(int i=0;i<partialStreams.size();i++)
        partialStreams[i].reserve(fileBytes.size()/25);
    
    std::array<std::array<std::string,2048>,dictionaryCount> arrays;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < dictionaryCount; i++)
       threads.push_back(std::thread(createSplitDictsAndStreams,std::ref(WordFrequencies[i]),std::ref(arrays[i]),std::ref(splitDictionaries[i]),std::ref(partialStreams[i])));
    
    for(auto &th:threads)
        th.join();
    
    //Each dictionary vector from threads are combined and dictionary stream is generated. 
    
    int size = 0;
    for(int i=0;i<dictionaryCount;i++)
        size+=partialStreams[i].size();
    dictionaryStream.reserve(size + 2 * dictionaryCount); 
    for(int i=0;i<dictionaryCount;i++){
        std::vector<char> bytes = getBytes(partialStreams[i].size());
        dictionaryStream.insert(dictionaryStream.end(), bytes.begin(), bytes.end());
        dictionaryStream.insert(dictionaryStream.end(), partialStreams[i].begin(), partialStreams[i].end());
    }
    return dictionaryStream;
}
