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

	REQUIRE(result->get_arguments_for_option("optarg").empty());

	REQUIRE(!result->get_last_argument_for_option("optarg").has_value());

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
			"app", "-r", "--", "-n"
		};

		auto result = default_parser.parse_argv(args.size(), args.data());

		REQUIRE(result.has_value());

		REQUIRE(result->get_parsed_options().size() == 2);
		REQUIRE(result->get_parsed_options().front().name == "reqarg");
		REQUIRE(result->get_parsed_options().front().arguments.size() == 1);
		REQUIRE(result->get_parsed_options().front().arguments.front() == "--");
		REQUIRE(result->get_parsed_options().back().name == "noarg");

		REQUIRE(result->get_positional_arguments().size() == 0);
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

		REQUIRE(result->get_arguments_for_option("optarg").size() == 2);

		REQUIRE(result->get_last_argument_for_option("optarg").value() == "arg2");

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

TEST_CASE("convert_to<signed>", "[cpparg]") {
	SECTION("simple") {
		REQUIRE(cpparg::convert_to<int>("42") == 42);
		REQUIRE(cpparg::convert_to<int>("-42") == -42);
	}

	SECTION("base 16") {
		REQUIRE(cpparg::convert_to<int>("0x20", 16) == 32);
		REQUIRE(cpparg::convert_to<int>("20", 16) == 32);
		REQUIRE(cpparg::convert_to<int>("-0x20", 16) == -32);
		REQUIRE(cpparg::convert_to<int>("-20", 16) == -32);
	}

	SECTION("base 8") {
		REQUIRE(cpparg::convert_to<int>("0644", 8) == 0b110'100'100);
		REQUIRE(cpparg::convert_to<int>("644", 8) == 0b110'100'100);
	}

	SECTION("base 2") {
		REQUIRE(cpparg::convert_to<int>("0b1011", 2) == 0b1011);
		REQUIRE(cpparg::convert_to<int>("1011", 2) == 0b1011);
	}


	SECTION("base 0") {
		REQUIRE(cpparg::convert_to<int>("0x20", 0) == 32);
		REQUIRE(cpparg::convert_to<int>("-0x20", 0) == -32);
		REQUIRE(cpparg::convert_to<int>("0644", 0) == 0b110'100'100);
		REQUIRE(cpparg::convert_to<int>("0b1011", 0) == 0b1011);
	}

	SECTION("limits") {
		REQUIRE(cpparg::convert_to<std::int8_t>("127") == 127);
		REQUIRE(!cpparg::convert_to<std::int8_t>("128"));
		REQUIRE(cpparg::convert_to<std::int8_t>("-128") == -128);
		REQUIRE(!cpparg::convert_to<std::int8_t>("-129"));
	}

	SECTION("invalid") {
		REQUIRE(!cpparg::convert_to<int>(""));
		REQUIRE(!cpparg::convert_to<int>("20h"));
		REQUIRE(!cpparg::convert_to<int>("42 "));
		REQUIRE(!cpparg::convert_to<int>(" 42"));
		REQUIRE(!cpparg::convert_to<int>("x20", 16));
		REQUIRE(!cpparg::convert_to<int>("0102010", 2));
		REQUIRE(!cpparg::convert_to<int>("0649", 8));
	}
}

TEST_CASE("convert_to<unsigned>", "[cpparg]") {
	SECTION("simple") {
		REQUIRE(cpparg::convert_to<unsigned int>("42") == 42U);
		REQUIRE(cpparg::convert_to<unsigned int>("-42") == -42U);
	}

	SECTION("base 16") {
		REQUIRE(cpparg::convert_to<unsigned int>("0x20", 16) == 32U);
		REQUIRE(cpparg::convert_to<unsigned int>("20", 16) == 32U);
		REQUIRE(cpparg::convert_to<unsigned int>("-0x20", 16) == -32U);
		REQUIRE(cpparg::convert_to<unsigned int>("-20", 16) == -32U);
	}

	SECTION("base 8") {
		REQUIRE(cpparg::convert_to<unsigned int>("0644", 8) == 0b110'100'100);
		REQUIRE(cpparg::convert_to<unsigned int>("644", 8) == 0b110'100'100);
	}

	SECTION("base 2") {
		REQUIRE(cpparg::convert_to<unsigned int>("0b1011", 2) == 0b1011);
		REQUIRE(cpparg::convert_to<unsigned int>("1011", 2) == 0b1011);
	}


	SECTION("base 0") {
		REQUIRE(cpparg::convert_to<unsigned int>("0x20", 0) == 32U);
		REQUIRE(cpparg::convert_to<unsigned int>("-0x20", 0) == -32U);
		REQUIRE(cpparg::convert_to<unsigned int>("0644", 0) == 0b110'100'100);
		REQUIRE(cpparg::convert_to<unsigned int>("0b1011", 0) == 0b1011);
	}

	SECTION("limits") {
		REQUIRE(cpparg::convert_to<std::uint8_t>("255") == 255U);
		REQUIRE(!cpparg::convert_to<std::uint8_t>("256"));
		REQUIRE(cpparg::convert_to<std::uint8_t>("-1") == 255U);
		REQUIRE(cpparg::convert_to<std::uint8_t>("-255") == 1U);
		REQUIRE(!cpparg::convert_to<std::uint8_t>("-256"));
	}

	SECTION("invalid") {
		REQUIRE(!cpparg::convert_to<unsigned int>(""));
		REQUIRE(!cpparg::convert_to<unsigned int>("20h"));
		REQUIRE(!cpparg::convert_to<unsigned int>("42 "));
		REQUIRE(!cpparg::convert_to<unsigned int>(" 42"));
		REQUIRE(!cpparg::convert_to<unsigned int>("x20", 16));
		REQUIRE(!cpparg::convert_to<unsigned int>("0102010", 2));
		REQUIRE(!cpparg::convert_to<unsigned int>("0649", 8));
	}
}

TEST_CASE("convert_to<bool>", "[cpparg]") {
	SECTION("simple") {
		REQUIRE(cpparg::convert_to<bool>("yes") == true);
		REQUIRE(cpparg::convert_to<bool>("true") == true);
		REQUIRE(cpparg::convert_to<bool>("on") == true);
		REQUIRE(cpparg::convert_to<bool>("1") == true);
		REQUIRE(cpparg::convert_to<bool>("no") == false);
		REQUIRE(cpparg::convert_to<bool>("false") == false);
		REQUIRE(cpparg::convert_to<bool>("off") == false);
		REQUIRE(cpparg::convert_to<bool>("0") == false);
	}

	SECTION("case") {
		REQUIRE(cpparg::convert_to<bool>("YeS") == true);
		REQUIRE(cpparg::convert_to<bool>("tRuE") == true);
		REQUIRE(cpparg::convert_to<bool>("On") == true);
		REQUIRE(cpparg::convert_to<bool>("nO") == false);
		REQUIRE(cpparg::convert_to<bool>("FaLsE") == false);
		REQUIRE(cpparg::convert_to<bool>("oFf") == false);
	}

	SECTION("invalid") {
		REQUIRE(!cpparg::convert_to<bool>(""));
		REQUIRE(!cpparg::convert_to<bool>(" true"));
		REQUIRE(!cpparg::convert_to<bool>("true "));
		REQUIRE(!cpparg::convert_to<bool>("yess"));
		REQUIRE(!cpparg::convert_to<bool>("noff"));
		REQUIRE(!cpparg::convert_to<bool>("0n"));
		REQUIRE(!cpparg::convert_to<bool>("2"));
		REQUIRE(!cpparg::convert_to<bool>("-1"));
	}
}
