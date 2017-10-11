#include "supermalloc_allocator.h"
#include <cool/Out.h>
#include <cool/pretty_name.h>
#include <iostream>
#include <unordered_map>
#include <string>

template<typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
using SMunordered_map = std::unordered_map<Key, T, Hash, Pred, supermalloc::allocator<std::pair<const Key, T>>>;

int main()
{
    SMunordered_map<int, char> mis;
    mis[2] = '2';
    mis[3] = '3';
    mis[5] = '5';
    mis[7] = '7';

    std::cout << cool::pretty_name(mis) << '\n' << cool::Out(mis) << '\n';

    return 0;
}
