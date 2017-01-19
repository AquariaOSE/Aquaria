#ifndef SUBTITLEPLAYER_H
#define SUBTITLEPLAYER_H

#include <string>
#include <vector>

struct SubLine
{
	SubLine() { timeStamp = 0; }
	float timeStamp;
	std::string line;
};

class SubtitlePlayer
{
public:
	SubtitlePlayer();
	void go(const std::string &subs);
	void update(float dt);
	void end();

	void hide(float t = 0);
	void show(float t = 0);

	bool isVisible();

	typedef std::vector<SubLine> SubLines;
	SubLines subLines;

	size_t curLine;
protected:
	bool vis, hidden;
};

#endif

