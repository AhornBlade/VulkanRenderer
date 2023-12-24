#pragma once

#include "receiver.hpp"
#include "type_list.hpp"

#include <tuple>

namespace vkr::exec
{
	template<typename S, template<typename...>typename Tuple = type_list,
		template<typename...>typename Variant = type_list>
	concept sender = 
		std::move_constructible<std::remove_cvref_t<S>> &&
		requires
	{
		typename std::remove_cvref_t<S>::template value_types<Tuple, Variant>;
		typename std::remove_cvref_t<S>::template error_types<Variant>;
		typename std::conditional_t<std::remove_cvref_t<S>::sends_done, void, void>;
	};

	template<sender S>
	struct sender_traits
	{
		template<template<typename...>typename Tuple, template<typename...>typename Variant>
		using value_types = typename std::remove_cvref_t<S>::template value_types<Tuple, Variant>;

		template<template<typename...>typename Variant>
		using error_types = typename std::remove_cvref_t<S>::template error_types<Variant>;

		static constexpr bool sends_done = std::remove_cvref_t<S>::sends_done;
	};

	namespace _connect
	{
		template<receiver R>
		struct connect_closure_t;

		struct connect_t
		{
			using Tag = connect_t;

			template<typename S, receiver R>
				// requires tag_invocable<Tag, S, R>
			auto operator()(S&& s, R&& r) const
				// noexcept(nothrow_tag_invocable<Tag, S, R>)
				// -> tag_invoke_result_t<Tag, S, R>
			{
				return tag_invoke(Tag{}, std::forward<S>(s), std::forward<R>(r));
			}

			template<receiver R>
			auto operator()(R&& r) const noexcept -> connect_closure_t<std::remove_cvref_t<R>>;
		};
	}// namespace _connect

	using connect_t = _connect::connect_t;
	inline constexpr connect_t connect{};

	template<typename S, typename R>
	concept sender_to = sender<S> && receiver<R> &&
		requires(std::remove_cvref_t<S> && s, std::remove_cvref_t<R> && r)
	{
		connect(std::forward<S>(s), std::forward<R>(r));
	};

	namespace _connect
	{
		template<receiver R>
		struct connect_closure_t
		{
			R r_;

			template<sender_to<R> S, typename Self>
				requires std::same_as<std::remove_cvref_t<Self>, connect_closure_t>
			friend auto operator|(S&& s, Self&& r) 
				noexcept(noexcept(connect(std::forward<S>(s), std::forward<Self>(r).r_)))
			{
				return connect(std::forward<S>(s), std::forward<Self>(r).r_);
			}
		};

		template<receiver R>
		auto connect_t::operator()(R&& r) const noexcept -> connect_closure_t<std::remove_cvref_t<R>>
		{
			return connect_closure_t<std::remove_cvref_t<R>>{std::forward<R>(r)};
		}

	}//namespace _connect

	namespace _start
	{
		struct start_closure_t;

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

			inline auto operator()() const noexcept -> start_closure_t;
		};
	}//namespace _start

	using start_t = _start::start_t;
	inline constexpr start_t start{};

	namespace _start
	{
		struct start_closure_t
		{
			template<typename O, typename Self> requires
				std::same_as<std::remove_cvref_t<Self>, start_closure_t>
			friend auto operator|(O&& o, Self&& self)
				noexcept(noexcept(start(std::forward<O>(o))))
			{
				return start(std::forward<O>(o));
			}
		};

		inline auto start_t::operator()() const noexcept -> start_closure_t
		{
			return start_closure_t{};
		}

	}// namespace _start

	namespace _just
	{
		template<typename ... Ts>
		struct just_sender
		{
			using Tuple = std::tuple<Ts...>;

			template<template<typename...>typename Tuple, template<typename...>typename Variant>
			using value_types = Variant<Tuple<Ts...>>;

			template<template<typename...>typename Variant>
			using error_types = Variant<std::exception_ptr>;

			static constexpr auto sends_done = false;

			template<typename R>
			struct operation
			{
				using Reveiver = R;

				[[no_unique_address]] Tuple values_;
				[[no_unique_address]] Reveiver r_;

				template<typename Self>requires
					std::same_as<std::remove_cvref_t<Self>, operation>
				friend void tag_invoke(start_t, Self&& s) noexcept
				{
					try
					{
						std::apply([&s]<typename ... Args>(Args&& ... values)
							{
								set_value(std::forward<Self>(s).r_, std::forward<Args>(values)...);
							}, std::forward<Self>(s).values_);
					}
					catch (...)
					{
						set_error(std::forward<Self>(s).r_, std::current_exception());
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
		template<typename R, typename F, typename ... Args>
		inline constexpr bool is_receiver_of_result = 
			std::invocable<F, Args...> &&
			((std::is_void_v<std::invoke_result_t<F, Args...>> && receiver_of<R>) ||
			(!std::is_void_v<std::invoke_result_t<F, Args...>> &&
			receiver_of<R, std::invoke_result_t<F, Args...>>));

		template<receiver R, typename F>
		struct then_reveiver
		{
			using Receiver = R;
			using Function = F;

			template<typename Self, typename ... Ts> requires 
				is_receiver_of_result<Receiver, Function, Ts...> &&
				std::same_as<std::remove_cvref_t<Self>, then_reveiver> &&
				std::is_nothrow_invocable_v<F, Ts...>
			friend auto tag_invoke(set_value_t, Self&& r, Ts&& ... args) noexcept
			{
				if constexpr (std::is_void_v<std::invoke_result_t<Function, Ts...>>)
				{
					std::invoke(std::forward<Self>(r).f_, std::forward<Ts>(args)...);
					set_value(std::forward<Self>(r).out_r_);
				}
				else
				{
					set_value(std::forward<Self>(r).out_r_, 
						std::invoke(std::forward<Self>(r).f_, std::forward<Ts>(args)...));
				}
			}

			template<typename Self, typename ... Ts> requires 
				is_receiver_of_result<Receiver, Function, Ts...> &&
				std::same_as<std::remove_cvref_t<Self>, then_reveiver> &&
				(!std::is_nothrow_invocable_v<Function, Ts...>)
			friend auto tag_invoke(set_value_t, Self&& r, Ts&& ... args) noexcept
			{
				try
				{
					if constexpr (std::is_void_v<std::invoke_result_t<Function, Ts...>>)
					{
						std::invoke(std::forward<Self>(r).f_, std::forward<Ts>(args)...);
						set_value(std::forward<Self>(r).out_r_);
					}
					else
					{
						set_value(std::forward<Self>(r).out_r_, 
							std::invoke(std::forward<Self>(r).f_, std::forward<Ts>(args)...));
					}
				}
				catch (...)
				{
					set_error(std::forward<Self>(r).out_r_, std::current_exception());
				}
			}

			template<typename Self, typename E> requires
				std::same_as<std::remove_cvref_t<Self>, then_reveiver>
			friend auto tag_invoke(set_error_t, Self&& r, E&& e) noexcept
			{
				set_error(std::forward<Self>(r).out_r_, std::forward<E>(e));
			}

			template<typename Self> requires
				std::same_as<std::remove_cvref_t<Self>, then_reveiver>
			friend auto tag_invoke(set_done_t, Self&& r) noexcept
			{
				set_done(std::forward<Self>(r).out_r_);
			}

			[[no_unique_address]] Receiver out_r_;
			[[no_unique_address]] Function f_;
		};

		template<typename F>
		struct list_of_type_list_of_result
		{
			template<typename ... Args>
			using result = type_list<std::conditional_t<
				std::is_void_v<std::invoke_result_t<F, Args...>>,
				type_list<>,
				type_list<std::invoke_result_t<F, Args...>>>>;
		};

		template<typename F>
		struct function_invocable
		{
			template<typename ... Args>
			struct args_invocable
			{
				static constexpr bool value = std::is_invocable_v<F, Args...>;
			};
		};

		template<typename S, typename F>
		concept sender_to_function = sender<S> &&
			sender_traits<S>::template value_types<type_list, concated_type_list>::template apply<
			function_invocable<F>::template args_invocable>::value;

		template<sender S, typename F> requires sender_to_function<S, F>
		struct then_sender
		{
			using Sender = S;
			using Function = F;

			template<template<typename...>typename Tuple, template<typename...>typename Variant>
			using value_types = zip_apply<
				typename sender_traits<S>::template value_types<
				list_of_type_list_of_result<Function>::template result, 
				concated_type_set>,
				Variant, Tuple
			>;

			template<template<typename ...> typename Variant>
			using error_types = concated_type_set<
				typename sender_traits<S>::template error_types<type_list>,
				type_list<std::exception_ptr>
			>::template apply<Variant>;

			static constexpr bool sends_done = sender_traits<S>::sends_done;

			template<typename R>
			using receiver_t = then_reveiver<std::remove_cvref_t<R>, std::remove_cvref_t<Function>>;

			template<typename Self, receiver R> requires
				std::same_as<std::remove_cvref_t<Self>, then_sender> &&
				sender_to<S, receiver_t<R>>
			friend auto tag_invoke(connect_t, Self&& s, R&& r)
				noexcept(nothrow_tag_invocable<connect_t, S, receiver_t<R>>)
				-> tag_invoke_result_t<connect_t, S, receiver_t<R>>
			{
				return connect(std::forward<Self>(s).pred_,
					receiver_t<R>{std::forward<R>(r), std::forward<Self>(s).f_});
			}

			[[no_unique_address]] Sender pred_;
			[[no_unique_address]] Function f_;
		};

		template<typename F>
		struct then_closure_t;

		struct then_t
		{
			using Tag = then_t;

			template<sender S, typename F> 
				requires tag_invocable<Tag, S, F>
			auto operator()(S&& s, F&& f) const
				noexcept(nothrow_tag_invocable<Tag, S, F>)
				-> tag_invoke_result_t<Tag, S, F>
			{
				return tag_invoke(Tag{}, std::forward<S>(s), std::forward<F>(f));
			}

			template<sender S, typename F> 
				requires (!tag_invocable<Tag, S, F>) 
			auto operator()(S&& s, F&& f) const noexcept
				-> then_sender<std::remove_cvref_t<S>, std::remove_cvref_t<F>>
			{
				return then_sender<std::remove_cvref_t<S>, std::remove_cvref_t<F>>{
					std::forward<S>(s), std::forward<F>(f)};
			}

			template<typename F>
			auto operator()(F&& f) const noexcept -> then_closure_t<std::remove_cvref_t<F>>;
		};
	}// namespace _then

	using then_t = _then::then_t;
	inline constexpr then_t then{};

	namespace _then
	{
		template<typename F>
		struct then_closure_t
		{
			F f_;

			template<sender_to_function<F> S, typename Self>requires 
				std::same_as<std::remove_cvref_t<Self>, then_closure_t>
			friend auto operator|(S&& s, Self&& self)
				noexcept(noexcept(then(std::forward<S>(s), std::forward<Self>(self).f_)))
			{
				return then(std::forward<S>(s), std::forward<Self>(self).f_);
			}
		};

		template<typename F>
		auto then_t::operator()(F&& f) const noexcept
			-> then_closure_t<std::remove_cvref_t<F>>
		{
			return then_closure_t<std::remove_cvref_t<F>>{std::forward<F>(f)};
		}

	}// namespace _then

}// namespace vkr::exec