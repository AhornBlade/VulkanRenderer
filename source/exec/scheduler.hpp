#pragma once

#include <memory>
#include <queue>
#include <list>
#include <condition_variable>

#include "execution.hpp"

namespace vkr::exec
{
    template<typename ... Args>
    struct operation_wrapper_base
    {
        operation_wrapper_base() = default;
        virtual ~operation_wrapper_base() {}
        virtual void execute(Args ... args) = 0;
    };

    template<typename R, typename ... Args>
    struct operation_wrapper : public operation_wrapper_base<Args...>
    {
        template<decays_to<R> T>
        operation_wrapper(T&& r) : r_{std::forward<T>(r)} {}
        
        virtual void execute(Args ... args)
        {
            if(get_stop_token(this->r_).stop_requested())
            {
                set_stopped(std::move(this->r_));
            }
            else
            {
                set_value(std::move(this->r_), std::forward<Args>(args)...);
            }
        }

        R r_;
    };

    template<typename ... Args>
    struct move_only_operation
    {
        move_only_operation() = default;
        move_only_operation(const move_only_operation&) = delete;
        move_only_operation& operator=(const move_only_operation&) = delete;
        move_only_operation(move_only_operation&&) noexcept = default;
        move_only_operation& operator=(move_only_operation&&) noexcept = default;

        template<receiver_of<completion_signatures<set_value_t(Args...)>> R>
        move_only_operation(R&& r) 
            : handle_{new operation_wrapper<std::remove_cvref_t<R>, Args...>{std::forward<R>(r)}} {}

        void execute(Args ... args)
        {
            handle_->execute(std::forward<Args>(args)...);
        }

        operator bool() const noexcept
        {
            return bool(handle_);
        }

        std::unique_ptr<operation_wrapper_base<Args...>> handle_;
    };

    namespace schedulers
    {
        struct inline_scheduler
        {
            struct env_
            {
                template<typename Tag>
                friend inline_scheduler tag_invoke(exec::get_completion_scheduler_t<Tag>, const env_&) noexcept
                {
                    return inline_scheduler{};
                }
            };

            struct sender_
            {
                using is_sender = void;

                using completion_signatures = exec::completion_signatures<set_value_t()>;

                template<typename R>
                struct operation
                {
                    friend void tag_invoke(start_t, operation& self) noexcept
                    {
                        set_value(std::move(self.r_));
                    }

                    R r_;
                };

                template<decays_to<sender_> Self, receiver R>
                friend auto tag_invoke(connect_t, Self&&, R&& r)
                    noexcept(nothrow_movable_value<R>)
                    ->operation<std::remove_cvref_t<R>>
                {
                    return {std::forward<R>(r)};
                }

                friend env_ tag_invoke(get_env_t, const sender_& self) noexcept
                {
                    return {};
                }
            };

            friend sender_ tag_invoke(schedule_t, const inline_scheduler& self) noexcept
            {
                return {};
            }

            bool operator==(const inline_scheduler& other) const
            {
                return this == &other;
            }
        };

        template<typename ... Args>
        class run_loop
        {
        public:
            run_loop() = default;
            run_loop(const run_loop&) = delete;
            run_loop& operator=(const run_loop&) = delete;
            run_loop(run_loop&& other) = delete;
            run_loop& operator=(run_loop&& other) = delete;

            void push(receiver_of<completion_signatures<set_value_t(Args...)>> auto&& r)
            {
                {
                    std::unique_lock lock{mutex_};
                    operations_.push(move_only_operation<Args...>{std::forward<decltype(r)>(r)});
                }
                cv_.notify_one();
            }

            move_only_operation<Args...> pop()
            {
                move_only_operation<Args...> op;
                {
                    std::unique_lock lock{mutex_};
                    cv_.wait(lock, [&](){return finished || (!operations_.empty());});
                    if(finished) return {};
                    op = std::move(operations_.front());
                    operations_.pop();
                }
                return op;
            }

            void run(Args ... args)
            {
                while(auto op = pop())
                {
                    op.execute(std::forward<Args>(args)...);
                }
            }

            void finish()
            {
                {
                    std::unique_lock lock{mutex_};
                    finished = true;
                }
                cv_.notify_all();
            }
            
            template<typename R>
            struct operation_
            {
                friend void tag_invoke(start_t, operation_& self) noexcept
                {
                    try
                    {
                        self.env_handle->push(std::move(self.r_));
                    }
                    catch(...)
                    {
                        set_error(std::move(self.r_), std::current_exception());
                    }
                }
                
                R r_;
                run_loop* env_handle;
            };

            struct sender_
            {
                using is_sender = void;

                using completion_signatures = exec::completion_signatures<
                    set_value_t(), set_stopped_t(), set_error_t(std::exception_ptr)>;

                template<decays_to<sender_> Self, receiver R>
                friend auto tag_invoke(connect_t, Self&& self, R&& r)
                    noexcept(nothrow_movable_value<R>) -> operation_<std::remove_cvref_t<R>>
                {
                    return {std::forward<R>(r), std::forward<Self>(self).env_handle};
                }

                friend auto& tag_invoke(get_env_t, const sender_& self) noexcept
                {
                    return *(self.env_handle);
                }

                run_loop* env_handle;
            };

            struct scheduler_
            {
                friend sender_ tag_invoke(schedule_t, const scheduler_& self) noexcept
                {
                    return {self.env_handle};
                }

                bool operator==(const scheduler_& other) const
                {
                    return this->env_handle == other.env_handle;
                }

                run_loop* env_handle;
            };

            friend scheduler_ tag_invoke(get_scheduler_t, const run_loop& self) noexcept
            {
                return {const_cast<run_loop*>(&self)};
            }
        
            template<typename Tag>
            friend scheduler_ tag_invoke(exec::get_completion_scheduler_t<Tag>, const run_loop& self) noexcept
            {
                return {const_cast<run_loop*>(&self)};
            }

        protected:
            bool finished = false;
            std::queue<move_only_operation<Args...>, std::list<move_only_operation<Args...>>> operations_;
            mutable std::mutex mutex_{};
            mutable std::condition_variable cv_;
        };

        class thread_run_loop : public run_loop<>
        {
        public:
            thread_run_loop() = default;
            explicit thread_run_loop(uint32_t threadCount)
            {
                threads.resize(threadCount);
                for(uint32_t i = 0; i < threadCount; i++)
                {
                    threads[i] = std::jthread{[this]{
                        this->run();
                    }};
                }
            }
            ~thread_run_loop() noexcept
            {
                finish();
            }

        private:
            std::vector<std::jthread> threads;
        };

    }// namespace schedulers

    using schedulers::inline_scheduler;
    using schedulers::run_loop;
    using schedulers::thread_run_loop;

}// namespace vkr::exec

namespace vkr::envs
{
    template<typename Tag>
    exec::inline_scheduler tag_invoke(exec::get_completion_scheduler_t<Tag>, const empty_env&) noexcept
    {
        return {};
    }

    inline exec::inline_scheduler tag_invoke(exec::get_scheduler_t, const empty_env&) noexcept
    {
        return {};
    }
}// namespace vkr::envs