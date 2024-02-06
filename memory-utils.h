#pragma once
#include <cstdlib>
#include <utility>
#include <exception>
#include <functional>
#include <atomic>
#include <memory>
#include <unordered_set>

namespace utils {
  // For life cycle manager family (LifecycleGuard and etc)
  struct _CounterBase {
    std::atomic_int32_t count;
    bool dead;
  };

  // Base class for life cycle manager family
  class _LifeCycleBase {
  public:
    virtual ~_LifeCycleBase() {}
    virtual bool Alive() const = 0;
  };

  // Intrusive class of basic dangling avoiding facility
  // Inherit this class to make ProtectedPointer<T> work.
  template <typename T>
  class LifeCycleGuard {
  private:
    _CounterBase *counter_;

  public:
    virtual ~LifeCycleGuard() {
      if (counter_->count == 0) delete counter_;
      else counter_->dead = true;
    }

    LifeCycleGuard() : counter_(new _CounterBase{ 0, false }) {}
    //Pointer copy must be wrapped by ProtectedPointer class to get protection.
    //Each object holds a unique counter. This is not a referece count manager.
    explicit
      LifeCycleGuard(const LifeCycleGuard<T> &) : counter_(new _CounterBase{ 0, false }) {}
    explicit
      LifeCycleGuard(const LifeCycleGuard<T> &&) : counter_(new _CounterBase{ 0, false }) {}

    constexpr _CounterBase *_GetCounter() const { return counter_; }
  };

  // Assistant class for avoiding dangling pointer
  // TODO: multithreading support
  template <typename T>
  class ProtectedPointer : virtual public _LifeCycleBase {
    static_assert(std::is_base_of<LifeCycleGuard<T>, T>::value, "Class is not managed by LifeCycleGuard");
  public:
    using ObjectType = T;
    using PointerType = T *;

  protected:
    _CounterBase *counter_;
    PointerType  ptr_;

  public:
    //This class doesn't need Release() method.

    void Swap(ProtectedPointer<T> &rhs) {
      std::swap(counter_, rhs.counter_);
      std::swap(ptr_, rhs.ptr_);
    }

    void Swap(ProtectedPointer<T> &&rhs) {
      std::swap(counter_, rhs.counter_);
      std::swap(ptr_, rhs.ptr_);
    }

  public:
    ~ProtectedPointer() {
      //Only myself ?
      if (counter_->count <= 1) delete counter_;
      //Reduce count number
      else counter_->count -= 1;
    }

    // Is instance alive
    bool Alive() const override { return !counter_->dead || counter_ != nullptr; }
    bool Dead() const { return counter_->dead || counter_ == nullptr; }

    ProtectedPointer() = delete;

    explicit ProtectedPointer(const typename ProtectedPointer<T> &rhs) :
      counter_(rhs.counter_), ptr_(rhs.ptr_) {
      if (counter_ != nullptr) {
        counter_.count += 1;
      }
    }

    explicit ProtectedPointer(const typename ProtectedPointer<T> &&rhs) :
      counter_(rhs.counter_), ptr_(rhs.ptr_) {
      if (counter_ != nullptr) {
        counter_.count += 1;
      }
    }

    ProtectedPointer(PointerType ptr) : counter_(ptr->_GetCounter()), ptr_(ptr) {
      counter_->count += 1;
    }

    ObjectType &Seek() { 
      if (counter_->dead) throw std::exception("Destination is dead");
      return *ptr_; 
    }

    PointerType Get() { 
      if (counter_->dead) throw std::exception("Destination is dead");
      return ptr_; 
    }

    PointerType operator->() { return Get(); }
    ObjectType &operator*() { return Seek(); }

    //assign operator
    void operator=(const ProtectedPointer<T> &rhs) {
      if (counter_ != nullptr) {
        if (counter_.count <= 1) delete counter_;
        else counter_->count -= 1;
      }

      counter_ = rhs.counter_;
      ptr_ = rhs.ptr_;
      if (counter_ != nullptr) {
        counter_->count += 1;
      }
    }

    void operator=(const ProtectedPointer<T> &&rhs) { this->operator=(rhs); }

    void operator=(const PointerType rhs) {
      if (counter_ != nullptr) {
        if (counter_.count <= 1) delete counter_;
        else counter_->count -= 1;
      }

      counter_ = rhs->_GetCounter();
      ptr_ = rhs;
    }

    //equals operator
    constexpr bool operator==(const ProtectedPointer<T> &rhs) { return ptr_ == rhs.ptr_; }
    constexpr bool operator==(const ProtectedPointer<T> &&rhs) { return ptr_ == rhs.ptr_; }
    constexpr bool operator==(const PointerType rhs) { return ptr_ == rhs; }
  };

  // Intrusive counter for IntrusivePointer.
  template <typename T>
  class IntrusiveCounter {
  private:
    std::atomic_int32_t count_;
  public:
    IntrusiveCounter() : count_(1) {}
    IntrusiveCounter(const IntrusiveCounter<T> &rhs) : count_(1) {}
    IntrusiveCounter(const IntrusiveCounter<T> &&rhs) : count_(1) {}

    void _Counter_Increase() { count_ += 1; }
    void _Counter_Decrease() { count_ -= 1; }
    auto _Counter_Value() const { return count_.operator int(); }
  };

  //Intrusive RC Pointer class
  template <typename T>
  class IntrusivePointer {
    static_assert(std::is_base_of<IntrusiveCounter<T>, T>::value, "Class is not managed by IntrusiveCounter");
  public:
    using ObjectType = T;
    using PointerType = T *;

  protected:
    PointerType ptr_;

  public:
    void Release() {
      if (ptr_ != nullptr && ptr_->_Counter_Value() == 1) delete ptr_;
      else if (ptr_ != nullptr) ptr_->_Counter_Decrease();

      ptr_ = nullptr;
    }

    void Swap(IntrusivePointer<T> &rhs) {
      std::swap(ptr_, rhs.ptr_);
    }

    void Swap(IntrusivePointer<T> &&rhs) {
      std::swap(ptr_, rhs.ptr_);
    }

  public:
    ~IntrusivePointer() {
      // Only myself?
      if (ptr_->_Counter_Value() == 1) delete ptr_;
      // Decrease count
      else ptr_->_Counter_Decrease();
    }

    IntrusivePointer() : ptr_(nullptr) {}
    IntrusivePointer(const IntrusivePointer<T> &rhs) : ptr_(rhs.ptr_) {
      ptr_->_Counter_Increase();
    }

    IntrusivePointer(const IntrusivePointer<T> &&rhs) : ptr_(rhs.ptr_) {
      ptr_->_Counter_Increase();
    }

    IntrusivePointer(const PointerType ptr) : ptr_(ptr) {}

    constexpr ObjectType &Seek() { return *ptr_; }
    constexpr PointerType Get() { return ptr_; }
    constexpr PointerType operator->() { return Get(); }
    constexpr ObjectType &operator*() { return Seek(); }

    //TODO:operator override
    //assign operator
    void operator=(const IntrusivePointer<T> &rhs) {
      //Preprocessing for current wrapped pointer
      Release();

      // Wrap new pointer
      ptr_ = rhs.ptr_;
      // Check reference count
      if (ptr_ != nullptr) ptr_->_Counter_Increase();
    }

    void operator=(const IntrusivePointer<T> &&rhs) { this->operator=(rhs); }

    void operator=(const PointerType rhs) {
      Release();
      // no need to increase count?
      bool empty = ptr_ == nullptr;
      ptr_ = rhs;
      // Check reference count
      if (ptr_ != nullptr && !empty) ptr_->_Counter_Increase();
    }

    constexpr bool operator==(const IntrusivePointer<T> &rhs) { return ptr_ == rhs.ptr_; }
    constexpr bool operator==(const IntrusivePointer<T> &&rhs) { return ptr_ == rhs.ptr_; }
    constexpr bool operator==(const PointerType rhs) { return ptr_ == rhs; }
  };
}
