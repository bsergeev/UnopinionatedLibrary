// This code is based on Peter Goldsborough's blog "Type Erasure for Unopinionated
// Interfaces in C++":
// https://www.goldsborough.me/cpp/2018/05/22/00-32-43-type_erasure_for_unopinionated_interfaces_in_c++/
// My changes:
//  - replaced Any with std::any
//  - used C++17 fold expression to simplify detail::collect_any_vector()
//  - implemented classes passed to Office::work(), just to get a working example
//  - moved everything into a single file
//  - renamed several classes and their members
//
// Explanation:
// Instantiate `Library::Office` by passing an object of `Library::Person` sub-class
// to `Library::Office` constructor. This object is then used to implicitly construct
// `AnyPerson` that gets stored in `Office::m_person`. When `Library::Office::work()` is
// invoked (with arbitrary arguments!), it forwards its arguments to `AnyPerson::work()`.
// `AnyPerson::work()` then wraps these arbitrary arguments into `std::vector<std::any>`
// and passes them to virtual `IPersonHolder::invoke_work()` method. The concrete sub-class
// of `IPersonHolder` that's stored in `AnyPerson::m_personHolder` is templated on the type
// of `Library::Person` sub-class and the signature of its `do_work()` method.
// So, when `AnyPerson::work()` calls the overriden `IPersonHolder::invoke_work()` on its 
// `m_personHolder` class variable, the `IPersonHolder` virtual table forwards this call to
// the (templated) `PersonHolder::invoke_work()`, which first verifies that the size of
// `std::vector<std::any>` argument matches the number of parameters in `do_work()` method
// of `Library::Person` sub-class that this `PersonHolder` was templated with. Then,
// `PersonHolder::invoke_work()` invokes the actual `do_work()` method of `Library::Person`
// sub-class contained in its `PersonHolder::m_person`, while invoking `std::any_cast<>`
// on each argument passed to `do_work()`.
//
// As Peter Goldsborough mentioned in his blog:
// "The primary drawback is that verification of argument types is moved to runtime
// instead of compile team. This is especially annoying since implicit conversions do 
// not work either, such that passing an int where a long is expected will result in a
// runtime exception. Furthermore, also the number of arguments can only at runtime be
// compared to the arity of the method. Finally, since the statically known number of
// arguments (sizeof...(Args)) given to AnyPerson::work is lost while passing through
// PersonHolder::invoke_work, we must expect the number of arguments to be equal to the
// arity of the concrete work() method. This means default arguments do not work out of
// the box."
//
// The code prints:
//   Alice is working on recipe with 3 ingredients: flour, eggs, milk
//   Peter is working on keyboard, monitor, and coffee
// where:
// - the name and " is" (e.g. "Alice is") is printed from `Library::Office::work()`
// - "working on" is printed from `AnyPerson::PersonHolder::invoke_work()`
// - and the rest is printed from `do_work()` of `Library::Person` sub-classes.
// This shows that some code can do common processing of `Library::Person` objects,
// while pseudo-virtual invocation of (arbitrarily different) `do_work() methods is
// taking place.


#include <any>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace std::string_literals;

struct Monitor  { [[nodiscard]] std::string name() const noexcept { return "monitor"s;  } };
struct Keyboard { [[nodiscard]] std::string name() const noexcept { return "keyboard"s; } };
struct Cup      { [[nodiscard]] std::string name() const noexcept { return "coffee"s;   } };
struct Recipe   { [[nodiscard]] std::string name() const noexcept { return "recipe"s;   } };
struct Ingredient {
  Ingredient(std::string name = "stuff"s) : m_name(std::move(name)) {}
  [[nodiscard]] const std::string& name() const noexcept { return m_name; }
private:
  const std::string m_name;
};

namespace detail {
  template<typename... T>
  void collect_any_vector(std::vector<std::any>& vector, T&&... args) {
    (vector.push_back(std::forward<T>(args)), ...);
  }
} // namespace detail

namespace Library { class Person; }

class AnyPerson {
public:
  template<typename P,
           typename = std::enable_if_t<std::is_base_of_v<Library::Person, std::decay_t<P>>>>
  AnyPerson(P&& person) 
    : m_personHolder(make_holder(std::forward<P>(person), &std::remove_reference_t<P>::do_work))
  {}

  [[nodiscard]] const std::string& name() const noexcept { return m_personHolder->name(); }

  template<typename... Args>
  void work(Args&&... arguments) {
    std::vector<std::any> any_arguments;
    detail::collect_any_vector(any_arguments, std::forward<Args>(arguments)...);
    return m_personHolder->invoke_work(std::move(any_arguments));
  }

private:
  struct IPersonHolder {
    virtual ~IPersonHolder() = default;
    virtual const std::string& name() const noexcept       = 0;
    virtual void invoke_work(std::vector<std::any>&& args) = 0;
  };

  template<typename P, typename... Args>
  struct PersonHolder : public IPersonHolder {
    template<typename Q>
    explicit PersonHolder(Q&& person) : m_person(std::forward<Q>(person)) { }

    [[nodiscard]] const std::string& name() const noexcept override { return m_person.name(); }

    void invoke_work(std::vector<std::any>&& arguments) override {
      assert(arguments.size() == sizeof...(Args));
      std::cout << "working on ";
      invoke_work_impl(std::move(arguments), std::make_index_sequence<sizeof...(Args)>());
    }
  private:
    template<size_t... Is>
    void invoke_work_impl(std::vector<std::any>&& arguments, std::index_sequence<Is...>) {
      // Expand the index sequence to access each std::any stored in `arguments` and cast
      // to the type expected at each index. Note we move each value out of the std::any.
      return m_person.do_work(std::move(std::any_cast<Args>(arguments[Is]))...);
    }

    P m_person;
  }; // struct PersonHolder

private:
  template<typename P, typename... Args>
  std::unique_ptr<IPersonHolder> make_holder(P&& person, void(std::remove_reference_t<P>::*)(Args...)) {
    return std::make_unique<PersonHolder<P, Args...>>(std::forward<P>(person));
  }

  std::unique_ptr<IPersonHolder> m_personHolder;
}; // class AnyPerson

namespace Library {
class Person {
public:
  explicit Person(std::string name) : m_name(std::move(name)) { }
  virtual ~Person() = default;
  [[nodiscard]] const std::string& name() const noexcept { return m_name; }
  // no virtual do_work() method!
private:
  const std::string m_name;
};

class Office {
public:
  explicit Office(AnyPerson person) : m_person(std::move(person)) { }

  template<typename... Args>
  void work(Args&&... args) {
    std::cout << m_person.name() << " is ";
    m_person.work(std::forward<Args>(args)...);
  }

private:
  AnyPerson m_person;
};
} // namespace Library

class Cook : public Library::Person {
public:
  using Library::Person::Person;
  void do_work(Recipe recipe, const std::vector<Ingredient>& ingredients) {
    std::cout << recipe.name() <<" with "<< ingredients.size() <<" ingredients: ";
    bool first = true;
    for (const auto& i : ingredients) {
      std::cout << (first? "" : ", ") << i.name();
      first = false;
    }
    std::cout << std::endl;
  }
};

class Programmer : public Library::Person {
public:
  using Library::Person::Person;
  void do_work(Monitor monitor, Keyboard keyboard, Cup coffee) {
    std::cout << keyboard.name() <<", "<< monitor.name() <<", and "<< coffee.name() << std::endl; 
  }
};

int main(int, char*[]) {
  Library::Office{Cook      {"Alice"s}}.work(Recipe{}, std::vector<Ingredient>{{"flour"s, "eggs"s, "milk"s}});
  Library::Office{Programmer{"Peter"s}}.work(Monitor{}, Keyboard{}, Cup{});
}
