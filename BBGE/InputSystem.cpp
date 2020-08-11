#include "InputSystem.h"
#include "InputMapper.h"
#include <vector>
#include <algorithm>

namespace InputSystem {

static std::vector<IInputMapper*> s_mappers;

void handleRawInput(const RawInput *inp)
{
	size_t N = s_mappers.size();
	for(size_t i = 0; i < N; ++i)
		s_mappers[i]->input(inp);
}

void addMapper(IInputMapper *mapper)
{
	removeMapper(mapper);
	s_mappers.push_back(mapper);
}

void removeMapper(IInputMapper *mapper)
{
	s_mappers.erase(std::remove(s_mappers.begin(), s_mappers.end(), mapper), s_mappers.end());
}

} // end namespace InputSystem
