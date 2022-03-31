#include "ReadXML.h"
#include "Base.h"
#include <sstream>

tinyxml2::XMLError readXML(const std::string& fn, tinyxml2::XMLDocument& doc)
{
	size_t sz = 0;
	char *buf = readFile(fn.c_str(), &sz);
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
		const char *e1 = doc->GetErrorStr1();
		const char *e2 = doc->GetErrorStr2();
		std::ostringstream os;
		os << "readXML(" << fn << ") failed!\n";
		if(e1)
			os << e1 << "\n";
		if(e2)
			os << e2 << "\n";
		errorLog(os.str());
		delete doc;
		doc = NULL;
	}
	return doc;
}
