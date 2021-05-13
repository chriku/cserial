#pragma once

#include "avro.hpp"
#include "json.hpp"
#include "serialize.hpp"
#include "utils.hpp"
#include <iomanip>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cserial {
  namespace avro_variable {
    template <typename variable_type> struct build_deserializer_stream;
    template <> struct build_deserializer_stream<int64_t> {
      static std::function<void(int64_t&, string_view_parser&)> build(nlohmann::json schema) {
        return [](int64_t& target, string_view_parser& parser) { target = parser.zig_zag(); };
      }
    };
    template <typename variable_type> void chomp_values(std::vector<std::function<void(variable_type&, string_view_parser&)>>& functions, nlohmann::json schema) {
      if (schema.at("type").get<std::string>() == "long") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.zig_zag(); });
      } else if (schema.at("type").get<std::string>() == "record") {
        for (const auto& f : schema.at("fields"))
          chomp_values<variable_type>(functions, f.at("type"));
      } else
        throw std::invalid_argument("unhandled chomp type: " + schema.dump());
    }
    template <typename variable_type> struct build_deserializer_stream {
      static std::function<void(variable_type&, string_view_parser&)> build(nlohmann::json schema) {
        std::cout << schema << std::endl;
        std::cout << avro::schema<variable_type>() << std::endl;
        if (schema.at("type").get<std::string>() != "record"s)
          throw std::invalid_argument("Cannot parse record into non-record");
        if (schema.at("name").get<std::string>() != serial<variable_type>::name())
          throw std::invalid_argument("Cannot convert "s + schema.at("name").get<std::string>() + " into "s + std::string(serial<variable_type>::name()));
        std::vector<std::function<void(variable_type&, string_view_parser&)>> functions;
        std::unordered_map<std::string, std::function<void(variable_type&, string_view_parser&)>> missing;
        serial<variable_type>::iterate(static_cast<void*>(nullptr), [&]<typename current_field>(void*, current_field*) {
          if constexpr (current_field::template has_parameter<default_value>())
            missing.emplace(current_field::name(), [](variable_type& v, string_view_parser&) {
              v.*current_field::member_pointer() = current_field::template parameter<default_value, typename current_field::value_type>();
            });
        });
        for (const auto& field : schema.at("fields").get<std::vector<nlohmann::json>>()) {
          bool drop_value = true;
          serial<variable_type>::iterate(static_cast<void*>(nullptr), [&]<typename current_field>(void*, current_field*) {
            if (current_field::name() == field.at("name").get<std::string>()) {
              auto f = build_deserializer_stream<typename current_field::value_type>::build(field["type"]);
              functions.push_back([f](variable_type& v, string_view_parser& p) { f(v.*current_field::member_pointer(), p); });
              missing.erase(std::string(current_field::name()));
              drop_value = false;
            }
          });
          if (drop_value) {
            chomp_values<variable_type>(functions, field.at("type"));
          }
        }
        for (const auto& [key_name, f] : missing) {
          functions.push_back(f);
        }
        return [functions](variable_type& target, string_view_parser& parser) {
          for (const auto& f : functions)
            f(target, parser);
        };
      }
    };
    template <typename variable_type> std::function<void(variable_type&, const std::string_view&)> build_deserializer(nlohmann::json schema) {
      auto lambda = build_deserializer_stream<variable_type>::build(schema);
      return [lambda](variable_type& target, const std::string_view& view) {
        string_view_parser p(view);
        lambda(target, p);
      };
    }
  } // namespace avro_variable
} // namespace cserial
