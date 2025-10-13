/*
  Copyright (c) 2009, Hideyuki Tanaka
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the <organization> nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY <copyright holder> ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

#if !defined(_MSC_VER) && (defined(__clang__) || defined(__GNUC__))
#include <cxxabi.h>
#endif
#include <cstdlib>

#ifdef max
#undef max
#endif

namespace cmdline
{

namespace detail
{

template<typename Target, typename Source, bool Same>
class lexical_cast_t
{
public:
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
class lexical_cast_t<Target, Source, true>
{
public:
    static Target cast(const Source &arg)
    {
        return arg;
    }
};

template<typename Source>
class lexical_cast_t<std::string, Source, false>
{
public:
    static std::string cast(const Source &arg)
    {
        std::ostringstream ss;
        ss << arg;
        return ss.str();
    }
};

template<typename Target>
class lexical_cast_t<Target, std::string, false>
{
public:
    static Target cast(const std::string &arg)
    {
        Target ret;
        std::istringstream ss(arg);
        if (!(ss >> ret && ss.eof()))
            throw std::bad_cast();
        return ret;
    }
};

// template<typename T1, typename T2>
// struct is_same
// {
//     static const bool value = false;
// };

// template<typename T>
// struct is_same<T, T>
// {
//     static const bool value = true;
// };

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

template<class T>
std::string readable_typename()
{
    return demangle(typeid(T).name());
}

template<class T>
std::string default_value(T def)
{
    return detail::lexical_cast<std::string>(def);
}

template<>
inline std::string readable_typename<std::string>()
{
    return "string";
}

} // namespace detail

class cmdline_error : public std::exception
{
public:
    cmdline_error(const std::string &message)
        : msg_(message)
    {
    }
    ~cmdline_error() = default;
    const char *what() const
    {
        return msg_.c_str();
    }

private:
    std::string msg_;
};

template<class T>
struct default_reader
{
    T operator()(const std::string &str)
    {
        return detail::lexical_cast<T>(str);
    }
};

template<class T>
struct range_reader
{
    range_reader(const T &low_bound, const T &upper_bound)
        : low_(low_bound)
        , high_(upper_bound)
    {
    }
    T operator()(const std::string &s) const
    {
        T ret = default_reader<T>()(s);
        if (!(ret >= low_ && ret <= high_))
            throw cmdline::cmdline_error("range_error");
        return ret;
    }

private:
    T low_, high_;
};

template<class T>
range_reader<T> range(const T &lower_bound, const T &upper_bound)
{
    return range_reader<T>(lower_bound, upper_bound);
}

template<class T>
struct oneof_reader
{
    T operator()(const std::string &s)
    {
        T ret = default_reader<T>()(s);
        if (std::find(alt_.begin(), alt_.end(), ret) == alt_.end())
            throw cmdline_error("");
        return ret;
    }
    void add(const T &v)
    {
        alt_.push_back(v);
    }

private:
    std::vector<T> alt_;
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

template<typename T, typename... Rest>
oneof_reader<T> oneof(Rest &&...rest)
{
    oneof_reader<T> reader;
    return oneof_impl(reader, std::forward<Rest>(rest)...);
}

// template <class T>
// oneof_reader<T> oneof(T a1)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   return ret;
// }

// template <class T>
// oneof_reader<T> oneof(T a1, T a2)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   return ret;
// }

// template <class T>
// oneof_reader<T> oneof(T a1, T a2, T a3)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   return ret;
// }

// template <class T>
// oneof_reader<T> oneof(T a1, T a2, T a3, T a4)
// {
//   oneof_reader<T> ret;
//   ret.add(a1);
//   ret.add(a2);
//   ret.add(a3);
//   ret.add(a4);
//   return ret;
// }

// template <class T>
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

// template <class T>
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

// template <class T>
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

// template <class T>
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

// template <class T>
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

// template <class T>
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
    const std::string brief_;
    const std::string detail_;
};

class parser
{
public:
    parser() = default;
    ~parser()
    {
        for (std::map<std::string, option_base *>::iterator p = options_.begin();
             p != options_.end(); p++)
            delete p->second;
    }

    // add flag
    parser &add(const std::string &full_name, char short_name = 0,
                const std::string &description = "")
    {
        if (options_.count(full_name))
            throw cmdline_error("multiple definition: " + full_name);
        options_[full_name] =
            new option_without_value(full_name, short_name, description);
        ordered_.push_back(options_[full_name]);
        return *this;
    }

    // add option with value
    template<class T>
    parser &add(const std::string &full_name, char short_name = 0,
                const std::string &description = "", bool is_needed = true,
                const T definition = T())
    {
        return add(full_name, short_name, description, is_needed, definition,
                   default_reader<T>());
    }

    // add option with value
    template<class T, class R>
    parser &add(const std::string &full_name, char short_name = 0,
                const std::string &description = "", bool is_needed = true,
                const T definition = T(), R value_reader = R())
    {
        if (options_.count(full_name))
            throw cmdline_error("multiple definition: " + full_name);
        options_[full_name] = new option_with_value_with_reader<T, R>(
            full_name, short_name, is_needed, definition, description,
            value_reader);
        ordered_.push_back(options_[full_name]);
        return *this;
    }

    // parser &add(const std::string &full_name, char short_name = 0,
    //             const std::string &description = "")
    // {
    //     if (options_.count(full_name))
    //         throw cmdline_error("multiple definition: " + full_name);
    //     options_[full_name] =
    //         new option_without_value(full_name, short_name, description);
    //     ordered_.push_back(options_[full_name]);
    //     return *this;
    // }

    // add option with value
    template<class T>
    parser &add(const std::string &full_name, char short_name = 0,
                const class description &desc = description(), bool is_needed = true,
                const T definition = T())
    {
        return add(full_name, short_name, desc, is_needed, definition,
                   default_reader<T>());
    }

    // add option with value
    template<class T, class R>
    parser &add(const std::string &full_name, char short_name = 0,
                const class description &desc = description(), bool is_needed = true,
                const T definition = T(), R value_reader = R())
    {
        if (options_.count(full_name))
            throw cmdline_error("multiple definition: " + full_name);
        options_[full_name] = new option_with_value_with_reader<T, R>(
            full_name, short_name, is_needed, definition, desc,
            value_reader);
        ordered_.push_back(options_[full_name]);
        return *this;
    }

    parser &footer(const std::string &f)
    {
        ftr_ = f;
        return *this;
    }

    parser &set_program_name(const std::string &name)
    {
        prog_name_ = name;
        return *this;
    }

    bool exist(const std::string &name) const
    {
        if (options_.count(name) == 0)
            throw cmdline_error("there is no flag: --" + name);
        return options_.find(name)->second->has_set();
    }

    template<class T>
    const T &get(const std::string &name) const
    {
        if (options_.count(name) == 0)
            throw cmdline_error("there is no flag: --" + name);
        const option_with_value<T> *p =
            dynamic_cast<const option_with_value<T> *>(options_.find(name)->second);
        if (p == nullptr)
            throw cmdline_error("type mismatch flag '" + name + "'");
        return p->get();
    }

    const std::vector<std::string> &rest() const
    {
        return others_;
    }

    bool parse(const std::string &arg)
    {
        std::vector<std::string> args;

        std::string buf;
        bool in_quote = false;
        for (std::string::size_type i = 0; i < arg.length(); i++) {
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
                if (i >= arg.length()) {
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

        if (buf.length() > 0)
            args.push_back(buf);

        for (size_t i = 0; i < args.size(); i++)
            std::cout << "\"" << args[i] << "\"" << std::endl;

        return parse(args);
    }

    bool parse(const std::vector<std::string> &args)
    {
        int argc = static_cast<int>(args.size());
        std::vector<const char *> argv(argc);

        for (int i = 0; i < argc; i++)
            argv[i] = args[i].c_str();

        return parse(argc, &argv[0]);
    }

    bool parse(int argc, const char *const argv[])
    {
        errors_.clear();
        others_.clear();

        if (argc < 1) {
            errors_.push_back("argument number must be longer than 0");
            return false;
        }
        if (prog_name_ == "")
            prog_name_ = argv[0];

        std::map<char, std::string> lookup; // <short_name, full_name>
        for (std::map<std::string, option_base *>::iterator p = options_.begin();
             p != options_.end(); p++) {
            if (p->first.length() == 0)
                continue;
            char initial = p->second->short_name();
            if (initial) {
                if (lookup.count(initial) > 0) {
                    lookup[initial] = "";
                    errors_.push_back(std::string("short option '") + initial + "' is ambiguous");
                    return false;
                } else
                    lookup[initial] = p->first;
            }
        }

        for (int i = 1; i < argc; i++) {
            if (std::strncmp(argv[i], "--", 2) == 0) {
                const char *p = strchr(argv[i] + 2, '=');
                if (p) {
                    std::string name(argv[i] + 2, p);
                    std::string val(p + 1);
                    set_option(name, val);
                } else {
                    std::string name(argv[i] + 2);
                    if (options_.count(name) == 0) {
                        errors_.push_back("undefined option: --" + name);
                        continue;
                    }
                    if (options_[name]->has_value()) {
                        if (i + 1 >= argc) {
                            errors_.push_back("option needs value: --" + name);
                            continue;
                        } else {
                            i++;
                            set_option(name, argv[i]);
                        }
                    } else {
                        set_option(name);
                    }
                }
            } else if (std::strncmp(argv[i], "-", 1) == 0) {
                if (!argv[i][1])
                    continue;
                char last = argv[i][1];
                for (int j = 2; argv[i][j]; j++) {
                    last = argv[i][j];
                    if (lookup.count(argv[i][j - 1]) == 0) {
                        errors_.push_back(std::string("undefined short option: -") + argv[i][j - 1]);
                        continue;
                    }
                    if (lookup[argv[i][j - 1]] == "") {
                        errors_.push_back(std::string("ambiguous short option: -") + argv[i][j - 1]);
                        continue;
                    }
                    set_option(lookup[argv[i][j - 1]]);
                }

                if (lookup.count(last) == 0) {
                    errors_.push_back(std::string("undefined short option: -") + last);
                    continue;
                }
                if (lookup[last] == "") {
                    errors_.push_back(std::string("ambiguous short option: -") + last);
                    continue;
                }

                if (i + 1 < argc && options_[lookup[last]]->has_value()) {
                    set_option(lookup[last], argv[i + 1]);
                    i++;
                } else {
                    set_option(lookup[last]);
                }
            } else {
                others_.push_back(argv[i]);
            }
        }

        for (std::map<std::string, option_base *>::iterator p = options_.begin();
             p != options_.end(); p++)
            if (!p->second->valid())
                errors_.push_back("need option: --" + std::string(p->first));

        return errors_.size() == 0;
    }

    void parse_check(const std::string &arg)
    {
        if (!options_.count("help"))
            add("help", '?', "print this message");
        check(0, parse(arg));
    }

    void parse_check(const std::vector<std::string> &args)
    {
        if (!options_.count("help"))
            add("help", '?', "print this message");
        check(static_cast<int>(args.size()), parse(args));
    }

    void parse_check(int argc, char *argv[])
    {
        if (!options_.count("help"))
            add("help", '?', "print this message");
        check(argc, parse(argc, argv));
    }

    std::string error() const
    {
        return errors_.size() > 0 ? errors_[0] : "";
    }

    std::string error_full() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < errors_.size(); i++)
            oss << errors_[i] << std::endl;
        return oss.str();
    }

    std::string usage() const
    {
        std::ostringstream oss;
        oss << "usage: " << prog_name_ << " ";
        for (size_t i = 0; i < ordered_.size(); i++) {
            if (ordered_[i]->must())
                oss << ordered_[i]->short_description() << " ";
        }

        oss << "[options] ... " << ftr_ << std::endl;
        oss << "options:" << std::endl;

        size_t max_width = 0;
        for (size_t i = 0; i < ordered_.size(); i++) {
            max_width = std::max(max_width, ordered_[i]->name().length());
        }
        for (size_t i = 0; i < ordered_.size(); i++) {
            if (ordered_[i]->short_name()) {
                oss << "  -" << ordered_[i]->short_name() << ", ";
            } else {
                oss << "      ";
            }

            oss << "--" << ordered_[i]->name();
            for (size_t j = ordered_[i]->name().length(); j < max_width + 4; j++)
                oss << ' ';
            oss << ordered_[i]->description(max_width + 12 + 2) << std::endl;
        }
        return oss.str();
    }

private:
    void check(int argc, bool ok)
    {
        if ((argc == 1 && !ok) || exist("help")) {
            std::cerr << usage();
            exit(0);
        }

        if (!ok) {
            std::cerr << error() << std::endl
                      << usage();
            exit(1);
        }
    }

    void set_option(const std::string &name)
    {
        if (options_.count(name) == 0) {
            errors_.push_back("undefined option: --" + name);
            return;
        }
        if (!options_[name]->set()) {
            errors_.push_back("option needs value: --" + name);
            return;
        }
    }

    void set_option(const std::string &name, const std::string &value)
    {
        if (options_.count(name) == 0) {
            errors_.push_back("undefined option: --" + name);
            return;
        }
        if (!options_[name]->set(value)) {
            errors_.push_back("option value is invalid: --" + name + "=" + value);
            return;
        }
    }

    class option_base
    {
    public:
        option_base(const std::string &full_name, char short_name,
                    const class description &desc)
            : nam_(full_name)
            , snam_(short_name)
            , desc_(desc)
            , has_(false)
        {
        }
        virtual ~option_base() = default;

        // is flag option
        virtual bool has_value() const = 0;
        // set option without value
        virtual bool set() = 0;
        // set option with value
        virtual bool set(const std::string &value) = 0;
        // if option value has been set
        bool has_set() const
        {
            return has_;
        }
        // if value is valid
        virtual bool valid() const
        {
            return true;
        }
        // if option must needed
        virtual bool must() const = 0;

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

    protected:
        const std::string nam_;
        const char snam_;
        const class description desc_;
        bool has_;
    };

    // flags are options without value
    class option_without_value : public option_base
    {
    public:
        option_without_value(const std::string &full_name, char short_name,
                             const class description &desc)
            : option_base(full_name, short_name, desc)
        {
        }
        ~option_without_value() = default;

        bool has_value() const override
        {
            return false;
        }

        bool set() override
        {
            has_ = true;
            return true;
        }

        bool set(const std::string &) override
        {
            return false;
        }

        bool must() const override
        {
            return false;
        }

        std::string short_description() const override
        {
            return "--" + nam_;
        }
    };

    template<class T>
    class option_with_value : public option_base
    {
    public:
        option_with_value(const std::string &full_name, char short_name,
                          bool is_needed, const T &definition,
                          const class description &description)
            : option_base(full_name, short_name, full_description(description))
            , need_(is_needed)
            , def_(definition)
            , actual_(definition)
        {
        }
        ~option_with_value() = default;

        const T &get() const
        {
            return actual_;
        }

        bool has_value() const override
        {
            return true;
        }

        bool set() override
        {
            return false;
        }

        bool set(const std::string &value) override
        {
            try {
                actual_ = read(value);
                has_ = true;
            } catch (const std::exception &) {
                return false;
            }
            return true;
        }

        bool valid() const override
        {
            if (need_ && !has_)
                return false;
            return true;
        }

        bool must() const override
        {
            return need_;
        }

        std::string short_description() const override
        {
            return "--" + nam_ + "=" + detail::readable_typename<T>();
        }

    protected:
        class description full_description(const class description &desc)
        {
            return cmdline::description(desc.brief() + " (" + detail::readable_typename<T>() + (need_ ? "" : " [=" + detail::default_value<T>(def_) + "]") + ")", desc.detail());
        }

        virtual T read(const std::string &s) = 0;

        const bool need_;
        const T def_;
        T actual_;
    };

    template<class T, class R>
    class option_with_value_with_reader : public option_with_value<T>
    {
    public:
        option_with_value_with_reader(const std::string &full_name, char short_name,
                                      bool is_needed, const T definition,
                                      const class description &desc,
                                      R value_reader)
            : option_with_value<T>(full_name, short_name, is_needed, definition,
                                   desc)
            , reader_(value_reader)
        {
        }

    private:
        T read(const std::string &s)
        {
            return reader_(s);
        }

        R reader_;
    };

    // options_ store all options in <full_name, option_base *>
    std::map<std::string, option_base *> options_;
    // options_ are sorted by the order they are added, used for printing help
    std::vector<option_base *> ordered_;
    // footer message
    std::string ftr_;
    // program name
    std::string prog_name_;
    // other arguments not parsed as options
    std::vector<std::string> others_;
    // errors during parsing
    std::vector<std::string> errors_;
};

} // namespace cmdline
