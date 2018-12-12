#pragma once

namespace fn
{
    template <typename T>
    struct view;

    template <typename Return, typename... Args>
    struct view<Return(Args...)> final
    {
        template <typename T, typename = std::enable_if_t<
                                  std::is_invocable_r_v<Return, T&, Args...> // only callable types
                                  && !std::is_same_v<std::decay_t<T>, view>  // disable copy constructor
                                  >>
        view(T&& x)
            : data_{(void*) std::addressof(x)}
            , func_{[](void* data, Args... args) -> Return
            {
                return (*reinterpret_cast<std::add_pointer_t<T>>(data))(std::forward<Args>(args)...);
            }}
        {
        }

        auto operator()(Args... args) const
        {
            return func_(data_, std::forward<Args>(args)...);
        }

      private:
        void* data_;
        Return (*func_)(void*, Args...);
    };
} // namespace fn
