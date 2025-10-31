// from https://github.com/libbitcoin/libbitcoin-system/wiki/Integer-Division-Unraveled

#include <concepts>
#include <type_traits>

namespace Details {

template <std::integral Integer>
constexpr bool is_negative(Integer value) noexcept {
    if (std::is_signed_v<Integer>) {
        return value < 0;
    } else {
        return true;
    }
}

template <std::integral Factor1, std::integral Factor2>
constexpr bool is_divsion_negative(Factor1 factor1, Factor2 factor2) noexcept {
    return is_negative(factor1) != is_negative(factor2);
}

template <std::integral Dividend, std::integral Divisor>
constexpr bool no_remainder(Dividend dividend, Divisor divisor) noexcept {
    return (dividend % divisor) == 0;
}

template <std::integral Dividend, std::integral Divisor>
constexpr bool is_ceilinged(Dividend dividend, Divisor divisor) noexcept {
    return is_divsion_negative(dividend, divisor) || no_remainder(dividend, divisor);
}

template <std::integral Dividend, std::integral Divisor>
constexpr bool is_floored(Dividend dividend, Divisor divisor) noexcept {
    return !is_divsion_negative(dividend, divisor) || no_remainder(dividend, divisor);
}

} // namespace Details

template <std::integral Dividend, std::integral Divisor>
constexpr auto truncated_divide(Dividend dividend, Divisor divisor) noexcept -> decltype(dividend / divisor) {
    return dividend / divisor;
}

template <std::integral Dividend, std::integral Divisor>
constexpr auto truncated_modulo(Dividend dividend, Divisor divisor) noexcept -> decltype(dividend % divisor) {
    return dividend % divisor;
}

template <std::integral Dividend, std::integral Divisor>
constexpr auto ceilinged_divide(Dividend dividend, Divisor divisor) noexcept -> decltype(dividend / divisor) {
    return truncated_divide(dividend, divisor) + (Details::is_ceilinged(dividend, divisor) ? 0 : 1);
}

template <std::integral Dividend, std::integral Divisor>
constexpr auto ceilinged_modulo(Dividend dividend, Divisor divisor) noexcept ->
    typename std::make_signed<decltype(dividend % divisor)>::type {
    return truncated_modulo(dividend, divisor) - (Details::is_ceilinged(dividend, divisor) ? 0 : divisor);
}

template <std::integral Dividend, std::integral Divisor>
constexpr auto floored_divide(Dividend dividend, Divisor divisor) noexcept -> decltype(dividend / divisor) {
    return truncated_divide(dividend, divisor) - (Details::is_floored(dividend, divisor) ? 0 : 1);
}

template <std::integral Dividend, std::integral Divisor>
constexpr auto floored_modulo(Dividend dividend, Divisor divisor) noexcept -> decltype(dividend % divisor) {
    return truncated_modulo(dividend, divisor) + (Details::is_floored(dividend, divisor) ? 0 : divisor);
}