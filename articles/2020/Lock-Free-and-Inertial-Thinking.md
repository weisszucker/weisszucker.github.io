# 无锁编程与惯性思维

> 2020/4/19
> 
> 不识庐山真面目，只缘身在此山中。——《题西林壁》苏轼

## 写在前面 TL;DR [no-toc]

最近在给一段代码写单元测试，在并发量较大时，偶然发现了 [**数据竞争** _(data race)_](https://en.wikipedia.org/wiki/Race_condition#Data_race) 问题。本文主要记录 从 “百思不得其解” 到 “豁然开朗” 的思考过程。😂

先分享一个思考题：

> 一对父子遭遇车祸，**父亲重度昏迷**，儿子被送到医院。但外科医生说：**我不能给我的儿子做手术**，这会影响我的发挥。
> 
> 很多人会想：为什么 **已经昏迷的父亲** 会在医院会 **给儿子做手术** 呢？😱
> 
> <p><details>
> <summary> 👉 点我看答案 👈 </summary>
> 答案很简单 —— 在人们的刻板印象中，“外科医生” 是男的；而这个题目中，外科医生却是儿子的母亲。
> </details></p>

人们往往会被自己的 **惯性思维/思维定势** 所误导，从而陷入困境之中。🙃

[TOC]

## 问题背景

代码实现了类似 [brpc `DoublyBufferedData<>`](https://github.com/apache/incubator-brpc/blob/master/docs/cn/lalb.md#doublybuffereddata) 的 [**双缓冲** _(double buffer)_](https://en.wikipedia.org/wiki/Multiple_buffering#Double_buffering_in_computer_graphics) 功能：

- 支持 多线程读取 + 单线程修改 _(SWMR, single-writer multi-reader)_
  - 读取频繁、并发，但每次读取 时间非常短
  - 修改不频繁，不存在 并发修改
  - 数据分为 前台和后台，分别用于 读取和修改
- 不能使用 读写锁 🔒（因为 每次修改 耗时较长）
  - 如果使用 单缓冲 + 读写锁，会导致 修改时不能读取
  - 如果使用 双缓冲，可以随时 **读取前台数据，不用加锁**
  - 所以，读取 不用等待修改完成，修改 需要等待没人读取
- 前后台数据 保持一致 🐾（因为 每个缓冲 数据较大，不能丢弃）
  - 如果 只维护一个缓冲，每次修改后台时，都需要 完整拷贝前台数据
  - 如果 同时维护两个缓冲，就可以直接 **切换前后台** 来修改
  - 所以，支持 增量修改，无需 全量修改

核心逻辑非常简单（[完整代码](Lock-Free-and-Inertial-Thinking/atomic_double_buffer.cc)）：

``` cpp
std::shared_ptr<T> data_[2];
std::atomic<int>   index_;

std::shared_ptr<T> Read() const {
  return data_[index_];  // get foreground, and ref_count++
}

template <typename... Args>
void Modify(Args&&... args) {
  data_[!index_]->Update(args...);         // update background
  index_ = !index_;                        // switch buffer
  while (data_[!index_].use_count() != 1)  // check use_count()
    std::this_thread::yield();             // busy waiting...
  data_[!index_]->Update(args...);         // update background
}
```

- 定义两个 缓冲数据 `data_[2]`
  - 其中，`std::shared_ptr<T>` 内部使用 [**原子引用计数** _(atomic reference counting)_](https://en.cppreference.com/w/cpp/memory/shared_ptr/use_count)
- 定义一个 [原子变量](https://en.cppreference.com/w/cpp/atomic/atomic) `index_` 表示 前台数据 的下标
  - 其中，`!index_` 表示 后台数据 的下标
- `Read()` **读取前台数据**，不用加锁
  - 返回 前台数据 `data_[index_]`
  - 前台数据 引用计数 + 1（使用结束后，引用计数 - 1）
- `Modify()` **修改后台数据，切换前后台，再改一遍 新后台（老前台）数据**
  - 通过 `data_[!index_]->Update(args...)` 修改 后台数据
  - 通过 `index_ = !index_` 切换前后台，此后只可能读取到 新前台（老后台）数据
  - 通过 `data_[!index_].use_count()` 检查 后台数据的引用计数，判断是否有人使用
  - 如果有人使用，通过 [`std::this_thread::yield()`](https://en.cppreference.com/w/cpp/thread/yield) 主动挂起线程，避免直接 [**忙等待** _(busy waiting)_](https://en.wikipedia.org/wiki/Busy_waiting)
  - 等到没人使用，通过 `data_[!index_]->Update(args...)` 修改 新后台（老前台）数据

## 思考过程

这个设计看起来 “天衣无缝”，核心流程可以抽象为：

| 读取线程 | 修改线程 |
|--------|--------|
| 读取 `data_[0]` 数据 | 修改 `data_[1]` 数据 |
| ... | 切换 `index_` 从 `0` 改成 `1` |
| 读取 `data_[1]` 数据 | 等待 `data_[0]` 没人使用 |
| ... | 修改 `data_[0]` 数据 |

- 由于 只能读取 前台数据
  - 即使 `data_[!index_].use_count()` 返回值 不是最新的，也能保证 **只减不增**
  - 从而保证 **在有限时间内** 后台数据的引用计数 可以减到 `1`，整个系统会持续 “前进”，修改线程不会 [**饿死** _(starvation)_](https://en.wikipedia.org/wiki/Starvation_(computer_science))
  - 另外，也保证了下一次调用 `Modify()` 时，**最先修改的** 后台数据 **没人使用**
- 由于 `index_ = ...` 满足 [**序列一致顺序** _(sequentially-consistent ordering)_](https://en.cppreference.com/w/cpp/atomic/memory_order#Sequentially-consistent_ordering)
  - 保证 读写 `index_` 前后的代码 **不会乱序执行** 和 **所有修改 对读取线程可见**
  - 从而保证 `Read()` 获取的 `index_` 对应的数据 **不可能正在修改**
- 由于 只有单线程调用 `Modify()`
  - 即使 `index_ = !index_` 不是原子操作，也 **不会导致** [ABA 问题](https://en.wikipedia.org/wiki/ABA_problem)
  - 所以 `Modify()` **不用加锁**

然而，通过编写单元测试（[在线运行](https://wandbox.org/permlink/GgbexdCBN9TD1wDa)），发现 `Read()` **可能返回** `Modify()` **正在修改的数据**，从而导致数据竞争。🤨

所以，问题出在哪呢？（可以先想一想 🤔 **别急着往下看** 😜）

> ~~据说不到 10% 的人能在 5 分钟内想出答案。😶~~

![Fun](Lock-Free-and-Inertial-Thinking/fun.jpg)

## 思考结果

> 最不可能的人，往往就是凶手。

实际上，我们一直忽略了一个重要的细节 —— `Read()` 函数 先读取 `index_`，再 增加引用计数 —— **这两个操作并不是原子的**。🙄

| 读取线程 | 修改线程 |
|--------|--------|
| 获取 `index_` 为 `0` | |
| | 切换 `index_` 从 `0` 改成 `1` |
| | 判断 `data_[0]` 没人使用 |
| `data_[0]` 引用计数 + 1 | |
| 读取 `data_[0]` 数据 | 修改 `data_[0]` 数据 |

- 直观上，看似只有一个原子变量 `index_`
- 实际上，还有另一个被忽略的原子变量 —— 引用计数（**“看不见的大猩猩”** _(invisible gorilla)_ 🦍）
- 理论上，**两个原子变量 在两个线程里 分别读写**，难以保证 **一致性** _(consistency)_

总结起来就是：

- 由于 读取线程 无法保证 “获取的 `index_` 不再变化” —— “通过 `index_` 获取 `data_[index_]`” 的做法是错误的
- 导致 修改线程 无法保证 “获取的引用计数 只减不增” —— “通过 引用计数 判断是否有人使用” 的做法也是错误的

## 写在最后 [no-toc]

误导你的，往往不是别人，而是 “经验”。

另外，证明 想法的错误 很简单，举一个反例即可；但要证明 想法的正确 很复杂，需要严谨的推导。（有时间继续学习 “如何证明无锁算法的正确性” 😐）

欲知如何解决，请看下篇：[《无锁双缓冲算法》](Lock-Free-Double-Buffer.md) 👈

感谢 [@Mengw](https://github.com/Mengw) 提供的想法 和 [@flythief](https://github.com/thiefuniverse) 提出的修改意见~

如果有什么问题，**欢迎交流**。😄

Delivered under MIT License &copy; 2020, BOT Man
