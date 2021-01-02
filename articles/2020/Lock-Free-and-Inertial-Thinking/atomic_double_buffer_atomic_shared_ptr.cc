#include <assert.h>

#include <atomic>
#include <future>
#include <memory>
#include <thread>

constexpr auto kInnerCount = 100;
constexpr auto kOuterCount = 100000;

template <typename T>
class DoubleBuffer {
 public:
  std::shared_ptr<T> Read() const {
    return std::atomic_load(&foreground_);  // get foreground, and ref_count++
  }

  template <typename... Args>
  void Modify(Args&&... args) {
    background_->Update(args...);         // update background
    background_ = std::atomic_exchange(&foreground_, background_);  // switch
    while (background_.use_count() != 1)  // check use_count()
      std::this_thread::yield();          // busy waiting...
    background_->Update(args...);         // update background
  }

 private:
  std::shared_ptr<T> foreground_ = std::make_shared<T>();  // atomic
  std::shared_ptr<T> background_ = std::make_shared<T>();  // non-atomic
};

struct Data {
  void Update(size_t loop_count) {
    is_updating = true;
    for (size_t i = 0; i < loop_count; ++i) {
      auto dummy = std::make_unique<int>(1);
    }
    is_updating = false;
  }
  bool is_updating = false;
};

int main() {
  DoubleBuffer<Data> double_buffer;
  std::promise<void> signal1, signal2, signal3;
  std::shared_future<void> event1(signal1.get_future());
  std::shared_future<void> event2(signal2.get_future());
  std::shared_future<void> event3(signal3.get_future());

  auto _1 = std::async(std::launch::async, [&] {
    signal1.set_value();
    event3.wait();
    for (size_t i = 0; i < kOuterCount; ++i) {
      double_buffer.Modify(kInnerCount);
    }
  });
  auto _2 = std::async(std::launch::async, [&] {
    signal2.set_value();
    event3.wait();
    for (size_t i = 0; i < kOuterCount * kInnerCount; ++i) {
      auto data = double_buffer.Read();
      assert(data);                // Good, not reached
      assert(!data->is_updating);  // Bad, read dirty data
    }
  });

  event1.wait();
  event2.wait();
  signal3.set_value();
  return 0;
}
