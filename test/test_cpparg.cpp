//
// cpparg - C++ parse argv
//
// Copyright 2025 Joergen Ibsen
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// SPDX-License-Identifier: MIT-0
//

#include "cpparg.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "catch.hpp"

auto default_parser = [] {
	cpparg::OptionParser parser;

	parser.add_option("n", "noarg",  "",      "option with no argument")
	      .add_option("o", "optarg", "[ARG]", "option with optional argument")
	      .add_option("r", "reqarg", "ARG",   "option with required argument");

	return parser;
}();

TEST_CASE("app only", "[cpparg]") {
	std::array args = {
		"app"
	};

	auto result = default_parser.parse_argv(args.size(), args.data());

	REQUIRE(result.has_value());

	REQUIRE(result->get_parsed_options().size() == 0);

	REQUIRE(result->get_positional_arguments().size() == 0);
}

TEST_CASE("dash", "[cpparg]") {
	std::array args = {
		"app", "-"
	};

	auto result = default_parser.parse_argv(args.size(), args.data());

	REQUIRE(result.has_value());

	REQUIRE(result->get_parsed_options().size() == 0);

	REQUIRE(result->get_positional_arguments().size() == 1);
	REQUIRE(result->get_positional_arguments().front() == "-");
}

TEST_CASE("double dash", "[cpparg]") {
	SECTION("only") {
		std::array args = {
			"app", "--"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("first") {
		std::array args = {
			"app", "--", "-n"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 1);
		REQUIRE(result->get_positional_arguments().front() == "-n");
	}

	SECTION("last") {
		std::array args = {
			"app", "-n", "--"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "noarg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("as arg") {
		std::array args = {
			"app", "-r", "--", "foo"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "--");

		REQUIRE(result->get_positional_arguments().size() == 1);
		REQUIRE(result->get_positional_arguments().front() == "foo");
	}
}

TEST_CASE("nonoption", "[cpparg]") {
	std::array args = {
		"app", "foo"
	};

	auto result = default_parser.parse_argv(args.size(), args.data());

	REQUIRE(result.has_value());

	REQUIRE(result->get_parsed_options().size() == 0);

	REQUIRE(result->get_positional_arguments().size() == 1);
	REQUIRE(result->get_positional_arguments().front() == "foo");
}

TEST_CASE("no match", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-u"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(!result.has_value());

		REQUIRE(result.error().originating_arg == 1);
	}

	SECTION("long") {
		std::array args = {
			"app", "--unknown"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(!result.has_value());

		REQUIRE(result.error().originating_arg == 1);
	}
}

TEST_CASE("noarg", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-n"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "noarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("long") {
		std::array args = {
			"app", "--noarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "noarg");
		REQUIRE(result->get_parsed_options().front().count == 1);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("multiple") {
		std::array args = {
			"app", "--noarg", "--noarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "noarg");
		REQUIRE(result->get_parsed_options().front().count == 2);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}

TEST_CASE("optarg missing", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-o"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("long") {
		std::array args = {
			"app", "--optarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 0);

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}

TEST_CASE("optarg inline", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-oarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("long") {
		std::array args = {
			"app", "--optarg=arg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("multiple") {
		std::array args = {
			"app", "--optarg=arg1", "--optarg", "--optarg=arg2"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().count == 3);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 2);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg1");
		REQUIRE(result->get_parsed_options().front().arguments.back() == "arg2");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}

TEST_CASE("reqarg missing", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-r"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(!result.has_value());

		REQUIRE(result.error().originating_arg == 1);
	}

	SECTION("long") {
		std::array args = {
			"app", "--reqarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(!result.has_value());

		REQUIRE(result.error().originating_arg == 1);
	}
}

TEST_CASE("reqarg inline", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-rarg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("long") {
		std::array args = {
			"app", "--reqarg=arg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("multiple") {
		std::array args = {
			"app", "--reqarg=arg1", "--reqarg=arg2"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().count == 2);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 2);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg1");
		REQUIRE(result->get_parsed_options().front().arguments.back() == "arg2");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}

TEST_CASE("reqarg next", "[cpparg]") {
	SECTION("short") {
		std::array args = {
			"app", "-r", "arg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("long") {
		std::array args = {
			"app", "--reqarg", "arg"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("multiple") {
		std::array args = {
			"app", "--reqarg", "arg1", "--reqarg", "arg2"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().count == 2);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 2);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg1");
		REQUIRE(result->get_parsed_options().front().arguments.back() == "arg2");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}

TEST_CASE("alt argument syntax", "[cpparg]") {
	cpparg::OptionParser parser;

	parser.add_option("n", "noarg",  "",       "option with no argument")
	      .add_option("o", "optarg", "[=ARG]", "option with optional argument")
	      .add_option("r", "reqarg", "=ARG",   "option with required argument");

	SECTION("optarg") {
		std::array args = {
			"app", "-o", "-oarg1", "--optarg", "--optarg=arg2"
		};

		auto result = parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "optarg");
		REQUIRE(result->get_parsed_options().front().count == 4);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 2);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg1");
		REQUIRE(result->get_parsed_options().front().arguments.back() == "arg2");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}

	SECTION("reqarg") {
		std::array args = {
			"app", "-rarg1", "--reqarg=arg2"
		};

		auto result = parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 1);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().count == 2);
		REQUIRE(result->get_parsed_options().front().arguments.size() == 2);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "arg1");
		REQUIRE(result->get_parsed_options().front().arguments.back() == "arg2");

		REQUIRE(result->get_positional_arguments().size() == 0);
	}
}
