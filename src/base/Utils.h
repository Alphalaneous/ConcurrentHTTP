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
#ifndef __AX_UTILS_H__
#define __AX_UTILS_H__

#include <vector>
#include <string>
#include "../base/Macros.h"
#include "../base/RefPtr.h"

/** @file ccUtils.h
Misc free functions
*/

/*
ccNextPOT function is licensed under the same license that is used in Texture2D.m.
*/

/** Returns the Next Power of Two value.

Examples:
- If "value" is 15, it will return 16.
- If "value" is 16, it will return 16.
- If "value" is 17, it will return 32.
@param value The value to get next power of two.
@return Returns the next power of two value.
@since v0.99.5
*/

int ccNextPOT(int value);

class Sprite;
class Image;

namespace utilsX
{

    /** Same to ::atof, but strip the string, remain 7 numbers after '.' before call atof.
     * Why we need this? Because in android c++_static, atof ( and std::atof ) is unsupported for numbers have long decimal
     * part and contain several numbers can approximate to 1 ( like 90.099998474121094 ), it will return inf. This function
     * is used to fix this bug.
     * @param str The string be to converted to double.
     * @return Returns converted value of a string.
     */
    double atof(const char* str);


    /**
    @brief Parses a list of space-separated integers.
    @return Vector of ints.
    * @js NA
    * @lua NA
    */
    std::vector<int> parseIntegerList(std::string_view intsString);

    /**
    @brief translate charstring/binarystream to hexstring.
    @return hexstring.
    * @js NA
    * @lua NA
    */
    std::string bin2hex(std::string_view binary /*charstring also regard as binary in C/C++*/,
        int delim = -1,
        bool prefix = false);



    // check a number is power of two.
    inline bool isPOT(int number)
    {
        return ((number > 0) && (number & (number - 1)) == 0);
    }

    // Convert ASCII hex digit to a nibble (four bits, 0 - 15).
    //
    // Use unsigned to avoid signed overflow UB.
    inline unsigned char hex2nibble(unsigned char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return 10 + (c - 'a');
        }
        else if (c >= 'A' && c <= 'F')
        {
            return 10 + (c - 'A');
        }
        return 0;
    }

    // Convert a nibble ASCII hex digit
    inline unsigned char nibble2hex(unsigned char c, unsigned char a = 'a')
    {
        return ((c) < 0xa ? ((c)+'0') : ((c)+a - 10));
    }

    // Convert ASCII hex string (two characters) to byte.
    //
    // E.g., "0B" => 0x0B, "af" => 0xAF.
    inline char hex2char(const char* p)
    {
        return hex2nibble((uint8_t)p[0]) << 4 | hex2nibble(p[1]);
    }

    // Convert byte to ASCII hex string (two characters).
    inline char* char2hex(char* p, unsigned char c, unsigned char a = 'a')
    {
        p[0] = nibble2hex(c >> 4, a);
        p[1] = nibble2hex(c & (uint8_t)0xf, a);
        return p;
    }

    std::string urlEncode(std::string_view s);

    std::string urlDecode(std::string_view st);

    /**
     * Encodes bytes into a 64base buffer
     * @returns base64 encoded string
     *
     @since axmol-1.0.0
     */
    std::string base64Encode(std::string_view s);

    /**
     * Decodes a 64base encoded buffer
     * @returns palintext
     *
     @since axmol-1.0.0
     */
    std::string base64Decode(std::string_view s);

    /**
     * Encodes bytes into a 64base encoded memory with terminating '\0' character.
     * The encoded memory is expected to be freed by the caller by calling `free()`
     *
     * @returns the length of the out buffer
     *
     @since v2.1.4, axmol-1.0.0 move from namespace ax to ax::utils
     */
    int base64Encode(const unsigned char* in, unsigned int inLength, char** out);

    /**
     * Decodes a 64base encoded memory. The decoded memory is
     * expected to be freed by the caller by calling `free()`
     *
     * @returns the length of the out buffer
     *
     @since v0.8.1, axmol-1.0.0 move from namespace ax to ax::utils
     */
    int base64Decode(const unsigned char* in, unsigned int inLength, unsigned char** out);

}
#endif  // __SUPPORT_AX_UTILS_H__
