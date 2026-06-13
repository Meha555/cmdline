#pragma once

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>
#if !defined(CMDLINE_USE_EXCEPTIONS) && __cplusplus >= 201703L
#include <optional>
#endif

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__))
#include <cxxabi.h>
#endif
#include <cstdlib>

#ifdef max
#undef max
#endif

namespace cmdline
{

class cmdline_error : public std::runtime_error
{
public:
    cmdline_error(const std::string &message)
        : std::runtime_error(message)
    {
    }
};

namespace detail
{

template<typename Target, typename Source, bool Same>
struct lexical_cast_t
{
    static Target cast(const Source &arg)
    {
        Target ret;
        std::stringstream ss;
        if (!(ss << arg && ss >> ret && ss.eof()))
            throw std::bad_cast();
        return ret;
    }
};

template<typename Target, typename Source>
struct lexical_cast_t<Target, Source, true>
{
    static Target cast(const Source &arg)
    {
        return arg;
    }
};

template<typename Source>
struct lexical_cast_t<std::string, Source, false>
{
    static std::string cast(const Source &arg)
    {
        std::ostringstream ss;
        ss << arg;
        return ss.str();
    }
};

template<typename Target>
struct lexical_cast_t<Target, std::string, false>
{
    static Target cast(const std::string &arg)
    {
        Target ret;
        std::istringstream ss(arg);
        if (!(ss >> ret && ss.eof()))
            throw std::bad_cast();
        return ret;
    }
};

template<>
struct lexical_cast_t<std::string, std::regex, false>
{
    static std::string cast(const std::regex &)
    {
        // We can't extract the pattern from std::regex, so we just return a placeholder
        // In practice, if you need this conversion, you should store the pattern separately
        return "<regex>";
    }
};

template<>
struct lexical_cast_t<std::regex, std::string, false>
{
    static std::regex cast(const std::string &arg)
    {
        return std::regex(arg);
    }
};

template<typename Target, typename Source>
Target lexical_cast(const Source &arg)
{
    return lexical_cast_t<Target, Source,
                          std::is_same<Target, Source>::value>::cast(arg);
}

static inline std::string demangle(const std::string &name)
{
#ifdef _MSC_VER
    return name;
#elif defined(__clang__) || defined(__GNUC__)
    int status = 0;
    char *p = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
    std::string ret(p);
    free(p);
    return ret;
#else
#error Unexpected complier (msvc/gnu/llvm), You need to implement this method for demangling!
#endif
}

template<typename T>
std::string readable_typename()
{
    return demangle(typeid(T).name());
}

template<>
inline std::string readable_typename<std::string>()
{
    return "string";
}

template<>
inline std::string readable_typename<std::regex>()
{
    return "regex";
}

template<typename T>
std::string default_value(T def)
{
    return detail::lexical_cast<std::string>(def);
}

template<typename T>
struct default_reader
{
    T operator()(const std::string &str)
    {
        return detail::lexical_cast<T>(str);
    }
    const std::string &constraint() const
    {
        static const std::string msg;
        return msg;
    }
};

template<typename T>
struct range_reader
{
    range_reader(const T &low_bound, const T &upper_bound)
        : low_(low_bound)
        , high_(upper_bound)
    {
    }
    T operator()(const std::string &s)
    {
        T ret = default_reader<T>()(s);
        if (!(ret >= low_ && ret <= high_))
            throw cmdline::cmdline_error(s + " out of range" + constraint());
        return ret;
    }
    const std::string &constraint()
    {
        // static const std::string msg = "[" + detail::lexical_cast<std::string>(low_) + ", " + detail::lexical_cast<std::string>(high_) + "]";
        msg_.clear();
        msg_ = "[" + detail::lexical_cast<std::string>(low_) + ", " + detail::lexical_cast<std::string>(high_) + "]";
        return msg_;
    }

private:
    const T low_, high_;
    std::string msg_;
};

template<typename T>
struct oneof_reader
{
    T operator()(const std::string &s)
    {
        T ret = default_reader<T>()(s);
        if (std::find(alts_.begin(), alts_.end(), ret) == alts_.end())
            throw cmdline_error(s + " not in " + constraint());
        return ret;
    }
    void add(T &&v)
    {
        alts_.emplace_back(std::forward<T>(v));
    }
    const std::string &constraint()
    {
        msg_.clear();
        msg_ = "{";
        for (size_t i = 0; i < alts_.size(); ++i) {
            msg_ += detail::lexical_cast<T>(alts_[i]);
            if (i != alts_.size() - 1) {
                msg_ += "|";
            }
        }
        msg_ += "}";
        return msg_;
    }

private:
    std::vector<T> alts_;
    std::string msg_;
};

template<typename Reader, typename T>
Reader oneof_impl(Reader &reader, T &&last)
{
    reader.add(std::forward<T>(last));
    return reader;
}

template<typename Reader, typename T, typename... Rest>
Reader oneof_impl(Reader &reader, T &&first, Rest &&...rest)
{
    reader.add(std::forward<T>(first));
    return oneof_impl(reader, std::forward<Rest>(rest)...);
}

template<typename T>
struct regex_reader
{
    regex_reader(const std::string &pattern)
        : re_(pattern)
        , pattern_(pattern)
    {
    }
    T operator()(const std::string &s)
    {
        std::match_results<typename T::const_iterator> match;
        if (!std::regex_match(s, match, re_)) { // use regex_match to ensure full match
            throw cmdline_error(s + " doesn't match " + constraint());
        }
        return match[0];
    }
    const std::string constraint()
    {
        return "\"" + pattern_ + "\"";
    }

private:
    const std::regex re_;
    const std::string pattern_;
};

} // namespace detail

template<typename T>
detail::range_reader<T> range(const T &lower_bound, const T &upper_bound)
{
    return detail::range_reader<T>(lower_bound, upper_bound);
}

template<typename T, typename... Rest>
detail::oneof_reader<T> oneof(Rest &&...rest)
{
    detail::oneof_reader<T> reader;
    return detail::oneof_impl(reader, std::forward<Rest>(rest)...);
}

template<typename T>
detail::regex_reader<T> regex(const std::string &pattern)
{
    return detail::regex_reader<T>(pattern);
}

// template <typename T>
// oneof_reader<T> oneof(T a1)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   ret.add(a6);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   ret.add(a6);
//   ret.add(a7);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   ret.add(a6);
//   ret.add(a7);
//   ret.add(a8);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   ret.add(a6);
//   ret.add(a7);
//   ret.add(a8);
//   ret.add(a9);
//   return ret;
// }

// template <typename T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4, T a5, T a6, T a7, T a8, T a9, T
// a10)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   ret.add(a5);
//   ret.add(a6);
//   ret.add(a7);
//   ret.add(a8);
//   ret.add(a9);
//   ret.add(a10);
//   return ret;
// }

class description
{
public:
    description() = default;
    description(const std::string &brief, const std::string &detail = "")
        : brief_(brief)
        , detail_(detail)
    {
    }

    const std::string &brief() const
    {
        return brief_;
    }
    const std::string &detail() const
    {
        return detail_;
    }

    description &wrap(const description &other)
    {
        brief_ += other.brief_;
        detail_ += other.detail_;
        return *this;
    }

    std::string dump(size_t indent) const
    {
        if (detail_.empty()) {
            return brief_;
        } else {
            std::string ret = brief_ + "\n";
            size_t pos = 0;
            size_t next_pos = 0;
            while ((next_pos = detail_.find('\n', pos)) != std::string::npos) {
                ret += std::string(indent, ' ') + detail_.substr(pos, next_pos - pos) + '\n';
                pos = next_pos + 1;
            }
            ret += std::string(indent, ' ') + detail_.substr(pos);
            return ret;
        }
    }

private:
    std::string brief_;
    std::string detail_;
};

#if __cplusplus >= 201703L
inline
#endif
    static struct config
{
    bool show_option_typename = true;
} g_config;

class parser
{
public:
    parser() = default;
    parser(const parser &other) = delete;
    parser &operator=(const parser &other) = delete;
    parser(parser &&other) noexcept = default;
    parser &operator=(parser &&other) noexcept = default;
    ~parser() = default;

#ifdef CMDLINE_USE_EXCEPTIONS
    // add flag
    parser &flag(const std::string &long_name, char short_name = 0,
                 const std::string &description = "")
    {
        if (long_name.empty())
            throw cmdline_error("flag only accetps long name");
        if (options_.count(long_name))
            throw cmdline_error("multiple flag definition: " + long_name);
        auto p = new option_without_value(long_name, short_name, description);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add flag
    parser &flag(const std::string &long_name, char short_name = 0,
                 const class description &desc = description())
    {
        if (long_name.empty())
            throw cmdline_error("flag only accetps long name");
        if (options_.count(long_name))
            throw cmdline_error("multiple flag definition: " + long_name);
        auto p = new option_without_value(long_name, short_name, desc);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }
#else
    // add flag
    parser &flag(const std::string &long_name, char short_name = 0,
                 const std::string &description = "")
    {
        if (long_name.empty()) {
            throw cmdline_error("flag only accetps long name");
        }
        if (options_.count(long_name)) {
            throw cmdline_error("multiple flag definition: " + long_name);
        }
        auto p = new option_without_value(long_name, short_name, description);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add flag
    parser &flag(const std::string &long_name, char short_name = 0,
                 const class description &desc = description())
    {
        if (long_name.empty()) {
            throw cmdline_error("flag only accetps long name");
        }
        if (options_.count(long_name)) {
            throw cmdline_error("multiple flag definition: " + long_name);
        }
        auto p = new option_without_value(long_name, short_name, desc);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }
#endif

    // add option with value
    template<typename T>
    parser &option_with_default(const std::string &long_name, char short_name = 0,
                                const std::string &description = "", bool required = true,
                                const T default_value = T())
    {
        return option_with_default(long_name, short_name, description, required, default_value,
                                   detail::default_reader<T>());
    }

#ifdef CMDLINE_USE_EXCEPTIONS
    // add option with value
    template<typename T>
    parser &option(const std::string &long_name, char short_name = 0,
                   const std::string &description = "", bool required = true)
    {
        return option<T>(long_name, short_name, description, required,
                         detail::default_reader<T>());
    }

    // add option with value
    template<typename T>
    parser &option(const std::string &long_name, char short_name = 0,
                   const class description &desc = description(), bool required = true)
    {
        return option<T>(long_name, short_name, desc, required,
                         detail::default_reader<T>());
    }
#else
    // add option with value
    template<typename T>
    parser &option(const std::string &long_name, char short_name = 0,
                   const std::string &description = "", bool required = true)
    {
        if (long_name.empty()) {
            throw cmdline_error("option only accepts long name");
        }
        if (options_.count(long_name)) {
            throw cmdline_error("multiple flag/option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, detail::default_reader<T>>(long_name, short_name, description, required, detail::default_reader<T>{});
        options_.emplace(long_name, std::unique_ptr<option_base>(p));
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T>
    parser &option(const std::string &long_name, char short_name = 0,
                   const class description &desc = description(), bool required = true)
    {
        if (long_name.empty()) {
            throw cmdline_error("option only accepts long name");
        }
        if (options_.count(long_name)) {
            throw cmdline_error("multiple flag/option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, detail::default_reader<T>>(long_name, short_name, desc, required, detail::default_reader<T>{});
        options_.emplace(long_name, std::unique_ptr<option_base>(p));
        ordered_.push_back(p);
        return *this;
    }
#endif

#ifdef CMDLINE_USE_EXCEPTIONS
    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option_with_default(const std::string &long_name, char short_name = 0,
                        const std::string &description = "", bool required = true,
                        const T default_value = T(), R value_reader = R())
    {
        if (options_.count(long_name))
            throw cmdline_error("multiple option definition: " + long_name);
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, description, required, default_value,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option(const std::string &long_name, char short_name = 0,
           const std::string &description = "", bool required = true,
           R value_reader = R())
    {
        if (options_.count(long_name))
            throw cmdline_error("multiple option definition: " + long_name);
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, description, required,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option(const std::string &long_name, char short_name = 0,
           const class description &desc = description(), bool required = true,
           R value_reader = R())
    {
        if (options_.count(long_name))
            throw cmdline_error("multiple option definition: " + long_name);
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, desc, required,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T>
    parser &option_with_default(const std::string &long_name, char short_name = 0,
                                const class description &desc = description(), bool required = true,
                                const T default_value = T())
    {
        return option_with_default(long_name, short_name, desc, required, default_value,
                                   detail::default_reader<T>());
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option_with_default(const std::string &long_name, char short_name = 0,
                        const class description &desc = description(), bool required = true,
                        const T default_value = T(), R value_reader = R())
    {
        if (options_.count(long_name))
            throw cmdline_error("multiple option definition: " + long_name);
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, desc, required, default_value,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }
#else
    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option_with_default(const std::string &long_name, char short_name = 0,
                        const std::string &description = "", bool required = true,
                        const T default_value = T(), R value_reader = R())
    {
        if (options_.count(long_name)) {
            throw cmdline_error("multiple option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, description, required, default_value,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option(const std::string &long_name, char short_name = 0,
           const std::string &description = "", bool required = true,
           R value_reader = R())
    {
        if (options_.count(long_name)) {
            throw cmdline_error("multiple option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, description, required,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option(const std::string &long_name, char short_name = 0,
           const class description &desc = description(), bool required = true,
           R value_reader = R())
    {
        if (options_.count(long_name)) {
            throw cmdline_error("multiple option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, desc, required,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }

    // add option with value
    template<typename T>
    parser &option_with_default(const std::string &long_name, char short_name = 0,
                                const class description &desc = description(), bool required = true,
                                const T default_value = T())
    {
        return option_with_default(long_name, short_name, desc, required, default_value,
                                   detail::default_reader<T>());
    }

    // add option with value
    template<typename T, typename R>
    typename std::enable_if<std::is_same<decltype(std::declval<R>().operator()(std::declval<const std::string &>())), T>::value, parser &>::type
    option_with_default(const std::string &long_name, char short_name = 0,
                        const class description &desc = description(), bool required = true,
                        const T default_value = T(), R value_reader = R())
    {
        if (options_.count(long_name)) {
            throw cmdline_error("multiple option definition: " + long_name);
        }
        auto p = new option_with_value_with_reader<T, R>(
            long_name, short_name, desc, required, default_value,
            value_reader);
        options_.emplace(long_name, p);
        ordered_.push_back(p);
        return *this;
    }
#endif

    parser &footer(const std::string &f)
    {
        ftr_ = f;
        return *this;
    }

    /**
     * @brief Set the introduction text printed before the options section.
     * @param intro Introduction text.
     * @return Reference to this parser for method chaining.
     */
    parser &introduction(const std::string &intro)
    {
        intro_ = intro;
        return *this;
    }

    /**
     * @brief Set the version string used by the automatically added version flag.
     * @param version Version text to return from version().
     * @return Reference to this parser for method chaining.
     */
    parser &version(const char *version)
    {
        version_.assign(version);
        return *this;
    }

    /**
     * @brief Set the program name displayed in generated usage text.
     * @param name Program name.
     * @return Reference to this parser for method chaining.
     */
    parser &program_name(const std::string &name)
    {
        prog_name_ = name;
        return *this;
    }

    // whether this option has occur in cmdline.
    // note that is option has default value, this
    // function will still return false if it
    // didn't occur in cmdline.
    /**
     * @brief Check whether an option or flag occurred in the parsed command line.
     * @param name Long option name without leading dashes.
     * @return true if the option was present in the command line; false otherwise.
     */
    bool exist(const std::string &name) const
    {
        auto it = options_.find(name);
        if (it == options_.end()) {
            std::cerr << "there is no flag: --" << name << '\n';
            return false;
        }
        return it->second->has_set();
    }

#ifdef CMDLINE_USE_EXCEPTIONS
    /**
     * @brief Get a parsed option value.
     * @tparam T Expected option value type.
     * @param name Long option name without leading dashes.
     * @return Const reference to the stored option value.
     * @throws cmdline_error if the option is missing, has a different type, or has no value.
     */
    template<typename T>
    const T &get(const std::string &name) const
    {
        if (options_.count(name) == 0)
            throw cmdline_error("there is no option: --" + name);
        const option_with_value<T> *p =
            dynamic_cast<const option_with_value<T> *>(options_.find(name)->second.get());
        if (p == nullptr)
            throw cmdline_error("type mismatch option: --" + name);
        if (!p->occurred() && !p->with_default())
            throw cmdline_error("option --" + name + " is not set");
        return p->get();
    }
#else
#if __cplusplus >= 201703L
    /**
     * @brief Try to get a parsed option value without throwing.
     * @tparam T Expected option value type.
     * @param name Long option name without leading dashes.
     * @return Parsed value when available; std::nullopt otherwise.
     */
    template<typename T>
    std::optional<T> get(const std::string &name) const
    {
        if (options_.count(name) == 0)
            return std::nullopt;
        const option_with_value<T> *p =
            dynamic_cast<const option_with_value<T> *>(options_.find(name)->second.get());
        if (p == nullptr)
            return std::nullopt;
        if (!p->occurred() && !p->with_default())
            return std::nullopt;
        return p->get();
    }
#else
    /**
     * @brief Try to get a parsed option value without throwing.
     * @tparam T Expected option value type.
     * @param name Long option name without leading dashes.
     * @return Pair containing the value and a success flag.
     */
    template<typename T>
    std::pair<T, bool> get(const std::string &name) const
    {
        if (options_.count(name) == 0) {
            std::cerr << "there is no option: --" << name << '\n';
            return std::make_pair(T(), false);
        }
        const option_with_value<T> *p =
            dynamic_cast<const option_with_value<T> *>(options_.find(name)->second.get());
        if (p == nullptr) {
            std::cerr << "type mismatch option '" << name << "'\n";
            return std::make_pair(T(), false);
        }
        if (!p->occurred() && !p->with_default()) {
            std::cerr << "option '" << name << "' is not set\n";
            return std::make_pair(T(), false);
        }
        return std::make_pair(p->get(), true);
    }
#endif
#endif

    /**
     * @brief Get arguments that were not parsed as options.
     * @return Reference to the remaining positional arguments.
     */
    const std::vector<std::string> &rest() const
    {
        return others_;
    }

    /**
     * @brief Parse a command line string.
     * @param arg Command line text to tokenize and parse.
     * @param with_program_name Whether the first token should be treated as the program name.
     * @return true if parsing succeeds; false if parse errors were recorded.
     */
    bool parse(const std::string &arg, bool with_program_name = true)
    {
        std::vector<std::string> args;

        std::string buf;
        bool in_quote = false;
        for (std::string::size_type i = 0; i < arg.size(); i++) {
            if (arg[i] == '\"') {
                in_quote = !in_quote;
                continue;
            }

            if (arg[i] == ' ' && !in_quote) {
                args.push_back(buf);
                buf = "";
                continue;
            }

            if (arg[i] == '\\') {
                i++;
                if (i >= arg.size()) {
                    errors_.push_back("unexpected occurrence of '\\' at end of string");
                    return false;
                }
            }

            buf += arg[i];
        }

        if (in_quote) {
            errors_.push_back("quote is not closed");
            return false;
        }

        if (buf.size() > 0)
            args.push_back(buf);

        for (size_t i = 0; i < args.size(); i++)
            std::cerr << "\"" << args[i] << "\"\n";

        return parse(args, with_program_name);
    }

    /**
     * @brief Parse command line arguments from a string vector.
     * @param args Argument vector.
     * @param with_program_name Whether args[0] should be treated as the program name.
     * @return true if parsing succeeds; false if parse errors were recorded.
     */
    bool parse(const std::vector<std::string> &args, bool with_program_name = true)
    {
        int argc = static_cast<int>(args.size());
        std::vector<const char *> argv(argc);

        for (int i = 0; i < argc; i++)
            argv[i] = args[i].c_str();

        return parse(argc, argv.data(), with_program_name);
    }

    /**
     * @brief Parse command line arguments.
     * @param argc Argument count.
     * @param argv Argument array.
     * @param with_program_name Whether argv[0] should be treated as the program name.
     * @return true if parsing succeeds; false if parse errors were recorded.
     */
    bool parse(int argc, const char *const argv[], bool with_program_name = true)
    {
        errors_.clear();
        others_.clear();

        if (argc < 1) {
            errors_.push_back("argument number required be longer than 0");
            return false;
        }
        if (prog_name_ == "")
            prog_name_ = argv[0];
        // filter argv
        std::map<char, std::string> lookup; // <short_name, long_name>
        for (const auto &p : options_) {
            if (p.first.empty())
                continue;
            char initial = p.second->short_name();
            if (initial) {
                if (lookup.count(initial) > 0) {
                    lookup[initial].clear();
                    errors_.push_back(std::string("short option -") + initial + " is ambiguous");
                    return false;
                } else {
                    lookup[initial] = p.first;
                }
            }
        }
        // parse argv, and check options
        for (int i = with_program_name ? 1 : 0; i < argc; i++) {
            // if is long option "--xxx"
            if (std::strncmp(argv[i], "--", 2) == 0) {
                const char *eq = std::strchr(argv[i] + 2, '=');
                if (eq) {
                    std::string name(argv[i] + 2, eq);
                    std::string value(eq + 1);
                    check_option(name, value);
                } else {
                    std::string name(argv[i] + 2);
                    auto it = options_.find(name);
                    if (it == options_.end()) {
                        errors_.push_back("undefined option: --" + name);
                        continue;
                    }
                    // an option with value
                    if (it->second->has_value()) {
                        if (i + 1 < argc) {
                            check_option(name, argv[++i]);
                        } else {
                            errors_.push_back("option needs value: --" + name);
                            continue;
                        }
                    }
                    // just a flag
                    else {
                        check_option(name);
                    }
                }
            }
            // if is short option "-x"
            else if (std::strncmp(argv[i], "-", 1) == 0) {
                if (!argv[i][1])
                    continue;
                char last_option = argv[i][1];
                // deal with combined short options
                for (int j = 2; argv[i][j]; j++) {
                    last_option = argv[i][j];
                    if (!check_short_option(lookup, argv[i][j - 1]))
                        continue;
                    check_option(lookup[argv[i][j - 1]]);
                }

                if (!check_short_option(lookup, last_option))
                    continue;
                const std::string &long_name = lookup[last_option];
                // an option with value
                if (i + 1 < argc && options_[long_name]->has_value()) {
                    check_option(long_name, argv[i + 1]);
                    i++;
                }
                // just a flag
                else {
                    check_option(long_name);
                }
            } else {
                others_.push_back(argv[i]);
            }
        }

        // check options requirements
        for (const auto &p : options_) {
            auto ret = p.second->valid();
            if (!ret.second) {
                errors_.push_back(ret.first + ": --" + std::string(p.first));
            }
        }

        return errors_.empty();
    }

    /**
     * @brief Parse and validate a command line string.
     * @param arg Command line text to tokenize and parse.
     * @param with_program_name Whether the first token should be treated as the program name.
     * @return true if parsing succeeds and execution should continue; false for help/version requests.
     * @throws cmdline_error if parsing fails.
     */
    bool parse_check(const std::string &arg, bool with_program_name = true)
    {
        if (!options_.count("help"))
            flag("help", '?', "print this message");
        if (!options_.count("version"))
            flag("version", 'V', "show version");
        return check(parse(arg, with_program_name));
    }

    /**
     * @brief Parse and validate command line arguments from a string vector.
     * @param args Argument vector.
     * @param with_program_name Whether args[0] should be treated as the program name.
     * @return true if parsing succeeds and execution should continue; false for help/version requests.
     * @throws cmdline_error if parsing fails.
     */
    bool parse_check(const std::vector<std::string> &args, bool with_program_name = true)
    {
        if (!options_.count("help"))
            flag("help", '?', "print this message");
        if (!options_.count("version"))
            flag("version", 'V', "show version");
        return check(parse(args, with_program_name));
    }

    /**
     * @brief Parse and validate command line arguments.
     * @param argc Argument count.
     * @param argv Argument array.
     * @param with_program_name Whether argv[0] should be treated as the program name.
     * @return true if parsing succeeds and execution should continue; false for help/version requests.
     * @throws cmdline_error if parsing fails.
     */
    bool parse_check(int argc, char *argv[], bool with_program_name = true)
    {
        if (!options_.count("help"))
            flag("help", '?', "print this message");
        if (!options_.count("version"))
            flag("version", 'V', "show version");
        return check(parse(argc, argv, with_program_name));
    }

    /**
     * @brief Get the first parse error message.
     * @return First parse error, or an empty string when no error exists.
     */
    std::string error() const
    {
        return errors_.size() > 0 ? errors_[0] : "";
    }

    /**
     * @brief Get all parse error messages.
     * @return Newline-separated parse errors, or an empty string when no error exists.
     */
    std::string error_full() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < errors_.size(); i++)
            oss << errors_[i] << '\n';
        return oss.str();
    }

    /**
     * @brief Generate help text for the current parser state.
     * @return Usage and options text.
     */
    std::string help() const
    {
        std::ostringstream oss;
        oss << "Usage: " << prog_name_ << ' ' << usage();
        if (intro_ != "")
            oss << intro_ << "\n\n";
        usage(oss);
        return oss.str();
    }

    /**
     * @brief Get the configured version string.
     * @return Version text set by version(const char *).
     */
    std::string version() const
    {
        return version_;
    }

protected:
    virtual std::string usage() const
    {
        return "[options] ... " + ftr_ + '\n';
    }

    virtual void usage(std::ostringstream &oss) const
    {
        if (options_.empty())
            return;

        oss << "Options:\n";

        size_t width = max_width();
        for (const auto &opt : ordered_) {
            if (opt->short_name()) {
                oss << "  -" << opt->short_name() << ", ";
            } else {
                oss << "      ";
            }

            oss << "--" << opt->name();
            for (size_t j = opt->name().size(); j < width + 4; j++)
                oss << ' ';
            oss << opt->description(width + 12 + 2) << '\n';
        }
    }

    virtual size_t max_width() const
    {
        size_t max_width = 0;
        for (const auto &opt : ordered_) {
            max_width = std::max(max_width, opt->name().size());
        }
        return max_width;
    }

private:
    bool check(bool ok) const
    {
        // 打印版本信息时，不打印错误信息
        if (!version_.empty() && option_is_set("version")) {
            return false;
        }
        // 显式输入 help 选项的情况：打印帮助信息
        if (option_is_set("help")) {
            return false;
        }
        // 输入 program_name xxx 的情况：输出错误信息和帮助信息
        if (!ok) {
            std::string msg = error_full();
            if (msg.empty()) {
                msg = error();
            }
            if (msg.empty()) {
                msg = "invalid command line";
            }
            throw cmdline_error(msg);
        }
        return true;
    }

    bool option_is_set(const std::string &name) const
    {
        auto it = options_.find(name);
        return it != options_.end() && it->second->has_set();
    }

    bool check_short_option(std::map<char, std::string> &lookup, char option)
    {
        auto it = lookup.find(option);
        if (it == lookup.end()) {
            errors_.push_back(std::string("undefined short option: -") + option);
            return false;
        }
        if (it->second == "") {
            errors_.push_back(std::string("ambiguous short option: -") + option);
            return false;
        }
        return true;
    }

    void check_option(const std::string &name)
    {
        auto it = options_.find(name);
        if (it == options_.end()) {
            errors_.push_back("undefined option: --" + name);
            return;
        }
        if (!it->second->set()) {
            std::string msg = "option needs value: --" + name;
            if (!it->second->last_error().empty()) {
                msg += " (" + it->second->last_error() + ")";
            }
            errors_.push_back(msg);
            return;
        }
    }

    void check_option(const std::string &name, const std::string &value)
    {
        auto it = options_.find(name);
        if (it == options_.end()) {
            errors_.push_back("undefined option: --" + name);
            return;
        }
        if (!it->second->set(value)) {
            std::string msg = "option value is invalid: --" + name + "=" + value;
            if (!it->second->last_error().empty()) {
                msg += " (" + it->second->last_error() + ")";
            }
            errors_.push_back(msg);
            return;
        }
    }

    class option_base
    {
    public:
        option_base(const std::string &long_name, char short_name,
                    const class description &desc)
            : nam_(long_name)
            , snam_(short_name)
            , desc_(desc)
            , has_value_(false)
        {
        }

        virtual ~option_base() = default;

        // is option with value
        virtual bool has_value() const = 0;
        // set option without value
        virtual bool set() = 0;
        // set option with value
        virtual bool set(const std::string &value) = 0;
        // if option value has been set
        bool has_set() const
        {
            return has_value_;
        }
        // if value is valid
        virtual std::pair<std::string, bool> valid() const
        {
            return {"", true};
        }
        // if option is required
        virtual bool required() const = 0;

        const std::string &name() const
        {
            return nam_;
        }

        char short_name() const
        {
            return snam_;
        }

        std::string description(size_t indent = 0) const
        {
            return desc_.dump(indent);
        }

        virtual std::string short_description() const = 0;

        const std::string &last_error() const
        {
            return error_;
        }

    protected:
        void clear_error()
        {
            error_.clear();
        }

        void set_error(const std::string &error)
        {
            error_ = error;
        }

        const std::string nam_;
        const char snam_;
        class description desc_;
        // whether value has been set
        bool has_value_;
        std::string error_;
    };

    // flags are options without value
    class option_without_value : public option_base
    {
    public:
        option_without_value(const std::string &long_name, char short_name,
                             const class description &desc)
            : option_base(long_name, short_name, desc)
        {
            this->desc_.wrap(full_description());
        }

        ~option_without_value() = default;

        bool has_value() const override
        {
            return false;
        }

        bool set() override
        {
            clear_error();
            has_value_ = true;
            return true;
        }

        bool set(const std::string &) override
        {
            set_error("flag does not accept a value");
            return false;
        }

        bool required() const override
        {
            return false;
        }

        std::string short_description() const override
        {
            return "--" + nam_;
        }

    private:
        class description full_description() const
        {
            return g_config.show_option_typename ? cmdline::description(" (bool)") : cmdline::description();
        }
    };

    template<typename T>
    class option_with_value : public option_base
    {
    public:
        option_with_value(const std::string &long_name, char short_name,
                          const class description &description,
                          bool required, const T &default_value)
            : option_base(long_name, short_name, description)
            , required_(required)
            , actual_(default_value)
            , with_default_(true)
            , occurred_(false)
        {
            this->desc_.wrap(full_description(required, default_value));
        }

        option_with_value(const std::string &long_name, char short_name, const class description &description,
                          bool required)
            : option_base(long_name, short_name, description)
            , required_(required)
            , with_default_(false)
            , occurred_(false)
        {
            this->desc_.wrap(full_description(required));
        }

        ~option_with_value() = default;

        const T &get() const
        {
            if (occurred_ && !with_default_ && !has_value_)
                throw cmdline_error("option without value: --" + nam_);
            return actual_;
        }

        bool has_value() const override
        {
            return true;
        }

        bool set() override
        {
            this->set_error("option requires a value");
            return false;
        }

        bool set(const std::string &value) override
        {
            try {
                actual_ = read(value);
                has_value_ = true;
                occurred_ = true;
                this->clear_error();
            } catch (const std::exception &ex) {
                this->set_error(ex.what());
                std::cerr << ex.what() << '\n';
                return false;
            }
            return true;
        }

        std::pair<std::string, bool> valid() const override
        {
            // if required and not set, invalid
            if (required_ && !has_value_) {
                return {"need option", false};
            }
            // if occurred and no value can be used, invalid
            if (occurred_ && !with_default_ && !has_value_)
                return {"option needs value", false};
            return {"", true};
        }

        bool required() const override
        {
            return required_;
        }

        std::string short_description() const override
        {
            return "--" + nam_ + "=" + detail::readable_typename<T>();
        }

        bool occurred() const
        {
            return occurred_;
        }

        bool with_default() const
        {
            return with_default_;
        }

    protected:
        class description full_description(bool required, const T &default_value = T()) const
        {
            const std::string typename_str = (g_config.show_option_typename ? detail::readable_typename<T>()  : "");
            const std::string required_str = (required ? "required" : "");
            const std::string default_value_str = (with_default_ ? "[=" + detail::default_value<T>(default_value) + "]" : "");

            std::string msg;
            msg += typename_str.empty() ? "" : typename_str;
            msg += required_str.empty() ? "" : required_str;
            msg += default_value_str.empty() ? "" : default_value_str;
            if (!msg.empty()) {
                msg = " (" + msg + ')';
            }
            return cmdline::description(msg);
        }

        virtual T read(const std::string &s) = 0;
        // whether this option is required
        const bool required_;
        // the actual value passed from command line
        T actual_;
        // whether this option has default value
        const bool with_default_;
        // whether this option has occurred
        bool occurred_;
    };

    template<typename T, typename R>
    class option_with_value_with_reader : public option_with_value<T>
    {
    public:
        option_with_value_with_reader(const std::string &long_name, char short_name, const class description &desc,
                                      bool required, const T default_value, R value_reader)
            : option_with_value<T>(long_name, short_name, desc, required, default_value)
            , reader_(value_reader)
        {
            this->desc_.wrap(full_description());
        }
        option_with_value_with_reader(const std::string &long_name, char short_name, const class description &desc,
                                      bool required, R value_reader)
            : option_with_value<T>(long_name, short_name, desc, required)
            , reader_(value_reader)
        {
            this->desc_.wrap(full_description());
        }

    private:
        class description full_description()
        {
            return cmdline::description(" " + reader_.constraint());
        }

        T read(const std::string &s)
        {
            return reader_(s);
        }

        R reader_;
    };

protected:
    // program name
    std::string prog_name_;

private:
    // options_ store all options in <long_name, option_base *>
    std::map<std::string, std::unique_ptr<option_base>> options_;
    // options_ are sorted by the order they are added, used for printing help
    std::vector<option_base *> ordered_;
    // introduction message
    std::string intro_;
    // footer message
    std::string ftr_;
    // other arguments not parsed as options
    std::vector<std::string> others_;
    // errors during parsing
    std::vector<std::string> errors_;
    // version
    std::string version_;
};

class command : public parser
{
public:
    // Here, passing a pointer also has an advantage that allows the callback function to use polymorphism.
    using callback_t = std::function<int(command *cmd)>;

    /**
     * @brief Construct a command with a callback.
     * @param name Command name.
     * @param desc Short command description used in help text.
     * @param callback Function invoked when this command runs.
     */
    command(const std::string &name, const std::string &desc, const callback_t &callback)
        : name_(name)
        , desc_(desc)
        , callback_(callback)
    {
        program_name(name);
    }
    command(const command &other) = delete;
    command &operator=(const command &other) = delete;
    command(command &&other) noexcept = default;
    command &operator=(command &&other) noexcept = delete;

    // for root command
    /**
     * @brief Execute the root command with command line arguments.
     * @param argc Argument count.
     * @param argv Argument array.
     * @return Callback return code, 0 for handled help/version requests.
     * @throws cmdline_error if parsing fails or a command is undefined.
     */
    int operator()(int argc, char *argv[])
    {
        bool has_cmd = true;
        if (argc < 2 || std::strncmp("-", argv[1], 1) == 0) {
            has_cmd = false;
        }
        int retcode = 0;
        if (has_cmd) {
            const std::string cmd_name = argv[1];
            auto it = subcommands_.find(cmd_name);
            if (it == subcommands_.end()) {
                throw cmdline_error("undefined command: " + cmd_name);
            } else {
                retcode = it->second(prog_name_ + " " + cmd_name, argc - 2, argv + 2);
            }
        } else {
            if (!parse_check(argc, argv)) {
                print_help_or_version();
                return 0;
            }
            retcode = run();
        }
        return retcode;
    }

    /**
     * @brief Add a subcommand.
     * @param cmd Subcommand to move into this command.
     * @return Reference to this command for method chaining.
     * @throws cmdline_error if a subcommand with the same name already exists.
     */
    command &add(command &&cmd)
    {
        auto res = subcommands_.emplace(cmd.name_, std::move(cmd));
        if (!res.second)
            throw cmdline_error("multiple command definition: " + cmd.name_);
        ordered_.push_back(res.first);
        return *this;
    }

    /**
     * @brief Get the command name.
     * @return Command name.
     */
    const std::string &name() const
    {
        return name_;
    }

    /**
     * @brief Get the command description.
     * @return Command description.
     */
    const std::string &description() const
    {
        return desc_;
    }

protected:
    std::string usage() const override
    {
        return "[command] " + parser::usage();
    }

    void usage(std::ostringstream &oss) const override
    {
        if (!subcommands_.empty()) {
            size_t max_width = 0;
            for (const auto &cmd : ordered_) {
                max_width = std::max(max_width, cmd->second.name().size());
            }
            oss << "Commands:\n";
            for (const auto &cmd : ordered_) {
                oss << "  " << cmd->first << ' ';
                for (size_t j = cmd->first.size(); j < max_width + 9; ++j)
                    oss << ' ';
                oss << cmd->second.description() << '\n';
            }
        }

        parser::usage(oss);
    }

    size_t max_width() const override
    {
        size_t max_width = parser::max_width();
        for (const auto &cmd : ordered_) {
            max_width = std::max(max_width, cmd->second.name().size());
        }
        return max_width;
    }

private:
    // for sub commands
    int operator()(const std::string &cmd_name, int argc, char *argv[])
    {
        program_name(cmd_name);
        if (!parse_check(argc, argv, false)) {
            print_help_or_version();
            return 0;
        }
        return run();
    }

    void print_help_or_version() const
    {
        if (exist("version") && !version().empty()) {
            std::cout << version() << std::endl;
        } else {
            std::cout << help();
        }
    }

    int run()
    {
        int retcode = 0;
        // prev hooks
        retcode = callback_(this);
        // post hooks
        return retcode;
    }

    const std::string name_;
    const std::string desc_;
    callback_t callback_;

    using command_map_t = std::map<std::string, command>;
    command_map_t subcommands_;
    std::vector<command_map_t::iterator> ordered_;
};

} // namespace cmdline
