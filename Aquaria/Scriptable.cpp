#include "Scriptable.h"
#include "ScriptInterface.h"
#include "DSQ.h"

Scriptable::Scriptable() : script(0)
{
}

int Scriptable::pushLuaVars(lua_State *L)
{
	return script ?  script->pushLocalVars(L) : 0;
}

void Scriptable::closeScript()
{
	if (script)
	{
		dsq->scriptInterface.closeScript(script);
		script = 0;
	}
}
