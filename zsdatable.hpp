/*******************************************************************************
 * library: zsdatable
 * package: zsdatab
 * version: 0.3.1
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *******************************************************************************
 * Copyright (C) 2021 zseri
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *******************************************************************************/

#pragma once
#include <istream>
#include <ostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>

#include <experimental/propagate_const>

namespace zsdatab {
  typedef std::vector<std::string> row_t;
  typedef std::vector<row_t> buffer_t;

  // metadata class
  class metadata final {
    struct impl;
    std::experimental::propagate_const<std::unique_ptr<impl>> _d;

    friend auto operator<<(std::ostream &stream, const metadata::impl &meta) -> std::ostream&;
    friend auto operator>>(std::istream &stream, metadata::impl &meta) -> std::istream&;

    friend bool operator==(const metadata &a, const metadata &b);
    friend auto operator<<(std::ostream &stream, const metadata &meta) -> std::ostream&;
    friend auto operator>>(std::istream &stream, metadata &meta) -> std::istream&;

   public:
    metadata();
    metadata(const char sep, row_t cols = {});
    metadata(const metadata &o);
    metadata(metadata &&o);

    ~metadata() noexcept;

    auto operator=(const metadata &o) -> metadata&;
    auto operator+=(const row_t &o) -> metadata&;
    auto operator+=(row_t &&o) -> metadata&;

    void swap(metadata &o) noexcept;
    auto get_cols() const noexcept -> const row_t&;

    bool empty() const noexcept
      { return get_cols().empty(); }
    auto get_field_count() const -> size_t
      { return get_cols().size(); }
    auto get_field_name(const size_t n) const -> std::string
      { return get_cols().at(n); }
    bool good() const noexcept
      { return !empty(); }

    bool has_field(const std::string &colname) const noexcept;
    auto get_field_nr(const std::string &colname) const -> size_t;
    bool rename_field(const std::string &from, const std::string &to);

    // simple setters and getters
    void separator(const char sep) noexcept;
    char separator() const noexcept;

    auto deserialize(const std::string &line) const -> row_t;
    auto serialize(const row_t &line) const -> std::string;
  };

  bool operator==(const metadata &a, const metadata &b);
  static inline bool operator!=(const metadata &a, const metadata &b)
    { return !(a == b); }

  std::ostream& operator<<(std::ostream &stream, const metadata &meta);
  std::istream& operator>>(std::istream &stream, metadata &meta);

  class table;
  struct table_interface;

  // buffer_interface class:
  //  common interface for table, context and const_context
  //  provides an interface to a buffer and the associated metadata
  struct buffer_interface {
    virtual ~buffer_interface() noexcept = default;

    virtual auto get_metadata() const noexcept -> const metadata& = 0;
    virtual auto get_const_table() const noexcept -> const table_interface& = 0;

    virtual auto data() const noexcept -> const buffer_t& = 0;
    virtual auto data_move_out() && -> buffer_t&& = 0;

    bool empty() const noexcept
      { return data().empty(); }
  };

  struct table_clone_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  // table_interface class:
  //  common interface for tables
  struct table_interface : public buffer_interface {
    virtual bool good() const noexcept = 0;
    virtual auto data() const noexcept -> const buffer_t& = 0;
    virtual void data(const buffer_t &n) = 0;
    virtual auto clone() const -> std::shared_ptr<table_interface> = 0;
  };

  class const_context;
  class context;

  // table (delegating) class
  class table final : public table_interface {
    std::experimental::propagate_const<std::shared_ptr<table_interface>> _t;

   public:
    // for permanent tables
    table(const std::string &_path);

    // for in-memory tables
    table(metadata m);
    table(metadata m, buffer_t n);

    table(std::shared_ptr<table_interface> &&o)
      : _t(std::move(o)) { }

    table(const table &o) = default;
    table(table &&o) = default;
    virtual ~table() noexcept = default;

    void swap(table &o) noexcept
      { std::swap(_t, o._t); }

    bool good() const noexcept
      { return _t->good(); }

    auto get_metadata() const noexcept -> const metadata&
      { return _t->get_metadata(); }

    auto get_const_table() const noexcept -> const table&
      { return *this; }

    auto data() const noexcept -> const buffer_t&
      { return _t->data(); }

    auto data_move_out() && -> buffer_t&&
      { return std::move(*_t).data_move_out(); }

    void data(const buffer_t &n);

    auto clone() const -> std::shared_ptr<table_interface>;

    auto filter(const size_t field, const std::string& value, const bool whole = true, const bool neg = false) -> context;
    auto filter(const std::string& field, const std::string& value, const bool whole = true, const bool neg = false) -> context;
    auto filter(const size_t field, const std::string& value, const bool whole = true, const bool neg = false) const -> const_context;
    auto filter(const std::string& field, const std::string& value, const bool whole = true, const bool neg = false) const -> const_context;
  };

  std::ostream& operator<<(std::ostream& stream, const table& tab);
  std::istream& operator>>(std::istream& stream, table& tab);

  // for permanent tables, metadata and main data in one file
  bool create_packed_table(const std::string &_path, const metadata &_meta);
  table make_packed_table(const std::string &_path);

  // for permanent tables, gzipped and packed
  bool create_gzipped_table(const std::string &_path, const metadata &_meta);
  table make_gzipped_table(const std::string &_path);

  namespace intern {
    class fixcol_proxy_common {
     public:
      explicit fixcol_proxy_common(const size_t nr) : _nr(nr) { }
      fixcol_proxy_common(const buffer_interface &uplink, const std::string &field);

      // report
      auto get(const bool _uniq = false) const -> std::vector<std::string>;

     protected:
      const size_t _nr;

      virtual auto _underlying_data() const -> const buffer_t& = 0;
    };

    class context_common;
    class const_fixcol_proxy;

    class fixcol_proxy final : public fixcol_proxy_common {
      friend class const_fixcol_proxy;
      context_common &_uplink;

     public:
      fixcol_proxy(context_common &uplink, const size_t nr);
      fixcol_proxy(context_common &uplink, const std::string &field);

      // change
      fixcol_proxy& set(const std::string &value);
      fixcol_proxy& append(const std::string &value);
      fixcol_proxy& remove(const std::string &value);
      fixcol_proxy& replace(const std::string &from, const std::string &to);

     protected:
      auto _underlying_data() const -> const buffer_t&;
    };

    class const_fixcol_proxy final : public fixcol_proxy_common {
      const buffer_interface &_uplink;

     public:
      const_fixcol_proxy(const buffer_interface &uplink, const size_t nr);
      const_fixcol_proxy(const buffer_interface &uplink, const std::string &field);
      const_fixcol_proxy(const fixcol_proxy &o);

     protected:
      auto _underlying_data() const -> const buffer_t&
        { return _uplink.data(); }
    };

    // base class for contexts
    class context_common : public buffer_interface {
      friend class fixcol_proxy;

     public:
      context_common(const buffer_interface &bif) : _buffer(bif.data())   { }
      context_common(const buffer_t &o)           : _buffer(o)            { }
      context_common(buffer_t &&o)                : _buffer(std::move(o)) { }
      context_common(const context_common &ctx) = default;
      context_common(context_common &&ctx) noexcept = default;
      virtual ~context_common() noexcept = default;

      auto operator=(const context_common &o) -> context_common&;
      auto operator=(const buffer_interface &o) -> context_common&;
      auto operator=(context_common &&o) -> context_common&;
      auto operator+=(const buffer_interface &o) -> context_common&;
      auto operator+=(const row_t &line) -> context_common&;

      context_common& pull();

      auto column(const std::string field) -> fixcol_proxy;
      auto column(const std::string field) const -> const_fixcol_proxy;

      // select
      context_common& clear() noexcept;
      context_common& sort();
      context_common& uniq();
      context_common& negate();
      context_common& filter(const size_t field, const std::string& value, const bool whole = true, const bool neg = false);
      context_common& filter(const std::string& field, const std::string& value, const bool whole = true, const bool neg = false);

      // change
      context_common& set_field(const size_t field, const std::string& value);
      context_common& append_part(const size_t field, const std::string& value);
      context_common& remove_part(const size_t field, const std::string& value);
      context_common& replace_part(const size_t field, const std::string& from, const std::string& to);

      context_common& set_field(const std::string& field, const std::string& value);
      context_common& append_part(const std::string& field, const std::string& value);
      context_common& remove_part(const std::string& field, const std::string& value);
      context_common& replace_part(const std::string& field, const std::string& from, const std::string& to);

      // report
      auto get_column_data(const size_t colnr, const bool _uniq = false) const -> std::vector<std::string>;
      auto get_column_data(const std::string &colname, const bool _uniq = false) const -> std::vector<std::string>;

      auto get_metadata() const noexcept -> const metadata&
        { return get_const_table().get_metadata(); }
      auto data() const noexcept -> const buffer_t&
        { return _buffer; }
      auto data_move_out() && -> buffer_t&&
        { return std::move(_buffer); }

      auto get_field_nr(const std::string &colname) const -> size_t;

      // main delegation and abstraction
      virtual auto get_const_table() const noexcept -> const table_interface& = 0;

     protected:
      buffer_t _buffer;

      fixcol_proxy get_fixcol_proxy(const size_t field);
    };

    bool operator==(const context_common &a, const context_common &b) noexcept;
    static inline bool operator!=(const context_common &a, const context_common &b) noexcept
      { return !(a == b); }
    std::ostream& operator<<(std::ostream& stream, const context_common &ctx);
    std::istream& operator>>(std::istream& stream, context_common& ctx);

    // minimal non-abstract template base class for contexts
    template<class T>
    class context_base : public context_common {
     public:
      explicit context_base(T &tab): context_common(tab), _table(tab) { }

      context_base(T &tab, const buffer_t &o)
        : context_common(o), _table(tab) { }

      context_base(T &tab, buffer_t &&o)
        : context_common(std::move(o)), _table(tab) { }

      context_base(const T &&tab) = delete;
      context_base(const T &&tab, const buffer_t &&o) = delete;

      auto get_const_table() const noexcept -> const table_interface& final
        { return _table; }

      // delegators
      template<class B>
      auto operator=(const B &o) -> context_base& {
        context_common::operator=(o);
        return *this;
      }

      template<class B>
      auto operator+=(const B &o) -> context_base& {
        context_common::operator+=(o);
        return *this;
      }

     protected:
      T &_table;
    };

    template<class Ta, class Tb>
    context_base<Ta> operator+(const context_base<Ta> &a, const context_base<Tb> &b) {
      return context_base<Ta>(a) += b;
    }

    template<class T>
    context_base<T> operator+(const context_base<const T> &a, const context_base<T> &b) {
      return (context_base<T>(b) = a) += b;
    }

    namespace ta {
      // base class for transaction parts
      struct action;
    }
  }

  class const_context final : public intern::context_base<const table> {
   public:
    using context_base<const table>::context_base;

    const_context(const context &o);
    const_context(const table &&tab) = delete;
    const_context(const buffer_interface &&o) = delete;
    const_context(const const_context &o) = default;
    const_context(const_context &&o) noexcept = default;
  };

  class context final : public intern::context_base<table> {
    friend class const_context;

   public:
    using context_base<table>::context_base;
    context(const context &o) = default;
    context(context &&o) noexcept = default;

    // transfer
    void push()
      { _table.data(_buffer); }
    void swap(context &o) noexcept
      { _buffer.swap(o._buffer); }

    // rm = negate push
    // rmexcept = push
  };

  class transaction final {
   public:
    transaction(metadata m);
    transaction(const transaction &o);
    transaction(transaction &&o) noexcept = default;

    void swap(transaction &o) noexcept;
    void apply(intern::context_common &ctx) const;

    transaction& operator+=(const row_t &line);

    // select
    transaction& clear();
    transaction& sort();
    transaction& uniq();
    transaction& negate();
    transaction& filter(const std::string& field, const std::string& value, const bool whole = true);

    // change
    transaction& set_field(const std::string& field, const std::string& value);
    transaction& append_part(const std::string& field, const std::string& value);
    transaction& remove_part(const std::string& field, const std::string& value);
    transaction& replace_part(const std::string& field, const std::string& from, const std::string& to);

   private:
    const metadata _meta;
    std::vector<std::shared_ptr<intern::ta::action>> _actions;
  };

  /* inner_join - join to buffers into a table via inner join (common subset)
   * @return : table : composed table
   *         - buffer : composed buffer
   *
   * @param sep : char : (metadata) column separator
   *
   * @param a, b : buffer_interface : buffers to join
   *             - metadata.cols (equal names are assumed equivalent and will be joined)
   */
  table inner_join(const char sep, const buffer_interface &a, const buffer_interface &b);

  // table_map_fields - map field names (mappings: {from, to}) (e.g. for an following join)
  table table_map_fields(const buffer_interface &in, std::unordered_map<std::string, std::string> mappings);
}
