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

    vkr::exec::operation_state auto op1 = vkr::exec::connect(just_sender, TestReceiver{});
    vkr::exec::operation_state auto op2 = vkr::exec::connect(just_error_sender, TestReceiver{});
    vkr::exec::operation_state auto op3 = vkr::exec::connect(just_stopped_sender, TestReceiver{});
    
    vkr::exec::start(op1);
    vkr::exec::start(op2);
    vkr::exec::start(op3);
}