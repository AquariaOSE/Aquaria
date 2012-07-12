#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "Base.h"

void setUsedLocale(const std::string& s);
std::string localisePath(const std::string &path, const std::string &modpath = "");
std::string getSystemLocale();

#endif
