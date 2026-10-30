module;
#include <sdbus-c++/sdbus-c++.h>
export module sdbus_cpp;

export namespace sdbus {

using sdbus::IConnection;
using sdbus::IObject;
using sdbus::Variant;
using sdbus::ServiceName;
using sdbus::ObjectPath;
using sdbus::Error;
using sdbus::InterfaceName;
using sdbus::MethodName;
using sdbus::Signature;
using sdbus::MethodVTableItem;
using sdbus::MethodCall;
using sdbus::createObject;
using sdbus::createSessionBusConnection;
using sdbus::registerMethod;
using sdbus::registerProperty;
using sdbus::registerSignal;

}
