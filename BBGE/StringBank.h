#ifndef STRINGBANK_H
#define STRINGBANK_H

#include <string>
#include <map>

class StringBank
{
public:
	StringBank();
	bool load(const std::string &file);
	void clear();
	bool empty() const;
	void set(int idx, const char *str);
	const std::string& get(int idx) const;
protected:

	typedef std::map<int, std::string> StringMap;
	StringMap stringMap;
};

extern StringBank stringbank;

#endif
