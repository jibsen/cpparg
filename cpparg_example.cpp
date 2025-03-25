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

#include <iostream>
#include <print>

#include "cpparg.hpp"

auto main(int argc, char *argv[]) -> int
{
	cpparg::OptionParser parser;

	parser.add_option("h", "help",     "",      "print this help and exit")
	      .add_option("r", "required", "ARG",   "option with required argument")
	      .add_option("o", "optional", "[ARG]", "option with optional argument");

	auto result = parser.parse_argv(argc, argv);

	if (!result) {
		std::println(std::cerr, "cpparg: {}", result.error().what);
		exit(-1);
	}

	// If the `help` option appeared, show help
	if (result->count("help")) {
		std::println(
			"usage: cpparg_example [options] POSITIONAL_ARG...\n"
			"\n"
			"Example program for cpparg.\n"
			"\n"
			"{}",
			parser.get_option_help(78)
		);

		return 0;
	}

	// Check that at least one positional argument was supplied
	if (result->get_positional_arguments().size() < 1) {
		std::println(std::cerr, "usage: cpparg_example [options] POSITIONAL_ARG...\n");

		return -1;
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
}
