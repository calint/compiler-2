#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "generics.hpp"
#include "toc.hpp"

// type names that stand for the arguments of a generic definition while an
// instance is parsed, they are unbound when the scope ends
class type_alias_scope final {
    toc& tc_;
    std::vector<std::string> names_;

  public:
    explicit type_alias_scope(toc& tc) : tc_{tc} {}

    type_alias_scope(const type_alias_scope&) = delete;
    type_alias_scope(type_alias_scope&&) = delete;
    auto operator=(const type_alias_scope&) -> type_alias_scope& = delete;
    auto operator=(type_alias_scope&&) -> type_alias_scope& = delete;

    ~type_alias_scope() {
        for (const std::string& name : names_) {
            tc_.unbind_type_alias(name);
        }
    }

    auto bind(const token& src_loc_tk, const std::string_view name,
              const type& tpe) -> void {

        tc_.bind_type_alias(src_loc_tk, name, tpe);
        names_.emplace_back(name);
    }

    // the generic type and the type arguments of the instance
    auto bind_instance(const token& src_loc_tk,
                       const generic_type_instance& instance) -> void {

        bind(src_loc_tk, instance.generic_name, *instance.type_ptr);

        for (const generic_binding& binding : instance.bindings) {
            if (binding.type_ptr != nullptr) {
                bind(src_loc_tk, binding.name, *binding.type_ptr);
            }
        }
    }
};

// an instance sees the types, not the type parameters of the callers that
// needed it, they are visible again when the scope ends
class generic_instance_scope final {
    toc& tc_;

  public:
    explicit generic_instance_scope(toc& tc) : tc_{tc} {
        tc_.enter_generic_instance();
    }

    generic_instance_scope(const generic_instance_scope&) = delete;
    generic_instance_scope(generic_instance_scope&&) = delete;
    auto operator=(const generic_instance_scope&)
        -> generic_instance_scope& = delete;
    auto operator=(generic_instance_scope&&)
        -> generic_instance_scope& = delete;

    ~generic_instance_scope() { tc_.exit_generic_instance(); }
};
