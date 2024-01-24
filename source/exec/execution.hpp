#pragma once

#include <utility>

#include "tag_invoke.hpp"
#include "stop_token.hpp"

namespace vkr
{
    template<typename T>
    concept movable_value = std::move_constructible<std::decay_t<T>> && 
        std::constructible_from<std::decay_t<T>, T>;

    template<typename From, typename To>
    concept decays_to = std::same_as<std::decay_t<From>, To>;

    template<typename T>
    concept class_type = decays_to<T, T> && std::is_class_v<T>;

    template<class T>
    concept queryable = std::destructible<T>;

    namespace queries
    {
        struct forwarding_query_t
        {
            using Tag = forwarding_query_t;

            template<typename Q>
                requires nothrow_tag_invocable<Tag, Q> && 
                    std::same_as<tag_invoke_result_t<Tag, Q>, bool>
            consteval bool operator()(Q&&) const noexcept
            {
                return tag_invoke(Tag{}, std::remove_cvref_t<Q>{});
            }

            template<typename Q>
            consteval bool operator()(Q&&) const noexcept
            {
                return std::derived_from<std::remove_cvref_t<Q>, Tag>;
            }
        };

        struct get_allocator_t : public forwarding_query_t
        {
            using Tag = get_allocator_t;

            template<typename R> 
                requires nothrow_tag_invocable<Tag, const R&>
            auto operator()(R&& r) const noexcept -> tag_invoke_result_t<Tag, const R&>
            {
                return tag_invoke(Tag{}, std::as_const(r));
            }
        };

        struct get_stop_token_t : public forwarding_query_t
        {
            using Tag = get_stop_token_t;

            template<typename R>
                requires nothrow_tag_invocable<Tag, const R&> &&
                stoppable_token<tag_invoke_result_t<Tag, const R&>>
            auto operator()(R&& r) const noexcept -> tag_invoke_result_t<Tag, const R&>
            {
                return tag_invoke(Tag{}, std::as_const(r));
            }

            template<typename R>
            auto operator()(R&& r) const noexcept
            {
                return never_stop_token{};
            }
        };
    }//namespace queries

    using queries::forwarding_query_t;
    using queries::get_allocator_t;
    using queries::get_stop_token_t;
    inline constexpr forwarding_query_t forwarding_query{};
    inline constexpr get_allocator_t get_allocator{};
    inline constexpr get_stop_token_t get_stop_token{};

    template<typename T>
    using stop_token_of_t = std::remove_cvref_t<std::invoke_result_t<get_stop_token_t, T>>;

    template<typename T>
    concept forwardingable_query = forwarding_query(T{});

    namespace envs
    {
        struct empty_env {};

        struct get_env_t
        {
            using Tag = get_env_t;

            template<typename O>
                requires tag_invocable<Tag, const O&>
            auto operator()(O&& o) const noexcept(nothrow_tag_invocable<Tag, const O&>)
                -> tag_invoke_result_t<Tag, const O&>
            {
                return tag_invoke(Tag{}, std::as_const(o));
            }

            template<typename O>
            auto operator()(O&& o) const noexcept
            {
                return empty_env{};
            }
        };
    }// namespace envs

    using envs::empty_env;
    using envs::get_env_t;
    inline constexpr get_env_t get_env{};

    template<typename T>
    using env_of_t = decltype(get_env(std::declval<T>()));
    
}// namespace vkr

namespace vkr::exec
{
}// namespace vkr::exec