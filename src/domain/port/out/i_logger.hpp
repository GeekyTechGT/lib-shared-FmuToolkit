#pragma once

/// @file i_logger.hpp
/// @brief Port interface for receiving library log messages.
/// @internal

#include <libfmu_toolkit/types.hpp>  // LogLevel
#include <string>

namespace libfmu_toolkit {

/// @brief Port interface for receiving library log messages (Dependency Inversion).
///
/// Inject a concrete implementation to capture log messages from the simulation
/// runtime. The default NullLogger discards all output.
///
/// **Example custom logger:**
/// @code
/// class ConsoleLogger : public libfmu_toolkit::ILogger {
/// public:
///     void log(libfmu_toolkit::LogLevel level, const std::string& msg) override {
///         std::cout << "[" << static_cast<int>(level) << "] " << msg << "\n";
///     }
/// };
/// @endcode
/// @req FR-04.1
class ILogger {
public:
    virtual ~ILogger() = default;

    /// @brief Receives a log message from the library.
    ///
    /// Implementations must be thread-compatible (not necessarily thread-safe).
    /// This method may be called from any thread that uses the FmuEngine.
    ///
    /// @param level    Severity level of the message.
    /// @param message  Human-readable log message.
    /// @req FR-04.1
    virtual void log(LogLevel level, const std::string& message) = 0;
};

/// @brief Default logger that silently discards all messages.
///
/// Used as the fallback when no custom logger is injected.
/// @req FR-04.1
class NullLogger final : public ILogger {
public:
    /// @brief No-op: discards `level` and `message`.
    /// @req FR-04.1
    void log(LogLevel /*level*/, const std::string& /*message*/) override {}
};

} // namespace libfmu_toolkit
