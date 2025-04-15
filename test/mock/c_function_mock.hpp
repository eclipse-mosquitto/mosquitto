#pragma once

#include <dlfcn.h>

#include <functional>
#include <stdexcept>

template <typename MockClass>
class CFunctionMock {
  static CFunctionMock<MockClass>*& get_obj() {
    static CFunctionMock<MockClass>* mock_obj = nullptr;
    return mock_obj;
  }

 public:
  CFunctionMock() {
    if (get_obj() != nullptr) {
      throw std::logic_error("only one instance of MockClass allowed");
    }
    get_obj() = this;
  }
  CFunctionMock(const CFunctionMock&) = delete;
  CFunctionMock(CFunctionMock&&) = delete;
  CFunctionMock& operator=(const CFunctionMock&) = delete;
  CFunctionMock& operator=(CFunctionMock&&) = delete;
  virtual ~CFunctionMock() { get_obj() = nullptr; }

  static bool mock_exists() { return get_obj() != nullptr; }

  static MockClass& get_mock() {
    auto* instance = dynamic_cast<MockClass*>(get_obj());
    if (instance == nullptr) {
      throw std::logic_error(std::string("mock ") + typeid(MockClass).name() +
                             " is not initialized");
    }
    return *instance;
  }

  template <class ReturnType, bool Void = std::is_void<ReturnType>::value, typename... Args>
  static std::function<ReturnType(Args...)> optional_mocked_fn(
      ReturnType (MockClass::*mock_fn)(Args...), const std::string& function_name) {
    return std::function<ReturnType(Args...)>{
        OptionalMockFnWrapper<ReturnType, std::is_void<ReturnType>::value, Args...>(mock_fn,
                                                                                    function_name)};
  }

  template <class ReturnType, bool Void = std::is_void<ReturnType>::value, typename... Args>
  static std::function<ReturnType(Args...)> original_fn(ReturnType (MockClass::*mock_fn)(Args...),
                                                        const std::string& function_name) {
    return std::function<ReturnType(Args...)>{
        OriginalFnWrapper<ReturnType, std::is_void<ReturnType>::value, Args...>(mock_fn,
                                                                                function_name)};
  }

 private:
  template <class ReturnType, bool Void = std::is_void<ReturnType>::value, typename... Args>
  class OriginalFnWrapper {
   public:
    OriginalFnWrapper(ReturnType (MockClass::* /* mock_fn */)(Args...),
                      const std::string& function_name)
        : orginial_fn{
              reinterpret_cast<ReturnType (*)(Args...)>(dlsym(RTLD_NEXT, function_name.c_str()))} {}
    ReturnType operator()(Args... args) { return (*orginial_fn)(args...); }

   private:
    ReturnType (*orginial_fn)(Args...);
  };

  template <class ReturnType, bool Void = std::is_void<ReturnType>::value, typename... Args>
  class OptionalMockFnWrapper : public OriginalFnWrapper<ReturnType, Void, Args...> {
   public:
    using base_class = OriginalFnWrapper<ReturnType, Void, Args...>;
    OptionalMockFnWrapper(ReturnType (MockClass::*mock_fn)(Args...),
                          const std::string& function_name)
        : base_class{mock_fn, function_name}, mock_fn_(mock_fn) {}
    ReturnType operator()(Args... args) {
      if (get_obj() != nullptr) {
        return (get_mock().*mock_fn_)(args...);
      }
      return base_class::operator()(args...);
    }

   private:
    ReturnType (MockClass::*mock_fn_)(Args...);
  };

  template <class ReturnType, typename... Args>
  class OriginalFnWrapper<ReturnType, true, Args...> {
   public:
    OriginalFnWrapper(void (MockClass::* /* mock_fn */)(Args...), const std::string& function_name)
        : orginial_fn{
              reinterpret_cast<void (*)(Args...)>(dlsym(RTLD_NEXT, function_name.c_str()))} {}
    void operator()(Args... args) { (*orginial_fn)(args...); }

   private:
    void (*orginial_fn)(Args...);
  };

  template <class ReturnType, typename... Args>
  class OptionalMockFnWrapper<ReturnType, true, Args...>
      : public OriginalFnWrapper<ReturnType, true, Args...> {
   public:
    using base_class = OriginalFnWrapper<ReturnType, true, Args...>;
    OptionalMockFnWrapper(void (MockClass::*mock_fn)(Args...), const std::string& function_name)
        : base_class(mock_fn, function_name), mock_fn_(mock_fn) {}

    void operator()(Args... args) {
      if (get_obj() != nullptr) {
        (get_mock().*mock_fn_)(args...);
      } else {
        base_class::operator()(args...);
      }
    }

   private:
    void (MockClass::*mock_fn_)(Args...);
  };
};
