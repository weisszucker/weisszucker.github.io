# Chromium UI 基础

> 2021/3/1
> 
> Chromium UI 基本概念 & 最佳实践

[TOC]

## 基本概念

### 编译项目

- gn 用于管理项目，需要手动编写
- ninja 控制实际编译，文件由 gn 自动生成

### 代码结构

- UI 库主要放在 `ui/` 目录下
- `ui/views/` 提供了 系统无关的 UI 框架
- `ui/views/controls/` 提供了基本控件（例如 按钮、标签）
- `ui/display/` 提供了 显示绘制 相关功能（例如 显示器信息）
- `ui/gfx/` 提供了 绘制相关的通用组件（例如 图像、画布、字体、动画）
- 如果修改 Chromium 的代码，使用编译保护宏（方便后续迁移我们的修改）

### Widget/View 关系

- `views::Widget` 一般对应一个独立的窗口/子窗口
- `views::View` 一般对应一个组件/模块
- `views::WidgetDelegate` 连接 Widget 和 View
- `views::WidgetDelegateView` 继承 WidgetDelegate 和 View
  - 通过重载 `GetContentsView() { return this; }` 关联当前 View 和 Widget
  - 所有权/生命周期：
    - Widget 销毁 Delegate `SetOwnedByWidget(true);`
    - 自行销毁 View `set_owned_by_client();`
- （不要直接使用）RootView 连接 Widget 和 View
  - 作为 Widget 事件派发到 View 的桥梁
  - 其 ContentsView 就是 Widget 的 ContentsView
    - 如果有边框（`TYPE_WINDOW` 和 `TYPE_BUBBLE`），则用 NonClientView
    - 如果无边框（其他 `InitParams::Type`），则直接用 WidgetDelegateView
- （不要直接使用）NonClientView 用于支持边框
  - 实现 放缩、标题、图标、非客户区检测 等边框相关功能
  - 包含以下两个重叠放置的子 View
    - NonClientFrameView 非客户区（一般是实际的边框）
    - ClientView 客户区（其 ContentsView 一般是 WidgetDelegateView）
- `views::WidgetDelegateView` 相对于 `views::Widget` 的位置
  - 如果不含边框，WidgetDelegateView 的区域和 Widget/RootView 相同
  - 如果包含边框，WidgetDelegateView 的区域是 Widget/RootView 减去窗口边框 NonClientFrameView 的部分（类似于 `View::GetContentsBounds()`）

### Widget 的自身属性

- 窗口位置
  - 对于 `Widget::InitParams::TYPE_CONTROL` 窗口，使用父 Widget 左上角 (0, 0) 坐标系
  - 对于其他类型窗口，使用主屏幕左上角 (0, 0) 的全局坐标系
  - 设置方法
    - `Widget::SetBounds/SetSize()` 设置在坐标系中的位置/大小
    - `Widget::CenterWindow()` 相对于屏幕/父窗口居中
    - `Widget::SetBoundsConstrained()` 设置在坐标系中的位置，并调整位置尽量保证可见
  - 获取方法
    - `Widget::GetWindowBoundsInScreen()` 获取窗口位置
- 窗口层级 `ui::ZOrderLevel`
  - 对于 macOS，映射到 `CGWindowLevel` 上
  - 对于 Windows，除了 `ZOrderLevel::kNormal` 的其他层级都设置为 `HWND_TOPMOST`
- 显示状态
  - `Widget::Show/ShowInactive/Hide()` 显示（并激活/不激活）/隐藏
  - `Widget::Maximize/Minimize/Restore()` 最大化/最小化/还原
  - `Widget::SetFullscreen()` 全屏（进入/退出）
  - `Widget::Activate/Deactivate()` 激活/取消激活
  - `Widget::Close/CloseWithReason/CloseNow()` 关闭（不会立即销毁，参考生命周期）
- 生命周期（手动 new + Init，自动 delete）
  - 开始创建
    - `WidgetDelegate::WidgetInitializing()`
  - 结束创建
    - `WidgetObserver::OnWidgetCreated()`
    - `WidgetDelegate::WidgetInitialized()`
  - 即将销毁（仅在 `Widget::Close/CloseNow()` 时同步触发，系统关闭时不会触发）
    - `WidgetObserver::OnWidgetClosing()`
    - `WidgetDelegate::WindowWillClose()`
  - 开始销毁（异步于 `Widget::Close/CloseNow()` 调用）
    - `WidgetObserver::OnWidgetDestroying()`
    - `WidgetDelegate::WindowClosing()`
  - 结束销毁
    - `WidgetObserver::OnWidgetDestroyed()`
    - `WidgetDelegate::DeleteDelegate()`

### Widget 属性组合

- 系统阴影/自绘阴影/异形窗口：`kSystemShadowBorder`/`kCustomShadowBorder`/`kNone`
  - 系统阴影 当且仅当 不透明、带阴影
  - 自绘阴影/异形窗口 当且仅当 透明、无阴影
  - Windows：
    - 系统阴影 当且仅当 可拉伸
    - 一般都 移除系统边框
  - macOS：
    - 移除系统边框 当且仅当 自绘阴影/异形窗口
    - 不限制 是否可拉伸
- 独立窗口/子窗口：`TYPE_WINDOW`/`TYPE_BUBBLE`、`TYPE_CONTROL`
  - 系统阴影/自绘阴影 必须是 `TYPE_WINDOW`/`TYPE_BUBBLE` 类型（依赖于 NonClientView）
  - `TYPE_CONTROL` 子窗口 必须是 异形窗口（不支持 NonClientView）
  - Windows：仅 `TYPE_BUBBLE` 子窗口 允许超出父窗口范围
  - macOS：所有类型子窗口 均可超出父窗口范围
- 可拉伸：`WidgetDelegate::SetCanResize()`
  - 一般使用 `WidgetDelegate::SetHasWindowSizeControls()` 同时设置 可最大/最小化
  - Windows：
    - 系统阴影 当且仅当 可拉伸（借助 `WS_THICKFRAME` 显示阴影边框）
    - 自绘阴影/异形窗口 当且仅当 不可拉伸
      - 拉伸行为依赖于 `WS_THICKFRAME` 才能生效（否则只能修改光标）
      - 而 `WS_THICKFRAME` 和 `kTranslucent` 冲突（`Layered windows do not work with Direct3D`）
  - macOS：无限制
- 移除系统边框：`remove_standard_frame`（在不同系统上含义不同）
  - Windows：控制是否显示 标题栏 `WS_CAPTION`、系统栏 `WS_SYSMENU`，一般都 移除系统边框
  - macOS：控制是否显示 系统阴影边框，移除 当且仅当 自绘阴影/异形窗口
- 透明/不透明：`WindowOpacity::kTranslucent`/`WindowOpacity::kOpaque`
  - `WindowOpacity::kInferred` 默认使用 `kOpaque`
  - 透明 当且仅当 自绘阴影/异形窗口
    - Windows：`WS_THICKFRAME` 和 `kTranslucent` 冲突
    - macOS：OS 10 不透明窗口背景为纯色，OS 11.5+ 深色模式下透明窗口无法显示系统阴影白色边框
- 无阴影：`ShadowType::kNone`
  - `ShadowType::kDefault` 默认对非 `TYPE_CONTROL` 使用阴影
  - 无阴影 当且仅当 自绘阴影/异形窗口
- 暂不考虑：
  - Windows 7 非 Aero 模式下，软渲染不支持透明
  - 无边框的不透明窗口（支持拉伸，带有四个方角），因为不符合一般设计规范

### Widget 的之间关系

- 如何设置
  - 初始化 Widget 时指定 `Widget::InitParams::parent` 字段
  - 调用 `Widget::ReparentNativeView()` 重新设置
- 逻辑子窗口
  - 父子 Widget 挂在同一个窗口树上，而不是独立窗口
  - 生命周期关联：父 Widget 关闭时，子 Widget 会被连带销毁
    - 父 Widget 开始销毁 -> 子 Widget 开始销毁 -> 子 Widget 结束销毁 -> 父 Widget 结束销毁
  - Windows 上，父窗口在任务栏上 不会失去激活状态（owner-owned 关系）
  - macOS 上，子窗口在调度中心里 不可随意独立拖动
- 物理子窗口
  - 使用 `Widget::InitParams::TYPE_CONTROL` 设置
  - 子 Widget 使用父 Widget 左上角 (0, 0) 坐标系，且不能超出父 Widget 范围
- 相对位置
  - `Widget::StackAbove/StackAboveWidget()` 调整到特定 Widget 之上
  - `Widget::StackAtTop()` 调整到所有 Widget 之上

### Widget 的跨平台机制

- Widget 提供平台无关的窗口逻辑
- NativeWidget 提供平台相关的窗口实现
  - `NativeWidgetAura` 用于 Windows 和 Linux
  - `NativeWidgetMac` 用于 macOS
- `gfx::NativeView`/`gfx::NativeWindow` 表示平台实际使用的窗口类型
  - 在 `USE_AURA` 的情况下，类型为 `aura::Window*`
    - Windows 下可通过 `views::HWNDForWidget/Native[Window|View]()` 获取句柄
  - 在 OS_MAC 的情况下，直接存储 `NSView*`/`NSWindow*` 的包装类
- `internal::NativeWidgetDelegate`
  - Widget 直接继承于它
  - NativeWidget 直接依赖于它（`NativeWidgetAura`/`NativeWidgetMac::delegate_`）
- `internal::NativeWidgetPrivate`
  - `NativeWidgetAura`/`NativeWidgetMac` 直接继承于它
  - Widget 直接依赖于它（`Widget::native_widget_`）

### View 的自身属性

- 可用性 `enabled_` + 可见性 `visible_`
- 绘制自身 `View::OnPaint(gfx::Canvas*)`
  - 背景 `background_`
  - 边框 `border_` -> 内边距 `View::GetInsets()`
    - 可以借助 `views::CreatePaddedBorder()` 解耦边框和内边距
- 绘制树结构（所有子 View 递归调用）`View::Paint()`
  - 裁剪区域 `clip_path_`（默认使用当前 View 的尺寸）
- 期望尺寸 `preferred_size_`（用于布局，不一定等于实际尺寸）
  - 可通过 `SetPreferredSize()` 设置固定尺寸
  - 或重载 `CalculatePreferredSize()` 计算动态尺寸
  - 或借助 `views::LayoutManager` 根据子 View 计算动态尺寸
- [Metadata & Properties](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/ui/views/metadata_properties.md) 提供 UI 属性化接口，用于 [UI devtools](https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/ui/ui_devtools/index.md) 动态调试

### View 的之间关系

- 父节点 `parent_` + 子节点列表 `children_`
  - 关联 `View::AddChildView()`
  - 移除 `View::RemoveChildViewT()`（返回 unique_ptr 表示所有权）
  - 排序 `View::ReorderChildView()`
- 相对于父 View 的实际位置 `bounds_` = 原点 `origin()` + 尺寸 `size()`
  - 相对自身的局部位置 `GetLocalBounds()`（原点为 (0, 0)，尺寸为 `size()`）
  - 除去边框的内容位置 `GetContentsBounds()`（Local Bounds 减去 4 条边框）
  - 相对自身的可见位置 `GetVisibleBounds()`（Local Bounds 减去不可见区域）
- 布局 = 在父 View 的 `View::Layout()` 里，调用子 View 的 `View::SetBounds()`
  - 方法一：重载父 View 的 `View::Layout()` 函数，布局所有的子 View（简单）
  - 方法二：设置父 View 的 `views::LayoutManager`（复杂）
  - 方法三：使用 Xaml 布局器（针对不需要动态变化的场景）
- 所有权/生命周期 `owned_by_client_`
  - false：由 `parent_` 持有（默认行为）
  - true：需要自行管理（很少使用，例如 `WidgetDelegateView` 自行销毁）

### 交互事件

- 键盘事件 `View::OnKey*()`
- 鼠标（触摸）事件 `View::OnMouse*()`
- 滚动（手势）事件 `View::OnMouseWheel()`
- 设置光标 `View::GetCursor()`
- 判断点击 `View::HitTestPoint()`

### 字符模型

- `std::string`/`char` ASCII 或 UTF-8（中英文字符长度不相等）
- `std::u16string`/`char16_t`（解决非 Windows 下 `sizeof(wchar_t) != 2` 的问题）

### 文案本地化

- GRIT
- https://www.chromium.org/developers/design-documents/ui-localization/

### 图片支持

- 1x/2x png/jpg
- svg -> [vector icon](https://chromium.googlesource.com/chromium/src/+/HEAD/components/vector_icons/README.md)
- lottie ([skottie](https://skia.org/docs/user/modules/skottie/))

### 动画支持

- `gfx::Animation` 动画驱动器，子类提供多种动画模版
- `gfx::AnimationDelegate` 用于接收动画事件并设置关键帧内容（位置/大小/颜色等）

### 无障碍

- https://source.chromium.org/chromium/chromium/src/+/main:ui/views/accessibility/view_accessibility.h

### 线程模型

- 无竞争任务模型，基本原则：
  - UI 数据只能在 UI 线程访问（[漫谈 C++ 的各种检查](../2019/Cpp-Check.md#线程安全检查)）
  - UI 线程不能阻塞（例如 I/O 操作，[漫谈 C++ 的各种检查](../2019/Cpp-Check.md#线程限制检查)）
  - 避免 加锁、共享内存
- 四种方式
  - UI 线程任务（Worker -> UI）、异步任务（UI -> UI）`base::PostTask()`
  - 延迟任务 `base::PostDelayedTask()`
  - 无结果回调 `base::PostTaskAndReply()`
  - 有结果回调 `base::PostTaskAndReplyWithResult()`
- 引用生命周期
  - 裸引用（可能不安全）`base::Unretained(ptr)`
  - 弱引用 `base::WeakPtr<T>`
  - 强引用 `base::RetainedRef(ptr)`
- [深入 C++ 回调](../2019/Inside-Cpp-Callback.md)
- [Chromium的无锁线程模型示例之PostTask](https://bingoli.github.io/2020/01/27/chromium_thread_model_post_task/)

### 设计模式

- Delegate
- Observer/Listener/Handler
- Singleton

### 代码规范

- C++ 11（智能指针、匿名函数）
- DCHECK（[漫谈 C++ 的各种检查](../2019/Cpp-Check.md)）
- [聊聊 C++ 的优雅写法](../2020/Conventional-Cpp.md)

### 延伸阅读

- https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/ui/index.md
- https://chromium.googlesource.com/chromium/src/+/refs/heads/main/docs/ui/views/overview.md

## 最佳实践

### 代码搜索

- https://source.chromium.org/chromium/chromium/src
- 关于 C++ 基础功能、跨平台功能，建议先到 base/ 目录下搜索

### View 相关

#### 如何调试 View 的位置问题

- 检查 `parent_` 判断 是否已挂载到树结构上、父节点是否正确
- 检查 `bounds_`/`border_` 判断位置、内容是否正确
- 检查 `visible_` 判断是否可见
- 修改 `background_`（或修改被重载的 `OnPaint()` 函数）测试能否显示

#### 如何触发 View 重新绘制、重新布局

- 调用 `View::SchedulePaint()` 标记 脏区域
- 调用 `View::InvalidateLayout()` 强制 所有父 View、对应 Widget 重新计算布局
- 上述操作均为异步

#### 为什么 View 会循环进入 重新绘制、重新布局

- 主要原因是
  - 在 `View::OnPaint()` 时，调用 `View::SchedulePaint()` ，触发重新绘制
    - 例如 `Label::SetEnabledColor()` 修改文本颜色
  - 在 `View::Layout()` 时，调用 `View::InvalidateLayout()` ，触发重新布局
    - 例如 `ImageView::SetImageSize()` 修改图片尺寸
  - 有可能是：直接或间接 调用、当前 View 或子 View 调用
- 所以，原则上
  - 在绘制时 不要修改任何 View 的状态
  - 在布局时 不要修改除了 Bounds 之外的状态

#### 如何修改已有边框的边距

- 默认情况下，每种 Border 有内置的样式和边距，但很多情况下需要保留样式、定制边距
- 可以使用 `views::CreatePaddedBorder()` 在已有边框上增加内边距，并保留原有样式
- 但是，最后生成的边框的边距等于两部分之和（尤其需要注意 1px 边框的情况）

#### 如何让 View 不响应事件

- `View::[Get|Set]CanProcessEventsWithinSubtree()` 透传到父 View 上
- `TransparentViewTargeterDelegate` 透传到下层的 View 上
- `OverlayViewTargeterDelegate` 透传到子 View 里

#### 如何让 View 接收到进出子 View 的鼠标事件

- `View::SetNotifyEnterExitOnChild(true)` hover 到子 View 时会触发 MouseExit 事件
- `View::SetNotifyEnterExitOnChild(false)` hover 到子 View 时不触发 MouseExit 事件

#### 如何让 View 不参与 Tab 焦点切换、快捷键响应

- `View::SkipDefaultKeyEventProcessing() == true` 不响应 Tab 焦点切换、其他快捷键
- `View::SkipDefaultKeyEventProcessing() == false` 默认参与上述行为

### Widget 相关

#### 如何观测 Widget 对象的有效性

- 两个常用途径
  - 监听 `WidgetObserver::OnWidgetDestroying()` 事件
  - 注册 `WidgetDelegate::RegisterWindowClosingCallback()` 回调
- `WidgetDeletionObserver` 封装了类似的功能
  - `WidgetDeletionObserver::IsWidgetAlive()` 返回 Widget 有效性
  - 析构函数和 `OnWidgetDestroying()` 都移除观察者

#### 如何保证 Widget、WidgetDelegateView 指针的有效性

- 在观测到 Widget 销毁时，移除 Widget 观察者、置空 Widget 指针变量
- 类似的，WidgetDelegateView 生命周期和对应 Widget 相同，都需要主动观测其有效性，而不能手动 delete

#### 如何阻止窗口关闭

- 可在 `ClientView::OnWindowCloseRequested()` 或 `WidgetDelegate::OnCloseRequested()` 拦截、取消 窗口关闭的请求

#### 如何让 Widget 不响应事件

- 全部穿透：创建 Widget 时设置 `Widget::InitParams::accept_events = false`
  - Windows 下，AuraWindow 支持穿透，DesktopAuraWindow 真窗口不支持
  - macOS 下，借助 `NSWindow.ignoresMouseEvents` 屏蔽鼠标事件
- 部分穿透：通过 `NonClientFrameView::NonClientHitTest()` 返回 `HTTRANSPARENT`
  - Windows 下，对应区域的事件 会透传给 同进程的后边的窗口
  - macOS 下，不支持部分穿透
- 另外，`Widget::InitParams::TYPE_CONTROL` 子窗口存在平台差异
  - Windows 下，使用 `aura::Window` 假窗口，没有独立的 `NonClientHitTest()` 逻辑（跟随父窗口对应区域的返回值）
  - macOS 下，使用 `NSWindow` 真窗口，有独立的 `hitTest:` 处理逻辑（默认 `non-draggable`）

#### View 的焦点 vs. Widget 的激活态

- View 的焦点由 Chromium UI 管理
  - 同一时间，每个 Widget 上有且仅有一个 View 获得焦点
  - `View::RequestFocus()` 在 `FocusManager` 中请求 View 的焦点（非 macOS 下会顺带激活 Widget，而 macOS 下仅在 Widget 激活时切换焦点）
- Widget 的激活态由操作系统管理
  - 同一时间，系统内 有且仅有 一个窗口处于激活态
  - `Widget::Activate()` 调用系统 API 激活窗口

#### 如何弹窗不抢焦点

- 调用 `Widget::ShowInactive()` 显示窗口，而不是 `Widget::Show()`

#### 如何避免窗口创建后再移动导致闪烁

- 原因：在 `Widget::Init()` 后立即调用 `Widget::Show()` 显示，过了一段时间后再调用 `Widget::SetBounds()` 移动，导致窗口位置闪烁
- 方法一：初始设置 —— 在 `Widget::InitParams::bounds` 里设置初始位置，不需要调用 `Widget::SetBounds()`
- 方法二：延迟显示 —— 在确定了窗口位置后，再调用 `Widget::Show()` 并立即调用 `Widget::SetBounds()`（由于在同一个调用栈里，不会在移动前真正显示窗口）

#### 为什么 `Widget::SetBounds()` 不生效

- 可能受到了 `Widget::GetMinimumSize()` 或 `Widget::GetMaximumSize()` 的限制
- 如果有 `non_client_view_` 则会返回 `View::GetMinimumSize/GetMaximumSize()`，否则不限制
- 默认情况下，`View::GetMinimumSize()` 返回 `LayoutManager::GetMinimumSize()` 或 `View::GetPreferredSize()`

### views/controls 相关

#### 禁用 LabelButton、Checkbox、RadioButton 自动添加边框

- 原因：`LabelButton::OnThemeChanged()` 自动添加边框
- 解决：调用 `LabelButton::SetBorder()` 一次后（如果不需要任何边框，可以使用 `views::NullBorder()`/`nullptr`），不再自动添加

#### 禁用 Label 自动修改颜色

- 原因：无障碍 _(accessibility)_ 特性，但我们不需要
- 解决：`Label::SetAutoColorReadabilityEnabled(false)` 关闭自动换色

#### 解决 Label 多行场景下更新文本后 PreferredSize 错误

- 原因：`Label::CalculatePreferredSize()` 调用 `Label::GetTextSize()` 根据当前 View 宽度计算 PreferredSize
  - 如果文本更新，则不会根据 MaximumWidth 重新计算 PreferredSize
  - 主要影响 多行文本、文字长度增加 的场景
- 解决：多行模式下，在调用 `Label::SetText()` 更新文本后、`GetPreferredSize()` 之前，手动设置 Label 的 `width` 为 0（或 `size` 为空）
- 参考：https://codereview.chromium.org/1024753005

#### 避免 Button 鼠标松开后，再次弹出失焦消失弹窗

- 原因：如果 Button 对应的操作是弹出一个 “失焦消失” 的弹窗，那么弹窗会在鼠标按下时先消失，而在鼠标松开时再次弹出（现象是点击 Button 一直重新弹窗）
  - 默认情况下，Button 在鼠标松开后执行操作（`NotifyAction::kOnRelease` 行为）
  - 如果改为 `NotifyAction::kOnPress` 行为，也可以规避这个问题
- 解决：重写 `Button::IsTriggerableEvent()` 逻辑，并额外记录 `is_popup_showing_`、`suppress_button_release_` 两个状态
  - 在 `Button::IsTriggerableEvent()` 判断如果 `is_bubble_showing_ || suppress_button_release_` 直接返回 `false`，跳过当前 Button 操作
  - 在 `Button::OnMousePressed()` 记录 `suppress_button_release_ = is_popup_showing_`，用于鼠标松开时 `Button::IsTriggerableEvent()` 的结果

#### 修改 Button 的启用、禁用状态

- 方法一 `View::SetEnabled(false)`：同时禁用 消息派发、焦点转移，不支持 `View::GetCursor()` 光标修改、tooltip 提示
- 方法二 `Button::SetState(Button::STATE_DISABLED)`：仍然支持 消息派发、焦点转移、光标修改、tooltip 提示

#### 不同平台下 ScrollView 的层级差异

- ScrollView 通过 `features::kUiCompositorScrollWithLayers` 开关控制是否使用 Layer
- 在 macOS 下默认将 `contents_view` 设置启用 Layer，而在 Windows 下默认不开启
- 可以在构造 ScrollView 对象时，显式启用、禁用 Layer，从而避免平台差异

#### 为什么 ScrollView 看不到内容

- 在布局 ScrollView 前，需要先布局 ScrollView 的内容 View，再布局 ScrollView 本身
- 一般可以通过 `scroll_view->contents()->SizeToPreferredSize()` 设置

### gfx 相关

#### 如何通过 ImageSkia 合成新的 ImageSkia

- `gfx::ImageSkiaOperations::CreateBlendedImage` 根据 alpha 合成两张图片
- `gfx::ImageSkiaOperations::CreateTransparentImage` 根据 alpha 创建半透明图片
- `gfx::ImageSkiaOperations::CreateSuperimposedImage` 叠加两张图片
- `gfx::ImageSkiaOperations::ExtractSubset` 裁剪图片
- `gfx::ImageSkiaOperations::CreateResizedImage` 放缩图片（可能导致半透明部分计算错误）
- `gfx::ImageSkiaOperations::CreateRotatedImage` 旋转图片
- `gfx::ImageSkiaOperations::CreateImageWithCircleBackground` 图片加圆形背景
- `gfx::CanvasImageSource::CreatePadded` 图片加边距
- 上述操作会存储原始的 ImageSkia 用于延迟创建，可能导致原始图片的内存无法释放
  - 可以使用 `ImageOperations` 直接操作 SkBitmap 避免
  - 另外，`gfx::CanvasImageSource` 内部仅记录绘制指令，叠加部分操作可能消耗大量 CPU

#### 如何修改字体样式

- 如果仅修改样式或字号、不需要修改 FontFamily，建议使用 `FontList::Derive()` 的方式，基于原有 FontList 生成新的 FontList 对象
- 仅在必须修改 FontFamily 时，才创建新的 FontList 对象，避免加载字体失败
- 默认字体通过 `gfx::FontList::SetDefaultFontDescription()` 设置

### 工程相关

#### 经常看到的 `xxx_aura/win/linux.cc` 和 `xxx_mac.mm` 是什么意思

- aura 是 Windows/Linux 平台下的跨平台 UI 库
- mac 平台的实现使用 Objective-C++ 语言，所以文件后缀为 `.mm`
- 在相关的 `BUILD.gn` 里，需要把对应的源文件加到 `is_win`/`is_mac`/`is_linux` 对应的 `sources` 里，否则无法编译

### 其他问题

#### 如何通过 `base::Bind*()` 绑定 lambda 表达式

- 对于没有捕获上下文的 lambda 表达式，可以直接绑定（但不建议，不好调试）
- 对于带有捕获上下文的 lambda 表达式，不支持直接绑定，一般可以改用成员函数等方式

#### 为什么 `base::Bind*()` 只能给“无返回值的函数”绑定 `base::WeakPtr<T>`

- 在异步回调中绑定 `base::WeakPtr<T>` 弱引用，主要是为了解决弱引用失效的场景（参考：[深入 C++ 回调](../2019/Inside-Cpp-Callback.md#如何处理失效的-弱引用-上下文)）
- 在弱引用失效时，“无返回值的函数” 可以直接不执行，而 “有返回值的函数” 无法计算最终的返回值
- 在编译时，可以检查并提示 `weak_ptrs can only bind to methods without return values` 错误
- 另外，不要把 UI 对象的成员函数抛到 线程池 里执行，尽量在线程池里异步使用纯函数，不要多线程同时访问 UI 对象

#### 如何构造 “与” 关系的回调

- 场景：同时发起 N 个请求，等待所有 请求得到结果后，回调一次（仅支持闭包）
- 方法：`base::BarrierClosure()`
  - 输入：OnceClosure（最终的一次回调）+ num_callbacks_left（多次调用的次数 N）
  - 输出：RepeatingClosure（可多次调用的回调，调用 N 次后触发输入的一次回调）

#### 如何构造 “或” 关系的回调

- 场景：发起请求后超时处理，在 请求得到结果 或 超时 后、在 成功 或 失败 后，回调一次（任意签名）
- 方法一：`base::SplitOnceCallback()`
  - 输入：OnceCallback（最终的一次回调）
  - 输出：`pair<OnceCallback, OnceCallback>`（两个一次回调，只能调用其中一个，调用另一个会触发 CHECK）
- 方法二：`base::BindRepeating([](base::OnceCallback<...>& ref_cb) { if(ref_cb) std::move(ref_cb).Run(...); }, base::OwnedRef(std::move(callback)))`
  - 输入：OnceCallback（最终的一次回调）
  - 输出：RepeatingCallback（可多次调用的回调，仅第一次调用时生效，之后无效）
  - 备注：新版本[已移除](https://bugs.chromium.org/p/chromium/issues/detail?id=730593) `base::AdaptCallbackForRepeating()`
