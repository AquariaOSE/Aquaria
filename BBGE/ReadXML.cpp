#include "ReadXML.h"
#include "Base.h"

tinyxml2::XMLError readXML(const std::string& fn, tinyxml2::XMLDocument& doc)
{
	unsigned long sz = 0;
	char *buf = readFile(fn, &sz);
	tinyxml2::XMLError err = doc.Parse(buf, sz);
	delete [] buf;
	return err;
}

tinyxml2::XMLDocument *readXML(const std::string& fn, tinyxml2::XMLError *perr /* = 0 */, bool keepEmpty /* = false */)
{
	tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument();
	tinyxml2::XMLError err = readXML(fn, *doc);
	if(perr)
		*perr = err;
	if(err != tinyxml2::XML_SUCCESS && !keepEmpty)
	{
		delete doc;
		doc = NULL;
	}
	return doc;
}
