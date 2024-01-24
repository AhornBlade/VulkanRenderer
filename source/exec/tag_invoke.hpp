#pragma once

#include <concepts>

namespace vkr
{
    namespace _tag_invoke
    {
        void tag_invoke();

        struct tag_invoke_t
        {
            template<typename Tag, typename ... Args>
            constexpr auto operator()(Tag tag, Args&& ... args) const
                noexcept(noexcept(tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...)))
                -> decltype(tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...))
            {
                tag_invoke(std::forward<Tag>(tag), std::forward<Args>(args)...);
            }
        };
    }// namespace _tag_invoke

    using tag_invoke_t = _tag_invoke::tag_invoke_t;
    inline constexpr tag_invoke_t tag_invoke{};

    template<typename Tag, typename ... Args>
    concept tag_invocable = std::invocable<tag_invoke_t, Tag, Args...>;

    template<typename Tag, typename ... Args>
    concept nothrow_tag_invocable = std::is_nothrow_invocable_v<tag_invoke_t, Tag, Args...>;

    template<typename Tag, typename ... Args>
        requires tag_invocable<Tag, Args...>
    struct tag_invoke_result
    {
        using type = std::invoke_result_t<tag_invoke_t, Tag, Args...>;
    };

    template<typename Tag, typename ... Args>
    using tag_invoke_result_t = tag_invoke_result<Tag, Args...>::type;

    template<auto& Tag>
    using tag_t = std::decay_t<decltype(Tag)>;

}// namespace vkr