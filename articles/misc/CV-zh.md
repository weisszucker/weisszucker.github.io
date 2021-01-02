[not-print]

[float-right]

[EN](CV-en.md) | 中文

[print-only]

<p id="cvQrCodeSec" style="position: fixed; width: 18%; height: 18%; top: 15px; right: 30px;"></p>

## 工作

### 2018.7 - 至今 | 腾讯

- 影响力：腾讯 **C++ 代码规范** 小组 **核心成员**
- 绩效：曾获 **5 星** (2019 H1) / **4 星** (2018 H2)
- 奖项：曾获 **腾讯知识奖**（[技术类文章](../2019/Inside-Cpp-Callback.md)）

#### 2019.11 - 至今 | 腾讯 智能平台 NLP

- **C++/Python 后台开发**，基于 [Tars](https://github.com/TarsCloud/TarsCpp) 开发后台服务
- 开发 **教育 AI 服务**，从零到一开发 自适应推荐、学情画像 等功能，接入 链路跟踪、远程日志、指标监控，基于 gperftools 分析性能、优化瓶颈
- 优化 **NLP 模型训练服务**，改造并行调用，完善告警
- 参与 **开源协同**，建设部门内 C++/Python 基础库，共建公司级 Python RPC 框架
- 提升 **研发效率**，推动部门内 单元测试、代码静态扫描、持续集成 建设和优化

#### 2018.7 - 2019.10 | 腾讯 QQ 浏览器

- **C++ 客户端开发**，基于 [Chromium](https://www.chromium.org) 开发 PC 浏览器
- 设计 **基于 XAML 原生渲染方案**，实现导航页秒开，50% 用户首屏提速 5s 以上，提高 15% 收入
- 降低 **UI 崩溃率** 40%~50%，使用 WinDbg 分析线上 dump，加入 [静态/动态检查](../2019/Cpp-Check.md)，减少安全隐患
- 开发 [Tencent/Hippy](https://github.com/Tencent/Hippy) **移动跨平台框架**，重构线程模型、日志模块，规范 C++ 代码，优化 JNI 调试
- 封装 **JavaScript 通用扩展接口**，提供内部 SDK，提升可扩展性，降低运营开发成本

#### 2017.7 - 2018.7 | 腾讯 QQ 浏览器（实习）

- Node.js/C++ 后台开发
  - 开发 **欢迎页运营** 服务、**导航页抓取** 服务
  - 重构 **A/B Test 任务灰度下发** 模块
- C++/Win32 客户端开发
  - 开发 **QQ Mini 浏览器书签** 功能，负责部分业务需求的日常开发工作
  - 开发 浏览器团队 **C++ 规范检查** 工具

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
  - Windows 桌面软件（C++）
  - Linux 后台服务（C++/Python）
- 熟悉 ⭐⭐
  - Node.js 后台服务（JavaScript）
  - Web 前端/微信小程序（JavaScript）
  - CLI 脚本（JavaScript/Python/Bash/BAT）
- 掌握 ⭐
  - Windows WFP 网络驱动（C）
  - Windows UWP 通用应用（C#）
  - ASP.NET (core) 后台服务（C#）

## 开源

- 2018 - 设计 **反流量嗅探** 的 [Windows 流量重定向驱动](https://github.com/BOT-Man-JL/WFP-Traffic-Redirection-Driver)，可以用于匿名通信
- 2017 - 参与 [Visual Studio Markdown 编辑器](https://github.com/madskristensen/MarkdownEditor) 项目
- 2016 - 使用 **现代 C++** 设计 **强类型、编译时** [对象关系映射 (ORM)](https://github.com/BOT-Man-JL/ORM-Lite)（[中文介绍](../2016/How-to-Design-a-Better-Cpp-ORM.md)）
- 2015 – 设计 **轻量、跨平台** [C++ 图形库](https://github.com/BOT-Man-JL/EggAche-GL)

<script>
var two_column_style = '&style=two-column';
if (document.location.search.indexOf('style') == -1)
  document.location += two_column_style;

if (qrCodeSVG)
  document.getElementById('cvQrCodeSec').innerHTML = qrCodeSVG(location.href.replace(two_column_style, ''), 360);
</script>
