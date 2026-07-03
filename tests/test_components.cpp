#include "aegis/aegis.hpp"
#include <cassert>
int main(){auto signals=aegis::subsystem_signals(9,4);assert(signals.size()>50);assert(aegis::subsystem_inventory().find("components=")!=std::string::npos);}
