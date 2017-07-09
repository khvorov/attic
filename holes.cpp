#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
 
int main() {
 
    //create a functor for generating uniformly
    //distributed, contiguous memory chunk sizes between 1 and
    //MAX_CHUNK_SIZE bytes
    constexpr int32_t MAX_CHUNK_SIZE{1000000};
    std::default_random_engine randEng{};
    std::uniform_int_distribution<int32_t> distribution{
        1, MAX_CHUNK_SIZE};
 
    //load up a vector of NUM_CHUNKS chunks;
    //with each chunk randomly sized
    //between 1 and MAX_CHUNK_SIZE long
    constexpr int32_t NUM_CHUNKS{1000};
    std::vector<int32_t> chunkSizes(NUM_CHUNKS); //no init braces!
    for(auto& el : chunkSizes) {
        el = distribution(randEng);
    }
 
    //declare the allocator/deallocator function
    //before calling it.
    void allocDealloc(std::vector<int32_t>& chunkSizes,
            std::default_random_engine& randEng);
 
    //loop until our free store is so "holy"
    //that we must bow down to the lord and leave
    //the premises!
    bool tooManyHoles{};
    while(!tooManyHoles) {
        try {
            allocDealloc(chunkSizes, randEng);
        }
        catch(const std::bad_alloc&) {
            tooManyHoles = true;
        }
    }
 
    std::cout << "The Free Store Is Too "
                  << "Holy to Continue!!!"
                  << std::endl;
}
 
//the function for randomly allocating/deallocating chunks of memory
void allocDealloc(std::vector<int32_t>& chunkSizes,
        std::default_random_engine& randEng) {
 
    //create a vector for storing the dynamic pointers
    //to each allocated memory block
    std::vector<char*> handles{};
 
    //randomize the memory allocations and store each pointer
    //returned from the runtime memory allocator
    std::shuffle(chunkSizes.begin(), chunkSizes.end(), randEng);
    for(const auto& numBytes : chunkSizes) {
        handles.emplace_back(new char[numBytes]{}); //array of Bytes
        std::cout << "Allocated "
                      << numBytes << " Bytes" << std::endl;
    }
 
    //randomize the memory deallocations
    std::shuffle(handles.begin(), handles.end(), randEng);
    for(const auto& elem : handles) {
        delete [] elem;
    }
 
}