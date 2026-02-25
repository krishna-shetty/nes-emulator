#ifndef NES_NOT_IMPLEMENTED_H
#define NES_NOT_IMPLEMENTED_H

#include <stdexcept>
#include <string>

namespace NES
{
    class NotImplemented : public std::logic_error
    {
    private:
        std::string _text;

        NotImplemented(const char* message, const char* functionName)
            : std::logic_error("Not Implemented")
            , _text(std::string(message) + " : " + (functionName ? functionName : ""))
        {}

    public:
        explicit NotImplemented(const char* functionName)
            : NotImplemented("Not Implemented", functionName)
        {}

        NotImplemented(const char* message, const char* functionName)
            : NotImplemented(message, functionName)
        {}

        const char* what() const noexcept override
        {
            return _text.c_str();
        }
    };
} // namespace NES

#endif // NES_NOT_IMPLEMENTED_H