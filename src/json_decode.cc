#include "src/json_decode.h"

#include "absl/strings/substitute.h"

namespace uchen::json {

JsonDecode JsonDecode::operator[](size_t index) const {
  if (std::holds_alternative<DecodeError>(contents_)) {
    return *this;
  }
  const auto& json = std::get<nlohmann::json>(contents_);
  if (!json.is_array()) {
    return JsonDecode{context_,
                      DecodeError(context_.path(), "Is not an array", json)};
  }
  if (index >= json.size()) {
    return JsonDecode{
        context_,
        DecodeError(
            context_.path(),
            absl::Substitute("Trying to access index $0, but array size is $1",
                             index, json.size()),
            json)};
  }
  return {context_.Append(absl::Substitute("[$0]", index)), json[index]};
}

JsonDecode JsonDecode::operator[](std::string_view key) const {
  if (std::holds_alternative<DecodeError>(contents_)) {
    return *this;
  }
  const auto& json = std::get<nlohmann::json>(contents_);
  if (!json.is_object()) {
    return JsonDecode{
        context_, DecodeError(context_.path(), "Not an object", json.dump())};
  }
  if (!json.contains(key)) {
    return JsonDecode{
        context_, DecodeError(context_.path(),
                              absl::Substitute("Key $0 not found", key), json)};
  }
  DecodeContext context = context_.Append(absl::Substitute(".$0", key));
  return {context, json[key]};
}

DecodeResult<std::string> JsonDecode::String() const {
  if (std::holds_alternative<DecodeError>(contents_)) {
    return std::get<DecodeError>(contents_);
  }
  const auto& json = std::get<nlohmann::json>(contents_);
  if (!json.is_string()) {
    return DecodeResult<std::string>{
        DecodeError{context_.path(), "Is not a string", json}};
  }
  return DecodeResult<std::string>{json.get<std::string>()};
}

}  // namespace uchen::json