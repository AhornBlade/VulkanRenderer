#pragma once

#include <type_traits>

namespace vkr::exec
{
	template<typename ... Ts>
	struct type_list
	{
		template<template<typename...>typename F>
		using apply = F<Ts...>;
	};

	template<typename ... Ts>
	struct type_count
	{
		constexpr static size_t value = sizeof...(Ts);
	};

	template<typename ... TypeLists>
	struct concat_type_list_impl
	{
		static_assert("the args of concat_type_list_impl must be vkr::exec::type_list");
	};

	template<>
	struct concat_type_list_impl<>
	{
		using Type = type_list<>;
	};

	template<typename ... Ts>
	struct concat_type_list_impl<type_list<Ts...>>
	{
		using Type = type_list<Ts...>;
	};

	template<typename ... Ts, typename ... Us, typename ... OtherTypeLists>
	struct concat_type_list_impl<type_list<Ts...>, type_list<Us...>, OtherTypeLists...>
	{
		using Type = concat_type_list_impl<type_list<Ts..., Us...>, OtherTypeLists...>::Type;
	};

	template<typename ... TypeLists>
	using concated_type_list = concat_type_list_impl<TypeLists...>::Type;

	template<typename ... TypeLists>
	struct concat_type_sets_impl
	{
		static_assert("the args of concat_type_sets_impl must be vkr::exec::type_list");
	};

	template<>
	struct concat_type_sets_impl<>
	{
		using Type = type_list<>;
	};

	template<typename ... Ts>
	struct concat_type_sets_impl<type_list<Ts...>>
	{
		using Type = type_list<Ts...>;
	};

	template<typename T, typename ... Ts>
	concept type_among = (std::is_same_v<T, Ts>|| ...);

	template<typename T, typename ... Ts>
	struct concat_type_sets_impl_helper
	{
		using Type = std::conditional_t<type_among<T, Ts...>, type_list<>, type_list<T>>;
	};

	template<typename ... Ts, typename ... Us>
	struct concat_type_sets_impl<type_list<Ts...>, type_list<Us...>>
	{
		using Type = concated_type_list<type_list<Ts...>,
			typename concat_type_sets_impl_helper<Us, Ts...>::Type...>;
	};

	template<typename ... Ts, typename ... Us, typename ... OtherTypeLists>
	struct concat_type_sets_impl<type_list<Ts...>, type_list<Us...>, OtherTypeLists...>
	{
		using Type = typename concat_type_sets_impl<
			typename concat_type_sets_impl<type_list<Ts...>, type_list<Us...>>::Type
			, OtherTypeLists...>::Type;
	};

	template<typename ... TypeLists>
	using concated_type_set = typename concat_type_sets_impl<TypeLists...>::Type;

	template<template <typename...> typename Outer, template<typename...> typename Inner>
	struct zip_apply_impl
	{
		template<class ... TypeLists>
		using apply = Outer<typename TypeLists::template apply<Inner>...>;
	};

	template<typename ListOfLists, 
		template<typename...> typename Outer, 
		template<typename...> typename Inner>
	using zip_apply = typename ListOfLists::template apply<zip_apply_impl<Outer, Inner>::template apply>;

}// namespace vkr::exec