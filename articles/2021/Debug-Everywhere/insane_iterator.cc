// https://bot-man-jl.github.io/articles/?post=2021/Debug-Everywhere

// Crash as expected on macOS and Linux :-(
// clang++ -std=c++17 -g insane_iterator.cc && lldb ./a.out

// Not crash surprisingly on macOS and Windows :-)
// clang++ -std=c++17 -g -fsanitize=address insane_iterator.cc && ./a.out

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <stack>
#include <string>

// base::BindStateBase
// https://github.com/chromium/chromium/blob/master/base/callback_internal.h
// https://github.com/chromium/chromium/blob/master/base/callback_internal.cc

struct BindStateBase {
  using InvokeFuncStorage = void (*)();

  BindStateBase(InvokeFuncStorage polymorphic_invoke,
                void (*destructor)(const BindStateBase*))
      : polymorphic_invoke_(polymorphic_invoke), destructor_(destructor) {}
  ~BindStateBase() { destructor_(this); }

  BindStateBase(const BindStateBase&) = delete;
  BindStateBase& operator=(const BindStateBase&) = delete;

#if defined(_WIN32) || defined(__APPLE__)
  char padding_[8] = {0};  // Note: chromium use this field for ref-counting.
#elif defined(__linux__)
  // No need to set padding.
#endif
  InvokeFuncStorage polymorphic_invoke_;
  void (*destructor_)(const BindStateBase*);
};

// base::BindState
// https://github.com/chromium/chromium/blob/master/base/bind_internal.h

template <typename Functor, typename... BoundArgs>
struct BindState final : BindStateBase {
  template <typename ForwardFunctor, typename... ForwardBoundArgs>
  BindState(BindStateBase::InvokeFuncStorage invoke_func,
            ForwardFunctor&& functor,
            ForwardBoundArgs&&... bound_args)
      : BindStateBase(invoke_func, &Destroy),
        functor_(std::forward<ForwardFunctor>(functor)),
        bound_args_(std::forward<ForwardBoundArgs>(bound_args)...) {}
  ~BindState() = default;

  // Note: call members' destructors rather than delete |self|.
  static void Destroy(const BindStateBase* self) {
    using Tuple = std::tuple<BoundArgs...>;
    static_cast<const BindState*>(self)->functor_.~Functor();
    static_cast<const BindState*>(self)->bound_args_.~Tuple();
  }

  Functor functor_;
  std::tuple<BoundArgs...> bound_args_;
};

// base::CallbackBase
// https://github.com/chromium/chromium/blob/master/base/callback_internal.h
// https://github.com/chromium/chromium/blob/master/base/callback_internal.cc

class CallbackBase {
 public:
  explicit operator bool() const { return !bind_state_; }

 protected:
  constexpr CallbackBase() = default;
  explicit CallbackBase(BindStateBase* bind_state) : bind_state_{bind_state} {}

  // Note: use std::shared_ptr<> rather than chromium's scoped_refptr<>.
  std::shared_ptr<BindStateBase> bind_state_;
};

// base::OnceCallback
// https://github.com/chromium/chromium/blob/master/base/callback.h

template <typename Signature>
class OnceCallback;

template <typename R, typename... Args>
class OnceCallback<R(Args...)> : public CallbackBase {
 public:
  using PolymorphicInvoke = R (*)(BindStateBase*, Args...);

  constexpr OnceCallback() = default;
  explicit OnceCallback(BindStateBase* bind_state) : CallbackBase(bind_state) {}

  OnceCallback(const OnceCallback&) = delete;
  OnceCallback& operator=(const OnceCallback&) = delete;

  OnceCallback(OnceCallback&&) noexcept = default;
  OnceCallback& operator=(OnceCallback&&) noexcept = default;

  R Run(Args... args) const& {
    static_assert(!sizeof(*this),
                  "OnceCallback::Run() may only be invoked on a non-const "
                  "rvalue, i.e. std::move(callback).Run().");
  }

  R Run(Args... args) && {
    OnceCallback cb = std::move(*this);
    PolymorphicInvoke f = reinterpret_cast<PolymorphicInvoke>(
        cb.bind_state_->polymorphic_invoke_);
    return f(cb.bind_state_.get(), std::forward<Args>(args)...);
  }
};

// base::BindTypeHelper
// https://github.com/chromium/chromium/blob/master/base/bind_internal.h

template <typename... Types>
struct TypeList {};

template <size_t n, typename List>
struct DropTypeListItemImpl;

template <size_t n, typename T, typename... List>
struct DropTypeListItemImpl<n, TypeList<T, List...>>
    : DropTypeListItemImpl<n - 1, TypeList<List...>> {};

template <typename... List>
struct DropTypeListItemImpl<0, TypeList<List...>> {
  using Type = TypeList<List...>;
};

template <typename Signature>
struct ExtractArgsImpl;

template <typename R, typename... Args>
struct ExtractArgsImpl<R(Args...)> {
  using ReturnType = R;
  using ArgsList = TypeList<Args...>;
};

template <typename R, typename ArgList>
struct MakeFunctionTypeImpl;

template <typename R, typename... Args>
struct MakeFunctionTypeImpl<R, TypeList<Args...>> {
  typedef R Type(Args...);
};

template <typename Functor>
struct FunctorTraits;

template <typename R, typename... Args>
struct FunctorTraits<R (*)(Args...)> {
  using RunType = R(Args...);

  template <typename Function, typename... RunArgs>
  static R Invoke(Function&& function, RunArgs&&... args) {
    return std::forward<Function>(function)(std::forward<RunArgs>(args)...);
  }
};

template <typename Functor, typename... BoundArgs>
struct BindTypeHelper {
  using RunType = typename FunctorTraits<std::decay_t<Functor>>::RunType;
  using UnboundRunType = typename MakeFunctionTypeImpl<
      typename ExtractArgsImpl<RunType>::ReturnType,
      typename DropTypeListItemImpl<
          sizeof...(BoundArgs),
          typename ExtractArgsImpl<RunType>::ArgsList>::Type>::Type;
};

// base::Invoker
// https://github.com/chromium/chromium/blob/master/base/bind_internal.h

template <typename StorageType, typename UnboundRunType>
struct Invoker;

template <typename StorageType, typename R, typename... UnboundArgs>
struct Invoker<StorageType, R(UnboundArgs...)> {
  static R RunOnce(BindStateBase* base, UnboundArgs... unbound_args) {
    StorageType* storage = static_cast<StorageType*>(base);
    constexpr size_t num_bound_args =
        std::tuple_size<decltype(storage->bound_args_)>::value;
    return RunImpl(std::move(storage->functor_),
                   std::move(storage->bound_args_),
                   std::make_index_sequence<num_bound_args>(),
                   std::forward<UnboundArgs>(unbound_args)...);
  }

 private:
  template <typename Functor, typename BoundArgsTuple, size_t... Indices>
  static inline R RunImpl(Functor&& functor,
                          BoundArgsTuple&& bound_args,
                          std::index_sequence<Indices...>,
                          UnboundArgs&&... unbound_args) {
    return FunctorTraits<std::decay_t<Functor>>::Invoke(
        std::forward<Functor>(functor),
        std::get<Indices>(std::forward<BoundArgsTuple>(bound_args))...,
        std::forward<UnboundArgs>(unbound_args)...);
  }
};

// base::BindOnce
// https://github.com/chromium/chromium/blob/master/base/bind.h

template <typename Functor, typename... Args>
decltype(auto) BindOnce(Functor&& functor, Args&&... args) {
  using BindState = BindState<std::decay_t<Functor>, std::decay_t<Args>...>;
  using UnboundRunType =
      typename BindTypeHelper<Functor, Args...>::UnboundRunType;
  typename OnceCallback<UnboundRunType>::PolymorphicInvoke invoke_func =
      Invoker<BindState, UnboundRunType>::RunOnce;

  return OnceCallback<UnboundRunType>(new BindState(
      reinterpret_cast<BindStateBase::InvokeFuncStorage>(invoke_func),
      std::forward<Functor>(functor), std::forward<Args>(args)...));
}

// base::AtExitManager
// https://github.com/chromium/chromium/blob/master/base/at_exit.h
// https://github.com/chromium/chromium/blob/master/base/at_exit.cc

class AtExitManager {
 public:
  AtExitManager() { global_manager_ = this; }

  ~AtExitManager() {
    while (!tasks_.empty()) {
      std::move(tasks_.top()).Run();
      tasks_.pop();
    }
  }

  using Task = OnceCallback<void()>;

  static void RegisterTask(Task task) {
    global_manager_->tasks_.push(std::move(task));
  }

 private:
  std::stack<Task> tasks_;

  inline static AtExitManager* global_manager_ = nullptr;
};

// base::Singleton
// https://github.com/chromium/chromium/blob/master/base/memory/singleton.h
// https://github.com/chromium/chromium/blob/master/base/lazy_instance_helpers.h
// https://github.com/chromium/chromium/blob/master/base/lazy_instance_helpers.cc

template <typename T>
class Singleton {
 public:
  static T* get() {
    if (!instance_) {
      instance_ = new T;
      AtExitManager::RegisterTask(BindOnce(&Singleton::OnExit));
    }
    return instance_;
  }

 private:
  static void OnExit() { delete instance_; }

  inline static T* instance_ = nullptr;
};

// My Code: TimestampManager

class TimestampManager {
 public:
  static TimestampManager* get() { return Singleton<TimestampManager>::get(); }
  using Timestamp = std::chrono::milliseconds::rep;

  void UpdateTimestamp(const std::string& key) {
    Timestamp now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    auto iter = timestamps_.find(key);
    if (iter == timestamps_.end()) {
      timestamps_.emplace(key, now);
      // Note: forgot to return... carelessly!
      // return;
    }
    iter->second = now;
  }

 private:
  TimestampManager() = default;
  ~TimestampManager() = default;

  std::map<std::string, Timestamp> timestamps_;
#if defined(_WIN32)
  char padding_[24] = {0};
#elif defined(__APPLE__)
  char padding_[16] = {0};
#elif defined(__linux__)
  // No need to set padding.
#endif
  friend class Singleton<TimestampManager>;
};

static_assert(sizeof(void*) == 8, "Available on 64-bit systems only");

#if defined(_WIN32) || defined(__APPLE__)
static_assert(sizeof(BindState<void (*)()>) == 40);
static_assert(sizeof(TimestampManager) == 40);
#elif defined(__linux__)
static_assert(sizeof(BindState<void (*)()>) == 32);
static_assert(sizeof(TimestampManager) == 48);
#endif

int main() {
  AtExitManager at_exit;
  TimestampManager::get()->UpdateTimestamp("???");
  return 0;
  // Note: invoke ~AtExitManager() here, and then crash!
}
