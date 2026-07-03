#pragma once
#include "aegis/domain.hpp"
#include <string>
namespace aegis {
std::string render_extension_projection(const ExtensionStore& store, const ExtensionProjection& projection);
}
