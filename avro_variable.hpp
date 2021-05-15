#pragma once

#include "avro.hpp"
#include "serialize.hpp"
#include "utils.hpp"
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace cserial {
  namespace avro_variable {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    template <typename variable_type> struct build_deserializer_stream;
    template <typename variable_type> void chomp_values(std::vector<std::function<void(variable_type&, string_view_parser&)>>& functions, nlohmann::json schema);
    template <> struct build_deserializer_stream<int64_t> {
      static std::function<void(int64_t&, string_view_parser&)> build(nlohmann::json schema) {
        if ((schema.get<std::string>() != "long") && (schema.get<std::string>() != "int"))
          throw std::runtime_error("Attempt to read number from " + schema.dump());
        return [](int64_t& target, string_view_parser& parser) { target = parser.zig_zag(); };
      }
    };
    template <> struct build_deserializer_stream<int32_t> {
      static std::function<void(int32_t&, string_view_parser&)> build(nlohmann::json schema) {
        if ((schema.get<std::string>() != "long") && (schema.get<std::string>() != "int"))
          throw std::runtime_error("Attempt to read number from " + schema.dump());
        return [](int32_t& target, string_view_parser& parser) { target = parser.zig_zag(); };
      }
    };
    template <> struct build_deserializer_stream<bool> {
      static std::function<void(bool&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema.get<std::string>() != "boolean")
          throw std::runtime_error("Attempt to read bool from " + schema.dump());
        return [](bool& target, string_view_parser& parser) { target = parser.fixed(1).at(0) != 0; };
      }
    };
    template <> struct build_deserializer_stream<float> {
      static std::function<void(float&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema.get<std::string>() != "float")
          throw std::runtime_error("Attempt to read float from " + schema.dump());
        return [](float& target, string_view_parser& parser) { target = *reinterpret_cast<const float*>(parser.fixed(sizeof(float)).data()); };
      }
    };
    template <> struct build_deserializer_stream<double> {
      static std::function<void(double&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema.get<std::string>() != "double")
          throw std::runtime_error("Attempt to read double from " + schema.dump());
        return [](double& target, string_view_parser& parser) { target = *reinterpret_cast<const double*>(parser.fixed(sizeof(double)).data()); };
      }
    };
    template <> struct build_deserializer_stream<std::string> {
      static std::function<void(std::string&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema.get<std::string>() != "string")
          throw std::runtime_error("Attempt to read string from " + schema.dump());
        return [](std::string& target, string_view_parser& parser) { target = parser.string(); };
      }
    };
    template <size_t len> struct build_deserializer_stream<std::array<char, len>> {
      static std::function<void(std::array<char, len>&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema["type"].get<std::string>() != "fixed")
          throw std::runtime_error("Attempt to read fixed from " + schema.dump());
        if (schema["size"].get<size_t>() != len)
          throw std::runtime_error("Attempt to read fixed from " + schema.dump());
        return [](std::array<char, len>& value, string_view_parser& parser) {
          auto data = parser.fixed(len);
          std::copy(data.begin(), data.end(), value.begin());
        };
      }
    };
    template <typename subtype> struct build_deserializer_stream<std::vector<subtype>> {
      static std::function<void(std::vector<subtype>&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema["type"].get<std::string>() != "array")
          throw std::runtime_error("Attempt to read fixed from " + schema.dump());
        auto f = build_deserializer_stream<subtype>::build(schema.at("items"));
        return [f](std::vector<subtype>& target, string_view_parser& parser) {
          target.clear();
          int64_t count = 0;
          while ((count = parser.zig_zag()) > 0) {
            for (size_t i = 0; i < count; i++) {
              subtype s;
              f(s, parser);
              target.push_back(s);
            }
          }
        };
      }
    };
    template <typename subtype> struct build_deserializer_stream<std::unordered_map<std::string, subtype>> {
      static std::function<void(std::unordered_map<std::string, subtype>&, string_view_parser&)> build(nlohmann::json schema) {
        if (schema["type"].get<std::string>() != "map")
          throw std::runtime_error("Attempt to read fixed from " + schema.dump());
        auto f = build_deserializer_stream<subtype>::build(schema.at("values"));
        return [f](std::unordered_map<std::string, subtype>& target, string_view_parser& parser) {
          target.clear();
          int64_t count = 0;
          while ((count = parser.zig_zag()) > 0) {
            for (size_t i = 0; i < count; i++) {
              std::string k(parser.string());
              subtype s;
              f(s, parser);
              target.emplace(k, s);
            }
          }
        };
      }
    };
    template <typename subtype> struct build_deserializer_stream<std::optional<subtype>> {
      static std::function<void(std::optional<subtype>&, string_view_parser&)> build(nlohmann::json schema) {
        if (!schema.is_array())
          throw std::runtime_error("Attempt to read optional from " + schema.dump());
        if (schema[0].get<std::string>() != "null")
          throw std::runtime_error("Attempt to read optional from " + schema.dump());
        if (schema.size() != 2)
          throw std::runtime_error("Attempt to read optional from " + schema.dump());
        auto f = build_deserializer_stream<subtype>::build(schema[1]);
        return [f](std::optional<subtype>& target, string_view_parser& parser) {
          target.reset();
          if (parser.zig_zag()) {
            subtype s;
            f(s, parser);
            target = s;
          }
        };
      }
    };
    template <typename... subtype> struct build_deserializer_stream<std::variant<subtype...>> {
      template <size_t i, typename current_type, typename... other_types>
      static inline void fill(std::unordered_map<std::string, std::function<std::function<void(std::variant<subtype...>&, string_view_parser&)>(nlohmann::json)>>& target) {
        std::string name(cserial::avro::serialize_value<current_type>::name());
        std::function<std::function<void(std::variant<subtype...>&, string_view_parser&)>(nlohmann::json)> f = [](nlohmann::json s) {
          auto f = build_deserializer_stream<current_type>::build(s);
          return [f](std::variant<subtype...>& t, string_view_parser& p) {
            current_type s;
            f(s, p);
            t = s;
          };
        };
        target.emplace(name, f);
        if constexpr (sizeof...(other_types) > 0) {
          fill<i + 1, other_types...>(target);
        }
      }
      static std::function<void(std::variant<subtype...>&, string_view_parser&)> build(nlohmann::json schema) {
        if (!schema.is_array())
          throw std::runtime_error("Attempt to read optional from " + schema.dump());
        std::unordered_map<std::string, std::function<std::function<void(std::variant<subtype...>&, string_view_parser&)>(nlohmann::json)>> type_map;
        fill<0, subtype...>(type_map);
        std::vector<std::function<void(std::variant<subtype...>&, string_view_parser&)>> iaf;
        for (const auto& f : schema) {
          std::string name;
          if (f.is_string())
            name = f.get<std::string>();
          else if (f.is_object() && f["name"].is_string())
            name = f["name"].get<std::string>();
          else if (f.is_object() && f["type"].is_string())
            name = f["type"].get<std::string>();
          else if (f.is_array())
            name = "union";
          else
            throw std::runtime_error("Cannot get name of: " + f.dump());
          if (type_map.contains(name)) {
            iaf.push_back(type_map.at(name)(f));
          } else {
            std::vector<std::function<void(std::variant<subtype...>&, string_view_parser&)>> ia;
            chomp_values(ia, f);
            iaf.push_back([ia](std::variant<subtype...>& v, string_view_parser& p) {
              for (const auto& f : ia)
                f(v, p);
            });
          }
        }
        return [iaf](std::variant<subtype...>& target, string_view_parser& parser) { iaf.at(parser.zig_zag())(target, parser); };
      }
    };
    template <typename variable_type> void chomp_values(std::vector<std::function<void(variable_type&, string_view_parser&)>>& functions, nlohmann::json schema) {
      std::string type;
      if (schema.is_object() && schema["type"].is_string())
        type = schema.at("type").get<std::string>();
      else if (schema.is_string())
        type = schema.get<std::string>();
      else if (schema.is_object() && schema["type"].is_array())
        type = "union";
      else
        throw std::invalid_argument("Cannot chomp "s + schema.dump());
      if (type == "long") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.zig_zag(); });
      } else if (type == "null") {
        functions.push_back([](variable_type& v, string_view_parser& p) {});
      } else if (type == "boolean") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.fixed(1); });
      } else if (type == "string") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.string(); });
      } else if (type == "double") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.fixed(8); });
      } else if (type == "float") {
        functions.push_back([](variable_type& v, string_view_parser& p) { p.fixed(4); });
      } else if (type == "fixed") {
        size_t size = schema.at("size").get<size_t>();
        functions.push_back([size](variable_type& v, string_view_parser& p) { p.fixed(size); });
      } else if (type == "record") {
        for (const auto& f : schema.at("fields"))
          chomp_values<variable_type>(functions, f.at("type"));
      } else if (type == "array") {
        std::vector<std::function<void(variable_type&, string_view_parser&)>> iaf;
        chomp_values<variable_type>(iaf, schema.at("items"));
        functions.push_back([iaf](variable_type& vt, string_view_parser& parser) {
          int64_t count = 0;
          while ((count = parser.zig_zag()) > 0) {
            for (size_t i = 0; i < count; i++) {
              for (const auto& f : iaf)
                f(vt, parser);
            }
          }
        });
      } else if (type == "map") {
        std::vector<std::function<void(variable_type&, string_view_parser&)>> iaf;
        chomp_values<variable_type>(iaf, schema.at("values"));
        functions.push_back([iaf](variable_type& vt, string_view_parser& parser) {
          int64_t count = 0;
          while ((count = parser.zig_zag()) > 0) {
            for (size_t i = 0; i < count; i++) {
              parser.string();
              for (const auto& f : iaf)
                f(vt, parser);
            }
          }
        });
      } else if (type == "union") {
        std::vector<std::vector<std::function<void(variable_type&, string_view_parser&)>>> iaf;
        {
          for (const auto& item : schema["type"]) {
            std::vector<std::function<void(variable_type&, string_view_parser&)>> ia;
            chomp_values<variable_type>(ia, item);
            iaf.push_back(ia);
          }
        }
        functions.push_back([iaf](variable_type& vt, string_view_parser& parser) {
          for (const auto& f : iaf.at(parser.zig_zag()))
            f(vt, parser);
        });
      } else
        throw std::invalid_argument("unhandled chomp type: " + schema.dump());
    }
    template <typename variable_type> struct build_deserializer_stream {
      static std::function<void(variable_type&, string_view_parser&)> build(nlohmann::json schema) {
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
            nlohmann::json type;
            if (field.is_object() && field["type"].is_object())
              type = field.at("type");
            else if (field.is_object() && !field["type"].is_object())
              type = nlohmann::json({{"type", field.at("type")}});
            else
              throw std::invalid_argument("Cannot chomp "s + field.dump());
            chomp_values<variable_type>(functions, type);
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
#endif
    /**
     * \brief build a deserializer for a given schema parsing into variable_type
     * \param[in] schema Apache Avro schema
     * \return deserializer
     */
    template <typename variable_type> std::function<void(variable_type&, const std::string_view&)> build_deserializer(nlohmann::json schema) {
      auto lambda = build_deserializer_stream<variable_type>::build(schema);
      return [lambda](variable_type& target, const std::string_view& view) {
        string_view_parser p(view);
        lambda(target, p);
      };
    }
  } // namespace avro_variable
} // namespace cserial
