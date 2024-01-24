#include <exec/execution.hpp>

#include <iostream>

int main()
{
    std::cout << vkr::forwarding_query(vkr::get_allocator) << '\n';
    std::cout << vkr::forwardingable_query<vkr::get_allocator_t> << '\n';


}