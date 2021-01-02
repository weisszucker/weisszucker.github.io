# 无锁双缓冲算法

> 2020/5/5
> 
> TODO

请先阅读上篇：[《无锁编程与惯性思维》](Lock-Free-and-Inertial-Thinking.md) 👈

本文主要针对上文提出的双缓冲算法，逐步改进、优化：

[TOC]

## 为什么要设计 无锁算法

先要纠正两个误区 🙃：

- **慢的不是 “锁”，而是 “竞争” 锁**
  - 一般锁的实现中，如果 **无竞争** _(uncontended)_，不会陷入内核态，所以 进出 **临界区** _(critical section)_ 的 **开销非常小**（但一些实现 调用后 立即进入内核态）
  - 参考：[Locks Aren't Slow; Lock Contention Is. _by Jeff Preshing_](https://preshing.com/20111118/locks-arent-slow-lock-contention-is/)
- 无锁 ≠ **不用锁 _(lock avoidance)_**

> - [**无等待** _(wait-free)_](https://en.wikipedia.org/wiki/Non-blocking_algorithm#Wait-freedom) 的程序 不管其它线程的 相对速度如何，都可以 在有限步之内结束（一定也是无锁的）
> - [**无锁** _(lock-free)_](https://en.wikipedia.org/wiki/Non-blocking_algorithm#Lock-freedom) 的程序 能确保执行它的所有线程中 至少有一个能够 继续往下执行
>   - 系统作为一个整体 总是在 [**前进** _(make progress)_](http://www.cs.tau.ac.il/~shanir/progress.pdf) 的
>   - 虽然有些线程可能会 被任意地延迟（例如 线程的调度 被其他线程抢占，导致线程挂起），但每一步 都至少有一个线程 能够往下执行
> - [**基于锁** _(lock-based)_](https://en.wikipedia.org/wiki/Lock_(computer_science)#Disadvantages) 的程序 一方面 无法保证 系统的前进（可能因为一个线程阻塞，而所有线程都阻塞），另一方面 还可能遇到：
>   - [**死锁** _(deadlock)_](https://en.wikipedia.org/wiki/Deadlock) 两个线程 互相等待 另一个所占有的互斥体
>   - [**活锁** _(livelock)_](https://en.wikipedia.org/wiki/Deadlock#Livelock) 两个家伙 在狭窄走廊里 面对面走，都试图 给对方让路，最终还是 走不过去
> - 参考：[Lock-Free Data Structures _by Andrei Alexandrescu_](https://www.drdobbs.com/lock-free-data-structures/184401865)（刘未鹏 译文：[锁无关的数据结构 —— 在避免死锁的同时确保线程继续](https://blog.csdn.net/pongba/article/details/588638)）

所以，上文提到的方案中：

- `Read()` 是无锁的
  - 不是无等待的，因为需要 **竞争修改** 同一个 原子引用计数
- `Modify()` 不是无锁的
  - 实质上类似于 [**自旋锁** _(spinlock)_](https://en.wikipedia.org/wiki/Spinlock)，通过 [**忙等待** _(busy waiting)_](https://en.wikipedia.org/wiki/Busy_waiting) **阻塞** 到没人使用为止
  - 假设 读取线程 在使用结束（引用计数 - 1）前 意外结束 或 死循环，那么 `Modify()` 将永远 **无法结束**

## 基于锁的引用计数 `std::atomic<std::shared_ptr>`

针对上文提到的方案，只要保证 “获取的 `index_` 不再变化” 和 “获取的引用计数 只减不增” 即可（[完整代码](Lock-Free-and-Inertial-Thinking/atomic_double_buffer_atomic_shared_ptr.cc)）：

``` cpp
std::shared_ptr<T> foreground_;  // atomic
std::shared_ptr<T> background_;  // non-atomic

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
```

通过 `std::atomic_load/atomic_exchange()`，保证 `foreground_` **读写的原子性**，即 “读取前台数据、并增加引用计数” 的操作不会被 “切换前后台” 的操作打断（[在线运行](https://wandbox.org/permlink/eQuBazNTrg0305JM)）：

| 读取线程 | 修改线程 |
|--------|--------|
| 获取 `foreground_` 为 `p0` <br/> 同时，引用计数 + 1 | |
| | 切换 `foreground_` 从 `p0` 改成 `p1` |
| 读取 `p0` 数据 | 切换 `background_` 从 `p1` 改成 `p0` |
| ... | 等待 `p0` 没人使用 |
| | 修改 `p0` 数据 |

**不足** —— 在主流的标准库实现中，C++ 11 [`std::atomic_...<std::shared_ptr>()`](https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic) 和 C++ 20 [`std::atomic<std::shared_ptr>`](https://en.cppreference.com/w/cpp/memory/shared_ptr/atomic2) 都是 **基于锁的**（注意 **原子操作 ≠ 无锁操作**）：

- [libstdc++](https://github.com/gcc-mirror/gcc/blob/5e7e8b98f49eda9ffb9817d97975a211c87c5a53/libstdc%2B%2B-v3/include/bits/shared_ptr_atomic.h#L105)/[libc++](https://github.com/llvm/llvm-project/blob/73812f3d0b418cafdde4fd5d8b88542655271918/libcxx/include/memory#L5107) 使用针对 `std::shared_ptr` 变量的地址（而不是变量的值）的锁 🔒
- [msvc](https://github.com/microsoft/STL/blob/343e62542701e7b14254afac8e8337fa2767b58e/stl/inc/memory#L2990) 不区分具体变量，直接使用全局的锁 🔒
- [`std::atomic_is_lock_free()`](https://en.cppreference.com/w/c/atomic/atomic_is_lock_free)/[`std::atomic<>::is_lock_free()`](https://en.cppreference.com/w/cpp/atomic/atomic/is_lock_free) 直接返回 `false`

## 无锁的引用计数 `folly::atomic_shared_ptr`

folly 提供了 x64/ppc64/aarch64 架构 **64 位系统** 的无锁版本的 `std::atomic<std::shared_ptr>`（[代码参考](https://github.com/facebook/folly/blob/master/folly/concurrency/AtomicSharedPtr.h)）：

- 假设：[内存地址 不使用 高 2 字节](http://en.wikipedia.org/wiki/X86-64#Canonical_form_addresses)，可以将 64 位指针的 **高 16 位用于 特殊用途**，而剩下的 **低 48 位存储 原始指针的值**
  - 使用 [`folly::PackedSyncPtr`](https://github.com/facebook/folly/blob/master/folly/PackedSyncPtr.h) 封装对 64 位指针的 高 16 位 和 低 48 位 的操作
  - 将高 16 位用于当前 `atomic_shared_ptr` 局部的 **引用计数**（独立于 共享控制块的 引用计数)，低 48 位指针指向所有 `atomic_shared_ptr`/`shared_ptr` 共享的 **控制块**
- 假设：系统 只需支持 **64 位无锁指令**，无需支持 128 位的指令（例如 `lock cmpxchg16b`）
  - 使用 [`folly::AtomicStruct`](https://github.com/facebook/folly/blob/master/folly/synchronization/AtomicStruct.h) 将 64 位的指针 强转为 `uint64_t` 值（内部调用 `memcpy()` 拷贝 64 位值），封装 [常用的原子操作](https://en.cppreference.com/w/cpp/atomic/atomic)
  - 通过 [**比较-交换** _(CAS, compare and swap)_](http://en.wikipedia.org/wiki/Compare-and-swap)（交换前 检查是否和上次读取到的结果一致，否则重新 读取、修改、尝试交换），同时读写 局部的引用计数 和 共享控制块的指针，保证 **读写的原子性**
- `folly::detail::shared_ptr_internals` 复用 libstdc++ 的实现，将 局部的引用计数 **同步到** 共享的控制块 中（[代码参考](https://github.com/facebook/folly/blob/master/folly/concurrency/detail/AtomicSharedPtr-detail.h)）

> libstdc++ `std::shared_ptr` **控制块** _(control block)_ 的原理（[代码参考](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/shared_ptr_base.h)）：
> 
> - `_Sp_counted_base` 控制块的 **基类**
>   - `_M_use_count` 强引用计数 控制 **共享对象** _(shared object)_ 析构（但不一定释放）
>   - `_M_weak_count` 弱引用计数 控制 **共享的控制块** _(shared control block)_ 析构（并释放）
>   - 参考：[Make_shared, almost a silver bullet _by Motti Lanzkron_](https://lanzkron.wordpress.com/2012/04/22/make_shared-almost-a-silver-bullet/)
> - `_Sp_counted_ptr : _Sp_counted_base` 普通的控制块
>   - `_M_ptr` **共享对象** 的指针
> - `_Sp_counted_deleter : _Sp_counted_base` 带 **分配器** _(allocator)_ / **删除器** _(deleter)_ 的控制块
>   - `_Del_base` **删除器** 对象
>   - `_Alloc_base` **分配器** 对象
>   - `_M_ptr` **共享对象** 的指针
> - `_Sp_counted_ptr_inplace : _Sp_counted_base` **内置** _(in-place)_ 共享对象的控制块
>   - `_A_base` **分配器** 对象
>   - `_M_storage` **共享对象**（而不是指针）
> - `__shared_count`/`__weak_count` 控制块的 **值语义** _(value semantic)_ 封装
>   - `_M_pi` 指向 **控制块对象** `_Sp_counted_base` 的指针（虚基类指针）
>   - 调用控制块接口，实现 **强弱引用计数 和 生命周期管理**
> - `__shared_ptr`/`__weak_ptr` 智能指针的 **内部实现**
>   - `_M_ptr` **共享对象** 的指针（用于 直接返回指针）
>   - `_M_refcount` 控制块的 `__shared_count`/`__weak_count` 值语义对象
> - `std::shared_ptr`/`std::weak_ptr` 直接继承了 内部实现（[代码参考](https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/include/bits/shared_ptr.h)）
> - 参考：[浅析 shared_ptr：Libstdc++ 篇 _by Kingsley Chen_](http://kingsamchen.github.io/2018/03/30/demystify-shared-ptr-and-weak-ptr-in-libstdcpp/)

使用方法和 `std::atomic<std::shared_ptr>` 类似：

``` cpp
folly::atomic_shared_ptr<T> foreground_;  // atomic
std::shared_ptr<T>          background_;  // non-atomic

std::shared_ptr<T> Read() const {
  return foreground_;  // get foreground, and ref_count++
}

template <typename... Args>
void Modify(Args&&... args) {
  background_->Update(args...);         // update background
  background_ = foreground_.exchange(background_);  // switch
  while (background_.use_count() != 1)  // check use_count()
    std::this_thread::yield();          // busy waiting...
  background_->Update(args...);         // update background
}
```

**优势** —— 相对于 “基于锁的” 方案：

- **读者 不受 写者 影响**，避免 “获取锁后 线程挂起，导致 其他线程 阻塞等待” 的问题
- **无需竞争** 同一个 **锁**，效率更高

**不足** ——

- 仍有 **读者之间的竞争**（修改 同一个 原子引用计数，尤其在 **多核、高并发** 环境下）
- 如果引用计数 超出最大值（小概率），读取函数 主动挂起线程，**忙等待** 直到 允许增加引用计数，所以 **不完全是无锁的**

## 基于线程局部锁的 `brpc::DoublyBufferedData<>`

brpc `DoublyBufferedData<>` 使用 [线程局部的 锁 替换 引用计数](https://github.com/apache/incubator-brpc/blob/master/docs/cn/lalb.md#doublybuffereddata)，避免读者冲突（[代码参考](https://github.com/apache/incubator-brpc/blob/master/src/butil/containers/doubly_buffered_data.h)）：

- 每个读取线程 包含一个 **线程局部锁** 🔒（封装为 `Wrapper`）
  - 每个线程 **首次读取数据前**（延迟创建），先创建 线程局部锁，并添加到 双缓冲对象的 `_wrappers` 列表中
  - 双缓冲对象销毁 或 线程退出时，从 `_wrappers` 列表中 移除
  - 双缓冲对象 使用一个 `_wrappers_mutex` 🔒 同步对 `_wrappers` 列表的操作（添加、移除、遍历）
- 读取线程 **使用数据前获取锁，使用结束后释放锁**（而不是 修改引用计数）
- 修改线程 在 “切换前后台” 后，**遍历、获取、立即释放** `_wrappers` 中 **每个锁**（而不是 检查引用计数），再 “修改 新后台（老前台）数据”
  - 获取锁时，持有锁的读取线程 可能正在读取数据，修改线程 需要等待 读取结束
  - 获取锁时，没有锁的读取线程 不可能在读取数据，修改线程 可以立即 获取到锁
  - 获取锁后，修改线程 立即释放，读取线程 再获取锁 只可能读到 切换后的新前台数据，即可保证 “没人使用 新后台（老前台）”
- 不需要区分 持有线程局部锁的线程 读取的是什么数据，只关心 是否有人正在读取数据

| 读取线程 | 修改线程 |
|--------|--------|
| 获取 🔒 👇↓ | 修改 `data_[1]` 数据 |
| 读取 `data_[0]` 数据 | 切换 `index_` 从 `0` 改成 `1` |
| 释放 🔒 👆↑ | 等待 🔒（开始遍历 `_wrappers`） |
| ... | 获取 🔒 👇↓ |
| 等待 🔒 💀💀💀 | 释放 🔒 👆↑ |
| 获取 🔒 👇↓ | ... |
| 读取 `data_[1]` 数据 | 修改 `data_[0]` 数据 |
| 释放 🔒 👆↑ | ... |

**优势** —— 相对于 “引用计数” 方案：

- **读者 不受 其他读者 影响**
- 通过 **锁同步** 读写操作，在 **读取时间较长** 的场景下，**避免** 修改线程 **忙等待**（参考：[No nuances, just buggy code (was: related to Spinlock implementation and the Linux Scheduler) _by Linus Torvalds_](https://www.realworldtech.com/forum/?threadid=189711&curpostid=189723)）

**不足** ——

- 仍有 **读写竞争**，不是无锁的（修改线程 持有锁时，读取线程 会阻塞）
  - 修改线程 临界区非常小，一般不太影响 读取线程
  - 如果换成 **自旋锁**，读取线程 **不会阻塞**，但会导致修改线程 **忙等待**（可用于 **读取时间较短** 的场景）
- 如果需要频繁 **创建新的线程**（而不使用线程池），则需要频繁 **创建、销毁** 线程局部锁，并频繁使用 `_wrappers_mutex` **同步** “添加、移除” 的操作，会导致严重的竞争

## 基于 Linux 调度机制的 `folly::rcu_reader`

[**读-拷贝修改** _(RCU, read-copy-update)_](https://en.wikipedia.org/wiki/Read-copy-update) 将 **更新** _(update)_ 分为 立即执行的 **移除** _(removal)_（红🟥）和 延迟执行的 **回收** _(reclamation)_（绿🟩）两个阶段，一般通过 QSBR _(quiescent state-based reclamation)_ 实现：

[align-center]

[![Grace Period](Lock-Free-and-Inertial-Thinking/GracePeriodGood.png)](https://lwn.net/Articles/262464/#Wait%20For%20Pre-Existing%20RCU%20Readers%20to%20Complete)

- 读者在 “读取数据时”，通过 **读侧临界区** _(read-side critical section)_（蓝🟦）保护
  - 不在 临界区 的读者 处于 **静默态** _(quiescent state)_
  - 处于 静默态 的读者 不允许访问 临界区保护的数据（使用者自己保证）
- 写者 先 “置换” 新数据（使读者可见），再 “移除” “待回收” 的老数据（红🟥）
- 写者在 “移除后、回收前”，**等待 所有线程 至少处于一次 静默态** —— 这个等待时间 称为 **宽限期** _(grace period)_（黄🟨）
  - 读者在 “宽限期开始时”，如果 不处于 静默态，可能还在使用 “待回收” 的老数据，写者 需要等待它们 **离开临界区**
  - 读者在 “宽限期开始时”，如果 已处于 静默态，已经不再使用 “待回收” 的老数据，写者 无需等待它们
  - 读者在 “宽限期开始后”，即使再进入临界区，也只能使用 新数据，而不可能再使用 “已被移除” 的 “待回收” 的老数据，写者 无需关心它们
- 写者在 “宽限期结束后”（绿🟩），执行 “回收” 操作
  - 在 “宽限期结束后”，没有 读者 在使用 “待回收” 的老数据，所以写者 可以放心 “回收”
- 相对于 [**读写锁** _(reader-writer lock)_](https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock) 的优势在于：[读者在 “宽限期内” 可以继续 读取数据，而不会 被写者阻塞](https://lwn.net/Articles/263130/#RCU%20is%20a%20Reader-Writer%20Lock%20Replacement)

使用 folly 提供的 RCU 接口，实现双缓冲算法（[接口代码参考](https://github.com/facebook/folly/blob/master/folly/synchronization/Rcu.h)）：

``` cpp
T                data_[2];  // non-atomic
std::atomic<int> index_;    // non-atomic

std::pair<T*, folly::rcu_reader> Read() const {
  return {&data_[index_], {}};  // get foreground with a guard
}

template <typename... Args>
void Modify(Args&&... args) {
  data_[!index_].Update(args...);  // update background
  index_ = !index_;                // switch buffer
  folly::synchronize_rcu();        // wait for grace period...
  data_[!index_].Update(args...);  // update background
}
```

- 读取数据时，使用 `folly::rcu_reader` 临界区保护
  - 对象构造时调用 `rcu_reader::lock()`
  - 对象析构时调用 `rcu_reader::unlock()`
  - 只要在 `rcu_reader` 对象生命周期内使用数据，即可保证在临界区内（蓝🟦）
- 修改数据时，也可以分成两个阶段
  - 移除阶段（红🟥）“切换前后台”
  - 回收阶段（绿🟩）“修改 新后台（老前台）数据”
  - 在两个阶段之间（黄🟨），调用 `folly::synchronize_rcu()` **进入宽限期**，并 **阻塞到 宽限期结束**
- 本质上，类似 brpc 线程局部锁 的方案 —— 只要 **等待 所有 正在读** 的线程读完，就可以 修改数据 了

**优势** ——

- 读取线程 **无等待**，没有资源浪费
- 相对于 “引用计数” 方案：**读者 不受 其他读者 影响**
- 相对于 “线程局部锁” 方案：**读者 不受 写者 影响**

**不足** ——

- 一般通过 Linux 内核的 调度机制 实现，无需 **原子原语** _(atomic primitive)_
- 要求 读取线程 不能阻塞调用，仅用于 **读取时间较短** 的场景

## 基于链表的延迟回收 `folly::hazptr_obj`

[Hazard Pointer](https://en.wikipedia.org/wiki/Hazard_pointer) 实现 “延迟回收” 策略，不限制 读取使用时间。

> - 每一读线程都 拥有一个 “单个写线程/多个读线程” 的共享指针，即 Hazard Pointer
> - 当一个读线程 将一个对象的地址 赋给它的 Hazard Pointer 时，即意味着 它在向其它（写）线程宣称：“我正在读该对象，如果你原意，你可以将它替换掉，但别改动它的内容，当然更不要去 delete 它。”
> - 写线程在 delete 任何被替换下来的旧对象之前，得检查 读线程的 Hazard Pointer（看看该旧对象 是否仍在被使用）
> - 如果某个旧对象 不与任何读线程的 Hazard Pointer 匹配，那么销毁该对象 就是安全的
> - 参考：[Lock-Free Data Structures _by Andrei Alexandrescu and Maged Michael_](https://www.drdobbs.com/lock-free-data-structures-with-hazard-po/184401890)（刘未鹏 译文：[锁无关的数据结构与 Hazard 指针 —— 操纵有限的资源](https://blog.csdn.net/pongba/article/details/589864)）

使用 folly 提供的 Hazard Pointer 接口，实现双缓冲算法（[接口代码参考](https://github.com/facebook/folly/blob/master/folly/synchronization/Hazptr.h)）：

TODO

## 基于阶段的延迟回收 EBR

TODO

- https://github.com/rmind/libqsbr
- https://stackoverflow.com/questions/36573370/quiescent-state-based-reclamation-vs-epoch-based-reclamation

## 写在最后 [no-toc]

各种方案 各有千秋，没有 好与不好，只有 在特定场景下 合不合适：

- TODO
- ref.perfbook

感谢 同事们的讨论和建议~

如果有什么问题，**欢迎交流**。😄

Delivered under MIT License &copy; 2020, BOT Man
