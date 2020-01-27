// Copyright 2019 Intel Corporation.

#pragma once

#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "plaidml/core/ffi.h"

namespace plaidml {

namespace ffi {

inline std::string str(plaidml_string* ptr) {
  std::string ret{plaidml_string_ptr(ptr)};
  plaidml_string_free(ptr);
  return ret;
}

template <typename T, typename F, typename... Args>
T call(F fn, Args... args) {
  plaidml_error err;
  auto ret = fn(&err, args...);
  if (err.code) {
    throw std::runtime_error(str(err.msg));
  }
  return ret;
}

template <typename F, typename... Args>
void call_void(F fn, Args... args) {
  plaidml_error err;
  fn(&err, args...);
  if (err.code) {
    throw std::runtime_error(str(err.msg));
  }
}

}  // namespace ffi

namespace details {

template <typename T>
struct Deleter {
  std::function<void(plaidml_error*, T*)> fn;
  void operator()(T* ptr) { ffi::call_void(fn, ptr); }
};

inline std::shared_ptr<plaidml_shape> make_plaidml_shape(plaidml_shape* ptr) {
  return std::shared_ptr<plaidml_shape>(ptr, Deleter<plaidml_shape>{plaidml_shape_free});
}

inline std::shared_ptr<plaidml_buffer> make_plaidml_buffer(plaidml_buffer* ptr) {
  return std::shared_ptr<plaidml_buffer>(ptr, Deleter<plaidml_buffer>{plaidml_buffer_free});
}

inline std::shared_ptr<plaidml_view> make_plaidml_view(plaidml_view* ptr) {
  return std::shared_ptr<plaidml_view>(ptr, Deleter<plaidml_view>{plaidml_view_free});
}

}  // namespace details

///
/// Initializes PlaidML's Core API.
///
inline void init() {  //
  ffi::call_void(plaidml_init);
}

///
/// \defgroup core_objects Objects
///

///
/// \ingroup core_objects
/// \enum DType
/// Enumerates all of the data types in PlaidML.
///
enum class DType {
  INVALID = PLAIDML_DATA_INVALID,
  BOOLEAN = PLAIDML_DATA_BOOLEAN,
  INT8 = PLAIDML_DATA_INT8,
  UINT8 = PLAIDML_DATA_UINT8,
  INT16 = PLAIDML_DATA_INT16,
  UINT16 = PLAIDML_DATA_UINT16,
  INT32 = PLAIDML_DATA_INT32,
  UINT32 = PLAIDML_DATA_UINT32,
  INT64 = PLAIDML_DATA_INT64,
  UINT64 = PLAIDML_DATA_UINT64,
  BFLOAT16 = PLAIDML_DATA_BFLOAT16,
  FLOAT16 = PLAIDML_DATA_FLOAT16,
  FLOAT32 = PLAIDML_DATA_FLOAT32,
  FLOAT64 = PLAIDML_DATA_FLOAT64,
};

inline const char* to_string(DType dtype) {
  switch (dtype) {
    case DType::BOOLEAN:
      return "bool";
    case DType::INT8:
      return "int8";
    case DType::UINT8:
      return "uint8";
    case DType::INT16:
      return "int16";
    case DType::UINT16:
      return "uint16";
    case DType::INT32:
      return "int32";
    case DType::UINT32:
      return "uint32";
    case DType::INT64:
      return "uint64";
    case DType::BFLOAT16:
      return "bfloat16";
    case DType::FLOAT16:
      return "float16";
    case DType::FLOAT32:
      return "float32";
    case DType::FLOAT64:
      return "float64";
    default:
      return "<invalid>";
  }
}

///
/// \ingroup core_objects
/// \class TensorShape
/// This is a TensorShape.
///
class TensorShape {
 public:
  ///
  /// TensorShape constructor
  ///
  TensorShape()
      : ptr_(details::make_plaidml_shape(
            ffi::call<plaidml_shape*>(plaidml_shape_alloc, PLAIDML_DATA_INVALID, 0, nullptr, nullptr))) {}

  ///
  /// TensorShape constructor
  /// \param dtype DType
  ///
  TensorShape(DType dtype,  //
              const std::vector<int64_t>& sizes) {
    size_t stride = 1;
    std::vector<int64_t> strides(sizes.size());
    for (int i = sizes.size() - 1; i >= 0; --i) {
      strides[i] = stride;
      stride *= sizes[i];
    }
    ptr_ = details::make_plaidml_shape(ffi::call<plaidml_shape*>(
        plaidml_shape_alloc, static_cast<plaidml_datatype>(dtype), sizes.size(), sizes.data(), strides.data()));
  }

  ///
  /// TensorShape constructor
  /// \param dtype DType
  /// \param sizes const vector<int64_t>
  /// \param strides const vector<int64_t>
  TensorShape(DType dtype,                        //
              const std::vector<int64_t>& sizes,  //
              const std::vector<int64_t>& strides) {
    if (sizes.size() != strides.size()) {
      throw std::runtime_error("Sizes and strides must have the same rank.");
    }
    ptr_ = details::make_plaidml_shape(ffi::call<plaidml_shape*>(
        plaidml_shape_alloc, static_cast<plaidml_datatype>(dtype), sizes.size(), sizes.data(), strides.data()));
  }

  ///
  /// TensorShape constructor
  /// \param ptr const shared_ptr<plaidml_shape>
  ///
  explicit TensorShape(const std::shared_ptr<plaidml_shape>& ptr) : ptr_(ptr) {}

  ///
  /// dtype
  /// \return DType
  ///
  DType dtype() const { return static_cast<DType>(ffi::call<plaidml_datatype>(plaidml_shape_get_dtype, as_ptr())); }

  ///
  /// Returns the number of dimensions in the TensorShape
  /// \return size_t
  ///
  size_t ndims() const { return ffi::call<size_t>(plaidml_shape_get_ndims, as_ptr()); }

  ///
  /// nbytes
  /// \return uint64_t
  ///
  uint64_t nbytes() const { return ffi::call<uint64_t>(plaidml_shape_get_nbytes, as_ptr()); }
  std::string str() const { return ffi::str(ffi::call<plaidml_string*>(plaidml_shape_repr, as_ptr())); }
  bool operator==(const TensorShape& rhs) const { return str() == rhs.str(); }
  plaidml_shape* as_ptr() const { return ptr_.get(); }

 private:
  std::shared_ptr<plaidml_shape> ptr_;
};

///
/// \ingroup core_objects
/// \class View
/// This is a View.
///
class View {
  friend class Buffer;

 public:
  ///
  /// data
  ///
  char* data() {  //
    return ffi::call<char*>(plaidml_view_data, ptr_.get());
  }

  ///
  /// size
  ///
  size_t size() {  //
    return ffi::call<size_t>(plaidml_view_size, ptr_.get());
  }

  ///
  /// writeback
  ///
  void writeback() {  //
    ffi::call_void(plaidml_view_writeback, ptr_.get());
  }

 private:
  explicit View(const std::shared_ptr<plaidml_view>& ptr) : ptr_(ptr) {}

 private:
  std::shared_ptr<plaidml_view> ptr_;
};

///
/// \ingroup core_objects
/// \class Buffer
/// This is a Buffer.
///
class Buffer {
 public:
  ///
  /// Buffer constructor
  ///
  Buffer() = default;

  ///
  /// Buffer constructor
  /// \param device string
  /// \param shape TensorShape
  ///
  Buffer(const std::string& device, const TensorShape& shape)
      : ptr_(details::make_plaidml_buffer(
            ffi::call<plaidml_buffer*>(plaidml_buffer_alloc, device.c_str(), shape.nbytes()))),
        shape_(shape) {}

  ///
  /// Buffer constructor
  /// \param ptr plaidml_buffer*
  /// \param shape TensorShape
  explicit Buffer(plaidml_buffer* ptr, const TensorShape& shape)
      : ptr_(details::make_plaidml_buffer(ptr)), shape_(shape) {}

  ///
  /// Returns a pointer to the Buffer.
  /// \return plaidml_buffer*
  ///
  plaidml_buffer* as_ptr() const {  //
    return ptr_.get();
  }

  ///
  /// mmap_current
  /// \return View
  ///
  View mmap_current() {
    return View(details::make_plaidml_view(ffi::call<plaidml_view*>(plaidml_buffer_mmap_current, as_ptr())));
  }

  ///
  /// mmap_discard
  /// \return View
  ///
  View mmap_discard() {
    return View(details::make_plaidml_view(ffi::call<plaidml_view*>(plaidml_buffer_mmap_discard, as_ptr())));
  }

  void copy_into(void* dst) {
    auto view = mmap_current();
    memcpy(dst, view.data(), view.size());
  }

  void copy_from(const void* src) {
    auto view = mmap_discard();
    memcpy(view.data(), src, view.size());
    view.writeback();
  }

 private:
  std::shared_ptr<plaidml_buffer> ptr_;
  TensorShape shape_;
};

///
/// \ingroup core_objects
/// \struct Settings
/// These are the Settings.
///
struct Settings {
  ///
  /// Gets the setting specified by `key`
  /// \param key string
  /// \return string
  ///
  static std::string get(const std::string& key) {
    return ffi::str(ffi::call<plaidml_string*>(plaidml_settings_get, key.c_str()));
  }

  ///
  /// Sets the setting specified by `key` to the `value` specified.
  /// \param key string
  /// \param value string
  ///
  static void set(const std::string& key, const std::string& value) {
    ffi::call_void(plaidml_settings_set, key.c_str(), value.c_str());
  }
};

}  // namespace plaidml