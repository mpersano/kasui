#include "utils.h"

#include <functional>

std::wstring format_number(int n)
{
    std::wstring result;

    std::function<void(int)> format_thousands = [&result, &format_thousands](int n) {
        if (n < 1000) {
            if (n >= 100)
                result.push_back('0' + (n / 100));
            if (n >= 10)
                result.push_back('0' + (n / 10) % 10);
            result.push_back('0' + n % 10);
            return;
        }

        format_thousands(n / 1000);

        const int m = n % 1000;
        result.push_back(',');
        result.push_back('0' + (m / 100));
        result.push_back('0' + (m / 10) % 10);
        result.push_back('0' + m % 10);
    };

    format_thousands(n);

    return result;
}
