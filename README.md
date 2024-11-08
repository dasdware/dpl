# DPL - Dasd's Programming Language

```bash
# Prints "Hello from DPL!" to stdout
print("Hello from DPL!\n");
```

```bash
# Compile hello.dpl into bytecode
$ ./dplc hello.dpl

# Run the compiled program in the virtual machine
$ ./dpl hello.dplp
```

This is my own take on creating a programming language from scratch. I use it to explore a lot of interesting concepts:

- Parsing text files and creating tokens and syntax trees from it, including error handling.
- Exploring the implications of an expression-based programming language.
- Building a type system and binding and checking the expressions of the program.
- Designing a virtual machine and its instruction set.
- Compiling the program into a bytecode representation that is then run in the virtual machine.

> [!WARNING]
> This is a work in progress and subject to massive changes. Use at your own risk.

## Bootstrapping, building and running

The build process uses [nob.h](https://github.com/tsoding/musializer/blob/master/src/nob.h) (tested with Windwos and Linux):

### Windows build

```bash
# Bootstrap the build system
gcc -o nob.exe nob.c

# Build all executables, compile  and run the "Hello World" example
nob.exe build -- run examples\hello_world.dpl
```

### Linux build

```bash
# Bootstrap the build system
gcc -o nob nob.c

# Build all executables, compile and run the "Hello World" example
./nob build -- run ./examples/hello_world.dpl
```

## Language features

DPL is an expression based and statically typed programming language that is compiled into bytecode and then run in a virtual machine. This has the following implications:

- **Expression based** means that every construct in the language that you create is an expression that ultimately yields some kind of value. Even calling `print` functions will return the `String` value that has been printed.
- **Statically typed** means that every value in the program has a type that is known beforehand at compile time and will always stay the same at runtime.
- **Compiled into bytecode** means that a compiled DPL program is not a native executable. Instead, it contains instruction that can be run on a virtual stack machine.

### Types

Since all expressions yield a value, they also have a type. Types are considered compatible, if they have the same structure. If, for example, two object type definitions have the same structure (individual fields and their types), they are considered compatible even if they have different names.

#### Builtin Types

The following types are built directly into the language:

| Type      | Examples        | Description                                                                                                                                                                                                                        |
| --------- | --------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Number`  | `1`, `3.14159`  | Numeric values. Can have a fractional part.                                                                                                                                                                                        |
| `String`  | `""`, `"Foo"`   | A string of characters, delimited by double quotes. A string may contain escape sequences: `\"` represents a literal double quote, `\n` is a linebreak, `\r` a carriage return and `\t` a tab.                                     |
| `Boolean` | `true`, `false` | A boolean value. Can be either `true` or `false`.                                                                                                                                                                                  |
| `None`    | -               | A type representing an expression that yields no value. There is no way to produce a value of this type. This type is only temporary and used for loops since they can be run zero times and therefore could have no value at all. |

#### User-defined Types

User defined types can be declared in a DPL program. They are used to form more complex structures from basic types.

| Type   | Declaration example      | Value example  | Description                                                   |
| ------ | ------------------------ | -------------- | ------------------------------------------------------------- |
| Object | `[x: number, y: number]` | `[x: 1, y: 2]` | Objects can be used to structure data into more complex bits. |

### Function calls

```bash
# Print "foo"
print("foo");

# Also print "foo", but using method syntax
"foo".print();
```

DPL supports [Uniform function call syntax](https://en.wikipedia.org/wiki/Uniform_Function_Call_Syntax), which means functions can be called either as free standing functions or as methods of their first argument. Internally, the latter is handled by the `.` operator which transforms the function call appropriately.

All operators except for the logical ones are also translated into function calls. This makes it easy to declare mathematical operations on user defined types.

```bash
# The following expressions are the same, they even compile into the same bytecode
1 + 2 * 3;
add(1, multiply(2, 3));
1.add(2.multiply(3));
```

The following sections contain tables listing the function names the individual operators try to resolve to.

### Arithmetic operators

```bash
print(1 + 2 + 3); # 6
print(-(2 + 1.5) * 3/4); # -2.625
```

Arithmetic operators take up to two operands and yield a result of the same type.

| Operator     | Function   | Description                                        |
| ------------ | ---------- | -------------------------------------------------- |
| `+`          | `add`      | Adds the left and the right operand.               |
| `-` (binary) | `subtract` | Subtracts the right operand from the left operand. |
| `-` (unary)  | `negate`   | Negates the operand.                               |
| `*`          | `multiply` | Multiplies the left and the right operand.         |
| `/`          | `divide`   | Divides the left operand by the right operand.     |

### Comparison operators

```bash
print(1 < 3); # true
print((1 > 3) == (2 < 4)); # false
```

Comparison operators take two operands and yield a `Boolean` result.

| Operator | Function       | Description                                                                                          |
| -------- | -------------- | ---------------------------------------------------------------------------------------------------- |
| `<`      | `less`         | Yields `true`, if the left operand is less than the right operand, `false` otherwise.                |
| `<=`     | `lessEqual`    | Yields `true`, if the left operand is less than or equal to the right operand, `false` otherwise.    |
| `>`      | `greater`      | Yields `true`, if the left operand is greater than the right operand, `false` otherwise.             |
| `>=`     | `greaterEqual` | Yields `true`, if the left operand is greater than or equal to the right operand, `false` otherwise. |
| `==`     | `equal`        | Yields `true`, if the left operand is equal to the right operand, `false` otherwise.                 |
| `!=`     | `notEqual`     | Yields `true`, if the left operand is not equal to the right operand, `false` otherwise.             |

### Logical operators

```bash
print(!true); # false
print(true || false); # true
```

Logical operators combine two `Boolean` values and yield another `Boolean`. Since they might short circuit, they are not resolved as functions.

| Operator    | Function | Description                                                                      |
| ----------- | -------- | -------------------------------------------------------------------------------- |
| `!` (unary) | `-`      | Logical not. Yields `true`, if its operand is `false`, and vice versa.           |
| `&&`        | `-`      | Logical and. Yields `true`, if both operands are `true`, `false` otherwise.      |
| `\|\|`      | `-`      | Logical or. Yields `true`, if at least one operand is `true`, `false` otherwise. |

### Variables

```bash
var x := 2;
print(2 * x); # 4

x := 3;
print(2 * x); # 6
```

Variables are declared via the `var` keyword. After that follows the name of the variable, the assignment operator `:=` and an initializer expression. After that, the variable can be referred to by its name and reassigned.

It is also possible to declare the type the variable should have. If the initializer yields the wrong type, that is an compiler error:

```bash
var x: Number := "foo"; # Cannot assign expression of type `String`
                        # to variable `x` of type `Number`.
```

This is useful if the expression is more complex and you want to be sure that you get the correct type. If the type declaration is omitted, the variable type is inferred from the initializer.

Once variables are declared, their type is fixed and cannot change later. Therefore, it is also not possible to assign an expression of another type:

```bash
var x := 2;
x := "foo"; # Cannot assign expression of type `String`
            # to variable `x` of type `Number`.
```

### Constants

```bash
constant PI := 3.14159;
print(2 * x); # 6.28318
```

Constants are declared via the `constant` keyword. After that follows the name of the constant, the assignment operator `:=` and an initializer expression. After that, the constant can be referred to by its name. Constants are very similar to variables but cannot be changed after they have been declared:

```bash
constant PI := 3.14159;
PI := 6.28318; # Cannot assign to constant `PI`.
```

It is also possible to declare the type the constant should have. If the initializer yields the wrong type, that is an compiler error:

```bash
constant PI: Number := "foo"; # Cannot assign expression of type `String`
                              # to constant `PI` of type `Number`.
```

This is useful if the expression is more complex and you want to be sure that you get the correct type. If the type declaration is omitted, the constant type is inferred from the initializer.

### Conditionals

```bash
var x := 1;
var y := 2;
print(if (x > y) x else y); # y (2)
```

A conditional is DPL's version of branching and works like the [ternary operator of C/C++](https://en.wikipedia.org/wiki/Ternary_conditional_operator).

It has the following form:

```bash
if (<Condition>) <ThenClause> else <ElseClause>
```

`<Condition>` is a `Boolean` expression and `<ThenClause>` and `<ElseClause>` are two expressions of the same type. The expression yields the value of `<ThenClause>`, if `<Condition>` yields `true`, otherwise it yields the value of `<ElseClause>`.

It is an compiler error to have a type other then `Boolean` for the `<Condition>` expression:

```bash
print(if ("foo") 1 else 2); # Condition operand type `String` does not
                            # match type `Boolean`.
```

Likewise, it is an compiler error to have different types for `<ThenClause>` and `<ElseClause>`:

```bash
print(if (true) 2 else "foo"); # Types `Number` and `String` do not match
                               # in the conditional expression clauses.
```
