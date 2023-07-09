/****************************************************************************
Copyright (c) 2010      cocos2d-x.org
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
Copyright (c) 2020 C4games Ltd
Copyright (c) 2021-2023 Bytedance Inc.

https://axmolengine.github.io/

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "../base/Utils.h"

#include <cmath>
#include <stdlib.h>

#include <signal.h>
#if !defined(_WIN32)
// for unix/linux kill
#    include <unistd.h>
#endif


#include "../base/Director.h"

#include "../base/base64.h"

int ccNextPOT(int x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

namespace utilsX
{
    namespace base64
    {
        inline int encBound(int sourceLen)
        {
            return (sourceLen + 2) / 3 * 4;
        }
        inline int decBound(int sourceLen)
        {
            return sourceLen / 4 * 3 + 1;
        }
    }
    // namespace base64


#define MAX_ITOA_BUFFER_SIZE 256
    double atof(const char* str)
    {
        if (str == nullptr)
        {
            return 0.0;
        }

        char buf[MAX_ITOA_BUFFER_SIZE];
        strncpy(buf, str, MAX_ITOA_BUFFER_SIZE);

        // strip string, only remain 7 numbers after '.'
        char* dot = strchr(buf, '.');
        if (dot != nullptr && dot - buf + 8 < MAX_ITOA_BUFFER_SIZE)
        {
            dot[8] = '\0';
        }

        return ::atof(buf);
    }


    std::vector<int> parseIntegerList(std::string_view intsString)
    {
        std::vector<int> result;

        if (!intsString.empty())
        {
            const char* cStr = intsString.data();
            char* endptr;

            for (auto i = strtol(cStr, &endptr, 10); endptr != cStr; i = strtol(cStr, &endptr, 10))
            {
                if (errno == ERANGE)
                {
                    errno = 0;
                    AXLOGWARN("%s contains out of range integers", intsString.data());
                }
                result.emplace_back(static_cast<int>(i));
                cStr = endptr;
            }
        }

        return result;
    }

    std::string bin2hex(std::string_view binary /*charstring also regard as binary in C/C++*/, int delim, bool prefix)
    {
        char low;
        char high;
        size_t len = binary.length();

        bool delim_needed = delim != -1 || delim == ' ';

        std::string result;
        result.reserve((len << 1) + (delim_needed ? len : 0) + (prefix ? (len << 1) : 0));

        for (size_t i = 0; i < len; ++i)
        {
            auto ch = binary[i];
            high = (ch >> 4) & 0x0f;
            low = ch & 0x0f;
            if (prefix)
            {
                result.push_back('0');
                result.push_back('x');
            }

            auto hic = nibble2hex(high);
            auto lic = nibble2hex(low);
            result.push_back(hic);
            result.push_back(lic);
            if (delim_needed)
            {
                result.push_back(delim);
            }
        }

        return result;
    }

    std::string urlEncode(std::string_view s)
    {
        std::string encoded;
        if (!s.empty())
        {
            encoded.reserve(s.length() * 3 / 2);
            for (const char c : s)
            {
                if (isalnum((uint8_t)c) || c == '-' || c == '_' || c == '.' || c == '~')
                {
                    encoded.push_back(c);
                }
                else
                {
                    encoded.push_back('%');

                    char hex[2];
                    encoded.append(char2hex(hex, c, 'A'), sizeof(hex));
                }
            }
        }
        return encoded;
    }

    std::string urlDecode(std::string_view st)
    {
        std::string decoded;
        if (!st.empty())
        {
            const char* s = st.data();
            const size_t length = st.length();
            decoded.reserve(length * 2 / 3);
            for (unsigned int i = 0; i < length; ++i)
            {
                if (!s[i])
                    break;

                if (s[i] == '%')
                {
                    decoded.push_back(hex2char(s + i + 1));
                    i = i + 2;
                }
                else if (s[i] == '+')
                {
                    decoded.push_back(' ');
                }
                else
                {
                    decoded.push_back(s[i]);
                }
            }
        }
        return decoded;
    }

    std::string base64Encode(std::string_view s)
    {
        size_t n = base64x::encoded_size(s.length());
        if (n > 0)
        {
            std::string ret;
            /**
             * @brief resize_and_overrite avaialbe on vs2022 17.1
             *  refer to:
             *    - https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=msvc-170
             *    - https://github.com/microsoft/STL/wiki/Changelog#vs-2022-171
             *    - https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170
             *
             */
#if _AX_HAS_CXX23
            ret.resize_and_overwrite(n, [&](char* p, size_t) { return ax::base64::encode(p, s.data(), s.length()); });
#else
            ret.resize(n);
            ret.resize(base64x::encode(&ret.front(), s.data(), s.length()));
#endif

            return ret;
        }
        return std::string{};
    }

    std::string base64Decode(std::string_view s)
    {
        size_t n = base64x::decoded_size(s.length());
        if (n > 0)
        {
            std::string ret;

#if _AX_HAS_CXX23
            ret.resize_and_overwrite(n, [&](char* p, size_t) { return ax::base64::decode(p, s.data(), s.length()); });
#else
            ret.resize(n);
            ret.resize(base64x::decode(&ret.front(), s.data(), s.length()));
#endif

            return ret;
        }
        return std::string{};
    }

    int base64Encode(const unsigned char* in, unsigned int inLength, char** out)
    {
        auto n = base64x::encoded_size(inLength);
        // should be enough to store 8-bit buffers in 6-bit buffers
        char* tmp = nullptr;
        if (n > 0 && (tmp = (char*)malloc(n + 1)))
        {
            auto ret = base64x::encode(tmp, in, inLength);
            tmp[ret] = '\0';
            *out = tmp;
            return ret;
        }
        *out = nullptr;
        return 0;
    }

    int base64Decode(const unsigned char* in, unsigned int inLength, unsigned char** out)
    {
        size_t n = base64x::decoded_size(inLength);
        unsigned char* tmp = nullptr;
        if (n > 0 && (tmp = (unsigned char*)malloc(n)))
        {
            n = static_cast<int>(base64x::decode(tmp, reinterpret_cast<const char*>(in), inLength));
            if (n > 0)
                *out = tmp;
            else
            {
                *out = nullptr;
                free(tmp);
            }
            return n;
        }
        *out = nullptr;
        return 0;
    }
}

  // namespace utils

