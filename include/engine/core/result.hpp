#pragma once

#include <string>
#include <utility>
#include <variant>

namespace Engine::Core {

// Recoverable-error type per DesignDoc §7.1 / §20.3. Lightweight std::variant
// wrapper — favor this over exceptions across module boundaries.
//
// The internal ErrorState wrapper exists so the variant can distinguish
// between a successful `T = std::string` payload and a failure carrying a
// message string.
template <typename T>
class Result {
private:
    struct ErrorState { std::string message; };

public:
    static Result Ok(T value)            { return Result(std::move(value)); }
    static Result Err(std::string message) { return Result(ErrorState{std::move(message)}); }

    bool IsOk()  const noexcept { return std::holds_alternative<T>(state_); }
    bool IsErr() const noexcept { return !IsOk(); }

    const T& Value() const&  { return std::get<T>(state_); }
    T&&      Value() &&      { return std::get<T>(std::move(state_)); }

    const std::string& Error() const { return std::get<ErrorState>(state_).message; }

private:
    explicit Result(T value)          : state_(std::move(value)) {}
    explicit Result(ErrorState err)   : state_(std::move(err))   {}

    std::variant<T, ErrorState> state_;
};

// Void specialization: success carries no payload.
template <>
class Result<void> {
public:
    static Result Ok()                    { return Result(true); }
    static Result Err(std::string message) { return Result(std::move(message)); }

    bool IsOk()  const noexcept { return ok_; }
    bool IsErr() const noexcept { return !ok_; }
    const std::string& Error() const { return error_; }

private:
    explicit Result(bool)        : ok_(true) {}
    explicit Result(std::string err) : ok_(false), error_(std::move(err)) {}

    bool        ok_;
    std::string error_;
};

}  // namespace Engine::Core
