#pragma once

#include <cstdint>
#include <string>
#include <tuple>

namespace Goonya {

namespace Display{
    void initalize(uint32_t width, uint32_t height);
    void drop();

    void set_title(const std::string& title);
    void poll_events();
    std::tuple<uint32_t, uint32_t> get_size();
}

}