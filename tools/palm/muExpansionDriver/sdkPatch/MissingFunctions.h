#ifndef _MISSINGFUNCTIONS_H_
#define _MISSINGFUNCTIONS_H_

#define PINS_TRAP(selector) _SYSTEM_API(_CALL_WITH_SELECTOR)(_SYSTEM_TABLE, sysTrapPinsDispatch, selector)

#endif // _MISSINGFUNCTIONS_H_
