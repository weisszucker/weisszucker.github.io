# 深入 C++ 元组 (tuple) 实现

> BOT Man, 2017/12/1 -> 2020/11/3
>
> 分享 C++ 元组 (tuple) 的实现方式

[slide-mode]

---

## 主要内容

- [sec|什么是 `tuple`]
- [sec|存储实现]
- [sec|构造函数]
- [sec|`tuple_element` & `get`]
- [sec|更多内容]

---

## 什么是 `tuple`

``` cpp
using Person = tuple<std::string, char, int>;
Person john { "John"s, 'M', 21 };
Person jess { "Jess"s, 'F', 19 };
Person jack = make_tuple("Jack"s, 'M', 20);

std::string john_name = get<0>(john);
int jess_age = get<int>(jess);
char gender_jack;
tie(ignore, gender_jack, ignore) = jack;
```

---

## 什么是 `tuple`

``` cpp
std::set<Person> group { john, jess, jack };

using Hobby = tuple<std::string, int>;
Hobby kongfu { "Kong Fu", 2 };  // implicit

// Select and Join from 2 tables :-)
//   tuple < name, gender, age, hobby, score >
//   tuple { "Johh"s, 'M', 21, "Kong Fu"s, 2 }
auto john_hobby = tuple_cat(john, kongfu);
```

## 什么是 `tuple`

> Reference: https://github.com/BOT-Man-JL/ORM-Lite

``` cpp
// ORM-Lite
auto usersOrderList = mapper.Query(userModel)
  .Join(userModel,
        field(userModel.user_id) ==
        field(orderModel.user_id)
  ).ToList();

// SELECT * FROM UserModel
// JOIN OrderModel
// ON UserModel.user_id = OrderModel.user_id
```

---

## 存储实现

``` cpp
template <typename... Types>
class tuple : Types... {};
```

- 错误：`tuple<int> : int`

---

## 存储实现

``` cpp
template <typename T>
struct _t_leaf { T _val; };

template <typename... Types>
class tuple : _t_leaf<Types>... {};
```

- 错误：`tuple<int, int>`
- 错误：`tuple<int, double>` == `tuple<double, int>`

---

## 存储实现

``` cpp
template <> class tuple<> {};

template <typename Head, typename... Tails>
class tuple<Head, Tails...> {
  Head _head;
  tuple<Tails...> _tails;
};
```

- `sizeof(tuple<int>) >= sizeof(int) + 1`
- 阻止 [EBO _(Empty Base Optimization)_](https://en.cppreference.com/w/cpp/language/ebo)

---

## 存储实现

``` cpp
template <> class tuple<> {};

template <typename Head, typename... Tails>
class tuple<Head, Tails...>
  : tuple<Tails...> { Head _head; };
```

- `sizeof(tuple<int, Empty>) >= sizeof(int) + 1`
- 但 C++ 20 可用 [`[[no_unique_address]]`](https://en.cppreference.com/w/cpp/language/attributes/no_unique_address) 代替 EBO
- 参考：[msvc](https://github.com/microsoft/STL/blob/39eb812426167fc7955005b53b28d696c50e8b61/stl/inc/tuple#L238)

---

## 存储实现

``` cpp
template <size_t, typename T,
          bool = std::is_empty_v<T> &&
                 !std::is_final_v<T>>
struct _t_leaf { T _val; };

template <size_t I, typename T>
struct _t_leaf<I, T, false> { T _val; };

template <size_t I, typename T>
struct _t_leaf<I, T, true> : private T {};
```

- 参考：[libc++](https://github.com/llvm/llvm-project/blob/691eb814c1ae38d5015bf070dfed3fd54d542582/libcxx/include/tuple#L167) / [libstdc++](https://github.com/gcc-mirror/gcc/blob/f0c0f124ebe28b71abccbd7247678c9ac608b649/libstdc%2B%2B-v3/include/std/tuple#L72)

---

## 存储实现

``` cpp
template <typename S, typename... Ts>
struct _t_impl;

template <size_t... Is, typename... Ts>
struct _t_impl<std::index_sequence<Is...>,
               Ts...>
  : _t_leaf<Is, Ts>... {};

template <typename... Ts>
class tuple : _t_impl<
  std::make_index_sequence<sizeof...(Ts)>,
  Ts...> {};
```

- 参考：[libc++](https://github.com/llvm/llvm-project/blob/691eb814c1ae38d5015bf070dfed3fd54d542582/libcxx/include/tuple#L479)

---

## 存储实现

``` cpp
template <size_t Idx, typename... Ts>
struct _t_impl;

template <size_t Idx>
struct _t_impl<Idx> {};

template <size_t Idx, typename Head,
          typename... Tails>
struct _t_impl<Idx, Head, Tails...>
  : _t_impl<Idx + 1, Tails...>,
    _t_leaf<Idx, Head> {};

template <typename... Ts>
class tuple : _t_impl<0, Ts...> {};
```

- 参考：[libstdc++](https://github.com/gcc-mirror/gcc/blob/f0c0f124ebe28b71abccbd7247678c9ac608b649/libstdc%2B%2B-v3/include/std/tuple#L244)

---

## 构造函数

``` cpp
// default ctor
tuple();

// value direct ctor
tuple(const Head&, const Tails&...);

// value convert ctor
template <typename T, typename... Ts>
tuple(T&& arg, Ts&& ...args);

// tuple convert ctor
template <typename... Rhs>
tuple(const tuple<Rhs...>& rhs);
template <typename... Rhs>
tuple(tuple<Rhs...>&& rhs);

// copy/move ctor
tuple(const tuple&);
tuple(tuple&&);
```

---

## 构造函数

``` cpp
template <typename T, typename... Ts>
tuple(T&& arg, Ts&& ...args) :
    Tail(std::forward<Ts>(args)...),
    _val(std::forward<T>(arg)) {}

template <typename... Rhs>
tuple(tuple<Rhs...>&& rhs) :
    Tail(std::move(rhs._tail())),
    _val(std::move(rhs._head())) {}
```

---

## 构造函数

``` cpp
// ambiguous:
//   tuple<int> { int }
//   tuple<int> { tuple<int> }

tuple<int> t(tuple<int> { 1 });
```

- 引入模板匹配错误 `SFINAE` + `is_convertible`
- 值转换方式声明为 `explicit`

---

## 构造函数

``` cpp
// only check T & Head here

template <typename T, typename... Ts,
    typename = std::enable_if_t<
        std::is_convertible_v<T, Head>
    >
> explicit tuple(T&& arg, Ts&& ...args) :
    Tail(std::forward<Ts>(args)...),
    _val(std::forward<T>(arg)) {}
```

---

## `tuple_element` & `get`

``` cpp
template <size_t I, typename Tuple>
struct tuple_element;

template <size_t I, typename... Ts>
tuple_element<I, tuple<Ts...>>::type& get(
    tuple<Ts...>&);
// (const Tuple&) -> const Elem&
// (Tuple&&) -> Elem&&
// (const Tuple&&) -> const Elem&&
```

---

## `tuple_element` & `get`

``` cpp
template<size_t I, typename T, typename... Ts>
struct tuple_element<I, tuple<T, Ts...>>
    : tuple_element<I - 1, tuple<Ts...>> {};

template <typename T, typename... Ts>
struct tuple_element<0, tuple<T, Ts...>> {
  using type = T;
  using _tuple = tuple<T, Ts...>;
};
```

---

## `tuple_element` & `get`

``` cpp
template <size_t I, typename...Ts>
tuple_element<I, tuple<Ts...>>::type& get(
  tuple<Ts...>& t) {
  // resolve to base, and return head value
  return ((tuple_element<I, tuple<Ts...>>::
           _tuple&) t)._val;
}
```

---

## 更多内容

- `tuple_size`
- `operator==` / `operator<`
- `get` (by type)
- `swap`
- `make_tuple` / `tie` / `forward_as_tuple` / `tuple_cat`

#### 感悟

- 严谨的推导
- 语言工具的最小正交

---

## Q & A

<br/>
<br/>
<br/>

[align-right]

# 谢谢 🙂

---
