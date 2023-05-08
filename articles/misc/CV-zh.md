[not-print]

[float-right]

[EN](CV-en.md) | 中文

[print-only]

<p id="cvQrCodeSec" style="position: fixed; width: 18%; height: 18%; top: 15px; right: 30px;"></p>

## 工作

### 2020.11 - 至今 | 字节跳动

- 参与飞书桌面端（Windows/macOS/Linux，基于 [Chromium](https://www.chromium.org) + Web 的 Hybrid 架构）功能开发、Code Review、品质保障，培养 6 名校招/实习生
- PC 性能&品质 负责人：
  - **整体关注** 稳定性（崩溃+OOM+卡顿+卡死+兼容性）/资源（CPU+内存+磁盘/包体积+IO）/成本（流量+存储）/体验（耗时）等方向在 开发/测试/灰度/线上（基建+指标+告警+工具）等阶段的品质保障工作（研发参与人数 10+）
  - 重点降低 Windows **OOM 率** 30%，macOS **卡顿率** 70%，**主进程内存** pct99 20%
- 视频会议 Native 改造项目 框架负责人：
  - 从 Web 架构改造为基于 Chromium 的 Native 架构，提升启动速度、降低CPU&内存占用及耗电量、减小安装包体积、支持更复杂的原生软件场景（研发参与人数 60+，历时 7 个月）
  - 提供 Chromium 技术支持（进程架构、基础能力），构建 UI 基础组件库，解决阻塞性难题（系统兼容、Chromium 底层）
  - 保障项目品质（崩溃/ASAN、卡顿、内存），优化工程效率（仓库解耦、[编译优化](../2022/Cpp-Project-Compile-Optimization.md)）

### 2018.7 - 2020.11 | 腾讯

- QQ 浏览器桌面端（基于 C++/[Chromium](https://www.chromium.org)）：
  - 设计 **基于 XAML 的导航页原生渲染方案**，50% 用户首屏加载提速 5s 以上，增加 15% 收入
  - 降低 **崩溃率** 40%~50%，借助 WinDbg 分析线上 dump，加入 [静态/动态检查](../2019/Cpp-Check.md)，减少安全隐患
  - 封装 JavaScript 通用扩展接口，降低运营活动开发成本，提升可扩展性
- 智能平台服务端（基于 C++/Python/[Tars](https://github.com/TarsCloud/TarsCpp)）：
  - 开发教育 AI 服务，从零到一开发自适应推荐、用户画像等功能，建设 C++/Python 基础库，接入全链路跟踪、远程日志、指标监控，基于 gperftools 优化性能瓶颈
  - 担任公司 C++ 代码规范小组核心成员，建设部门单元测试、代码静态扫描、持续集成

[page-break]

## 教育

#### 2014.9 - 2018.6 | 北京邮电大学

- **计算机科学与技术**（排名 10%，GPA 8.6/10.0）
- 课外：参加 3 场黑客马拉松、微软学生夏令营、大学生创新创业实践（均开源在 [Github](https://github.com/BOT-Man-JL) 上）

#### 2016.3 - 2017.4 | （北邮）叶培大学院项目

- 北京邮电大学 网络技术研究院 网络与交换技术 **国家重点实验室**（方向：**虚拟网络映射算法**）
- 发表 **SCI** 论文：[_Clustering Based Energy-Aware Virtual Network Embedding_](http://journals.sagepub.com/doi/full/10.1177/1550147717726714)

## 技能

- **技术博客**：https://bot-man-jl.github.io/articles
- 熟练 ⭐⭐⭐
  - **现代** C++ 11/14/17
  - Chromium 跨平台（C++）
  - Windows 桌面软件（C++）
- 熟悉 ⭐⭐
  - Web 前端小程序（JavaScript）
  - Linux 后台服务（C++/Python）
  - Node.js 后台服务（JavaScript）
  - CLI 脚本（JavaScript/Python/Bash/BAT）
- 掌握 ⭐
  - Windows WFP 网络驱动（C）
  - Windows UWP 通用应用（C#）
  - ASP.NET (core) 后台服务（C#）

## 开源

- 2019 - 参与 [Tencent/Hippy](https://github.com/Tencent/Hippy) 移动跨平台框架开发，负责早期的线程模型/日志模块重构、优化 JNI 调试
- 2018 - 设计 **反流量嗅探** 的 [Windows 流量重定向驱动](https://github.com/BOT-Man-JL/WFP-Traffic-Redirection-Driver)，可以用于匿名通信
- 2016 - 使用 **现代 C++** 设计 **强类型、编译时** [对象关系映射 (ORM)](https://github.com/BOT-Man-JL/ORM-Lite)（[中文介绍](../2016/How-to-Design-a-Better-Cpp-ORM.md)）

<script>
var two_column_style = '&style=two-column';
if (document.location.search.indexOf('style') == -1)
  document.location += two_column_style;

var cvStyleElem = document.createElement('style');
cvStyleElem.innerHTML += '@media print { header { margin-bottom: 0; } }';
document.head.appendChild(cvStyleElem);

if (qrCodeSVG)
  document.getElementById('cvQrCodeSec').innerHTML = qrCodeSVG(location.href.replace(two_column_style, ''), 360);
</script>
