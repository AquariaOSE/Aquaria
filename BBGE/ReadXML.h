#ifndef BBGE_READ_XML_H
#define BBGE_READ_XML_H

#include <tinyxml2.h>
#include <string>

tinyxml2::XMLDocument *readXML(const std::string& fn, tinyxml2::XMLError *perr = 0, bool keepEmpty = false);
tinyxml2::XMLError readXML(const std::string& fn, tinyxml2::XMLDocument& doc);

#endif
