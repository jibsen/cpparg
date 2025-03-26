# `cpparg` - C++ parser for `argv`

## About

`cpparg` is a parser for command-line options. It is a header-only library,
written in C++23.

It is made available under the [MIT No Attribution License](LICENSE) (MIT-0).

## Usage

Parsing command-line options is handled by `cpparg::OptionParser`.

### Declaring Options

You use the `add_option()` function to add options, it takes four strings
as arguments:

  - short flag as a string containing one character, for instance `"s"`
  - long flag, for instance `"size"`
  - display name of the argument to the option (if any), for instance
    `"SIZE"`, or `"[SIZE]"` if the argument is optional
  - description of the option

The long flag is used to identify the option. So for instance

```cpp
    cpparg::OptionParser parser;

    parser.add_option("h", "help",     "",      "print this help and exit")
          .add_option("r", "required", "ARG",   "option with required argument")
          .add_option("o", "optional", "[ARG]", "option with optional argument");
```

will create an `OptionParser` that handles 3 options named `help`,
`required`, and `optional`. The `help` option takes no arguments, the
`required` option requires an argument, and the `optional` option may
take an argument if present.

The short flag may be empty, in which case the option only supports the
long flag.

Likewise, the long flag may be empty, which results in a long flag with
the name of the short flag being created (because it is used to identify
the option), but not shown in the help.

### Printing Help

`OptionParser` has a function `get_option_help()` that returns a string
with formatted help for the options it supports. For instance our example
above would return a string containing

```
  -h, --help            print this help and exit
  -r, --required ARG    option with required argument
  -o, --optional[=ARG]  option with optional argument
```

If you prefer required arguments be printed as part of the option, you can
add a `=` to the string, as in `"=ARG"`.

You can group options in the help output by inserting a newline at the end
of the preceeding option description.

`get_option_help()` takes the desired line width as an optional parameter.
If you supply a line width, it will attempt to break the option description
at spaces.

`cpparg` only handles this part of the help because there are choices for
how to display usage that are difficult to combine. Instead you can print
your own usage instructions and include this option help string.

### Parsing `argv`

Parsing is done with the `parse_argv()` function, which returns a
`std::expected` containing either a `cpparg::ParseResult` or a
`cpparg::ParseError`.

`ParseError` is a struct containing two members, `originating_arg` which is
the index of the element of `argv` that contained the error, and `what`,
which is a string describing the error.

```cpp
    auto result = parser.parse_argv(argc, argv);

    if (!result) {
        std::println(std::cerr, "cpparg: {}", result.error().what);

        return EXIT_FAILURE;
    }
```

### Using `ParseResult`

`cpparg::ParseResult` has a number of functions:

  - `count("name")` returns the number of times option `name` occured
  - `get_last_argument_for_option("name")` returns an optional containing
    the last option argument given for option `name`, if any
  - `get_arguments_for_option("name")` returns a reference to a vector
    containing all the arguments that were provided for the `name` option
  - `get_parsed_options()` returns a reference to a vector of
    `cpparg::ParsedOption`
  - `get_positional_arguments()` returns a reference to a vector containing
    all the positional arguments

A `cpparg::ParsedOption` is a struct containing three members, `name` which
is the name of the option, `count` which is how many times it occured, and
`arguments`, which is a vector of arguments supplied to the option.

Option arguments are provided as `std::string`.

```cpp
    // If the `help` option appeared, show help
    if (result->count("help")) {
        std::println(
            "usage: cpparg_example [options] POSITIONAL_ARG...\n"
            "\n"
            "Example program for cpparg.\n"
            "\n"
            "{}",
            parser.get_option_help()
        );

        return EXIT_SUCCESS;
    }

    // Check that at least one positional argument was supplied
    if (result->get_positional_arguments().size() < 1) {
        std::println(std::cerr, "usage: cpparg_example [options] POSITIONAL_ARG...\n");

        return EXIT_FAILURE
    }

    // Iterate over options
    for (const auto &option : result->get_parsed_options()) {
        std::print("option '{}' appeared {} time(s)", option.name, option.count);

        if (!option.arguments.empty()) {
            std::print(" with argument(s):");

            for (const auto &argument : option.arguments) {
                std::print(" '{}'", argument);
            }
        }

        std::print("\n");
    }

    // Iterate over positional arguments
    for (const auto &argument : result->get_positional_arguments()) {
        std::println("positional argument '{}'", argument);
    }
```

## Known Limitations

`cpparg` parses all options at once, you cannot know the ordering between
options and positional arguments.

Only exact matches for long options are supported. You cannot use an
unambiguous prefix of a long option like with `getopt_long` or `parg`.

`cpparg` provides option arguments as `std::string`, you have to do your
own conversion if needed.

## Alternatives

Please see the README file for [`parg`][parg] for a list of alternatives.

[parg]: https://github.com/jibsen/parg
