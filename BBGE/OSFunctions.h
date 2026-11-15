#ifndef BBGE_OSFUNCTIONS_H
#define BBGE_OSFUNCTIONS_H

#include <string>

void initIcon(void *screen);
void destroyIcon();
void messageBox(const std::string &title, const std::string& msg);

typedef void (*FileIterationCallback)(const std::string &filename, void *param);

void forEachFile(const std::string& inpath, std::string type, FileIterationCallback callback, void *param = 0);
void forEachDir(const std::string& inpath, FileIterationCallback callback, void *param = 0);
std::string adjustFilenameCase(const char *_buf);
std::string adjustFilenameCase(const std::string&);
bool createDir(const std::string& d);
void triggerBreakpoint();
void openURL(const std::string &url);
std::string getSystemLocale();
std::string getWorkingDir();

#endif
