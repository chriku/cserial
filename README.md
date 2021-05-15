# C++20 template based serialization/deserialization library

## Usage:

### Struct declaration

Example Struct declaration:
```C++
struct substruct_t {
  int64_t f;
  int64_t o;
};
template <>
struct cserial::serial<substruct_t> : serializer<"substruct_t",                                                      //
                                                 field<&substruct_t::f, "f", parameter<cserial::default_value, 42>>, //
                                                 field<&substruct_t::o, "o">> {};

struct serial_struct_t {
  substruct_t child;
  int32_t x = 0x100;
};
template <>
struct cserial::serial<serial_struct_t> : serializer<"serial_struct_t",                       //
                                                     field<&serial_struct_t::child, "child">, //
                                                     field<&serial_struct_t::x, "integer_field">> {};
```

For each custom serializable structure a specialization of `cserial::serial` is needed to be serialized.

The default `cserial::serializer` takes the name for the schema as the first template parameter and a list of `field`s as parameter pack.

Each field takes a member-pointer to the field as the first argument, the name of the field in the schema as second and a parameter pack of `parameter` containing miscellaneous options for each field.

### Apache Avro serializer

Currently supported types are simple structs using `cserial::serial`, `int64_t`, `double`, `float`, `std::string`, `std::vector<T>`, `std::unordered_map<std::string, T>`, `std::optional<T>`, `std::variant<T...>` and `std::array<char, len>` are also supported.

#### Schema

To get the schema as json object, use `cserial::avro::schema<serial_struct_t>()`. For this example this results in

```Json
{
  "name": "serial_struct_t",
  "type": "record",
  "fields": [
    {
      "name": "child",
      "type": {
        "fields": [
          {
            "name": "f",
            "type": "long"
          },
          {
            "name": "o",
            "type": "long"
          }
        ],
        "name": "substruct_t",
        "type": "record"
      }
    },
    {
      "name": "integer_field",
      "type": "long"
    }
  ]
}
```

#### Serialization

At the current time only fixed serialization to the generated schema for thsi exact is supported.
To use this, `std::string cserial::avro::serialize(auto)` returns the serialized string of the given object.

#### Fixed Desertialzation

To deserialize into the generated schema use `void deserialize(àuto& value, const std::string_view&)` to deserialize a `string_view` back into a struct.

#### Variable deserialization

`cserial::avro_variable::build_deserializer<target_type_t>(nlohmann::json)` returns a `std::function<void(target_type_t&, const std::string_view&)>` reading an avro object of the given schema into the struct.
