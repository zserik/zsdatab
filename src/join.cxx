/*************************************************
 *         part: table joining
 *      library: zsdatable
 *      package: zsdatab
 *      version: 0.1.5
 **************| *********************************
 *       author: Erik Kai Alain Zscheile
 *        email: erik.zscheile.ytrizja@gmail.com
 **************| *********************************
 * organisation: Ytrizja
 *     org unit: Zscheile IT
 *     location: Chemnitz, Saxony
 *************************************************
 *
 * Copyright (c) 2016 Erik Kai Alain Zscheile
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

#include <iterator>
#include <map>
#include <set>
#include "zsdatable.hpp"

namespace zsdatab {
  namespace intern {
    struct column_join {
      size_t a, b, c;
    };

    enum column_join_from_where {
      FROM_COMMON, FROM_A, FROM_B
    };
  }

  table inner_join(char sep, const buffer_interface &a, const buffer_interface &b) {
    using namespace std;
    metadata metatmp;
    metatmp.separator(sep);

    const metadata &ma = a.get_metadata();
    const metadata &mb = b.get_metadata();

    // compute column names
    map<string, intern::column_join> merge_cols;
    map<string, intern::column_join_from_where> join_cols;
    {
      vector<string> cols = ma.get_cols();
      for(auto &&i : mb.get_cols()) {
        if(find(cols.begin(), cols.end(), i) != cols.end())
          merge_cols[i] = { 0, 0, 0 };
        else
          cols.push_back(i);
      }
      metatmp += cols;

      for(auto &&i : cols) {
        join_cols[i] = intern::column_join_from_where(
                 (merge_cols.find(i) != merge_cols.end()) ? intern::column_join_from_where::FROM_COMMON
                 : (ma.has_field(i) ? intern::column_join_from_where::FROM_A
                   : (mb.has_field(i) ? intern::column_join_from_where::FROM_B
                     : intern::column_join_from_where::FROM_COMMON
                     )
                   )
               );
      }
    }

    for(auto &i : merge_cols) {
      i.second.a = ma.get_field_nr(i.first);
      i.second.b = mb.get_field_nr(i.first);
      i.second.c = metatmp.get_field_nr(i.first);
    }

    // compute table
    table ret(metatmp);
    vector<vector<string>> table_data;

    for(auto &&x : a.get_data()) {
      for(auto &&y : b.get_data()) {
        bool match = true;
        vector<string> line(metatmp.get_field_count());
        for(auto &&col : merge_cols) {
          const string value = x[col.second.a];
          if(value == y[col.second.b]) {
            // match
            line[col.second.c] = value;
          } else {
            // no match
            match = false;
            break;
          }
        }
        if(match) {
          for(size_t i = 0; i < line.size(); ++i) {
            if(!line[i].empty()) continue;

            const string colname = metatmp.get_field_name(i);
            switch(join_cols[colname]) {
              case intern::column_join_from_where::FROM_A:
                line[i] = x[ma.get_field_nr(colname)];
                break;
              case intern::column_join_from_where::FROM_B:
                line[i] = y[mb.get_field_nr(colname)];
                break;

              default: break;
            }
          }
          table_data.push_back(line);
        }
      }
    }

    ret.update_data(table_data);
    return ret;
  }
}