# Javascript 异步调用模型

> 2023/10/21
> 
> 横看成岭侧成峰。——《题西林壁》苏轼

很早之前写过一篇比较抽象的 [从时空维度看 I/O 模型](../2019/IO-Models.md)，本文将用 Javascript 举例说明 **阻塞与非阻塞** 的区别。

## 为什么要异步

同步调用 `fs.readFileSync()` 会阻塞当前的 Javascript 线程，而由于一般的 Javascript 环境只有一个线程，导致在读文件的过程中无法执行其他代码（如果程序有 UI，那么 UI 此时无法响应用户操作）。

``` javascript
try {
  const data = fs.readFileSync(filename);
  // use |data|
} catch (err) {
  // handle |err|
}
```

为此，大部分涉及 I/O 调用的 Javascript 接口都会被设计为异步接口，使得在 I/O 执行过程中 Javascript 线程仍能执行其他代码（例如 UI 继续响应用户操作）。

根据使用方法，异步接口主要分为 `callback`、`Promise`、`async/await` 三类。

## 原始的 `callback` 异步非阻塞模型

早期 Node.js 的 fs 接口都以回调函数的形式提供，一般的格式为 `fs.method(...param, callback)`，其中最后一个参数 `callback` 回调函数的参数 `(err, data)` 分别对应 I/O 的错误和结果。

``` javascript
fs.readFile(filename, (err, data) => {
  console.log('2. finish file I/O');
  if (err) {
    // handle |err|
  }
  // use |data|
});
console.log('1. started file I/O');
```

如果回调函数存在多层 **相互依赖** 的嵌套，就会出现 [回调地狱 _(callback hell)_](http://callbackhell.com/)，降低代码可读性。

``` javascript
fs.readFile(filename1, (err1, data1) => {
  console.log('2. finish file I/O');
  if (err1) {
    // handle |err1|
  }
  // use |data1|

  fs.readFile(filename2, (err2, data2) => {
    console.log('4. finish file I/O again');
    if (err2) {
      // handle |err2|
    }
    // use |data2|
  });
  console.log('3. started file I/O again');
});
console.log('1. started file I/O');
```

## `Promise` 异步非阻塞模型

为了化简回调逻辑的控制流，Javascript 引入了 [Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Using_promises) 的概念。

Promise 通过 [链式调用的方式](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise#Chained_Promises) 解决回调地狱的问题：

- `Promise.then()` 的返回值仍是一个 `Promise` 对象（返回相同类型对象是链式调用的基础）
- `Promise.then()` 的参数 `callback` 的返回值将会作为 `Promise.then()` 返回的 `Promise` 对象参数 `callback` 的参数（链式传递 continuation 的执行结果）
- 如果 `callback` 的返回值是 `Promise` 对象，则会转换为异步调用的实际结果，再作为参数传递给下一个 `callback`（使用者无需关心异步处理细节）

``` javascript
fs.promises.readFile(filename1)
  .then(data1 => {
    console.log('2.1. finish file I/O with success');
    // use |data1|

    const promise = fs.promises.readFile(filename2);
    console.log('3. started file I/O again');
    return promise;
  })
  .then(data2 => {
    console.log('4.1. finish file I/O again with success');
    // use |data2|
  })
  .catch(err => {
    console.log('2.2. or 4.2. finish file I/O with failure');
    // handle |err|
  });
console.log('1. started file I/O');
```

基于 callback 的接口 如何转换为 生成 Promise 的接口 呢：

- 构造 `Promise` 对象的参数是一个回调函数 `(resolve, reject) => { ... }`
- 在这个回调函数内，调用 `resolve(data)` 会将结果传递给 `Promise.then()`，调用 `reject(err)` 会将错误传递给 `Promise.catch()`

``` javascript
fs.promises.readFile = (filename) => {
  return new Promise((resolve, reject) => {
    fs.readFile(filename, (err, data) => {
      if (err) {
        reject(err);
      } else {
        resolve(data);
      }
    });
  });
}
```

## `async/await` 异步阻塞模型

由于 Promise 仍是非阻塞模型，代码中充斥着大量的 callback，执行顺序并不直观；Javascript 引入了 [`async`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/async_function)/[`await`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/await) 阻塞模型，让代码得以顺序执行。

在上述代码中，改用 `await` 阻塞等待 `Promise` 对象，即可拿到 I/O 结果 `data` 或异常 `err`。

``` javascript
try {
  const data = await fs.promises.readFile(filename);
  // use |data|
} catch (err) {
  // handle |err|
}
```

此时的代码不仅像本文开头使用 `fs.readFileSync()` 一样简单，而且还没有同步调用导致的线程阻塞问题（虽然阻塞代码，但不阻塞线程）。

很巧妙的是，`async/await` 和 `Promise` 可以相互转换。

``` javascript
async function getFileLength(filename) {
  const data = await fs.promises.readFile(filename);
  return data.byteLength;
}
console.log(await getFileLength(filename));
```

等价于

``` javascript
function getFileLength(filename) {
  return fs.promises.readFile(filename).then(data => data.byteLength);
}
getFileLength(filename).then(length => console.log(length));
```

## 阻塞 + 非阻塞

所以什么时候使用阻塞模型，什么时候使用非阻塞模型呢？

同时发起多个 **相互独立** 的 I/O，如果直接使用 `await` 阻塞调用，会导致 I/O **顺序执行**，效率不高。

``` javascript
const length1 = await getFileLength(filename1);
const length2 = await getFileLength(filename2);
console.log(Math.max(length1, length2));
```

可以改用 **非阻塞的发起 I/O** + **阻塞等待 I/O 执行结果** 的形式，使得 I/O **并发执行**，提高效率。

``` javascript
const lengthPromise1 = getFileLength(filename1);
const lengthPromise2 = getFileLength(filename2);
console.log(Math.max(await lengthPromise1, await lengthPromise2));
```

另外，Javascript 还提供了若干 Promise 相关封装，例如 [`Promise.all()`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise/all) 聚合多个 Promise 变为一个 Promise 对象，可用于化简上述并发代码。

``` javascript
const lengthPromises = Promise.all([
  getFileLength(filename1),
  getFileLength(filename2),
]);
console.log(Math.max(...(await lengthPromises)));
```

## 写在最后

阻塞和非阻塞模型并没有优劣之分，在正确、高效的前提下，最优雅的组织代码方式才是最合适的。

如果有什么问题，**欢迎交流**。😄

Delivered under MIT License &copy; 2023, BOT Man
