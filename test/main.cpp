#include "gallocator.h"
#include <vector>

int main()
{
    std::vector<int, SharedAllocator<int>> vec;
	return 0;
}