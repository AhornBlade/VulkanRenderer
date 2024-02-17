#include <exec/execution.hpp>

#include <iostream>

using List1 = vkr::type_list<int, bool, int>;
using List2 = vkr::type_list<bool, float>;
using List3 = vkr::type_list<int, long, float, char*>;

using F1 = vkr::concat_type_lists_t<vkr::type_list<List1, List2, List3>>;
using F2 = vkr::concat_type_sets_t<vkr::type_list<List1, List2, List3>>;

using Sender_Sig = vkr::type_list<
    vkr::exec::set_value_t(int, bool), 
    vkr::exec::set_value_t(uint32_t, char),
    vkr::exec::set_value_t(char*),
    vkr::exec::set_stopped_t(),
    vkr::exec::set_error_t(std::exception_ptr)>;

using Sig1 = vkr::exec::gather_signatures_impl<vkr::exec::set_value_t, Sender_Sig, std::tuple, std::variant>;

class TestReceiver
{
public:
    struct is_receiver{};

    friend void tag_invoke(vkr::exec::set_value_t, TestReceiver&&, auto&& ... args) noexcept
    {
        (std::cout << ... << args) << '\n';
    }

    friend void tag_invoke(vkr::exec::set_error_t, TestReceiver&&, const std::exception& e) noexcept
    {
        std::cout << e.what() << '\n';
    }

    friend void tag_invoke(vkr::exec::set_stopped_t, TestReceiver&&) noexcept
    {
        std::cout << "stopped\n";
    } 
};

template<typename R>
class TestReceiverAdaptor: public vkr::exec::receiver_adaptor<TestReceiverAdaptor<R>, R>
{
public:
    using vkr::exec::receiver_adaptor<TestReceiverAdaptor<R>, R>::receiver_adaptor;

    template<typename ... Ts>
    void set_value(Ts&& ... args) && noexcept
    {
        vkr::exec::set_value(std::move(get_base(*this)), "TestReceiverAdaptor out ", std::forward<Ts>(args)...);
    }
};

int main()
{
    std::cout << vkr::forwarding_query(vkr::get_allocator) << '\n';
    std::cout << vkr::forwardingable_query<vkr::get_allocator_t> << '\n';

    std::cout << vkr::type_list_like<int> << '\n';
    std::cout << vkr::type_count_v<vkr::type_list<int, bool, char>> << '\n';
    std::cout << vkr::type_count_v<F1> << '\n';
    std::cout << vkr::type_count_v<F2> << '\n';

    vkr::exec::sender auto just_sender = vkr::exec::just(1, " ", 0.5f, " ", '0');
    vkr::exec::sender auto just_error_sender = vkr::exec::just_error(std::runtime_error("test error"));
    vkr::exec::sender auto just_stopped_sender = vkr::exec::just_stopped();

    using JustSig = vkr::exec::completion_signatures_of_t<decltype(just_sender), vkr::empty_env>;
    using JustSetValue = vkr::exec::value_types_of_t<decltype(just_sender), vkr::empty_env, 
        vkr::type_list, vkr::type_list>;
    using JustSetError = vkr::exec::error_types_of_t<decltype(just_sender), vkr::empty_env, 
        vkr::type_list>;
    constexpr bool JustStopped = vkr::exec::sends_stopped<decltype(just_sender), vkr::empty_env>;

    using MakeJustSig = vkr::exec::make_completion_signatures<decltype(just_sender), vkr::empty_env>;

    vkr::exec::operation_state auto op1 = vkr::exec::connect(just_sender, TestReceiver{});
    vkr::exec::operation_state auto op2 = vkr::exec::connect(just_error_sender, TestReceiver{});
    vkr::exec::operation_state auto op3 = vkr::exec::connect(just_stopped_sender, TestReceiver{});
    
    vkr::exec::start(op1);
    vkr::exec::start(op2);
    vkr::exec::start(op3);

    vkr::exec::operation_state auto op4 = vkr::exec::connect(just_sender, TestReceiverAdaptor<TestReceiver>{});

    vkr::exec::start(op4);

    vkr::exec::sender auto then_sender = vkr::exec::then(just_sender, 
        []<typename ... Ts>(Ts...) -> uint32_t {return sizeof...(Ts);});

    vkr::exec::operation_state auto op5 = vkr::exec::connect(then_sender, TestReceiver{});

    vkr::exec::start(op5);
}