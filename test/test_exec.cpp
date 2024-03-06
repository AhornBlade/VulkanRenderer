#include <exec/execution.hpp>
#include <exec/scheduler.hpp>

#include <iostream>
#include <format>

using namespace std::chrono_literals;

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

    friend void tag_invoke(vkr::exec::set_value_t, TestReceiver&&) noexcept
    {

    }

    friend void tag_invoke(vkr::exec::set_value_t, TestReceiver&&, auto&& ... args) noexcept
    {
        std::cout << "set_value ";
        (std::cout << ... << args) << '\n';
    }

    friend void tag_invoke(vkr::exec::set_error_t, TestReceiver&&, std::string_view sv) noexcept
    {
        std::cout << "set_error std::string_view " << sv << '\n';
    }

    friend void tag_invoke(vkr::exec::set_error_t, TestReceiver&&, const std::exception& e) noexcept
    {
        std::cout << "set_error std::exception " << e.what() << '\n';
    }

    friend void tag_invoke(vkr::exec::set_error_t, TestReceiver&&, std::exception_ptr eptr) noexcept
    {
        try {
            if(eptr)
            {
                std::rethrow_exception(eptr);
            }
        } catch (const std::exception& e) {
            std::cout << "set_error std::exception_ptr " << e.what() << '\n';
        }
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

struct Test
{
    Test() = default;
    Test(const Test&) { std::cout << "copy constructor\n";}
    Test(Test&&) noexcept { std::cout << "move constructor\n";}
    Test& operator=(const Test&) {std::cout << "copy assignment\n"; return *this;}
    Test& operator=(Test&&) noexcept {std::cout << "move assignment\n"; return *this;}

    friend std::ostream& operator<<(std::ostream& out, const Test& test)
    {
        out << " Test ";
        return out;
    }
};

struct TestOpHandle
{
    void execute()
    {
        std::cout << "TestOP\n";
    }

    TestOpHandle() = default;
    TestOpHandle(const TestOpHandle&) = delete;
    TestOpHandle& operator=(const TestOpHandle&) = delete;
    TestOpHandle(TestOpHandle&&) noexcept = default;
    TestOpHandle& operator=(TestOpHandle&&) noexcept = default;
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
    auto then_sig = vkr::exec::get_completion_signatures(then_sender, vkr::empty_env{});
    vkr::exec::operation_state auto op5 = vkr::exec::connect(then_sender, TestReceiver{});
    vkr::exec::start(op5);

    vkr::exec::sender auto upon_error_sender = vkr::exec::upon_error(just_error_sender,
        [](const std::exception& e) { return e.what(); });
    vkr::exec::operation_state auto op6 = vkr::exec::connect(upon_error_sender, TestReceiver{});
    vkr::exec::start(op6);

    vkr::exec::sender auto upon_stopped_sender = vkr::exec::upon_stopped(just_stopped_sender,
        []{});
    vkr::exec::operation_state auto op7 = vkr::exec::connect(upon_stopped_sender, TestReceiver{});
    vkr::exec::start(op7);

    vkr::exec::sender auto pipe_sender = 
        vkr::exec::just(42) |
        vkr::exec::then([](int i){ return i * 2;});
    vkr::exec::operation_state auto op8 = vkr::exec::connect(pipe_sender, TestReceiver{});
    vkr::exec::start(op8);

    vkr::exec::sender auto pipe_sender2 = 
        vkr::exec::just_error(std::runtime_error("just error")) |
        vkr::exec::upon_error([](const std::exception& e) {return e.what();});
    vkr::exec::operation_state auto op9 = vkr::exec::connect(pipe_sender2, TestReceiver{});
    vkr::exec::start(op9);

    vkr::exec::scheduler auto inline_scheduler = vkr::exec::inline_scheduler{};

    Test test{};
    vkr::exec::sender auto test_just = vkr::exec::just(std::move(test));
    vkr::exec::sender auto test_then = vkr::exec::then(test_just, [](const Test&){return 1;});
    vkr::exec::sender auto test_on = vkr::exec::on(inline_scheduler, test_then);
    using TestOP2Sigs = vkr::exec::completion_signatures_of_t<decltype(test_then), vkr::empty_env>;
    vkr::exec::operation_state auto test_op = vkr::exec::connect(test_just, TestReceiver{});
    vkr::exec::start(test_op);

    vkr::exec::operation_state auto test_op2 = 
        vkr::exec::connect(
            vkr::exec::on(inline_scheduler) |
            vkr::exec::just(Test{}) | 
            vkr::exec::then([](const Test&){return 1;}), TestReceiver{});
    vkr::exec::start(test_op2);

    std::cout << "main thread id: " << std::this_thread::get_id() << '\n';

    std::mutex mutex_{};

    vkr::exec::thread_run_loop test_loop_1{3};
    vkr::exec::thread_run_loop test_loop_2{3};
    vkr::exec::scheduler auto loop_sch_1 = vkr::exec::get_scheduler(test_loop_1);
    vkr::exec::scheduler auto loop_sch_2 = vkr::exec::get_scheduler(test_loop_2);
    vkr::exec::sender auto loop_sender = 
        vkr::exec::on(loop_sch_1) |
        vkr::exec::just() |
        vkr::exec::then([]
        {
            std::this_thread::sleep_for(1ms);
            return std::this_thread::get_id();
        }) |
        vkr::exec::transfer(loop_sch_2) |
        vkr::exec::then([&](std::thread::id id)
        {
            std::unique_lock lock{mutex_};
            std::this_thread::sleep_for(1ms);
            std::cout << "before: " << id << ", after: " << std::this_thread::get_id() << '\n';
        });
    vkr::exec::operation_state auto loop_op = 
        vkr::exec::connect(loop_sender, TestReceiver{});
    for(uint32_t index = 0; index < 10; index++)
    {
        vkr::exec::start(loop_op);
    }

    std::this_thread::sleep_for(1s);
}