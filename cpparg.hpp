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

#ifndef CPPARG_HPP_INCLUDED
#define CPPARG_HPP_INCLUDED

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <expected>
#include <format>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cpparg {

namespace detail {

/// @brief Word wrap `sv` to `width`.
///
/// Attempt to break `sv` into substrings (lines) that are length `width`
/// or shorter. Breaks only at spaces. If a line ends up being longer
/// than `width`, yet contains no spaces, it is added as is.
///
/// @note Ensure that the returned vector of string_views does not
/// outlive the data `sv` points to.
///
/// @param sv string to wrap
/// @param width line width
/// @return vector of substrings (lines)
constexpr auto word_wrap(std::string_view sv, std::size_t width) {
	std::vector<std::string_view> res;

	auto is_space = [](char ch) { return ch == ' '; };

	// Skip leading space
	auto line_start = std::find_if_not(sv.begin(), sv.end(), is_space);

	auto space_begin = std::find_if(line_start, sv.end(), is_space);

	while (space_begin != sv.end()) {
		auto space_end = std::find_if_not(space_begin, sv.end(), is_space);

		auto next_space_begin = std::find_if(space_end, sv.end(), is_space);

		// If breaking at the next space would exceed width, break here
		if (next_space_begin - line_start > width) {
			res.emplace_back(line_start, space_begin);

			line_start = space_end;
		}

		space_begin = next_space_begin;
	}

	if (line_start != sv.end()) {
		res.emplace_back(line_start, sv.end());
	}

	return res;
}

/// @brief Case insensitive string equality (ASCII only)
constexpr auto iequal(std::string_view lhs, std::string_view rhs) -> bool {
	auto tolower = [](char ch) {
		return ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch;
	};

	return std::ranges::equal(lhs, rhs,
		[&tolower](char lch, char rch) {
			return tolower(lch) == tolower(rch);
		}
	);
}

template<typename T>
concept NonBoolIntegral = std::integral<T> && !std::same_as<std::remove_cv_t<T>, bool>;

} // namespace detail

struct ParseError {
	std::size_t originating_arg = 0;
	std::string what;
};

struct ParsedOption {
	explicit ParsedOption(std::string_view name) : name(name) {}
	std::string name;
	std::size_t count = 0;
	std::vector<std::string> arguments;
};

class ParseResult {
public:
	/// @brief Get number of times option `name` occured.
	auto count(std::string_view name) const -> std::size_t {
		if (auto it = lookup.find(std::string(name)); it != lookup.end()) {
			return parsed_options[it->second].count;
		}

		return 0;
	}

	/// @brief Get last option argument for option `name`.
	auto get_last_argument_for_option(std::string_view name) const -> std::optional<std::string> {
		if (auto it = lookup.find(std::string(name)); it != lookup.end()) {
			if (!parsed_options[it->second].arguments.empty()) {
				return parsed_options[it->second].arguments.back();
			}
		}

		return {};
	}

	/// @brief Access vector of option arguments for option `name`.
	auto get_arguments_for_option(std::string_view name) const -> const std::vector<std::string>& {
		if (auto it = lookup.find(std::string(name)); it != lookup.end()) {
			return parsed_options[it->second].arguments;
		}

		return empty_arguments;
	}

	/// @brief Access vector of `ParsedOption`.
	auto get_parsed_options() const -> const std::vector<ParsedOption>& {
		return parsed_options;
	}

	/// @brief Access vector of positional arguments.
	auto get_positional_arguments() const -> const std::vector<std::string>& {
		return positional_args;
	}

	/// @brief Add occurence of parsed option `name`.
	auto add_parsed_option(std::string_view name) -> void {
		auto [it, inserted] = lookup.emplace(std::string(name), parsed_options.size());

		if (inserted) {
			parsed_options.emplace_back(name);
		}

		parsed_options[it->second].count++;
	}

	/// @brief Add occurence of parsed option `name` with argument `argument`.
	auto add_parsed_option(std::string_view name, std::string_view argument) -> void {
		auto [it, inserted] = lookup.emplace(std::string(name), parsed_options.size());

		if (inserted) {
			parsed_options.emplace_back(name);
		}

		parsed_options[it->second].count++;
		parsed_options[it->second].arguments.emplace_back(argument);
	}

	/// @brief Add positional argument.
	auto add_positional_argument(std::string_view argument) -> void {
		positional_args.emplace_back(argument);
	}

	/// @brief Add positional arguments from range [first, last).
	template<std::input_iterator I, std::sentinel_for<I> S>
		requires std::convertible_to<std::iter_reference_t<I>, std::string>
	auto add_positional_arguments(I first, S last) -> void {
		positional_args.insert(positional_args.end(), first, last);
	}

private:
	std::unordered_map<std::string, std::size_t> lookup;
	std::vector<ParsedOption> parsed_options;
	std::vector<std::string> positional_args;
	inline static const std::vector<std::string> empty_arguments;
};

class OptionParser {
	struct Option {
		std::string short_flag;
		std::string long_flag;
		std::string arg_name;
		std::string description;

		auto takes_argument() const noexcept -> bool {
			return !arg_name.empty();
		}

		auto requires_argument() const noexcept -> bool {
			return takes_argument() && !arg_name.starts_with('[');
		}
	};

public:
	/// @brief Add option.
	///
	/// If `arg_name` is enclosed in square brackets, like "[ARG]", the
	/// option argument is optional. Otherwise it is required.
	///
	/// By default, required arguments are printed as separate elements
	/// in option help. You can change this by adding a '=' to `arg_name`.
	///
	///     "f" "foo" "ARG"  becomes -f, --foo ARG
	///     "f" ""    "ARG"  becomes -f ARG
	///     "f" "foo" "=ARG" becomes -f, --foo=ARG
	///     "f" ""    "=ARG" becomes -fARG
	///
	/// @param short_flag string containing single character short flag
	/// @param long_flag long flag
	/// @param arg_name option argument name, optional if in square brackets
	/// @param description option description
	/// @return reference to this, so calls can be chained
	auto add_option(std::string short_flag, std::string long_flag,
	                std::string arg_name, std::string description) -> OptionParser& {
		if (short_flag.size() > 1) {
			short_flag.resize(1);
		}

		// Adjust arg_name for use in option help
		if (arg_name.starts_with('=')) {
			// Required argument starting with '='
			if (long_flag.empty()) {
				// No long flag, remove '=' so -f=ARG becomes -fARG
				arg_name.erase(0, 1);
			}
		}
		else if (arg_name.starts_with('[')) {
			// Optional argument
			if (long_flag.empty()) {
				// No long flag, remove any '=' so -f[=ARG] becomes -f[ARG]
				if (arg_name.starts_with("[=")) {
					arg_name.erase(1, 1);
				}
			}
			else {
				// Long flag, add '=' so --foo[ARG] becomes --foo[=ARG]
				if (!arg_name.starts_with("[=")) {
					arg_name.insert(1, 1, '=');
				}
			}
		}
		else if (!arg_name.empty()) {
			// Required argument not starting with '=', insert space
			// so -fARG becomes -f ARG and --fooARG becomes --foo ARG
			arg_name.insert(0, 1, ' ');
		}

		if (long_flag.empty()) {
			long_flag = short_flag;
		}

		options.emplace_back(
			std::move(short_flag), std::move(long_flag),
			std::move(arg_name), std::move(description)
		);

		return *this;
	}

	/// @brief Generate help text for options added to parser.
	/// @param line_width width to word wrap lines at, or 0 to disable
	/// @return string containing option help
	auto get_option_help(std::size_t line_width = 0) const -> std::string {
		// Help for each option consists of flags and description.
		// This is a limit for how long the flags part can be.
		// Options that exceed this will have their description
		// start on the next line.
		constexpr std::size_t flags_len_limit = 29;

		auto max_flags_length = [&] {
			std::size_t longest_flags = 0;

			for (const auto &option : options) {
				// Length of "  -f, --" plus long name
				std::size_t len = 8 + option.long_flag.size();

				len += option.arg_name.size();

				// Two spaces before description
				len += 2;

				longest_flags = std::max(longest_flags, len);
			}

			return longest_flags;
		};

		std::size_t flags_len = std::min(flags_len_limit, max_flags_length());

		// Adjust line width to at least flags_len
		if (line_width && line_width < flags_len) {
			line_width = flags_len;
		}

		std::string help;

		for (const auto &option : options) {
			// Add short flag or space
			std::string option_line = option.short_flag.empty() ? "    " : std::format("  -{}", option.short_flag);

			// Add long flag if present
			if (!option.long_flag.empty() && option.long_flag != option.short_flag) {
				if (!option.short_flag.empty()) {
					option_line.append(", ");
				}
				else {
					option_line.append("  ");
				}

				option_line.append(std::format("--{}", option.long_flag));
			}

			// Add option argument
			option_line.append(option.arg_name);

			// The flags part of the option is done now. We need
			// to format the description, starting with any
			// leading space to align all the descriptions, or
			// possibly a newline if the flags part was too long.

			// Find leading space before description for first line
			std::size_t leading_space = flags_len;

			if (option_line.size() + 2 > flags_len) {
				option_line.append("\n");
			}
			else {
				leading_space = flags_len - option_line.size();
			}

			// Add description
			if (line_width) {
				auto wrapped_description = detail::word_wrap(option.description, line_width - flags_len);

				for (auto line : wrapped_description) {
					option_line.append(leading_space, ' ');

					option_line.append(line);
					option_line.append("\n");

					leading_space = flags_len;
				}
			}
			else {
				option_line.append(leading_space, ' ');
				option_line.append(option.description);
				option_line.append("\n");
			}

			help.append(option_line);
		}

		return help;
	}

	/// @brief Parse arguments in range [first, last).
	/// @return ParseResult on success, ParseError otherwise
	template<std::forward_iterator I>
		requires std::convertible_to<std::iter_reference_t<I>, std::string_view>
	auto parse(I first, I last) const -> std::expected<ParseResult, ParseError> {
		ParseResult res;

		for (std::size_t idx = 0; first != last; ++first, ++idx) {
			std::string_view arg(*first);

			// Check for nonoption element (including '-')
			if (!arg.starts_with('-') || arg == "-") {
				res.add_positional_argument(arg);

				continue;
			}

			// Check for long option
			if (arg.starts_with("--")) {
				if (arg == "--") {
					res.add_positional_arguments(++first, last);

					return res;
				}

				arg.remove_prefix(2);

				auto name_end = arg.find('=');

				auto name = arg.substr(0, name_end);

				auto it = find_long_option(name);

				if (it == options.end()) {
					return std::unexpected<ParseError>(std::in_place, idx,
						std::format("unrecognized long option '--{}'", name)
					);
				}

				// Handle option argument included in element (--foo=argument)
				if (name_end != std::string_view::npos) {
					auto argument = arg.substr(name_end + 1);

					if (!it->takes_argument()) {
						return std::unexpected<ParseError>(std::in_place, idx,
							std::format("extraneous argument in '--{}'", arg)
						);
					}

					res.add_parsed_option(name, argument);

					continue;
				}

				// No option argument in element, so if
				// required take next element
				if (it->requires_argument()) {
					if (++first == last) {
						return std::unexpected<ParseError>(std::in_place, idx,
							std::format("missing required argument for '--{}'", arg)
						);
					}

					res.add_parsed_option(name, *first);

					++idx;

					continue;
				}

				// No option argument in element, none required
				res.add_parsed_option(name);

				continue;
			}

			// Short option
			arg.remove_prefix(1);

			for (std::size_t pos = 0; pos < arg.size(); ++pos) {
				auto flag = arg.substr(pos, 1);

				auto it = find_short_option(flag);

				if (it == options.end()) {
					return std::unexpected<ParseError>(std::in_place, idx,
						std::format("unrecognized short option '{}' in '-{}'", flag, arg)
					);
				}

				if (!it->takes_argument()) {
					res.add_parsed_option(it->long_flag);

					continue;
				}

				// If more characters, take as option argument
				if (pos + 1 < arg.size()) {
					auto argument = arg.substr(pos + 1);

					res.add_parsed_option(it->long_flag, argument);

					break;
				}

				if (!it->requires_argument()) {
					res.add_parsed_option(it->long_flag);

					break;
				}

				// Option argument required, so take next element
				if (++first == last) {
					return std::unexpected<ParseError>(std::in_place, idx,
						std::format("missing required argument for '{}' in '-{}'", flag, arg)
					);
				}

				res.add_parsed_option(it->long_flag, *first);

				++idx;

				break;
			}
		}

		return res;
	}

	/// @brief Parse arguments in `argv`.
	/// @return ParseResult on success, ParseError otherwise
	auto parse_argv(int argc, const char * const argv[]) const -> std::expected<ParseResult, ParseError> {
		if (argc < 1) {
			return std::unexpected<ParseError>(std::in_place, 0,
				"argc less than 1"
			);
		}

		auto result = parse(argv + 1, argv + argc);

		if (!result) {
			result.error().originating_arg++;
		}

		return result;
	}

private:
	std::vector<Option> options;

	using option_iterator = decltype(options)::const_iterator;

	auto find_long_option(std::string_view name) const -> option_iterator {
		return std::ranges::find_if(options, [&](const auto &option) {
			return option.long_flag == name;
		});
	}

	auto find_short_option(std::string_view flag) const -> option_iterator {
		return std::ranges::find_if(options, [&](const auto &option) {
			return option.short_flag == flag;
		});
	}
};

/// @brief Convert a string to integral type `T` (except `bool`).
///
/// Minus is recognized for both signed and unsigned types.
///
/// If `base` is 0, base is auto-detected from the prefix. If the prefix is
/// "0x" or "0X" base is 16, if the prefix is "0b" or "0B" base is 2, if the
/// prefix is "0" base is 8, otherwise base is 10.
///
template<detail::NonBoolIntegral T>
constexpr auto convert_to(std::string_view sv, int base = 10) -> std::expected<T, std::errc> {
	bool negative = false;

	if (sv.starts_with('-')) {
		sv.remove_prefix(1);
		negative = true;
	}

	if (base == 16 && (sv.starts_with("0x") || sv.starts_with("0X"))) {
		sv.remove_prefix(2);
	}

	if (base == 2 && (sv.starts_with("0b") || sv.starts_with("0B"))) {
		sv.remove_prefix(2);
	}

	// If base is 0, auto-detect base
	if (base == 0) {
		base = 10;

		if (sv.starts_with('0') && sv.size() > 1) {
			switch (sv[1]) {
			case 'x':
			case 'X':
				sv.remove_prefix(2);
				base = 16;
				break;
			case 'b':
			case 'B':
				sv.remove_prefix(2);
				base = 2;
				break;
			default:
				sv.remove_prefix(1);
				base = 8;
				break;
			}
		}
	}

	// Unsigned integer type corresponding to T
	using UT = std::make_unsigned_t<std::remove_cv_t<T>>;

	UT res{};

	auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), res, base);

	if (ec != std::errc()) {
		return std::unexpected(ec);
	}

	if (ptr != sv.data() + sv.size()) {
		return std::unexpected(std::errc::invalid_argument);
	}

	// If T is a signed type, check that the unsigned value we read
	// is within the range of T
	if constexpr (std::signed_integral<T>) {
		if (res > static_cast<UT>(std::numeric_limits<T>::max()) + negative) {
			return std::unexpected(std::errc::result_out_of_range);
		}
	}

	// Safely negate the result if needed
	return static_cast<T>(negative ? (UT(0) - res) : res);
}

static_assert(convert_to<int>("42") == 42);
static_assert(convert_to<int>("-42") == -42);
static_assert(convert_to<unsigned int>("-1") == std::numeric_limits<unsigned int>::max());

/// @brief Convert a string to `bool`.
///
/// Accepts "yes", "true", "on" and "1" as true,
/// and "no", "false", "off" and "0" as false.
///
template<typename T>
	requires std::same_as<std::remove_cv_t<T>, bool>
constexpr auto convert_to(std::string_view sv) -> std::expected<T, std::errc> {
	using namespace std::string_view_literals;

	if (detail::iequal(sv, "yes"sv) || detail::iequal(sv, "true"sv)
	 || detail::iequal(sv, "on"sv) || detail::iequal(sv, "1"sv)) {
		return true;
	}

	if (detail::iequal(sv, "no"sv) || detail::iequal(sv, "false"sv)
	 || detail::iequal(sv, "off"sv) || detail::iequal(sv, "0"sv)) {
		return false;
	}

	return std::unexpected(std::errc::invalid_argument);
}

static_assert(convert_to<bool>("true") == true);
static_assert(convert_to<bool>("false") == false);

} // namespace cpparg

#endif // CPPARG_HPP_INCLUDED
