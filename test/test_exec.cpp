#include <exec/tag_invoke.hpp>
#include <exec/receiver.hpp>
#include <exec/sender.hpp>
#include <exec/type_list.hpp>

#include <iostream>
#include <ranges>

struct test_receiver
{
	friend void tag_invoke(vkr::exec::set_value_t, test_receiver&&, auto&& ... args)
	{
		std::cout << "test_receiver set_value" << '\n';
		(std::cout << ... << args) << '\n';
	}

	friend void tag_invoke(vkr::exec::set_error_t, test_receiver&&, auto&& e) noexcept
	{
		std::cout << "test_receiver set_error" << '\n';
		std::cout << e << '\n';
	}

	friend void tag_invoke(vkr::exec::set_error_t, test_receiver&&, std::exception_ptr e) noexcept
	{
		std::cout << "test_receiver set_error" << '\n';
		if (e)
		{
			std::rethrow_exception(e);
		}
	}

	friend void tag_invoke(vkr::exec::set_done_t, test_receiver&&) noexcept
	{
		std::cout << "test_reveiver set_done" << '\n';
	}
};

using TypeList1 = vkr::exec::type_list<int, float, double, char>;
using TypeList2 = vkr::exec::type_list<bool, float, double, char*>;
using TypeList3 = vkr::exec::type_list<uint32_t, float, double*, char>;

using Result = typename vkr::exec::concat_type_sets_impl<TypeList1, TypeList2, TypeList3>::Type;

using List = typename vkr::exec::type_list<TypeList1, TypeList2, TypeList3>;

//using zip = vkr::exec::zip_apply< List, 

int main()
{
	vkr::exec::receiver auto r = test_receiver{};
	vkr::exec::set_value(std::move(r), 2387, ' ', "value");
	vkr::exec::set_error(std::move(r), false);
	vkr::exec::set_done(std::move(r));

	std::cout << vkr::exec::type_among<int, int, double, char> << '\n';
	std::cout << vkr::exec::type_among<int, float, double, char> << '\n';

	std::cout << Result::apply<vkr::exec::type_count>::value << '\n';

	Result::apply<std::tuple> t;

	vkr::exec::sender auto just_s = vkr::exec::just(1, 3, 5);
	vkr::exec::sender_traits<decltype(just_s)> s_traits;

	auto op1 = vkr::exec::connect(just_s, r);

	vkr::exec::start(op1);

	auto add = [](auto ... args) {return (args + ...); };

	vkr::exec::sender auto then_s = vkr::exec::then(just_s, add);

	auto op2 = vkr::exec::connect(then_s, r);
	vkr::exec::start(op2);

	// std::cout << vkr::exec::_then::sender_to_function<decltype(std::move(just_s)), decltype(std::move(add))> << '\n';

	auto op3 = vkr::exec::just(2, 4, 6) 
		| vkr::exec::then([](auto ... args) {return (args + ...); })
		| vkr::exec::connect(r);

	vkr::exec::start(op3);
}