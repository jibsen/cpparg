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

#include <cstdint>
#include <cstring>
#include <vector>

#include "cpparg.hpp"

auto default_parser = [] {
	cpparg::OptionParser parser;

	parser.add_option("n", "noarg",  "",      "option with no argument")
	      .add_option("o", "optarg", "[ARG]", "option with optional argument")
	      .add_option("r", "reqarg", "ARG",   "option with required argument");

	return parser;
}();

extern "C"
auto LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) -> int
{
	std::vector<char> buffer(data, data + size);

	buffer.push_back('\0');

	std::vector<const char *> args;

	for (char *ptr = buffer.data(); ptr != buffer.data() + buffer.size(); ) {
		args.push_back(ptr);

		ptr += strlen(ptr) + 1;
	}

	auto result = default_parser.parse_argv(args.size(), args.data());

	return 0;
}
