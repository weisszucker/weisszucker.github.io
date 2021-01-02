__LINE__;;;

#define PP_REMOVE_PARENS(T) PP_REMOVE_PARENS_IMPL T
#define PP_REMOVE_PARENS_IMPL(...) __VA_ARGS__

#define FOO(A, B) int foo(A x, B y)
#define BAR(A, B) FOO(PP_REMOVE_PARENS(A), PP_REMOVE_PARENS(B))

FOO(bool, IntPair)                  // -> int foo(bool x, IntPair y)
BAR((bool), (std::pair<int, int>))  // -> int foo(bool x, std::pair<int, int> y)

__LINE__;;;

#define PP_COMMA() ,
#define PP_LPAREN() (
#define PP_RPAREN() )
#define PP_EMPTY()

__LINE__;;;
#undef FOO
#undef BAR

#define FOO(SYMBOL) foo_ ## SYMBOL
#define BAR() bar

FOO(bar)    // -> foo_bar
FOO(BAR())  // -> foo_BAR()

__LINE__;;;
#undef FOO

#define PP_CONCAT(A, B) PP_CONCAT_IMPL(A, B)
#define PP_CONCAT_IMPL(A, B) A##B

#define FOO(N) PP_CONCAT(foo_, N)

FOO(bar)    // -> foo_bar
FOO(BAR())  // -> foo_bar

__LINE__;;;

/*
PP_CONCAT(x PP_COMMA() y)  // too few arguments (before prescan)
PP_CONCAT(x, PP_COMMA())   // too many arguments (after prescan)
*/

__LINE__;;;

#define PP_INC(N) PP_CONCAT(PP_INC_, N)
#define PP_INC_0 1
#define PP_INC_1 2
// ...
#define PP_INC_254 255
#define PP_INC_255 256

#define PP_DEC(N) PP_CONCAT(PP_DEC_, N)
#define PP_DEC_256 255
#define PP_DEC_255 254
// ...
#define PP_DEC_2 1
#define PP_DEC_1 0

PP_INC(1)    // -> 2
PP_DEC(2)    // -> 1
PP_INC(256)  // -> PP_INC_256 (overflow)
PP_DEC(0)    // -> PP_DEC_0  (underflow)

__LINE__;;;

#define PP_NOT(N) PP_CONCAT(PP_NOT_, N)
#define PP_NOT_0 1
#define PP_NOT_1 0

#define PP_AND(A, B) PP_CONCAT(PP_AND_, PP_CONCAT(A, B))
#define PP_AND_00 0
#define PP_AND_01 0
#define PP_AND_10 0
#define PP_AND_11 1

PP_AND(PP_NOT(0), 1)  // -> 1
PP_AND(PP_NOT(2), 0)  // -> PP_AND_PP_NOT_20

__LINE__;;;

#define PP_BOOL(N) PP_CONCAT(PP_BOOL_, N)
#define PP_BOOL_0 0
#define PP_BOOL_1 1
#define PP_BOOL_2 1
// ...

PP_AND(PP_NOT(PP_BOOL(2)), PP_BOOL(0))  // -> 0
PP_NOT(PP_BOOL(1000))                   // -> PP_NOT_PP_BOOL_1000

__LINE__;;;

#define PP_IF(PRED, THEN, ELSE) PP_CONCAT(PP_IF_, PP_BOOL(PRED))(THEN, ELSE)
#define PP_IF_1(THEN, ELSE) THEN
#define PP_IF_0(THEN, ELSE) ELSE

#define DEC_SAFE(N) PP_IF(N, PP_DEC(N), 0)

DEC_SAFE(2)  // -> 1
DEC_SAFE(1)  // -> 0
DEC_SAFE(0)  // -> 0

__LINE__;;;

#define PP_COMMA_IF(N) PP_IF(N, PP_COMMA(), PP_EMPTY())

/*
PP_COMMA_IF(1)  // -> PP_IF(1, , , ) (too many arguments after prescan)
*/

__LINE__;;;
#undef PP_COMMA_IF

#define PP_COMMA_IF(N) PP_IF(N, PP_COMMA, PP_EMPTY)()

PP_COMMA_IF(0)  // (empty)
PP_COMMA_IF(1)  // -> ,
PP_COMMA_IF(2)  // -> ,

#define SURROUND(N) PP_IF(N, PP_LPAREN, [ PP_EMPTY)() \
                    N                                 \
                    PP_IF(N, PP_RPAREN, ] PP_EMPTY)()

SURROUND(0)  // -> [0]
SURROUND(1)  // -> (1)
SURROUND(2)  // -> (2)

__LINE__;;;

#define log(format, ...) printf("LOG: " format, __VA_ARGS__)

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world", );
log("hello world", );  // -> printf("LOG: hello world", );

__LINE__;;;
#undef log

#define log(format, ...) printf("LOG: " format, ## __VA_ARGS__)

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world");
log("hello world", );  // -> printf("LOG: hello world", );

__LINE__;;;
#undef log

#define log(format, ...) printf("LOG: " format __VA_OPT__(,) __VA_ARGS__)

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world");
log("hello world", );  // -> printf("LOG: hello world");

__LINE__;;;

#define PP_GET_N(N, ...) PP_CONCAT(PP_GET_N_, N)(__VA_ARGS__)
#define PP_GET_N_0(_0, ...) _0
#define PP_GET_N_1(_0, _1, ...) _1
#define PP_GET_N_2(_0, _1, _2, ...) _2
// ...
#define PP_GET_N_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8

PP_GET_N(0, foo, bar)  // -> foo
PP_GET_N(1, foo, bar)  // -> bar

__LINE__;;;

#define PP_GET_TUPLE(N, T) PP_GET_N(N, PP_REMOVE_PARENS(T))

PP_GET_TUPLE(0, (foo, bar))  // -> foo
PP_GET_TUPLE(1, (foo, bar))  // -> bar

__LINE__;;;
#undef FOO

#define FOO(P, T) PP_IF(P, PP_GET_TUPLE(1, T), PP_GET_TUPLE(0, T))

FOO(0, (foo, bar))  // -> foo
FOO(1, (foo, bar))  // -> bar
/*
FOO(0, (baz))       // -> PP_GET_N_1(baz) (too few arguments)
*/

__LINE__;;;
#undef FOO

#define FOO(P, T) PP_IF(P, PP_GET_N_1, PP_GET_N_0) T

FOO(0, (foo, bar))  // -> foo
FOO(1, (foo, bar))  // -> bar
FOO(0, (baz))       // -> baz

__LINE__;;;

#define PP_IS_EMPTY(...)                                      \
  PP_AND(PP_AND(PP_NOT(PP_HAS_COMMA(__VA_ARGS__)),            \
                PP_NOT(PP_HAS_COMMA(__VA_ARGS__()))),         \
         PP_AND(PP_NOT(PP_HAS_COMMA(PP_COMMA_V __VA_ARGS__)), \
                PP_HAS_COMMA(PP_COMMA_V __VA_ARGS__())))
#define PP_HAS_COMMA(...) PP_GET_N_8(__VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 0, 0)
#define PP_COMMA_V(...) ,

PP_IS_EMPTY()          // -> 1
PP_IS_EMPTY(foo)       // -> 0
PP_IS_EMPTY(foo())     // -> 0
PP_IS_EMPTY(())        // -> 0
PP_IS_EMPTY(()foo)     // -> 0
PP_IS_EMPTY(PP_EMPTY)  // -> 0
PP_IS_EMPTY(PP_COMMA)  // -> 0
PP_IS_EMPTY(, )        // -> 0
PP_IS_EMPTY(foo, bar)  // -> 0
PP_IS_EMPTY(, , , )    // -> 0

__LINE__;;;
#undef log

#define PP_VA_OPT_COMMA(...) PP_COMMA_IF(PP_NOT(PP_IS_EMPTY(__VA_ARGS__)))
#define log(format, ...) \
  printf("LOG: " format PP_VA_OPT_COMMA(__VA_ARGS__) __VA_ARGS__)

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world");
log("hello world", );  // -> printf("LOG: hello world");

__LINE__;;;

#define PP_NARG(...)                                                           \
  PP_GET_N(8, __VA_ARGS__ PP_VA_OPT_COMMA(__VA_ARGS__) 8, 7, 6, 5, 4, 3, 2, 1, \
           0)

PP_NARG()          // -> 0
PP_NARG(foo)       // -> 1
PP_NARG(foo())     // -> 1
PP_NARG(())        // -> 1
PP_NARG(()foo)     // -> 1
PP_NARG(PP_EMPTY)  // -> 1
PP_NARG(PP_COMMA)  // -> 1
PP_NARG(, )        // -> 2
PP_NARG(foo, bar)  // -> 2
PP_NARG(, , , )    // -> 4

__LINE__;;;

PP_GET_N(0, 1 PP_COMMA() 2)  // -> 1
PP_GET_N_0(1 PP_COMMA() 2)   // -> 1 , 2

__LINE__;;;

#define PP_FOR_EACH(DO, CTX, ...) \
  PP_CONCAT(PP_FOR_EACH_, PP_NARG(__VA_ARGS__))(DO, CTX, 0, __VA_ARGS__)
#define PP_FOR_EACH_0(DO, CTX, IDX, ...)
#define PP_FOR_EACH_1(DO, CTX, IDX, VAR, ...) DO(VAR, IDX, CTX)
#define PP_FOR_EACH_2(DO, CTX, IDX, VAR, ...) \
  DO(VAR, IDX, CTX)                           \
  PP_FOR_EACH_1(DO, CTX, PP_INC(IDX), __VA_ARGS__)
#define PP_FOR_EACH_3(DO, CTX, IDX, VAR, ...) \
  DO(VAR, IDX, CTX)                           \
  PP_FOR_EACH_2(DO, CTX, PP_INC(IDX), __VA_ARGS__)
// ...

#define DO_EACH(VAR, IDX, CTX) PP_COMMA_IF(IDX) CTX VAR

PP_FOR_EACH(DO_EACH, void, )        // (empty)
PP_FOR_EACH(DO_EACH, int, a, b, c)  // -> int a, int b, int c
PP_FOR_EACH(DO_EACH, bool, x)       // -> bool x

__LINE__;;;
#undef log

#define log(format, ...) \
  printf("LOG: " format PP_FOR_EACH(DO_EACH, , __VA_ARGS__))

log("%d%f", 1, .2);    // -> printf("LOG: %d%f", 1, .2);
log("hello world");    // -> printf("LOG: hello world");
log("hello world", );  // -> printf("LOG: hello world");

__LINE__;;;

#define PP_IS_SYMBOL(PREFIX, SYMBOL) PP_IS_EMPTY(PP_CONCAT(PREFIX, SYMBOL))
#define IS_VOID_void

PP_IS_SYMBOL(IS_VOID_, void)            // -> 1
PP_IS_SYMBOL(IS_VOID_, )                // -> 0
PP_IS_SYMBOL(IS_VOID_, int)             // -> 0
PP_IS_SYMBOL(IS_VOID_, void*)           // -> 0
PP_IS_SYMBOL(IS_VOID_, void x)          // -> 0
PP_IS_SYMBOL(IS_VOID_, void(int, int))  // -> 0

__LINE__;;;

#define PP_EMPTY_V(...)
#define PP_IS_PARENS(SYMBOL) PP_IS_EMPTY(PP_EMPTY_V SYMBOL)

PP_IS_PARENS()                // -> 0
PP_IS_PARENS(foo)             // -> 0
PP_IS_PARENS(foo())           // -> 0
PP_IS_PARENS(()foo)           // -> 0
PP_IS_PARENS(())              // -> 1
PP_IS_PARENS((foo))           // -> 1
PP_IS_PARENS(((), foo, bar))  // -> 1

__LINE__;;;
#undef FOO
#undef BAR

#define PP_IDENTITY(N) N
#define TRY_REMOVE_PARENS(T) \
  PP_IF(PP_IS_PARENS(T), PP_REMOVE_PARENS, PP_IDENTITY)(T)

#define FOO(A, B) int foo(A x, B y)
#define BAR(A, B) FOO(TRY_REMOVE_PARENS(A), TRY_REMOVE_PARENS(B))

FOO(bool, IntPair)                // -> int foo(bool x, IntPair y)
BAR(bool, IntPair)                // -> int foo(bool x, IntPair y)
BAR(bool, (std::pair<int, int>))  // -> int foo(bool x, std::pair<int, int> y)

__LINE__;;;

#define OUTER(N, T) PP_FOR_EACH(DO_EACH_1, N, PP_REMOVE_PARENS(T))
#define DO_EACH_1(VAR, IDX, CTX)                   \
  PP_FOR_EACH(DO_EACH_2, CTX.PP_GET_TUPLE(0, VAR), \
              PP_REMOVE_PARENS(PP_GET_TUPLE(1, VAR)))
#define DO_EACH_2(VAR, IDX, CTX) CTX .VAR = VAR;

// -> PP_FOR_EACH(DO_EACH_2, obj.x, x1, x2) PP_FOR_EACH(DO_EACH_2, obj.y, y1)
OUTER(obj, ((x, (x1, x2)), (y, (y1))))

__LINE__;;;
#undef OUTER
#undef DO_EACH_1
#undef DO_EACH_2

#define OUTER(N, T) PP_FOR_EACH(DO_EACH_1, N, PP_REMOVE_PARENS(T))
#define DO_EACH_1(VAR, IDX, CTX) CTX.VAR;
#define INNER(N, T) PP_FOR_EACH(DO_EACH_2, N, PP_REMOVE_PARENS(T))
#define DO_EACH_2(VAR, IDX, CTX) PP_COMMA_IF(IDX) CTX .VAR = VAR

// -> obj.x.x1 = x1; obj.x.x2 = x2; obj.y.y1 = y1;
OUTER(obj, (INNER(x, (x1, x2)), INNER(y, (y1))))

__LINE__;;;
#undef OUTER
#undef DO_EACH_1
#undef DO_EACH_2

#define PP_FOR_EACH_INNER(DO, CTX, ...)               \
  PP_CONCAT(PP_FOR_EACH_INNER_, PP_NARG(__VA_ARGS__)) \
  (DO, CTX, 0, __VA_ARGS__)
#define PP_FOR_EACH_INNER_0(DO, CTX, IDX, ...)
#define PP_FOR_EACH_INNER_1(DO, CTX, IDX, VAR, ...) DO(VAR, IDX, CTX)
#define PP_FOR_EACH_INNER_2(DO, CTX, IDX, VAR, ...) \
  DO(VAR, IDX, CTX)                                 \
  PP_FOR_EACH_INNER_1(DO, CTX, PP_INC(IDX), __VA_ARGS__)
// ...

#define OUTER(N, T) PP_FOR_EACH(DO_EACH_1, N, PP_REMOVE_PARENS(T))
#define DO_EACH_1(VAR, IDX, CTX)                         \
  PP_FOR_EACH_INNER(DO_EACH_2, CTX.PP_GET_TUPLE(0, VAR), \
                    PP_REMOVE_PARENS(PP_GET_TUPLE(1, VAR)))
#define DO_EACH_2(VAR, IDX, CTX) CTX .VAR = VAR;

// -> obj.x.x1 = x1; obj.x.x2 = x2; obj.y.y1 = y1;
OUTER(obj, ((x, (x1, x2)), (y, (y1))))

__LINE__;;;

#define PP_WHILE PP_WHILE_1
#define PP_WHILE_1(PRED, OP, VAL)              \
  PP_IF(PRED(VAL), PP_WHILE_2, VAL PP_EMPTY_V) \
  (PRED, OP, PP_IF(PRED(VAL), OP, PP_EMPTY_V)(VAL))
#define PP_WHILE_2(PRED, OP, VAL)              \
  PP_IF(PRED(VAL), PP_WHILE_3, VAL PP_EMPTY_V) \
  (PRED, OP, PP_IF(PRED(VAL), OP, PP_EMPTY_V)(VAL))
#define PP_WHILE_3(PRED, OP, VAL)              \
  PP_IF(PRED(VAL), PP_WHILE_4, VAL PP_EMPTY_V) \
  (PRED, OP, PP_IF(PRED(VAL), OP, PP_EMPTY_V)(VAL))
#define PP_WHILE_4(PRED, OP, VAL)              \
  PP_IF(PRED(VAL), PP_WHILE_5, VAL PP_EMPTY_V) \
  (PRED, OP, PP_IF(PRED(VAL), OP, PP_EMPTY_V)(VAL))
// ...

#define PRED(VAL) PP_GET_TUPLE(1, VAL)
#define OP(VAL) \
  (PP_GET_TUPLE(0, VAL) + PP_GET_TUPLE(1, VAL), PP_DEC(PP_GET_TUPLE(1, VAL)))

PP_GET_TUPLE(0, PP_WHILE(PRED, OP, (x, 2)))  // -> x + 2 + 1

__LINE__;;;
#undef PP_WHILE

#define PP_WHILE PP_CONCAT(PP_WHILE_, PP_AUTO_DIM(PP_WHILE_CHECK))

#define PP_AUTO_DIM(CHECK) \
  PP_IF(CHECK(2), PP_AUTO_DIM_12, PP_AUTO_DIM_34)(CHECK)
#define PP_AUTO_DIM_12(CHECK) PP_IF(CHECK(1), 1, 2)
#define PP_AUTO_DIM_34(CHECK) PP_IF(CHECK(3), 3, 4)

#define PP_WHILE_CHECK(N) \
  PP_CONCAT(PP_WHILE_CHECK_, PP_WHILE_##N(0 PP_EMPTY_V, , 1))
#define PP_WHILE_CHECK_1 1
#define PP_WHILE_CHECK_PP_WHILE_1(PRED, OP, VAL) 0
#define PP_WHILE_CHECK_PP_WHILE_2(PRED, OP, VAL) 0
#define PP_WHILE_CHECK_PP_WHILE_3(PRED, OP, VAL) 0
#define PP_WHILE_CHECK_PP_WHILE_4(PRED, OP, VAL) 0
// ...

#define OP_1(VAL)                                                        \
  (PP_GET_TUPLE(0, PP_WHILE(PRED, OP_2,                                  \
                            (PP_GET_TUPLE(0, VAL), PP_GET_TUPLE(1, VAL), \
                             PP_GET_TUPLE(1, VAL)))),                    \
   PP_DEC(PP_GET_TUPLE(1, VAL)))
#define OP_2(VAL)                                                      \
  (PP_GET_TUPLE(0, VAL) + PP_GET_TUPLE(2, VAL) * PP_GET_TUPLE(1, VAL), \
   PP_DEC(PP_GET_TUPLE(1, VAL)), PP_GET_TUPLE(2, VAL))

PP_GET_TUPLE(0, PP_WHILE(PRED, OP_1, (x, 2)))  // -> x + 2 * 2 + 2 * 1 + 1 * 1

__LINE__;;;

#define PP_WHILE_RECURSIVE(PRED, OP, VAL)          \
  PP_IF(PRED(VAL), PP_WHILE_DEFER, VAL PP_EMPTY_V) \
  (PRED, OP, PP_IF(PRED(VAL), OP, PP_EMPTY_V)(VAL))
#define PP_WHILE_INDIRECT() PP_WHILE_RECURSIVE
#define PP_WHILE_DEFER PP_WHILE_INDIRECT PP_EMPTY PP_EMPTY PP_EMPTY()()()()

// -> PP_WHILE_INDIRECT PP_EMPTY PP_EMPTY()()()
PP_WHILE_DEFER
// -> PP_WHILE_INDIRECT PP_EMPTY()()
PP_IDENTITY(PP_WHILE_DEFER)
// -> PP_WHILE_INDIRECT ()
PP_IF(1, PP_WHILE_DEFER, )
// -> PP_WHILE_RECURSIVE
PP_IDENTITY(PP_IF(1, PP_WHILE_DEFER, ))

__LINE__;;;

#define PP_EXPAND(...) __VA_ARGS__

// -> PP_WHILE_INDIRECT() (PRED, OP, (x + 2, 1))
PP_WHILE_RECURSIVE(PRED, OP, (x, 2))
// -> PP_WHILE_INDIRECT() (PRED, OP, (x + 2 + 1, 0))
PP_EXPAND(PP_WHILE_RECURSIVE(PRED, OP, (x, 2)))
// -> (x + 2 + 1, 0)
PP_EXPAND(PP_EXPAND(PP_WHILE_RECURSIVE(PRED, OP, (x, 2))))

__LINE__;;;

#define PP_ADD(X, Y) PP_GET_TUPLE(0, PP_WHILE(PP_ADD_P, PP_ADD_O, (X, Y)))
#define PP_ADD_P(V) PP_GET_TUPLE(1, V)
#define PP_ADD_O(V) (PP_INC(PP_GET_TUPLE(0, V)), PP_DEC(PP_GET_TUPLE(1, V)))

PP_ADD(0, 2)  // -> 2
PP_ADD(1, 1)  // -> 2
PP_ADD(2, 0)  // -> 2

__LINE__;;;

#define PP_SUB(X, Y) PP_GET_TUPLE(0, PP_WHILE(PP_SUB_P, PP_SUB_O, (X, Y)))
#define PP_SUB_P(V) PP_GET_TUPLE(1, V)
#define PP_SUB_O(V) (PP_DEC(PP_GET_TUPLE(0, V)), PP_DEC(PP_GET_TUPLE(1, V)))

PP_SUB(2, 2)  // -> 0
PP_SUB(2, 1)  // -> 1
PP_SUB(2, 0)  // -> 2

__LINE__;;;

#define PP_MUL(X, Y) PP_GET_TUPLE(0, PP_WHILE(PP_MUL_P, PP_MUL_O, (0, X, Y)))
#define PP_MUL_P(V) PP_GET_TUPLE(2, V)
#define PP_MUL_O(V)                                                    \
  (PP_ADD(PP_GET_TUPLE(0, V), PP_GET_TUPLE(1, V)), PP_GET_TUPLE(1, V), \
   PP_DEC(PP_GET_TUPLE(2, V)))

PP_MUL(1, 2)  // -> 2
PP_MUL(2, 1)  // -> 2
PP_MUL(2, 0)  // -> 0
PP_MUL(0, 2)  // -> 0

__LINE__;;;

#define PP_CMP(X, Y) PP_WHILE(PP_CMP_P, PP_CMP_O, (X, Y))
#define PP_CMP_P(V) \
  PP_AND(PP_BOOL(PP_GET_TUPLE(0, V)), PP_BOOL(PP_GET_TUPLE(1, V)))
#define PP_CMP_O(V) (PP_DEC(PP_GET_TUPLE(0, V)), PP_DEC(PP_GET_TUPLE(1, V)))

#define PP_EQUAL(X, Y) PP_IDENTITY(PP_EQUAL_IMPL PP_CMP(X, Y))
#define PP_EQUAL_IMPL(RX, RY) PP_AND(PP_NOT(PP_BOOL(RX)), PP_NOT(PP_BOOL(RY)))

PP_EQUAL(1, 2)  // -> 0
PP_EQUAL(1, 1)  // -> 1
PP_EQUAL(1, 0)  // -> 0

__LINE__;;;

#define PP_LESS(X, Y) PP_IDENTITY(PP_LESS_IMPL PP_CMP(X, Y))
#define PP_LESS_IMPL(RX, RY) PP_AND(PP_NOT(PP_BOOL(RX)), PP_BOOL(RY))

PP_LESS(0, 1)  // -> 1
PP_LESS(1, 2)  // -> 1
PP_LESS(1, 1)  // -> 0
PP_LESS(2, 1)  // -> 0

__LINE__;;;

#define PP_MIN(X, Y) PP_IF(PP_LESS(X, Y), X, Y)
#define PP_MAX(X, Y) PP_IF(PP_LESS(X, Y), Y, X)

PP_MIN(0, 1)  // -> 0
PP_MIN(1, 1)  // -> 1
PP_MAX(1, 2)  // -> 2
PP_MAX(2, 1)  // -> 2

__LINE__;;;

#define PP_DIV_BASE(X, Y) PP_WHILE(PP_DIV_BASE_P, PP_DIV_BASE_O, (0, X, Y))
#define PP_DIV_BASE_P(V) \
  PP_NOT(PP_LESS(PP_GET_TUPLE(1, V), PP_GET_TUPLE(2, V)))  // X >= Y
#define PP_DIV_BASE_O(V)                                                       \
  (PP_INC(PP_GET_TUPLE(0, V)), PP_SUB(PP_GET_TUPLE(1, V), PP_GET_TUPLE(2, V)), \
   PP_GET_TUPLE(2, V))

#define PP_DIV(X, Y) PP_GET_TUPLE(0, PP_DIV_BASE(X, Y))
#define PP_MOD(X, Y) PP_GET_TUPLE(1, PP_DIV_BASE(X, Y))

PP_DIV(2, 1), PP_MOD(2, 1)  // -> 2, 0
PP_DIV(1, 1), PP_MOD(1, 1)  // -> 1, 0
PP_DIV(0, 1), PP_MOD(0, 1)  // -> 0, 0
PP_DIV(1, 2), PP_MOD(1, 2)  // -> 0, 1

__LINE__;;;

template <typename T, bool Condition = std::is_class_v<T>>
using maybe_cref_t =
    std::conditional_t<Condition,
                       std::add_lvalue_reference_t<std::add_const_t<T>>,
                       T>;

#define MAKE_ARG(TYPE, IDX, _) \
  PP_COMMA_IF(IDX) maybe_cref_t<TYPE> PP_CONCAT(v, IDX)

// -> void foo(maybe_cref_t<int> v0, maybe_cref_t<std::string> v1);
// -> void foo(int v0, const std::string& v1);
void foo(PP_FOR_EACH(MAKE_ARG, , int, std::string));
