# 从时空维度看 I/O 模型

> 2019/5/6 -> 2023/10/21
> 
> 从时间/空间的维度，阐述 **同步异步**、**阻塞非阻塞** I/O 模型的异同

在学习了 [V8 JavaScript 引擎](https://v8.dev/) 后，我发现：JavaScript 代码不知道外部 C++ 代码的存在，而 C++ 代码可以灵活的控制多个 JavaScript 代码环境。

> “地球是圆的，还是平的？” ——
> 
> 看问题的维度（不仅是角度）不同，对世界的认识也不一样。

本文用简单的例子阐述 **同步异步**、**阻塞非阻塞** 的 **不同世界观**。

## 同步、阻塞

例如，调用 UNIX 系统的 `send()` 通过 **普通的** `fd` 发送数据：

``` cpp
ssize_t ret = send(fd, buffer, len, flags);
```

- 当前线程的函数调用 **阻塞到 I/O 完成时**

## 同步、非阻塞

例如，调用 UNIX 系统的 `send()` 通过 **非阻塞的** `fd` 发送数据：

``` cpp
evutil_make_socket_nonblocking(fd);

while (len) {
  ssize_t ret = send(fd, buffer, len, flags);
  // case1: ready to send
  if (ret >= 0) {
    len -= ret;
    continue;
  }
  // case2: not ready
  if (EVUTIL_SOCKET_ERROR() == EAGAIN)
    continue;
  // case3: socket error
}
```

- 使用 [libevent](http://libevent.org/) 提供的 [`evutil_make_socket_nonblocking()`](https://github.com/libevent/libevent/blob/master/include/event2/util.h) 将 fd 设置为非阻塞
- 函数调用 **立即返回**：
  - 如果可以发送数据，则立即发送数据
  - 如果暂时无法发送数据，[`EVUTIL_SOCKET_ERROR()`](https://github.com/libevent/libevent/blob/master/include/event2/util.h) 返回 `EAGAIN` / `EWOULDBLOCK`
  - 否则，socket 错误（比如断开、异常）

## 异步、非阻塞

例如，Node.js 通过 [`fs.readFile()`](https://nodejs.org/api/fs.html#fsreadfilepath-options-callback) 读取文件：

``` javascript
fs.readFile(filename, (err, data) => {
  console.log('2. finish file I/O, and handle result');
  if (err) {
    // handle |err|
  }
  // use |data|
});
console.log('1. started file I/O, and continue');
```

- 需要系统/语言支持，一般提供基于 [**回调** _(callback)_](../2017/Callback-Explained.md) 的接口：
  - 函数 `fs.readFile()` **发起 I/O 请求**，然后 **立即返回**
  - 在 “发起 I/O 请求” 到 “I/O 完成” 之间，当前线程会 **往下执行**
  - **I/O 完成时**，通过 **回调** `(err, data) => { ... }` 传入数据 `data`（如果成功）或错误 `err`（如果失败）
- 如果系统/语言不支持，则可以在 **用户态** 通过 [**I/O 多路复用** _(I/O multiplexing)_](https://en.wikipedia.org/wiki/Multiplexing) 模拟 “异步”：
  - 例如 libevent 在底层封装 [`epoll()`](https://linux.die.net/man/4/epoll) 的轮询调用，向应用层提供了基于回调的接口
  - 虽然 libevent 的底层实现是 **同步** 的（单线程同步处理 I/O + 同步调用上层回调函数），但应用层能以 **异步** 的方式组织代码（通过回调的形式处理 I/O 结果）
- 执行回调的 **线程** 在不同环境下不一样：
  - Unix 的 [`aio_read()`](https://linux.die.net/man/3/aio_read) 在 **系统** 创建的 **新线程** 上回调
  - Windows 的 [`ReadFileEx()`](https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfileex) 由 **系统** 在 **调用函数的线程** 上回调（参考 [asynchronous procedure call _(APC)_](https://learn.microsoft.com/en-us/windows/win32/sync/asynchronous-procedure-calls)）
  - Node.js 的 `fs.readFile()` 由 **JavaScript 引擎** 在 **主线程**（也是调用函数的线程）上回调
  - 用户态 的 I/O 多路复用在 **执行分派的线程** 上回调（例如 libevent `event_base_dispatch()` 负责调用回调）
- 从程序设计语言上看，通过 [CPS _(continuation-passing style)_](https://en.wikipedia.org/wiki/Continuation-passing_style) 将 “I/O 结果的处理逻辑” 作为 continuation 传递：
  - 如果需要连续进行多个 **相互依赖** 的 I/O，回调函数多层嵌套 会出现 [回调地狱 _(callback hell)_](http://callbackhell.com/)
  - 但可以通过 **链式传递** continuation 化简（参考：[Chained Promises (JavaScript)](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise#Chained_Promises)）

## 异步、阻塞

例如，Node.js 通过 [`fs.promises.readFile()`](https://nodejs.org/api/fs.html#fspromisesreadfilepath-options) 读取文件：

``` javascript
try {
  const data = await fs.promises.readFile(filename);
  // use |data|
} catch (err) {
  // handle |err|
}
```

- 需要系统/语言支持，一般采用基于 [**协程** _(coroutine)_](https://en.wikipedia.org/wiki/Coroutine) `async/await` 语法：
  - 函数 `fs.promises.readFile` **发起 I/O 请求**，然后 **阻塞到 I/O 完成时**
  - 在 “发起 I/O 请求” 到 “I/O 完成” 之间，当前线程会 **切换执行其他代码**
  - **I/O 完成时**，当前线程 **切换回** `await` 的下一行，并返回数据 `data`（如果成功）或抛出异常 `err`（如果失败）
- 如果系统/语言不支持，则无法实现：
  - 例如 UNIX 系统/C 语言 本身不支持协程（参考：[Asynchronous I/O Forms](https://en.wikipedia.org/wiki/Asynchronous_I/O#Forms)）
- 协程属于 **非抢占式/协作式 多任务模型** _(nonpreemptive/cooperative multitasking)_，相对于 **多线程 + 同步、阻塞** 调度模型的优势在于：
  - 更简单 —— 没有多线程的 **数据竞争** 问题，不需要考虑 线程同步问题
  - 开销小 —— 无需 **系统调用**，自己管理调用栈内存，没有 线程数量限制
  - 更高效 —— 有更多机会被执行（不管怎么切换，代码都运行在 **同一个线程**）

## 世界观

**阻塞/非阻塞** 像是 **空间** 维度的对比：

|   | 阻塞模型 | 非阻塞模型 |
|---|---|---|
| I/O 请求调用 **何时返回** | I/O 完成时返回 | 立即返回 |
| **何处处理** I/O 结果 | 调用返回后 | 轮询结束后 或 回调函数里 |
| 代码（空间）连续性 | 连续 | 非连续 |
| 代码可读性 | 逻辑连贯 | 逻辑分散 |

**同步/异步** 像是 **时间** 维度的对比：

|   | 同步模型 | 异步模型 |
|---|---|---|
| 在执行 **I/O 期间** | 只等待 I/O 完成 | 会执行其他代码 |
| 执行（时间）连续性 | 连续 | 非连续 |
| 代码执行效率 | 线程利用率低 | 线程利用率高 |

## 模型对比

- 对于 **同步、阻塞模型**，常用 **多进程/多线程** 提高 I/O 吞吐量（多个进程/线程 同时发起 I/O，分别等待 各自 I/O 结果）
- 对于 **同步、非阻塞模型**，常用 **I/O 多路复用** 提高 I/O 吞吐量（一个线程 同时发起 多个 I/O，同时轮询 所有 I/O 结果）
- 对于 **异步模型**，由于 回调/协程 **调度顺序不确定**，需要在 I/O 完成后检查 **上下文** _(context)_ 的 **有效性**（参考：[深入 C++ 回调](Inside-Cpp-Callback.md#回调是同步还是异步的)）
- [proactor 模式](https://en.wikipedia.org/wiki/Proactor_pattern) 被认为是 [reactor 模式](https://en.wikipedia.org/wiki/Reactor_pattern)（I/O 多路复用，**同步模型**）的 **异步模型** 变体（TBD：不要太纠结）
- [future-promise 模型](https://en.wikipedia.org/wiki/Futures_and_promises) 可以认为是 **非阻塞（不阻塞发起 I/O 请求）+ 阻塞（阻塞等待 I/O 完成）** 的模型（同步或异步 取决于 最后阻塞等待的实现方式）
- 推荐阅读：[Javascript 异步调用模型](2023/JS-Async-Model.md)（如果读懂了这篇文章，就能深入理解本文阐述的内容了）

## 写在最后

随着编程语言的发展，I/O 模型不断优化：

- 效率优化 —— 从 同步 到 异步
- 可读性优化 —— 从 非阻塞 到 阻塞

感谢 [@flythief](https://github.com/thiefuniverse) / [@WalkerJG](https://github.com/WalkerJG) 的 review~

如果有什么问题，**欢迎交流**。😄

Delivered under MIT License &copy; 2019, BOT Man
