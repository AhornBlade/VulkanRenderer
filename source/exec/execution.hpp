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

    template<typename T>
    concept nothrow_movable_value = movable_value<T> && 
        std::is_nothrow_constructible_v<std::decay_t<T>, T>;

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
            
            constexpr auto operator()() const noexcept;
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
            
            constexpr auto operator()() const noexcept;
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
        struct empty_env{};

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
                requires (!tag_invocable<Tag, const O&>)
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

            constexpr auto operator()() const noexcept;
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
            
            constexpr auto operator()() const noexcept;
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

    template<typename CPO, typename Q>
    using completion_scheduler_of_t = std::invoke_result_t<get_completion_scheduler_t<CPO>, Q>;

    template<typename R>
    inline constexpr bool enable_receiver = requires { typename R::is_receiver; };

    template<typename R>
    concept receiver = enable_receiver<std::remove_cvref_t<R>> &&
        requires (const std::remove_cvref_t<R>& r) { {get_env(r)} -> queryable; } &&
        std::move_constructible<std::remove_cvref_t<R>> &&
        std::constructible_from<std::remove_cvref_t<R>, R>;

    template<typename Sig>
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

    template<valid_completion ... Sigs>
    struct completion_signatures
    {
        struct is_completion_signatures{};

        using Type = type_list<Sigs...>;

        template<template<typename ...> typename Fn>
        using apply = Fn<Sigs...>;
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
                return tag_invoke(Tag{}, std::forward<R>(r), std::forward<Args>(args)...);
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

    template<typename O>
    concept operation_state = queryable<O> &&
        std::is_object_v<O> && 
        requires(O& o)
        {
            {start(o)} noexcept;
        };

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

    namespace signatures
    {
        struct get_completion_signatures_t
        {
            using Tag = get_completion_signatures_t;

            template<typename S, typename E>
                requires nothrow_tag_invocable<Tag, S, E>
            consteval auto operator()(S&& s, E&& e) const noexcept
                -> tag_invoke_result_t<Tag, S, E>
            {
                return tag_invoke(Tag{}, std::forward<S>(s), std::forward<E>(e));
            }

            template<typename S, typename E>
                requires (!nothrow_tag_invocable<Tag, S, E>) &&
                    requires{typename std::remove_cvref_t<S>::completion_signatures;}
            consteval auto operator()(S&& s, E&& e) const noexcept
                -> typename std::remove_cvref_t<S>::completion_signatures
            {
                return typename std::remove_cvref_t<S>::completion_signatures{};
            }
        };
    }// namespace signatures

    using signatures::get_completion_signatures_t;
    inline constexpr get_completion_signatures_t get_completion_signatures{};

    template<typename S, typename E = empty_env>
    concept sender_in = sender<S> &&
        requires(S&& s, E&& e)
        {
            {get_completion_signatures(std::forward<S>(s), std::forward<E>(e))} 
                -> valid_completion_signatures;
        };

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

    template<typename ... Ts>
    using default_set_value = completion_signatures<set_value_t(Ts...)>;

    template<typename E>
    using default_set_error = completion_signatures<set_error_t(E)>;

    template<sender S, typename E = empty_env, 
        valid_completion_signatures AddSigs = completion_signatures<>,
        template<typename...> typename SetValue = default_set_value,
        template<typename> typename SetError = default_set_error,
        valid_completion_signatures SetStopped = completion_signatures<set_stopped_t()>>
    using make_completion_signatures = 
        concat_type_sets_t<completion_signatures<
            AddSigs, 
            concat_type_sets_t<value_types_of_t<S, E, SetValue, completion_signatures>>, 
            concat_type_sets_t<gather_signatures<set_error_t, S, E, SetError, completion_signatures>>,
            SetStopped>>;

    namespace sender_connect
    {
        struct connect_t
        {
            using Tag = connect_t;

            template<sender S, receiver R>
                requires tag_invocable<Tag, S, R>
            constexpr auto operator()(S&& s, R&& r) const
                noexcept(nothrow_tag_invocable<Tag, S, R>)
                -> tag_invoke_result_t<Tag, S, R>
            {
                return tag_invoke(Tag{}, std::forward<S>(s), std::forward<R>(r));
            }
        };
    }// namespace sender_connect

    using sender_connect::connect_t;
    inline constexpr connect_t connect{};

    template<sender S, receiver R>
        requires std::invocable<connect_t, S, R>
    using connect_result_t = std::invoke_result_t<connect_t, S, R>;

    template<typename S, typename R>
    concept sender_to = sender_in<S, env_of_t<R>> &&
        receiver_of<R, completion_signatures_of_t<S, env_of_t<R>>> &&
        requires (S&& s, R && r)
        {
            connect(std::forward<S>(s), std::forward<R>(r));
        };

    namespace sender_factories
    {
        template<typename Tag, typename ... Ts>
        struct just_sender
        {
            using is_sender = void;

            using completion_signatures = exec::completion_signatures<Tag(Ts...)>;

            template<typename R>
            struct operation
            {
                std::tuple<Ts...> values_;
                R r_;

                friend void tag_invoke(start_t, operation& op) noexcept
                {
                    std::apply([&](Ts& ... args){
                        Tag{}(std::move(op.r_), std::move(args)...);
                    }, op.values_);
                }
            };

            template<decays_to<just_sender> Self, receiver R>
            friend auto tag_invoke(connect_t, Self&& self, R&& r)
                noexcept(nothrow_movable_value<Self> && nothrow_movable_value<R>)
                -> operation<std::decay_t<R>>
            {
                return operation<std::decay_t<R>>{std::forward<Self>(self).values_, std::forward<R>(r)};
            }

            std::tuple<Ts...> values_;
        };

        struct just_t
        {
            using Tag = set_value_t;

            template<movable_value ... Ts>
            constexpr auto operator()(Ts&& ... args) const
                noexcept((nothrow_movable_value<Ts> && ...))
                -> just_sender<Tag, std::decay_t<Ts>...>
            {
                return just_sender<Tag, std::decay_t<Ts>...>{{std::forward<Ts>(args)...}};
            }
        };

        struct just_error_t
        {
            using Tag = set_error_t;

            template<movable_value E>
            constexpr auto operator()(E&& error) const
                noexcept(nothrow_movable_value<E>)
                -> just_sender<Tag, std::decay_t<E>>
            {
                return just_sender<Tag, std::decay_t<E>>{{std::forward<E>(error)}};
            }
        };

        struct just_stopped_t
        {
            using Tag = set_stopped_t;

            constexpr auto operator()() const noexcept
                -> just_sender<Tag>
            {
                return just_sender<Tag>{};
            }
        };

        struct schedule_t
        {
            using Tag = schedule_t;

            template<typename Sch>
                requires tag_invocable<Tag, Sch> &&
                    sender<tag_invoke_result_t<Tag, Sch>>
            constexpr auto operator()(Sch&& sch) const
                noexcept(nothrow_tag_invocable<Tag, Sch>)
                -> tag_invoke_result_t<Tag, Sch>
            {
                return tag_invoke(Tag{}, std::forward<Sch>(sch));
            }
        };

        template<typename Tag>
        struct read_sender
        {
            using is_sender = void;

            template<typename R>
            struct operation
            {
                R r_;

                friend auto tag_invoke(start_t, operation& op)
                {
                    try 
                    {
                        set_value(op.r_, Tag{}(get_env(op.r_)));
                    } catch (...) {
                        set_error(op.r_, std::current_exception());
                    }
                }
            };

            template<receiver R>
            friend auto tag_invoke(connect_t, read_sender, R&& r)
                noexcept(nothrow_movable_value<R>)
                ->operation<std::decay_t<R>>
            {
                return operation<std::decay_t<R>>{std::forward<R>(r)};
            }

            template<typename Env>
                requires std::invocable<Tag, Env> && (!std::is_nothrow_invocable_v<Tag, Env>)
            friend auto tag_invoke(get_completion_signatures_t, read_sender, Env)
                ->completion_signatures<set_value_t(std::invoke_result_t<Tag, Env>), 
                    set_error_t(std::exception_ptr)> { return {};}

            template<typename Env>
                requires std::is_nothrow_invocable_v<Tag, Env>
            friend auto tag_invoke(get_completion_signatures_t, read_sender, Env)
                ->completion_signatures<set_value_t(std::invoke_result_t<Tag, Env>)>{ return {}; }
        };

        struct read_t
        {
            template<typename Tag>
            constexpr read_sender<Tag> operator()(Tag) const noexcept
            {
                return read_sender<Tag>{};
            }
        };

    }// namespace sender_factories

    using sender_factories::just_t;
    using sender_factories::just_error_t;
    using sender_factories::just_stopped_t;
    using sender_factories::schedule_t;
    using sender_factories::read_t;
    inline constexpr just_t just{};
    inline constexpr just_error_t just_error{};
    inline constexpr just_stopped_t just_stopped{};
    inline constexpr schedule_t schedule{};
    inline constexpr read_t read{};

    template<typename S>
    concept scheduler = queryable<S> &&
        requires(S&& s)
        {
            { schedule(std::forward<S>(s)) } -> sender;
            { get_completion_scheduler<set_value_t>(get_env(schedule(std::forward<S>(s)))) } 
                -> std::same_as<std::remove_cvref_t<S>>;
        } &&
        std::equality_comparable<std::remove_cvref_t<S>> &&
        std::copy_constructible<std::remove_cvref_t<S>>;

    template<scheduler Sch>
    using schedule_result_t = std::invoke_result_t<schedule_t, Sch>;

    namespace queries
    {
        constexpr auto get_scheduler_t::operator()() const noexcept
        {
            return read(get_scheduler_t{});
        }

        constexpr auto get_delegatee_scheduler_t::operator()() const noexcept
        {
            return read(get_delegatee_scheduler_t{});
        }
    }// namespace queries

    template<typename Base = void>
    struct receiver_adaptor_base
    {
        template<typename Derived>
            requires std::derived_from<std::remove_cvref_t<Derived>, receiver_adaptor_base>
        friend decltype(auto) get_base(Derived&& d) noexcept
        {
            return static_cast<receiver_adaptor_base>(d).base_;
        }

        Base base_;
    };

    template<>
    struct receiver_adaptor_base<void>
    {
        template<typename Derived>
            requires std::derived_from<std::remove_cvref_t<Derived>, receiver_adaptor_base>
        friend decltype(auto) get_base(Derived&& d) noexcept
        {
            return d.base();
        }
    };

    template<class_type Derived, typename Base = void>
    struct receiver_adaptor : public receiver_adaptor_base<Base>
    {
        friend Derived;
        struct is_receiver{};

        receiver_adaptor() = default;

        template<typename B>
            requires std::constructible_from<Base, B>
        receiver_adaptor(B&& base) :receiver_adaptor_base<Base>(std::forward<B>(base)){}

        template<typename ... As>
        friend void tag_invoke(set_value_t, Derived&& self, As&& ... as) noexcept
            requires requires{{std::move(self).set_value(std::forward<As>(as)...)} noexcept; }
        {
            std::move(self).set_value(std::forward<As>(as)...);
        }

        template<typename ... As>
        friend void tag_invoke(set_value_t, Derived&& self, As&& ... as) noexcept
            requires (!requires{{std::move(self).set_value(std::forward<As>(as)...)} noexcept; }) &&
                std::invocable<set_value_t, decltype(std::move(get_base(self))), As...>
        {
            set_value(std::move(get_base(self)), std::forward<As>(as)...);
        }

        template<typename E>
        friend void tag_invoke(set_error_t, Derived&& self, E&& e) noexcept
            requires requires{ {std::move(self).set_error(std::forward<E>(e))} noexcept; }
        {
            std::move(self).set_error(std::forward<E>(e));
        }

        template<typename E>
        friend void tag_invoke(set_error_t, Derived&& self, E&& e) noexcept
            requires (!requires{ {std::move(self).set_error(std::forward<E>(e))} noexcept; }) &&
                std::invocable<set_error_t, decltype(std::move(get_base(self))), E>
        {
            set_error(std::move(get_base(self)), std::forward<E>(e));
        }

        friend void tag_invoke(set_stopped_t, Derived&& self) noexcept
            requires requires{ {std::move(self).set_stopped()} noexcept; }
        {
            std::move(self).set_stopped();
        }

        friend void tag_invoke(set_stopped_t, Derived&& self) noexcept
            requires (!requires{ {std::move(self).set_stopped()} noexcept; }) &&
                std::invocable<set_stopped_t, decltype(std::move(get_base(self)))>
        {
            set_stopped(std::move(get_base(self)));
        }

        friend decltype(auto) tag_invoke(get_env_t, Derived&& self) 
            noexcept( noexcept(self.get_env()) )
            requires requires{ self.get_env(); }
        {
            return self.get_env();
        }

        friend decltype(auto) tag_invoke(get_env_t, const Derived& self)
            noexcept( noexcept(get_env(get_base(self))) )
            requires (!requires{ self.get_env(); }) && requires{get_env(get_base(self)); }
        {
            return get_env(get_base(self));
        }
    };

    namespace sender_adaptors
    {
        template<class_type Tag, typename T>
        struct sender_adaptor_closure
        {
            template<sender S, decays_to<sender_adaptor_closure> Self>
                requires std::invocable<Tag, S, decltype(std::declval<Self>().value_)>
            friend constexpr auto operator|(S&& s, Self&& self)
                noexcept(std::is_nothrow_invocable_v<Tag, S, decltype(std::declval<Self>().value_)>)
                -> std::invoke_result_t<Tag, S, decltype(std::declval<Self>().value_)>
            {
                return Tag{}(std::forward<S>(s), std::forward<Self>(self).value_); 
            }

            template<decays_to<sender_adaptor_closure> Self, sender S>
                requires std::invocable<Tag, decltype(std::declval<Self>().value_), S>
            friend constexpr auto operator|(Self&& self, S&& s)
                noexcept(std::is_nothrow_invocable_v<Tag, decltype(std::declval<Self>().value_), S>)
                -> std::invoke_result_t<Tag, decltype(std::declval<Self>().value_), S>
            {
                return Tag{}(std::forward<Self>(self).value_, std::forward<S>(s)); 
            }

            T value_;
        };

        template<class_type Derived>
        struct sender_adaptor_base
        {
            template<movable_value T>
            constexpr auto operator()(T&& second) const noexcept(nothrow_movable_value<T>)
                ->sender_adaptor_closure<Derived, std::decay_t<T>>
            {
                return sender_adaptor_closure<Derived, std::decay_t<T>>{std::forward<T>(second)};
            }
        };

        template<typename R>
        struct upon_completion_signatures_helper 
        {
            using Type = set_value_t(R);
        };

        template<>
        struct upon_completion_signatures_helper<void>
        {
            using Type = set_value_t();
        };

        template<typename CPO, typename TargetCPO, typename Default, typename F, typename ... Ts>
        struct upon_completion_signatures
        {
            using Type = Default;
        };

        template<typename CPO, typename Default, typename F, typename ... Ts>
        struct upon_completion_signatures<CPO, CPO, Default, F, Ts...>
        {
            using Type = completion_signatures<
                typename upon_completion_signatures_helper<std::invoke_result_t<F, Ts...>>::Type,
                std::conditional_t<std::is_nothrow_invocable_v<F, Ts...>, 
                    set_error_t(), set_error_t(std::exception_ptr)>>;
        };

        template<typename CPO, typename TargetCPO, typename Default, typename F, typename ... Ts>
        using upon_completion_signatures_t = 
            upon_completion_signatures<CPO, TargetCPO, Default, F, Ts...>::Type;

        template<typename CPO, typename R, typename F>
        struct upon_receiver
        {
            using is_receiver = void;

            template<typename ... Ts>
                requires std::invocable<F, Ts...>
            friend void tag_invoke(CPO, upon_receiver&& self, Ts&& ... args) noexcept
            {        
                try{
                    if constexpr(std::is_void_v<std::invoke_result_t<F&&, Ts...>>)
                    {
                        std::move(self.f_)(std::forward<Ts>(args)...);
                        exec::set_value(std::move(self.r_));
                    }
                    else
                    {
                        exec::set_value(std::move(self.r_), std::move(self.f_)(std::forward<Ts>(args)...));
                    }
                }catch(...)
                {
                    exec::set_error(std::move(self.r_), std::current_exception());
                }
            }

            template<typename OtherCPO, decays_to<upon_receiver> Self, typename ... Ts>
                requires std::invocable<OtherCPO, decltype(std::declval<Self>().r_), Ts...> &&
                    (!std::same_as<OtherCPO, CPO>)
            friend auto tag_invoke(OtherCPO, Self&& self, Ts&& ... args) 
                noexcept(std::is_nothrow_invocable_v<OtherCPO, decltype(std::declval<Self>().r_), Ts...>)
                -> std::invoke_result_t<OtherCPO, decltype(std::declval<Self>().r_), Ts...>
            {
                return OtherCPO{}(std::forward<Self>(self).r_, std::forward<Ts>(args)...);
            }

            R r_;
            F f_;
        };

        template<typename CPO, typename S, typename F>
        struct upon_sender
        {
            using is_sender = void;

            template<typename ... Ts>
            using SetValue = upon_completion_signatures_t<CPO, set_value_t, 
                default_set_value<Ts...>, F, Ts...>;

            template<typename E>
            using SetError = upon_completion_signatures_t<CPO, set_error_t, 
                default_set_error<E>, F, E>;
            
            using SetStopped = upon_completion_signatures_t<CPO, set_stopped_t, 
                completion_signatures<set_stopped_t()>, F>;

            template<decays_to<upon_sender> Self, typename Env>
            friend consteval auto tag_invoke(get_completion_signatures_t, Self&&, Env&&) noexcept
                ->make_completion_signatures<S, Env, completion_signatures<>, SetValue, SetError, SetStopped>
            {
                return {};
            }

            template<decays_to<upon_sender> Self, receiver R>
                requires sender_to<decltype(std::declval<Self>().s_), 
                    upon_receiver<CPO, std::remove_cvref_t<R>, F>>
            friend auto tag_invoke(connect_t, Self&& self, R&& r)
                noexcept(std::is_nothrow_invocable_v<connect_t, decltype(std::declval<Self>().s_), 
                    upon_receiver<CPO, std::remove_cvref_t<R>, F>> && 
                    nothrow_movable_value<Self> && nothrow_movable_value<R>)
                -> connect_result_t<decltype(std::declval<Self>().s_), 
                    upon_receiver<CPO, std::remove_cvref_t<R>, F>>
            {
                return connect(std::forward<Self>(self).s_, upon_receiver<CPO, std::remove_cvref_t<R>, F>
                    {std::forward<R>(r), std::forward<Self>(self).f_});
            }

            friend decltype(auto) tag_invoke(get_env_t, const upon_sender& self) noexcept
            {
                return get_env(self.s_);
            }

            S s_;
            F f_;
        };

        template<typename CPO>
        struct upon : public sender_adaptor_base<upon<CPO>>
        {
            using Tag = upon;

            using sender_adaptor_base<upon<CPO>>::operator();

            template<sender S, movable_value F>
                requires tag_invocable<Tag, completion_scheduler_of_t<CPO, env_of_t<S>>, S, F>
            constexpr auto operator()(S&& s, F&& f) const
                noexcept(nothrow_tag_invocable<Tag, completion_scheduler_of_t<CPO, env_of_t<S>>, S, F>)
                -> tag_invoke_result_t<Tag, completion_scheduler_of_t<CPO, env_of_t<S>>, S, F>
            {
                return tag_invoke(Tag{}, get_completion_scheduler<CPO>(get_env(s)), 
                    std::forward<S>(s), std::forward<F>(f));
            }

            template<sender S, movable_value F>
                requires (!tag_invocable<Tag, completion_scheduler_of_t<CPO, env_of_t<S>>, S, F>) &&
                    tag_invocable<Tag, S, F>
            constexpr auto operator()(S&& s, F&& f) const
                noexcept(nothrow_tag_invocable<Tag, S, F>)
                -> tag_invoke_result_t<Tag, S, F>
            {
                return tag_invoke(Tag{}, std::forward<S>(s), std::forward<F>(f));
            }

            template<sender S, movable_value F>
                requires (!tag_invocable<Tag, completion_scheduler_of_t<CPO, env_of_t<S>>, S, F>) &&
                    (!tag_invocable<Tag, S, F>)
            constexpr auto operator()(S&& s, F&& f) const
                noexcept(nothrow_movable_value<S> && nothrow_movable_value<F>)
                -> upon_sender<CPO, std::remove_cvref_t<S>, std::decay_t<F>>
            {
                return {std::forward<S>(s), std::forward<F>(f)};
            }
        };

        using then_t = upon<set_value_t>;
        using upon_error_t = upon<set_error_t>;
        using upon_stopped_t = upon<set_stopped_t>;

        template<typename Sch, typename Env>
        struct on_receiver_wrapper_env
        {
            friend Sch tag_invoke(get_scheduler_t, const on_receiver_wrapper_env& self) noexcept
            {
                return self.sch_;
            }

            template<forwardingable_query Tag>
                requires (!std::same_as<Tag, get_scheduler_t>)
            friend auto tag_invoke(Tag tag, const on_receiver_wrapper_env& self) noexcept
                -> std::invoke_result_t<Tag, const Env&>
            {
                return tag(self.env_);
            }

            Sch sch_;
            Env env_;
        };

        template<typename R, typename Sch>
        struct on_receiver_wrapper : public receiver_adaptor<on_receiver_wrapper<R, Sch>, R>
        {
            on_receiver_wrapper_env<Sch, env_of_t<R>> get_env() noexcept
            {
                return {this->sch_, get_env(get_base(*this))};
            }

            Sch sch_;
        };

        template<typename R, typename S, typename Sch>
        struct on_receiver : public receiver_adaptor<on_receiver<R, S, Sch>, R>
        {
            void set_value() && noexcept
            {
                try {
                    operation_state auto op = connect(std::move(this->s_), on_receiver_wrapper<R, Sch>
                        {std::move(get_base(*this)), std::move(this->sch_)});
                    start(op);
                } catch (...) {
                    exec::set_error(std::move(get_base(*this)), std::current_exception());
                }
            }

            S s_;
            Sch sch_;
        };

        template<typename Sch, typename S>
        struct on_sender
        {
            using is_sender = void;

            template<typename ... Ts>
            using SetValue = completion_signatures<>;

            template<decays_to<on_sender> Self, typename Env>
            friend consteval auto tag_invoke(get_completion_signatures_t, Self&&, Env&&) noexcept
                -> make_completion_signatures<decltype(std::declval<Self>().s_), 
                on_receiver_wrapper_env<Sch, Env>, make_completion_signatures<schedule_result_t<Sch>, 
                Env, completion_signatures<set_error_t(std::exception_ptr)>, SetValue>>
            {
                return {};
            }

            template<decays_to<on_sender> Self, receiver R>
                requires sender_to<schedule_result_t<Sch>, 
                    on_receiver<std::remove_cvref_t<R>, std::remove_cvref_t<S>, std::remove_cvref_t<Sch>>>
            friend auto tag_invoke(connect_t, Self&& self, R&& r)
                noexcept(std::is_nothrow_invocable_v<connect_t, schedule_result_t<Sch>, 
                    on_receiver<std::remove_cvref_t<R>, std::remove_cvref_t<S>, std::remove_cvref_t<Sch>>>)
                -> connect_result_t<schedule_result_t<Sch>, 
                    on_receiver<std::remove_cvref_t<R>, std::remove_cvref_t<S>, std::remove_cvref_t<Sch>>>
            {
                return connect(schedule(std::forward<Self>(self).sch_), 
                    on_receiver<std::remove_cvref_t<R>, std::remove_cvref_t<S>, std::remove_cvref_t<Sch>>
                    {std::forward<R>(r), std::forward<Self>(self).s_, std::forward<Self>(self).sch_});
            }

            Sch sch_;
            S s_;
        };

        struct on_t : public sender_adaptor_base<on_t>
        {
            using Tag = on_t;

            using sender_adaptor_base<on_t>::operator();

            template<scheduler Sch, sender S>
                requires tag_invocable<Tag, Sch, S>
            constexpr auto operator()(Sch&& sch, S&& s) const
                noexcept(nothrow_tag_invocable<Tag, Sch, S>)
                -> tag_invoke_result_t<Tag, Sch, S>
            {
                return tag_invoke(Tag{}, std::forward<Sch>(sch), std::forward<S>(s));
            }

            template<scheduler Sch, sender S>
                requires (!tag_invocable<Tag, Sch, S>)
            constexpr auto operator()(Sch&& sch, S&& s) const 
                noexcept(nothrow_movable_value<Sch> && nothrow_movable_value<S>)
                -> on_sender<std::remove_cvref_t<Sch>, std::remove_cvref_t<S>>
            {
                return {std::forward<Sch>(sch), std::forward<S>(s)};
            }
        };

        template<typename Sch, typename R>
        struct schedule_from_receiver : public receiver_adaptor<schedule_from_receiver<Sch, R>, R>
        {
            template<typename ... Args>
            void set_value(Args&& ... args) && noexcept
            {
                try
                {
                    operation_state auto op = connect(
                        on_t{}(std::move(sch_)) |
                        just_t{}(std::forward<Args>(args)...), std::move(get_base(*this)));
                    start(op);
                }
                catch(...)
                {
                    exec::set_error(std::move(get_base(*this)), std::current_exception());
                }
            }

            Sch sch_;
        };

        template<typename Sch, typename S>
        struct schedule_from_sender
        {
            using is_sender = void;

            template<typename Env>
            struct SetValue
            {
                template<typename ... Ts>
                using apply = make_completion_signatures<on_sender<Sch, 
                    decltype(just(std::declval<Ts>()...))>, Env>;
            };

            template<decays_to<schedule_from_sender> Self, typename Env>
            friend consteval auto tag_invoke(get_completion_signatures_t, Self&&, Env&&) noexcept
                -> make_completion_signatures<S, Env, 
                    completion_signatures<set_error_t(std::exception_ptr)>, 
                    SetValue<Env>::template apply>
            {
                return {};
            }

            template<decays_to<schedule_from_sender> Self, receiver R>
                requires sender_to<decltype(std::declval<Self>().s_), 
                    schedule_from_receiver<Sch, std::remove_cvref_t<R>>>
            friend auto tag_invoke(connect_t, Self&& self, R&& r)
                noexcept(std::is_nothrow_invocable_v<connect_t, decltype(std::declval<Self>().s_), 
                    schedule_from_receiver<Sch, std::remove_cvref_t<R>>>)
                -> connect_result_t<decltype(std::declval<Self>().s_), 
                    schedule_from_receiver<Sch, std::remove_cvref_t<R>>>
            {
                return connect(std::forward<Self>(self).s_, 
                    schedule_from_receiver<Sch, std::remove_cvref_t<R>>
                    {std::forward<R>(r), std::forward<Self>(self).sch_});
            }

            Sch sch_;
            S s_;
        };

        struct schedule_from_t : public sender_adaptor_base<schedule_from_t>
        {
            using Tag = schedule_from_t;

            using sender_adaptor_base<schedule_from_t>::operator();

            template<scheduler Sch, sender S>
                requires tag_invocable<Tag, Sch, S>
            constexpr auto operator()(Sch&& sch, S&& s) const
                noexcept(nothrow_tag_invocable<Tag, Sch, S>)
                -> tag_invoke_result_t<Tag, Sch, S>
            {
                return tag_invoke(Tag{}, std::forward<Sch>(sch), std::forward<S>(s));
            }

            template<scheduler Sch, sender S>
                requires (!tag_invocable<Tag, Sch, S>)
            constexpr auto operator()(Sch&& sch, S&& s) const 
                noexcept(nothrow_movable_value<Sch> && nothrow_movable_value<S>)
                ->schedule_from_sender<std::remove_cvref_t<Sch>, std::remove_cvref_t<S>>
            {
                return {std::forward<Sch>(sch), std::forward<S>(s)};
            }
        };

        struct transfer_t: public sender_adaptor_base<transfer_t>
        {
            using Tag = transfer_t;

            using sender_adaptor_base<transfer_t>::operator();

            template<sender S, scheduler Sch>
                requires tag_invocable<Tag, completion_scheduler_of_t<set_value_t, env_of_t<S>>, S, Sch>
            constexpr auto operator()(S&& s, Sch&& sch) const
                noexcept(nothrow_tag_invocable<Tag, completion_scheduler_of_t<set_value_t, 
                    env_of_t<S>>, S, Sch>)
                -> tag_invoke_result_t<Tag, completion_scheduler_of_t<set_value_t, env_of_t<S>>, S, Sch>
            {
                return tag_invoke(Tag{}, get_completion_scheduler_t<set_value_t>(get_env_t(s)), 
                    std::forward<S>(s), std::forward<Sch>(sch));
            }

            template<sender S, scheduler Sch>
                requires (!tag_invocable<Tag, completion_scheduler_of_t<set_value_t, env_of_t<S>>, S, Sch>) &&
                    tag_invocable<Tag, S, Sch>
            constexpr auto operator()(S&& s, Sch&& sch) const
                noexcept(nothrow_tag_invocable<Tag, S, Sch>)
                -> tag_invoke_result_t<Tag, S, Sch>
            {
                return tag_invoke(Tag{}, std::forward<S>(s), std::forward<Sch>(sch));
            }

            template<sender S, scheduler Sch>
                requires (!tag_invocable<Tag, completion_scheduler_of_t<set_value_t, env_of_t<S>>, S, Sch>) &&
                    (!tag_invocable<Tag, S, Sch>)
            constexpr auto operator()(S&& s, Sch&& sch) const
                noexcept(std::is_nothrow_invocable_v<schedule_from_t, Sch, S>)
                -> std::invoke_result_t<schedule_from_t, Sch, S>
            {
                return schedule_from_t{}(std::forward<Sch>(sch), std::forward<S>(s));
            }
        };

    }// namespace sender_adaptors

    using sender_adaptors::sender_adaptor_closure;
    using sender_adaptors::then_t;
    using sender_adaptors::upon_error_t;
    using sender_adaptors::upon_stopped_t;
    using sender_adaptors::on_t;
    using sender_adaptors::schedule_from_t;
    using sender_adaptors::transfer_t;

    inline constexpr then_t then{};
    inline constexpr upon_error_t upon_error{};
    inline constexpr upon_stopped_t upon_stopped{};
    inline constexpr on_t on{};
    inline constexpr schedule_from_t schedule_from{};
    inline constexpr transfer_t transfer{};

}// namespace vkr::exec

namespace vkr::queries
{
    constexpr auto get_allocator_t::operator()() const noexcept
    {
        return exec::read(get_allocator_t{});
    }

    constexpr auto get_stop_token_t::operator()() const noexcept
    {
        return exec::read(get_stop_token_t{});
    }

}// namespace vkr::queries

namespace vkr::exec
{
    namespace sender_factories
    {
        struct transfer_just_t
        {
            using Tag = transfer_just_t;

            template<typename Sch, typename ... Ts>
                requires tag_invocable<Tag, Sch, Ts...>
            constexpr auto operator()(Sch&& sch, Ts&& ... args) const
                noexcept(nothrow_tag_invocable<Tag, Sch, Ts...>)
                -> tag_invoke_result_t<Tag, Sch, Ts...>
            {
                return tag_invoke(Tag{}, std::forward<Sch>(sch), std::forward<Ts>(args)...);
            }

            template<typename Sch, typename ... Ts>
                requires (!tag_invocable<Tag, Sch, Ts...>) && 
                std::invocable<transfer_t, just_sender<set_value_t, std::remove_cvref_t<Ts>...>, Sch>
            constexpr auto operator()(Sch&& sch, Ts&& ... args) const
            {
                return transfer(just(std::forward<Ts>(args)...), std::forward<Sch>(sch));
            }
        };
    }// namespace sender_factories

    using sender_factories::transfer_just_t;
    inline constexpr transfer_just_t transfer_just{};

}// namespace vkr::exec