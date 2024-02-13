#pragma once

#include <utility>
#include <tuple>
#include <variant>

#include "tag_invoke.hpp"
#include "stop_token.hpp"
#include "type_list.hpp"

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
            constexpr auto operator()(R&& r) const noexcept -> tag_invoke_result_t<Tag, const R&>
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
            constexpr auto operator()(R&& r) const noexcept -> tag_invoke_result_t<Tag, const R&>
            {
                return tag_invoke(Tag{}, std::as_const(r));
            }

            template<typename R>
            constexpr auto operator()(R&& r) const noexcept
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
            constexpr auto operator()(O&& o) const noexcept(nothrow_tag_invocable<Tag, const O&>)
                -> tag_invoke_result_t<Tag, const O&>
            {
                return tag_invoke(Tag{}, std::as_const(o));
            }

            template<typename O>
            constexpr auto operator()(O&& o) const noexcept
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
    enum class forward_progress_guarantee 
    {
        concurrent,
        parallel,
        weakly_parallel
    };

    namespace queries
    {
        struct get_scheduler_t : public forwarding_query_t
        {
            using Tag = get_scheduler_t;
            
            template<typename R>
                requires nothrow_tag_invocable<Tag, const R&>
            constexpr auto operator()(R&& r) const noexcept 
                -> tag_invoke_result_t<Tag, const R&>
            {
                return tag_invoke(Tag{}, std::as_const(r));
            }
        };

        struct get_delegatee_scheduler_t : public forwarding_query_t
        {
            using Tag = get_delegatee_scheduler_t;

            template<typename R>
                requires nothrow_tag_invocable<Tag, const R&>
            constexpr auto operator()(R&& r) const noexcept
                -> tag_invoke_result_t<Tag, const R&>
            {
                return tag_invoke(Tag{}, std::as_const(r));
            }
        };

        struct get_forward_progress_guarantee_t
        {
            using Tag = get_forward_progress_guarantee_t;

            template<typename S>
                requires nothrow_tag_invocable<Tag, const S&>
            constexpr auto operator()(S&& s) const noexcept
                -> tag_invoke_result_t<Tag, const S&>
            {
                return tag_invoke(Tag{}, std::as_const(s));
            }

            template<typename S>
            constexpr auto operator()(S&& s) const noexcept
            {
                return forward_progress_guarantee::weakly_parallel;
            }
        };

        template<typename CPO>
        struct get_completion_scheduler_t : public forwarding_query_t
        {
            using Tag = get_completion_scheduler_t;

            template<typename Q> 
                requires nothrow_tag_invocable<Tag, const Q&>
            constexpr auto operator()(Q&& q) const noexcept
                -> tag_invoke_result_t<Tag, const Q&>
            {
                return tag_invoke(Tag{}, std::as_const(q));
            }
        };
    };// namespace queries

    using queries::get_scheduler_t;
    using queries::get_delegatee_scheduler_t;
    using queries::get_forward_progress_guarantee_t;
    using queries::get_completion_scheduler_t;
    inline constexpr get_scheduler_t get_scheduler{};
    inline constexpr get_delegatee_scheduler_t get_delegatee_scheduler{};
    inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee_t{};
    template<typename CPO>
    inline constexpr get_completion_scheduler_t<CPO> get_completion_scheduler{};

    template<typename R>
    inline constexpr bool enable_receiver = requires { typename R::is_receiver; };

    template<typename R>
    concept receiver = enable_receiver<std::remove_cvref_t<R>> &&
        requires (const std::remove_cvref_t<R>& r) { {get_env(r)} -> queryable; } &&
        std::move_constructible<std::remove_cvref_t<R>> &&
        std::constructible_from<std::remove_cvref_t<R>, R>;

    template<typename Fn>
    struct completion_signature_traits;

    template<typename Tag, typename ... Args>
    struct completion_signature_traits<Tag(Args...)>
    {
        using tag_type = Tag;
        using args_type_list = type_list<Args...>;
    };

    template<typename Sig>
    concept valid_completion = requires{ typename completion_signature_traits<Sig>; };

    template<typename Signature, typename R>
    concept valid_completion_for = valid_completion<Signature> && receiver<R> &&
        requires (completion_signature_traits<Signature>::args_type_list args)
        {
            []<typename ... Args>(type_list<Args...>)
                requires std::invocable<typename completion_signature_traits<Signature>::tag_type, 
                    std::remove_cvref_t<R>, Args...>
            {}(args);
        };

    template<valid_completion ... Fns>
    struct completion_signatures
    {
        struct is_completion_signatures{};

        using Type = type_list<Fns...>;
    };

    template<typename Sigs>
    concept valid_completion_signatures = requires { typename Sigs::is_completion_signatures; };

    template<typename R, typename Completions>
    concept receiver_of = receiver<R> &&
        requires (Completions* completions)
        {
            []<valid_completion_for<R> ... Sigs>(completion_signatures<Sigs...>*)
            {}(completions);
        };

    namespace receivers
    {
        struct set_value_t
        {
            using Tag = set_value_t;

            template<receiver R, typename ... Args>
                requires nothrow_tag_invocable<Tag, R, Args...> &&
                    (!std::is_lvalue_reference_v<R>) && (!std::is_const_v<R>)
            constexpr auto operator()(R&& r, Args&& ... args) const noexcept
                -> tag_invoke_result_t<Tag, R, Args...>
            {
                return tag_invoke(Tag{}, std::forward<R>(r), std::forward<Args...>(args...));
            }
        };

        struct set_error_t
        {
            using Tag = set_error_t;

            template<receiver R, typename E>
                requires nothrow_tag_invocable<Tag, R, E> && 
                    (!std::is_lvalue_reference_v<R>) && (!std::is_const_v<R>)
            constexpr auto operator()(R&& r, E&& e) const noexcept
                -> tag_invoke_result_t<Tag, R, E>
            {
                return tag_invoke(Tag{}, std::forward<R>(r), std::forward<E>(e));
            }
        };

        struct set_stopped_t
        {
            using Tag = set_stopped_t;

            template<receiver R>
                requires nothrow_tag_invocable<Tag, R> && 
                    (!std::is_lvalue_reference_v<R>) && (!std::is_const_v<R>)
            constexpr auto operator()(R&& r) const noexcept
                -> tag_invoke_result_t<Tag, R>
            {
                return tag_invoke(Tag{}, std::forward<R>(r));
            }
        };
    }// namespace receivers

    using receivers::set_value_t;
    using receivers::set_error_t;
    using receivers::set_stopped_t;
    inline constexpr set_value_t set_value{};
    inline constexpr set_error_t set_error{};
    inline constexpr set_stopped_t set_stopped{};

    template<typename O>
    concept operation_state = queryable<O> &&
        std::is_object_v<O> && 
        requires(O& o)
        {
            {start(o)} noexcept;
        };

    namespace op_state
    {
        struct start_t
        {
            using Tag = start_t;

            template<typename O>
                requires nothrow_tag_invocable<Tag, O&>
            constexpr auto operator()(O& o) const noexcept
                -> tag_invoke_result_t<Tag, O&>
            {
                return tag_invoke(Tag{}, o);
            }
        };
    }// namespace op_state

    using op_state::start_t;
    inline constexpr start_t start{};

    template<typename S>
    inline constexpr bool enable_sender = requires{typename S::is_sender; };

    template<typename S>
    concept sender = enable_sender<std::remove_cvref_t<S>> &&
        requires( const std::remove_cvref_t<S>& s)
        {
            {get_env(s)} -> queryable;
        } &&
        std::move_constructible<std::remove_cvref_t<S>> &&
        std::constructible_from<std::remove_cvref_t<S>, S>;

    template<typename S, typename E = empty_env>
    concept sender_in = sender<S> &&
        requires(S&& s, E&& e)
        {
            {get_completion_signatures(std::forward<S>(s), std::forward<E>(e))} 
                -> valid_completion_signatures;
        };

    namespace signatures
    {
        struct get_completion_signatures_t
        {
            using Tag = get_completion_signatures_t;

            template<typename S, typename E>
                requires sender_in<S, E> &&
                    nothrow_tag_invocable<Tag, S, E>
            consteval auto operator()(S&& s, E&& e) const noexcept
                -> tag_invoke_result_t<Tag, S, E>
            {
                return tag_invoke(Tag{}, std::forward<S>(s), std::forward<E>(e));
            }

            template<typename S, typename E>
                requires sender_in<S, E> &&
                    (!nothrow_tag_invocable<Tag, S, E>) &&
                    requires{typename S::completion_signatures;}
            consteval auto operator()(S&& s, E&& e) const noexcept
                -> typename S::completion_signatures
            {
                return typename S::completion_signatures{};
            }
        };
    }// namespace signatures

    using signatures::get_completion_signatures_t;
    inline constexpr get_completion_signatures_t get_completion_signatures{};

    template<typename S, typename E>
        requires sender_in<S, E>
    using completion_signatures_of_t = std::invoke_result_t<get_completion_signatures_t, S, E>;

    template<typename S, typename Sig, typename E = empty_env>
    concept sender_of = sender_in<S, E> && is_include_type_v<Sig, completion_signatures_of_t<S, E>>;

    template<typename ... Ts>
    using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

    template<typename ... Ts>
    using decayed_variant = std::tuple<std::decay_t<Ts>...>;

    template<typename Sig>
    using args_type_list_of_sig = completion_signature_traits<Sig>::args_type_list;

    template<typename Tag>
    struct sig_of
    {
        template<typename Sig>
        struct Apply
        {
            constexpr static bool value = valid_completion<Sig> &&
                std::same_as<typename completion_signature_traits<Sig>::tag_type, Tag>;
        };
    };

    template<typename Tag, typename Signatures,
        template<typename...> typename Tuple = type_list, 
        template<typename...> typename Variant = type_list>
    using gather_signatures_impl = 
        Signatures::template apply<fliter_type<sig_of<Tag>::template Apply>::template apply>::
            template apply<transform_type<args_type_list_of_sig>::template apply>::
            template apply<zip_apply<Variant, Tuple>::template apply>;

    template<typename Tag, typename S, typename E = empty_env, 
        template<typename...> typename Tuple = type_list, 
        template<typename...> typename Variant = type_list>
        requires sender_in<S, E>
    using gather_signatures = 
        gather_signatures_impl<Tag, completion_signatures_of_t<S, E>, Tuple, Variant>;

    template<typename S, typename E = empty_env, 
        template<typename...> typename Tuple = decayed_tuple, 
        template<typename...> typename Variant = decayed_variant>
        requires sender_in<S, E>
    using value_types_of_t = gather_signatures<set_value_t, S, E, Tuple, Variant>;

    template<typename S, typename E = empty_env,
        template<typename...> typename Variant = decayed_variant>
        requires sender_in<S, E>
    using error_types_of_t = gather_signatures<set_error_t, S, E, std::type_identity_t, Variant>;

    template<typename S, typename E = empty_env>
        requires sender_in<S, E>
    inline constexpr bool sends_stopped = !std::is_same_v<
        gather_signatures<set_stopped_t, S, E, type_list, type_list>, type_list<>>;

}// namespace vkr::exec