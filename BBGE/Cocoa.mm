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
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *nstitle = [NSString stringWithUTF8String:title.c_str()];
    NSString *nsmsg = [NSString stringWithUTF8String:msg.c_str()];
    NSRunAlertPanel(nstitle, @"%@", nil, nil, nil, nsmsg);
    [pool drain];
}

