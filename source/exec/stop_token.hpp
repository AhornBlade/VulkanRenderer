#pragma once

#include <concepts>
#include <stop_token>

namespace vkr
{
    template<typename T, typename CB>
    using stop_callback_for_t = typename T::template callback_type<CB>;

    template<template<typename> typename Alias>
    struct stoppable_token_helper{};

    template<typename T>
    concept stoppable_token = 
        std::copyable<T> &&
        std::equality_comparable<T> && requires(const T token)
        {
            {T(token)} noexcept;
            {token.stop_requested()} -> std::same_as<bool>;
            {token.stop_possible()} -> std::same_as<bool>;
            typename stoppable_token_helper<T::template callback_type>;
        };

    template<typename T, typename CB, typename Initializer = CB>
    concept stoppable_token_for = 
        stoppable_token<T> &&
        std::invocable<CB> &&
        std::constructible_from<CB, Initializer> && 
        requires{typename stop_callback_for_t<T, CB>;} &&
        std::constructible_from<stop_callback_for_t<T, CB>, const T&, Initializer>;

    template<typename T>
    concept unstoppable_token = 
        stoppable_token<T> && requires
        {
            {std::bool_constant<T::stop_possible()>{}} -> std::same_as<std::false_type>;
        };

    template<typename CB>
    using stop_callback = std::stop_callback<CB>;

    class stop_token : public std::stop_token
    {
    public:
        using std::stop_token::stop_token;

        template<typename CB>
        using callback_type = stop_callback<CB>;
    };

    class never_stop_token 
    {
    public:
        template<class>
        struct callback_type
        {
            explicit callback_type(never_stop_token, auto&&) noexcept {}
        };

        [[nodiscard]] static constexpr bool stop_requested() noexcept { return false; }
        [[nodiscard]] static constexpr bool stop_possible() noexcept { return false; }

        [[nodiscard]] friend bool operator==(const never_stop_token&, const never_stop_token&) noexcept = default;
    };

}// namespace vkr