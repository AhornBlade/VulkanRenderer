#pragma once

#include <type_traits>

namespace vkr
{
    template<typename ... Ts>
    struct type_list
    {
        template<template <typename ...> typename Fn>
        using apply = Fn<Ts...>;
    };

    template<typename T>
    concept type_list_like = requires { typename T::template apply<type_list>; };

    template<typename ... Ts>
    struct type_count
    {
        constexpr static size_t value = sizeof...(Ts);
    };

    template<type_list_like T>
    constexpr size_t type_count_v = T::template apply<type_count>::value;

    template<typename T>
    struct type_list_traits;

    template<template<typename ...> typename Temp, typename ... Ts>
    struct type_list_traits<Temp<Ts...>>
    {
        template<typename ... Us>
        using temp = Temp<Us...>;

        template<template<typename ...>typename Fn>
        using apply = Fn<Ts...>;
    };

    template<template <typename ...> typename Outer, template <typename ...> typename Inner>
    struct zip_apply
    {
        template<type_list_like ... TypeLists>
        using apply = Outer<typename TypeLists::template apply<Inner> ...>;
    };

    template<template <typename ...> typename Outer,
        template <typename ...> typename Inner,
        type_list_like TypeLists>
    using zip_apply_t = TypeLists::template apply<zip_apply<Outer, Inner>>;

    template<template<typename...> typename Temp, typename ... TypeLists>
    struct concat_type_lists_impl;

    template<template<typename...> typename Temp>
    struct concat_type_lists_impl<Temp>
    {
        using Type = Temp<>;
    };

    template<template<typename...> typename Temp, template <typename ...> typename TypeList, typename ... Ts>
    struct concat_type_lists_impl<Temp, TypeList<Ts...>>
    {
        using Type = Temp<Ts...>;
    };

    template<template<typename...> typename Temp, template <typename ...> typename TypeList, typename ... Ts, typename ... Us, typename ... OtherTypeLists>
    struct concat_type_lists_impl<Temp, TypeList<Ts...>, TypeList<Us...>, OtherTypeLists...>
    {
        using Type = typename concat_type_lists_impl<Temp, TypeList<Ts..., Us...>, OtherTypeLists...>::Type;
    };

    template<template<typename...> typename Temp>
    struct concat_type_lists
    {
        template<typename ... TypeLists>
        using apply = concat_type_lists_impl<Temp, TypeLists...>::Type;
    };

    template<typename TypeLists>
    using concat_type_lists_t = TypeLists::template apply<
        concat_type_lists<type_list_traits<TypeLists>::template temp>::template apply>;

    template<typename Ts, typename Us>
    struct concat_type_sets_impl;

    template<typename T, typename ... Ts>
    concept one_of = (std::is_same_v<T, Ts> || ...);

    template<template <typename ...> typename TypeList, typename ... Ts>
    struct concat_type_sets_impl<TypeList<Ts...>, TypeList<>>
    {
        using Type = TypeList<Ts...>;
    };

    template<template <typename ...> typename TypeList, typename ... Ts, typename U, typename ... Us>
    struct concat_type_sets_impl<TypeList<Ts...>, TypeList<U, Us...>>
    {
        using Type = typename concat_type_sets_impl<
            std::conditional_t<one_of<U, Ts...>, TypeList<Ts...>, TypeList<Ts..., U>>,
            TypeList<Us...>>::Type;
    };

    template<template<typename ...> typename Temp>
    struct concat_type_sets
    {
        template<typename ... TypeLists>
        using apply = concat_type_sets_impl<Temp<>, 
            typename concat_type_lists<Temp>::template apply<TypeLists...>>::Type;
    };

    template<typename TypeLists>
    using concat_type_sets_t = TypeLists::template apply<
        concat_type_sets<type_list_traits<TypeLists>::template temp>::template apply>;

    template<template <typename> typename Fn, typename Ts, typename Us>
    struct fliter_type_impl;

    template<template <typename ...> typename TypeList, template <typename> typename Fn, typename ... Ts>
    struct fliter_type_impl<Fn, TypeList<Ts...>, TypeList<>>
    {
        using Type = TypeList<Ts...>;
    };

    template<template <typename ...> typename TypeList, template <typename> typename Fn, typename ... Ts, typename U, typename ... Us>
    struct fliter_type_impl<Fn, TypeList<Ts...>, TypeList<U, Us...>>
    {
        using Type = 
            typename fliter_type_impl<Fn, 
                std::conditional_t<Fn<U>::value, 
                    TypeList<Ts..., U>, 
                    TypeList<Ts...>>, 
                TypeList<Us...>>::Type;
    };

    template<template <typename> typename Fn>
    struct fliter_type
    {
        template<typename ... Ts>
        using apply = fliter_type_impl<Fn, type_list<>, type_list<Ts...>>::Type;
    };

    template<template <typename> typename Fn, typename TypeList>
    using fliter_type_t = fliter_type_impl<Fn, 
        typename type_list_traits<TypeList>::template temp<>, TypeList>::Type;

    template<template <typename> typename Fn, typename T>
    struct transform_type_impl;

    template<template <typename> typename Fn, template <typename...> typename TypeList, typename ... Ts>
    struct transform_type_impl<Fn, TypeList<Ts...> >
    {
        using Type = TypeList<Fn<Ts>...>;
    };

    template<template <typename> typename Fn>
    struct transform_type
    {
        template<typename ... Ts>
        using apply = transform_type_impl<Fn, type_list<Ts...>>::Type;
    };

    template<template <typename> typename Fn, typename TypeList>
    using transform_type_t = transform_type_impl<Fn, TypeList>::Type;

    template<typename T, typename Ts>
    struct is_include_type;

    template<template<typename...>typename TypeList, typename T, typename ... Ts>
    struct is_include_type<T, TypeList<Ts...>>
    {
        static constexpr bool value = one_of<T, Ts...>;
    };

    template<typename T, typename Ts>
    inline constexpr bool is_include_type_v = is_include_type<T, Ts>::value;

}// namespace vkr