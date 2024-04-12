#pragma once

#include <coroutine>
#include <cstdlib>
#include <vector>

struct Co;
struct CoP;
using CoH = std::coroutine_handle<CoP>;
using CoStack = std::vector<CoH>;

template <typename... Args>
struct std::coroutine_traits<Co, Args...> {
  using promise_type = CoP;
};

struct Co {
  CoH h;
  struct CoAwait {
    Co &co;
    bool await_ready() {
      return false;
    }
    inline void await_suspend(CoH h);
    void await_resume() {}
  };
  CoAwait operator co_await() {
    return {*this};
  }
};

struct CoP {
  CoStack *stack = nullptr;
  Co get_return_object() {
    return Co{CoH::from_promise(*this)};
  }
  std::suspend_always initial_suspend() {
    return {};
  }
  std::suspend_always final_suspend() noexcept {
    return {};
  }
  void unhandled_exception() {
    abort();
  }
  void return_void() {}
};

void Co::CoAwait::await_suspend(CoH h) {
  auto stack = h.promise().stack;
  if (not stack) {
    abort();
  }
  co.h.promise().stack = stack;
  stack->emplace_back(co.h);
}

void co_run(Co &co) {
  if (co.h.promise().stack) {
    abort();
  }
  CoStack stack{co.h};
  co.h.promise().stack = &stack;
  while (not stack.empty()) {
    if (stack.back().done()) {
      stack.pop_back();
    } else {
      stack.back().resume();
    }
  }
}
void co_run(Co &&co) {
  co_run(co);
}
