#ifndef STRINGBANK_H
#define STRINGBANK_H

#include <string>
#include <map>

class StringBank
{
public:
	StringBank();
	void load();

	const std::string& get(int idx);
protected:
	void _load(const std::string &file);

	typedef std::map<int, std::string> StringMap;
	StringMap stringMap;
};

#endif
