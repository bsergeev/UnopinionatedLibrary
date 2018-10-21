// This code is based on Peter Goldsborough's blog "Type Erasure for Unopinionated
// Interfaces in C++":
// https://www.goldsborough.me/cpp/2018/05/22/00-32-43-type_erasure_for_unopinionated_interfaces_in_c++/
//
// Main changes:
//  - replaces Any with std::any
//  - used C++17 fold expression to simplify detail::collect_any_vector()
//  - implemented classes passed to Office::work()
//  - moved everything into a single file
//  - renamed a couple of classes and their members

#include <any>
#include <cassert>
#include <iostream>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

using namespace std::string_literals;

struct Monitor  { std::string name() const noexcept { return "monitor"s;  } };
struct Keyboard { std::string name() const noexcept { return "keyboard"s; } };
struct Cup      { std::string name() const noexcept { return "coffee"s;   } };
struct Recipe   { std::string name() const noexcept { return "recipe"s;   } };
struct Ingredient {
  Ingredient(std::string name = "stuff"s) : m_name(std::move(name)) {}
  const std::string& name() const noexcept { return m_name; }
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
  AnyPerson(P&& person) : m_content(make_holder(std::forward<P>(person), &std::remove_reference_t<P>::work)) {}
  AnyPerson(const AnyPerson& other) : m_content(other.m_content->clone()) {}
  AnyPerson(AnyPerson&& other) noexcept { swap(other); }

  AnyPerson& operator=(AnyPerson other) { swap(other); return *this; }
  void swap(AnyPerson& other) noexcept { m_content.swap(other.m_content); }

  const std::string& name() const noexcept { return m_content->name(); }

  template<typename... Args>
  void work(Args&&... arguments) {
    std::vector<std::any> any_arguments;
    detail::collect_any_vector(any_arguments, std::forward<Args>(arguments)...);
    return m_content->invoke_work(std::move(any_arguments));
  }

  template<typename P>
  P& get() {
    if (std::type_index(typeid(P)) == std::type_index(m_content->type_info())) {
      return static_cast<Holder<P>&>(*m_content).value_;
    }
    throw std::bad_cast();
  }

private:
  struct IHolder {
    virtual ~IHolder() = default;
    virtual const std::type_info&    type_info() const = 0;
    virtual std::unique_ptr<IHolder> clone()           = 0;
    virtual const std::string&       name() const noexcept = 0;
    virtual void invoke_work(std::vector<std::any>&& args) = 0;
  };

  template<typename P, typename... Args>
  struct Holder : public IHolder {
    template<typename Q>
    explicit Holder(Q&& person) : m_person(std::forward<Q>(person)) { }

    const std::type_info& type_info() const override { return typeid(P); }

    std::unique_ptr<IHolder> clone() override {
      return std::make_unique<Holder<P, Args...>>(m_person);
    }

    const std::string& name() const noexcept override { return m_person.name(); }

    void invoke_work(std::vector<std::any>&& arguments) override {
      assert(arguments.size() == sizeof...(Args));
      std::cout << "working on ";
      invoke_work(std::move(arguments), std::make_index_sequence<sizeof...(Args)>());
    }

    template<size_t... Is>
    void invoke_work(std::vector<std::any>&& arguments, std::index_sequence<Is...>) {
      // Expand the index sequence to access each std::any stored in `arguments` and cast
      // to the type expected at each index. Note we move each value out of the std::any.
      return m_person.work(std::move(std::any_cast<Args>(arguments[Is]))...);
    }

    P m_person;
  }; // struct Holder

  template<typename P, typename... Args>
  std::unique_ptr<IHolder> make_holder(P&& person, void(std::remove_reference_t<P>::*)(Args...)) {
    return std::make_unique<Holder<P, Args...>>(std::forward<P>(person));
  }

  std::unique_ptr<IHolder> m_content;
}; // class AnyPerson

namespace Library {
class Person {
public:
  explicit Person(std::string name) : m_name(std::move(name)) { }
  virtual ~Person() = default;
  const std::string& name() const noexcept { return m_name; }
  // no virtual work() method!
protected:
  std::string m_name;
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
  void work(Recipe recipe, const std::vector<Ingredient>& ingredients) { 
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
  void work(Monitor monitor, Keyboard keyboard, Cup coffee) { 
    std::cout << keyboard.name() <<", "<< monitor.name() <<", and "<< coffee.name() << std::endl; 
  }
};

int main(int, char*[]) {
  Library::Office{Cook      ("Alice"s)}.work(Recipe{}, std::vector<Ingredient>{{"flour"s, "eggs"s, "milk"s}});
  Library::Office{Programmer("Peter"s)}.work(Monitor{}, Keyboard{}, Cup{});
}
