# DPL - Dasd's Programming Language

```bash filename="hello.dpl"
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

```cmd
REM Bootstrap the build system
gcc -o nob.exe nob.c

REM Build all executables, compile  and run the "Hello World" example
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

* **Expression based** means that every construct in the language that you create is an expression that ultimately yields some kind of value. Even calling `print` functions will return the `String` value that has been printed.
* **Statically typed** means that every value in the program has a type that is known beforehand at compile time and will always stay the same at runtime.
* **Compiled into bytecode** means that a compiled DPL program is not a native executable. Instead, it contains instruction that can be run on a virtual stack machine.
 
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

### Expressions

