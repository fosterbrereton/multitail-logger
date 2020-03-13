/**************************************************************************************************/
// Multitail-savvy file-based logging facility.
//
// Multitail docs: https://www.vanheusden.com/multitail/index.php
//
// The only API worth knowing about is multitail::post(name, value). It will serialize `value` and
// append it to the end of a file named `name` within a session-unique directory. These files can
// then be monitored simultaneously with an invocation of `multitail` to that directory. On macOS,
// this will happen automatically in Terminal.app the first time `multitail::post` is called.
//
// Note: This has been written with macOS in mind, though could be easily ported to other platforms.
//
// To Do:
//     - Ability for a multitail window to only display the last thing in the log file.
//     - Use std::filesystem instead of std::string for path work (requires macOS 10.15).
//     - Improve the mutex/blocking nature of the implementation
/**************************************************************************************************/

#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <random>

#include <unistd.h>

/**************************************************************************************************/

namespace multitail {

/**************************************************************************************************/

namespace detail {

/**************************************************************************************************/

inline const std::string& session_id() {
    static const auto id = []{
        std::random_device rd{};
        std::mt19937 gen{rd()};
        std::uniform_int_distribution<> dis(100000000, 999999999);
        return "multitail-" + std::to_string(dis(gen));
    }();
    return id;
}

/**************************************************************************************************/

inline std::string make_path(std::string filename) {
    static auto inited{[]{
        std::system(("mkdir -p /tmp/" + session_id()).c_str());
        return true;
    }()};
    (void)inited;

    return "/tmp/" + session_id() + "/" + std::move(filename);
}

/**************************************************************************************************/
// Creates the file on ctor and (optionally) deletes it on dtor
struct auto_file {
    explicit auto_file(std::string f) : _p{make_path(std::move(f))}, _s{_p, std::ios_base::binary} {
        assert(static_cast<bool>(_s));
    }

    ~auto_file() {
        if (_self_destruct) {
            unlink(_p.c_str());
        }
    }

    template <typename T>
    friend inline auto operator<<(auto_file& f, T&& x) {
        static auto open_terminal{[]{
            const std::string thescript[] = {
                "tell application \"Terminal\"",
                //"    activate",
                "    set thescript to \"multitail -s 4 -n 1 -q 0 \\\"/tmp/" + session_id() + "/*\\\" --basename\"",
                "    if exists window 0 then",
                "        set curtab to do script thescript in window 0",
                "    else",
                "        set curtab to do script thescript",
                "    end if",
                "    set number of rows of curtab to 50",
                "    set number of columns of curtab to 200",
                "end tell"
            };
            std::string cmdline("osascript");
    
            for (const auto& line : thescript) {
                cmdline += " -e '" + line + "'";
            }

            std::system(cmdline.c_str());
            return true;
        }()};
        (void)open_terminal;

        f._s << std::forward<T>(x) << '\n' << std::flush;
    }

    void self_destruct() { _self_destruct = true; }

private:
    std::string _p;
    std::ofstream _s;
    bool _self_destruct{false};
};

/**************************************************************************************************/

struct block {
    block(std::string p) : _f{std::move(p)} {}

    template <typename T>
    friend inline auto operator<<(block& b, T&& x) {
        b._f << std::forward<T>(x);
    }

    std::mutex _m;
    auto_file _f;
};

/**************************************************************************************************/

inline auto block(const std::string& name) -> block& {
    static std::unordered_map<std::string, std::unique_ptr<struct block>> map;
    static std::mutex m;

    std::lock_guard<std::mutex> lock{m};

    auto found = map.find(name);

    if (found != map.end()) {
        return *found->second;
    }

    return *map.emplace(name, std::make_unique<struct block>(name)).first->second;
}

/**************************************************************************************************/

} // namespace detail

/**************************************************************************************************/

template <typename T>
void post(const std::string& name, T&& value) {
    multitail::detail::block(name) << std::forward<T>(value);
}

/**************************************************************************************************/

} // namespace multitail

/**************************************************************************************************/
