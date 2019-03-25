//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#define __CONCATENATE(__PREFIX__,__SUFFIX__) __PREFIX__##__SUFFIX__
#define CONCATENATE(__HEAD__,__TAIL__) __CONCATENATE(__HEAD__,__TAIL__)

#define UNIQUE(__NAME__) CONCATENATE(__NAME__, __COUNTER__)

#define defer(__EXPR__) auto UNIQUE(__deferred) = __defer_func([&]() { __EXPR__; })

template<typename Function>
struct Deferred {
    Function func;

    Deferred(Function func) : func(func) {}
    ~Deferred() { func(); }
};

template<typename Function>
Deferred<Function> __defer_func(Function func) {
    return Deferred<Function>(func);
}
