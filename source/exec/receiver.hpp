#pragma once

#include "tag_invoke.hpp"

#include <exception>

namespace vkr::exec 
{
	namespace _set_done
	{
		struct set_done_t
		{
			using Tag = set_done_t;

			template<class R> requires nothrow_tag_invocable<Tag, R>
			auto operator()(R&& r) const noexcept -> tag_invoke_result_t<Tag, R>
			{
				return tag_invoke(Tag{}, static_cast<R&&>(r));
			}
		};
	}//namespace _set_done

	using set_done_t = _set_done::set_done_t;
	inline constexpr set_done_t set_done{};

	namespace _set_error
	{
		struct set_error_t
		{
			using Tag = set_error_t;

			template<class R, class E> requires nothrow_tag_invocable<Tag, R, E>
			auto operator()(R&& r, E&& e) const noexcept -> tag_invoke_result_t<Tag, R, E>
			{
				return tag_invoke(Tag{}, static_cast<R&&>(r), static_cast<E&&>(e));
			}
		};
	}// namespace _set_error

	using set_error_t = _set_error::set_error_t;
	inline constexpr set_error_t set_error{};

	namespace _set_value
	{
		struct set_value_t
		{
			using Tag = set_value_t;

			template<class R, class ... Args> requires tag_invocable<Tag, R, Args...>
			auto operator()(R&& r, Args&& ... args) const
				noexcept(nothrow_tag_invocable<Tag, R, Args...>)
				-> tag_invoke_result_t<Tag, R, Args...>
			{
				return tag_invoke(Tag{}, static_cast<R&&>(r), static_cast<Args>(args)...);
			}
		};
	}// namespace _set_value

	using set_value_t = _set_value::set_value_t;
	inline constexpr set_value_t set_value{};

	template<class T, class E = std::exception_ptr>
	concept receiver = 
		std::move_constructible<std::remove_cvref_t<T>>&&
		std::constructible_from<std::remove_cvref_t<T>, T>&&
		requires(std::remove_cvref_t<T> && t, E && e)
	{
		{set_done(std::move(t))} noexcept;
		{set_error(std::move(t), (E&&)e)} noexcept;
	};

	template<class T, class ... An>
	concept receiver_of = receiver<T> &&
		requires(std::remove_cvref_t<T> && t, An&& ... an)
	{
		set_value(std::move(t), static_cast<An&&>(an)...);
	};

}// namespace vkr::exec