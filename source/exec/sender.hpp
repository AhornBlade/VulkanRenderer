#pragma once

#include "receiver.hpp"
#include "type_list.hpp"

#include <tuple>

namespace vkr::exec
{
	template<typename S, template<typename...>typename Tuple = type_list,
		template<typename>typename Variant = std::type_identity_t>
	concept sender = 
		std::move_constructible<std::remove_cvref_t<S>> &&
		requires
	{
		typename S::template value_types<Tuple, Variant>;
		typename S::template error_types<Variant>;
		typename std::conditional_t<S::sends_done, void, void>;
	};

	template<typename S, typename R>
	concept sender_to = sender<S> && receiver<R> &&
		requires(std::remove_cvref_t<S> && s, std::remove_cvref_t<R> && r)
	{
		connect(std::move(s), std::move(r));
	};

	template<sender S>
	struct sender_traits
	{
		template<template<typename...>typename Tuple, template<typename>typename Variant>
		using value_types = typename S::template value_types<Tuple, Variant>;

		template<template<typename>typename Variant>
		using error_types = typename S::template error_types<Variant>;

		static constexpr bool sends_done = S::sends_done;
	};

	namespace _connect
	{
		struct connect_t
		{
			using Tag = connect_t;

			template<typename S, receiver R>
				requires tag_invocable<Tag, S, R>
			auto operator()(S&& s, R&& r) const
				noexcept(nothrow_tag_invocable<Tag, S, R>)
				-> tag_invoke_result_t<Tag, S, R>
			{
				return tag_invoke(Tag{}, std::forward<S>(s), std::forward<R>(r));
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

			template<typename O>
			requires tag_invocable<Tag, O>
			auto operator()(O&& o) const
				noexcept(nothrow_tag_invocable<Tag, O>)
				->tag_invoke_result_t<Tag, O>
			{
				return tag_invoke(Tag{}, std::forward<O>(o));
			}
		};
	}//namespace _start

	using start_t = _start::start_t;
	inline constexpr start_t start{};

	namespace _just
	{
		template<typename ... Ts>
		struct just_sender
		{
			using Tuple = std::tuple<Ts...>;

			template<template<class...>class Tuple, template<class>class Variant>
			using value_types = Variant<Tuple<Ts...>>;

			template<template<class>class Variant>
			using error_types = Variant<std::exception_ptr>;

			static constexpr auto sends_done = false;

			template<typename R>
			struct operation
			{
				using Reveiver = R;

				[[no_unique_address]] Tuple values_;
				[[no_unique_address]] Reveiver r_;

				friend void tag_invoke(start_t, operation& s) noexcept
				{
					try
					{
						std::apply([&s](Ts& ... values)
							{
								set_value(std::move(s.r_), values...);
							}, s.values_);
					}
					catch (...)
					{
						set_error(std::move(s.r_), std::current_exception());
					}
				}
			};

			template<typename Self, receiver_of<Ts...> R>
				requires std::same_as<std::remove_cvref_t<Self>, just_sender>
			friend auto tag_invoke(connect_t, Self&& s, R&& r) noexcept
				-> operation<std::remove_cvref_t<R>>
			{
				return operation<std::remove_cvref_t<R>>{ std::forward<Self>(s).values_, std::forward<R>(r) };
			}

			[[no_unique_address]] Tuple values_;
		};
	}//namespace _just

	template<typename ... Ts>
	auto just(Ts&& ... args) noexcept -> _just::just_sender<std::remove_cvref_t<Ts>...>
	{
		return _just::just_sender<std::remove_cvref_t<Ts>...>(
			std::tuple{ std::forward<Ts>(args)... });
	}

	namespace _then
	{


	}// namespace _then

}// namespace vkr::exec