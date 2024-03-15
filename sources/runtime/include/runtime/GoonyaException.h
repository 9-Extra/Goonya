#pragma once
#include <stdexcept>

namespace Goonya {
class RuntimeError: public std::runtime_error{
    using std::runtime_error::runtime_error;
};

}