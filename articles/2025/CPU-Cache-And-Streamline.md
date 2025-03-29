# CPU Cache 和流水线对代码执行的影响

> 都是细节

## 1. CPU Cache 的影响

CPU Cache 是位于 CPU 和主存（RAM）之间的高速存储器，用于减少 CPU 访问内存的延迟。Cache 通常分为三级：

- **L1 Cache**：最快，最小，通常 32KB~64KB
- **L2 Cache**：较大，通常 256KB~1MB
- **L3 Cache**：共享，通常 2MB~32MB

### Cache 如何影响代码执行？

#### (1) 缓存命中（Cache Hit）与缓存未命中（Cache Miss）
- **缓存命中**：CPU 需要的数据在 Cache 中找到，访问时间极短（1~5 个时钟周期）。
- **缓存未命中**：数据不在 Cache 中，需从主存加载（100~300 个时钟周期），性能下降。

#### (2) 局部性原理（Locality）
- **时间局部性（Temporal Locality）**：如果某个数据被访问，它很可能在短期内再次被访问（例如循环变量）。
- **空间局部性（Spatial Locality）**：如果某个数据被访问，其附近的数据也可能被访问（例如数组遍历）。

##### 优化方法：

提升数据空间局部性：
- 非随机访问情况，使用缓存友好型数据结构，如vector, array。反例：hash map, linked list
- 优化相关数据在内存中的排布，如
    - 同时访问的数据定义在内存中相邻位置
    - 值代替指针
    - 使用尽可能小的数据类型

时间局部性：
- 对相同数据的操作放在相邻代码中

参考：[知乎-缓存友好程序设计指南](https://zhuanlan.zhihu.com/p/484951383)

#### (3) Cache Line 的影响
Cache Line 是 Cache 的最小存储单元（通常 64 字节）。

##### 访问模式影响：
- **顺序访问（Sequential Access）**：高效，Cache Prefetching（预取）可提前加载数据。
- **随机访问（Random Access）**：Cache Miss 率高，性能下降。

---

## 2. 流水线（Pipeline）的影响
流水线是一种并行执行指令的技术，将指令分解为多个阶段（Fetch, Decode, Execute, Memory Access, Write Back），使多条指令可以重叠执行。

### 流水线如何影响代码执行？
#### (1) 流水线停顿（Pipeline Stall）
- **数据依赖（Data Hazard）**：当前指令依赖前一条指令的结果，导致流水线必须等待。

    ```cpp
    a = b + c;  // 指令 1
    d = a + e;  // 指令 2 依赖指令 1 的结果，流水线必须等待
    ```

- **控制依赖（Control Hazard）**：分支指令（if/for/while）导致流水线需要清空（Branch Misprediction）。

    ```cpp
    for(i=0; i<N; ++i>) {
        if (array[i] > 128) {  // 分支预测错误会导致流水线清空
            // 分支 1
        } else {
            // 分支 2
        }
    }
    ```

`sorted(array)`会带来更好的性能，因为提升了正确分支预测的概率。

#### (2) 分支预测（Branch Prediction）
- **静态预测**：编译器优化（如 likely/unlikely 宏）。

    ```cpp
    if (__builtin_expect(condition, 1)) {  // 提示编译器 condition 很可能为真
        // 优化代码路径
    }
    ```

- **动态预测**：CPU 硬件记录分支历史，提高预测准确率。

    ```cpp
    if (condition) {
        // 分支 1
    } else {
        // 分支 2
    }
    ```

##### 优化方法：
- **减少分支**：使用查表法（Lookup Table）代替 if-else。

- **分支预测提示（GCC/Clang）**：

    ```cpp
    if (__builtin_expect(condition, 1)) {  // 提示编译器 condition 很可能为真
        // 优化代码路径
    }
    ```

#### (3) 指令级并行（ILP）
现代 CPU 支持超标量（Superscalar）架构，可同时执行多条指令（如 Intel 的 4-way 超标量）。

- **依赖链（Dependency Chain）**：

    ```cpp
    a = b + c;  // 指令 1
    d = a + e;  // 指令 2 依赖指令 1
    f = g + h;  // 指令 3 可并行执行
    ```

##### 优化方法：

- **循环展开（Loop Unrolling）**：减少分支，提高 ILP。
- **SIMD（Single Instruction Multiple Data）**：如 AVX/SSE 指令集，并行处理数据。

## 4. 总结

| 优化方向 | Cache 优化 | 流水线优化 |
|---------|------------|------------|
| 关键问题 | 缓存未命中（Cache Miss） | 流水线停顿（Pipeline Stall） |
| 优化方法 | - 提高局部性<br>- 减少随机访问 | - 减少分支<br>- 提高指令级并行 |
| 典型技术 | - 数据对齐<br>- 循环优化 | - SIMD<br>- 循环展开 |
| 适用场景 | 内存密集型计算（如矩阵运算） | 计算密集型任务（如数值计算） |

实际建议：

- Cache 优化：尽量顺序访问数据，减少随机访问。
- 流水线优化：减少分支，使用 SIMD 指令集。
- 工具辅助：使用 perf、VTune 分析 Cache Miss 和分支预测失败率。

通过合理优化 Cache 和流水线，代码的执行效率可提升数倍甚至数十倍。