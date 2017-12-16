/*************************************************
 *        class: zsdatab::table::filter
 *      library: zsdatable
 *      package: zsdatab
 *      version: 0.2.6
 **************| *********************************
 *       author: Erik Kai Alain Zscheile
 *        email: erik.zscheile.ytrizja@gmail.com
 **************| *********************************
 * organisation: Ytrizja
 *     org unit: Zscheile IT
 *     location: Chemnitz, Saxony
 *************************************************
 *
 * Copyright (c) 2017 Erik Kai Alain Zscheile
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *************************************************/

#include <zsdatable.hpp>
using namespace std;

namespace zsdatab {
  namespace intern {
    namespace {
      buffer_t buffer_filter(const buffer_t &buf, const size_t field, const string& value, const bool whole, const bool neg) {
        if(buf.empty()) return buf;

        buffer_t ret;
        ret.reserve(buf.size());

        for(auto &&s : buf)
          if(neg == ((s[field].find(value) == string::npos) || (whole && s[field] != value)))
            ret.emplace_back(std::move(s));

        ret.shrink_to_fit();
        return ret;
      }
    }
  }

  auto table::filter(const size_t field, const std::string& value, const bool whole, const bool neg) -> context {
    return context(*this, intern::buffer_filter(data(), field, value, whole, neg));
  }

  auto table::filter(const std::string& field, const std::string& value, const bool whole, const bool neg) -> context {
    return filter(get_metadata().get_field_nr(field), value, whole, neg);
  }

  auto table::filter(const size_t field, const std::string& value, const bool whole, const bool neg) const -> const_context {
    return const_context(*this, intern::buffer_filter(data(), field, value, whole, neg));
  }

  auto table::filter(const std::string& field, const std::string& value, const bool whole, const bool neg) const -> const_context {
    return filter(get_metadata().get_field_nr(field), value, whole, neg);
  }
}