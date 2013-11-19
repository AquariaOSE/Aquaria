#ifndef __APPLE__
#error This file is for Mac OS X only.
#endif

#ifndef __OBJC__
#error This is Objective-C code. Please compile it as such.
#endif

#include <Cocoa/Cocoa.h>
#include <string>

void cocoaMessageBox(const std::string &title, const std::string &msg)
{
    @autoreleasepool {
        NSString *nstitle = @(title.c_str());
        NSString *nsmsg = @(msg.c_str());
        NSRunAlertPanel(nstitle, nsmsg, nil, nil, nil);
    }
}

