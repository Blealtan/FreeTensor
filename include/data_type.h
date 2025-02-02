#ifndef FREE_TENSOR_DATA_TYPE_H
#define FREE_TENSOR_DATA_TYPE_H

#include <array>
#include <functional>

#include <container_utils.h>
#include <except.h>
#include <serialize/to_string.h>

namespace freetensor {

enum class BaseDataType : size_t {
    Void = 0,
    Float32,
    Float64,
    Int32,
    Int64,
    Bool,
    Custom,
    // ------
    NumTypes,
    Invalid,
};

constexpr std::array baseDataTypeNames = {
    "void", "float32", "float64", "int32", "int64", "bool", "custom",
};
static_assert(baseDataTypeNames.size() == (size_t)BaseDataType::NumTypes);

inline std::ostream &operator<<(std::ostream &os, BaseDataType dtype) {
    return os << baseDataTypeNames.at((size_t)dtype);
}

inline BaseDataType parseBaseDataType(const std::string &_str) {
    auto &&str = tolower(_str);
    for (auto &&[i, s] : views::enumerate(baseDataTypeNames)) {
        if (s == str) {
            return (BaseDataType)i;
        }
    }
    std::string msg = "Unrecognized base data type \"" + _str +
                      "\". Candidates are (case-insensitive): ";
    for (auto &&[i, s] : views::enumerate(baseDataTypeNames)) {
        msg += (i > 0 ? ", " : "");
        msg += s;
    }
    ERROR(msg);
}

enum class SignDataType : size_t {
    Any = 0,
    GT0,
    GE0,
    LT0,
    LE0,
    NE0,
    EQ0, // EQ0 is only for "0" literals. No need to type-inference EQ0 because
         // we have const_fold
    // ------
    NumTypes,
};

constexpr std::array signDataTypeNames = {
    "", ">0", ">=0", "<0", "<=0", "!=0", "==0",
};
static_assert(signDataTypeNames.size() == (size_t)SignDataType::NumTypes);

inline std::ostream &operator<<(std::ostream &os, SignDataType dtype) {
    return os << signDataTypeNames.at((size_t)dtype);
}

inline SignDataType parseSignDataType(const std::string &str) {
    for (auto &&[i, s] : views::enumerate(signDataTypeNames)) {
        if (s == str) {
            return (SignDataType)i;
        }
    }
    std::string msg =
        "Unrecognized sign data type \"" + str + "\". Candidates are: ";
    for (auto &&[i, s] : views::enumerate(signDataTypeNames)) {
        msg += (i > 0 ? ", " : "");
        msg += s;
    }
    ERROR(msg);
}

class DataType {
    BaseDataType base_;
    SignDataType sign_;

  public:
    DataType() {} // Construct without initialization
    DataType(BaseDataType base, SignDataType sign = SignDataType::Any)
        : base_(base), sign_(sign) {}

    // Expose BaseDataType::* to DataType::*
    //
    // TODO: Use the following line after GCC 12.3. GCC is buggy with `using
    // enum` before 12.3 (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=103081)
    //
    // using enum BaseDataType;
    //
    // and remove the following lines
    constexpr static auto Bool = BaseDataType::Bool;
    constexpr static auto Custom = BaseDataType::Custom;
    constexpr static auto Float32 = BaseDataType::Float32;
    constexpr static auto Float64 = BaseDataType::Float64;
    constexpr static auto Int32 = BaseDataType::Int32;
    constexpr static auto Int64 = BaseDataType::Int64;
    constexpr static auto Void = BaseDataType::Void;
    constexpr static auto Invalid = BaseDataType::Invalid;

    const auto &base() const { return base_; }
    const auto &sign() const { return sign_; }

    friend bool operator==(const DataType &, const DataType &) = default;
};

inline std::ostream &operator<<(std::ostream &os, const DataType &dtype) {
    return os << dtype.base() << dtype.sign();
}

inline DataType parseDType(const std::string &str) {
    auto split = str.find_first_of("<>=!");
    if (split == std::string::npos) {
        split = str.length();
    }
    auto base = parseBaseDataType(str.substr(0, split));
    auto sign = parseSignDataType(str.substr(split));
    return DataType{base, sign};
}

size_t sizeOf(BaseDataType dtype);
inline size_t sizeOf(const DataType &dtype) { return sizeOf(dtype.base()); }

bool isInt(BaseDataType dtype);
inline bool isInt(const DataType &dtype) { return isInt(dtype.base()); }

bool isFloat(BaseDataType dtype);
inline bool isFloat(const DataType &dtype) { return isFloat(dtype.base()); }

inline bool isNumber(BaseDataType dtype) {
    return isInt(dtype) || isFloat(dtype);
}
inline bool isNumber(const DataType &dtype) { return isNumber(dtype.base()); }

inline bool isBool(BaseDataType dtype) { return dtype == BaseDataType::Bool; }
inline bool isBool(const DataType &dtype) { return isBool(dtype.base()); }

inline bool isGT0(SignDataType dtype) { return dtype == SignDataType::GT0; }
inline bool isGT0(const DataType &dtype) { return isGT0(dtype.sign()); }

inline bool isGE0(SignDataType dtype) {
    return dtype == SignDataType::GE0 || dtype == SignDataType::GT0 ||
           dtype == SignDataType::EQ0;
}
inline bool isGE0(const DataType &dtype) { return isGE0(dtype.sign()); }

inline bool isLT0(SignDataType dtype) { return dtype == SignDataType::LT0; }
inline bool isLT0(const DataType &dtype) { return isLT0(dtype.sign()); }

inline bool isLE0(SignDataType dtype) {
    return dtype == SignDataType::LE0 || dtype == SignDataType::LT0 ||
           dtype == SignDataType::EQ0;
}
inline bool isLE0(const DataType &dtype) { return isLE0(dtype.sign()); }

inline bool isNE0(SignDataType dtype) {
    return dtype == SignDataType::LT0 || dtype == SignDataType::GT0 ||
           dtype == SignDataType::NE0;
}
inline bool isNE0(const DataType &dtype) { return isNE0(dtype.sign()); }

BaseDataType upCast(BaseDataType lhs, BaseDataType rhs);
inline DataType upCast(const DataType &lhs, const DataType &rhs) {
    return {upCast(lhs.base(), rhs.base()), SignDataType::Any};
}

} // namespace freetensor

namespace std {

template <> class hash<freetensor::DataType> {
    std::hash<size_t> h_;

  public:
    size_t operator()(const freetensor::DataType &dtype) const;
};

} // namespace std

#endif // FREE_TENSOR_DATA_TYPE
