#pragma once

#include "receiver.hpp"

namespace vkr::exec
{
	//template<class T>
	//concept sender =
	//	std::move_constructible<std::remove_cvref_t<T>> &&
	//	std::constructible_from<std::remove_cvref_t<T>, T>

	namespace _connect
	{
		struct connect_t
		{
			using Tag = connect_t;

			template<class S, receiver R>
				requires tag_invocable<Tag, S, R>
			auto operator()(S&& s, R&& r) const
				noexcept(nothrow_tag_invocable<Tag, S, R>)
				-> tag_invoke_result_t<Tag, S, R>
			{
				return tag_invoke(Tag, static_cast<S&&>(s), static_cast<R&&>(r));
			}
		};
	}// namespace _connect

	using connect_t = _connect::connect_t;
	inline constexpr connect_t connect{};

	namespace _start
	{
		struct start_t
		{
			using Tag = start_t;

			template<class O>
			requires tag_invocable<Tag, O>
			auto operator()(O&& o) noexcept(nothrow_tag_invocable<Tag, O>)
				->tag_invoke_result_t<Tag, O>
			{
				return tag_invoke(Tag{}, static_cast<O&&>(o));
			}
		};
	}//namespace _start

	using start_t = _start::start_t;
	inline constexpr start_t start{};

}// namespace vkr::exec