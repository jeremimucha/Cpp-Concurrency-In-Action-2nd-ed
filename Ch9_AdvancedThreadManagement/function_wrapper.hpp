#pragma once

#include <memory>
#include <utility>

class function_wrapper {
    struct impl_base {
        constexpr impl_base() noexcept = default;
        impl_base(impl_base const&) = delete;
        impl_base(impl_base&&) = delete;
        impl_base& operator=(impl_base const&) = delete;
        impl_base& operator=(impl_base&&) = delete;
        virtual void invoke() = 0;
        virtual ~impl_base() noexcept = default;
    };

    template <typename Function>
    struct impl_type : impl_base {
        Function function_;
        constexpr impl_type(Function&& f) : function_{std::move(f)} {}
        void invoke() override { function_(); }
    };

    std::unique_ptr<impl_base> impl_{};

public:
    function_wrapper() noexcept = default;

    template <typename F, typename = std::enable_if_t<!std::is_same_v<F, function_wrapper>>>
    function_wrapper(F&& f)
        : impl_{std::make_unique<impl_type<std::decay_t<F>>>(std::forward<F>(f))}
    {
    }

    function_wrapper(function_wrapper&& other) noexcept : impl_{std::move(other.impl_)} {}
    function_wrapper& operator=(function_wrapper&& other) noexcept
    {
        impl_ = std::move(other.impl_);
        return *this;
    }

    function_wrapper(function_wrapper const&) = delete;
    function_wrapper& operator=(function_wrapper const&) = delete;

    void operator()() { impl_->invoke(); }
};
