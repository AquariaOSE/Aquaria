#ifndef STRINGBANK_H
#define STRINGBANK_H

#include <string>
#include <map>

// Some strings used in BBGE
enum StringBankIndexBBGE
{
	SB_BBGE_NO_KEY = 2153,
	SB_BBGE_INVALID_KEY_ID = 2154,
	SB_BBGE_INVALID_JOY_BTN = 2155,
	SB_BBGE_INVALID_MOUSE_BTN = 2156,
	SB_BBGE_INVALID_JOY_AXIS_POS = 2157,
	SB_BBGE_INVALID_JOY_AXIS_NEG = 2158,
};

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
