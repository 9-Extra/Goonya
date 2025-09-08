#pragma once

#include <exception>
#include <string>

namespace Goonya {

std::string format_exception(const std::exception &e) noexcept;

} // namespace Goonya