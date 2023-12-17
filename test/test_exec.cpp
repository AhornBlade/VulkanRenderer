#include <exec/tag_invoke.hpp>
#include <exec/receiver.hpp>
#include <exec/sender.hpp>

#include <iostream>

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

	friend void tag_invoke(vkr::exec::set_done_t, test_receiver&&) noexcept
	{
		std::cout << "test_reveiver set_done" << '\n';
	}
};

int main()
{
	vkr::exec::receiver auto r = test_receiver{};
	vkr::exec::set_value(static_cast<test_receiver&&>(r), 2387, ' ', "value");
	vkr::exec::set_error(static_cast<test_receiver&&>(r), false);
	vkr::exec::set_done(static_cast<test_receiver&&>(r));

}