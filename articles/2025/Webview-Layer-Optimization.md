# 浏览器 GPU 渲染卡顿的小故事

> 2025/3/13
> 
> 揪出那条藏在 “沙丁鱼群里的鲶鱼”。

## 背景

最近用手机浏览一个经常访问的页面时，发现

<p><details>
<summary> 👉 点我预览：卡顿的效果 👈 </summary>
<div style="
  width: 100%;
  height: 480px;
  background: url('2025/Webview-Layer-Optimization/complicated.svg') center/cover no-repeat;"
>
  <img src="Webview-Layer-Optimization/simple.webp" alt="Simple WebP"
       style="
         position: relative;
         top: 40%;
         left: 40%;
         width: 20%;
         height: 20%;
         object-fit: contain;"
       onerror="this.style.display='none'" />
</div>
</details></p>

## 试验


<p><details>
<summary> 👉 点我预览：只有 SVG 的效果 👈 </summary>
<div style="
  width: 100%;
  height: 480px;
  background: url('2025/Webview-Layer-Optimization/complicated.svg') center/cover no-repeat;"
>
</div>
</details></p>

<p><details>
<summary> 👉 点我预览：只有 WebP 的效果 👈 </summary>
<div style="
  width: 100%;
  height: 480px;
  background: grey;"
>
  <img src="Webview-Layer-Optimization/simple.webp" alt="Simple WebP"
       style="
         position: relative;
         top: 40%;
         left: 40%;
         width: 20%;
         height: 20%;
         object-fit: contain;"
       onerror="this.style.display='none'" />
</div>
</details></p>

<p><details>
<summary> 👉 点我预览：优化后的效果 👈 </summary>
<div style="
  width: 100%;
  height: 480px;
  background: url('2025/Webview-Layer-Optimization/complicated.svg') center/cover no-repeat;"
>
  <img src="Webview-Layer-Optimization/simple.webp" alt="Simple WebP"
       style="
         transform: translateZ(0);
         position: relative;
         top: 40%;
         left: 40%;
         width: 20%;
         height: 20%;
         object-fit: contain;"
       onerror="this.style.display='none'" />
</div>
</details></p>

## 写在最后



如果有什么问题，**欢迎交流**。😄

Delivered under MIT License &copy; 2025, BOT Man
