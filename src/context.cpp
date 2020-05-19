#include <csignal>

#include "output.hpp"
#include "util.hpp"
#include "context.hpp"
#include "thirdparty/termcolor.hpp"

namespace mamba
{
    namespace
    {
// armv6l and armv7l
#if defined(__arm__) || defined(__thumb__)
    #ifdef ___ARM_ARCH_6__
        const std::string MAMBA_PLATFORM = "armv6l";
    #elif __ARM_ARCH_7__
        const std::string MAMBA_PLATFORM = "armv7l";
    #else
        #error "Unknown platform"
    #endif
#elif _M_ARM==6
        const std::string MAMBA_PLATFORM = "armv6l";
#elif _M_ARM==7
        const std::string MAMBA_PLATFORM = "armv7l";
// aarch64
#elif defined(__aarch64__)
        const std::string MAMBA_PLATFORM = "aarch64";
#elif defined(__ppc64__) || defined(__powerpc64__)
        const std::string MAMBA_PLATFORM = "ppc64";
// TODO: detect ppc64le
// Linux
#elif defined(__linux__)
    #if __x86_64__
        const std::string MAMBA_PLATFORM = "linux-64";
    #else
        const std::string MAMBA_PLATFORM = "linux-32";
    #endif
// OSX
#elif defined(__APPLE__) || defined(__MACH__)
        const std::string MAMBA_PLATFORM = "osx-64";
// Windows
#elif defined(_WIN64)
        const std::string MAMBA_PLATFORM = "win-64";
#elif defined (_WIN32)
        const std::string MAMBA_PLATFORM = "win-32";
#else
    #error "Unknown platform"
#endif
    }
    Context::Context()
    {
        set_verbosity(0);
        on_ci = (std::getenv("CI") != nullptr);
        if (on_ci || !termcolor::_internal::is_atty(std::cout))
        {
            no_progress_bars = true;
        }

        std::signal(SIGINT, [](int signum) {
            instance().sig_interrupt = true;
        });
    }

    Context& Context::instance()
    {
        static Context ctx;
        return ctx;
    }

    void Context::set_verbosity(int lvl)
    {
        MessageLogger::global_log_severity() = mamba::LogSeverity::error;
        if (lvl == 1)
        {
            MessageLogger::global_log_severity() = mamba::LogSeverity::info;
        }
        else if (lvl > 1)
        {
            MessageLogger::global_log_severity() = mamba::LogSeverity::debug;
        }
        this->verbosity = lvl;
    }

    std::string Context::platform() const
    {
        return MAMBA_PLATFORM;
    }

    std::vector<std::string> Context::platforms() const
    {
        return { platform(), "noarch" };
    }

    std::string env_name(const fs::path& prefix)
    {
        if (prefix.empty())
        {
            throw std::runtime_error("Empty path");
        }
        if (paths_equal(prefix, Context::instance().root_prefix))
        {
            return ROOT_ENV_NAME;
        }
        fs::path maybe_env_dir = prefix.parent_path();
        for (const auto& d : Context::instance().envs_dirs)
        {
            if (paths_equal(d, maybe_env_dir))
            {
                return prefix.filename();
            }
        }
        return prefix;
    }

    // Find the location of a prefix given a conda env name.
    // If the location does not exist, an error is raised.
    fs::path locate_prefix_by_name(const std::string& name)
    {
        assert(!name.empty());
        if (name == ROOT_ENV_NAME)
        {
            return Context::instance().root_prefix;
        }
        for (auto& d : Context::instance().envs_dirs)
        {
            if (!fs::exists(d) || !fs::is_directory(d))
            {
                continue;
            }
            fs::path prefix = d / name;
            if (fs::exists(prefix) && fs::is_directory(prefix))
            {
                return fs::absolute(prefix);
            }
        }

        throw std::runtime_error("Environment name not found " + name);
    }
}
