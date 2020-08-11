#ifndef BBGE_LEGACYKEYCODES_H
#define BBGE_LEGACYKEYCODES_H

#include <string>

unsigned getLegacyInputCodeFromKeyName(const std::string& name);
const char *getLegacyKeyNameFromInputCode(unsigned k);

#endif
