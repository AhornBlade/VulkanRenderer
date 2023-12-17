#pragma once

#include <concepts>

namespace vkr::exec
{
	namespace _tag
	{
		void tag_invoke();
		struct tag_invoke_t
		{
			template<class Tag, class ... Args>
			constexpr auto operator()(Tag tag, Args&& ... args) const
				noexcept(noexcept(tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...)))
				-> decltype(tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...))
			{
				return tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...);
			}
		};
	}

	inline constexpr _tag::tag_invoke_t tag_invoke{};

	template<auto& Tag>
	using tag_t = std::decay_t<decltype(Tag)>;

	template<class Tag, class ... Args>
	concept tag_invocable = 
		std::is_invocable_v<decltype(tag_invoke), Tag, Args...>;

	template<class Tag, class ... Args>
	concept nothrow_tag_invocable =
		std::is_nothrow_invocable_v<decltype(tag_invoke), Tag, Args...>;

	template<class Tag, class ... Args>
	using tag_invoke_result_t = 
		std::invoke_result_t<decltype(tag_invoke), Tag, Args...>;

}// namespace vkr::exec