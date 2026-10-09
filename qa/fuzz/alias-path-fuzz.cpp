// fuzz of 'access_path::overlaps' against a brute force of the bytes: random
// types, two random accesses to one variable, every value of every run-time
// index is tried
//
// usage: alias-path-fuzz [iterations] [seed]
// exits 1 when the check says "disjoint" for bytes that can be the same
// (unsound) or "may overlap" for bytes that never are (imprecise)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "../../src/ub_alias.hpp"

namespace {

struct node;

struct field {
    size_t offset{};
    const node* type{};
};

// a scalar (no fields, no element), a struct or an array
struct node {
    size_t size_bytes{};
    size_t alignment{1};
    std::vector<field> fields;
    const node* elem{};
    size_t length{};
};

// one step of an access with what the brute force needs
struct eval_step {
    path_step::kind_type kind{};
    size_t offset{};     // field offset or constant index
    size_t elem_bytes{}; // size of an element
    size_t length{};     // elements of the array
};

struct access {
    access_path path;
    std::vector<eval_step> steps;
    size_t final_bytes{};
};

class generator {
    std::mt19937_64 rng_;
    std::vector<std::unique_ptr<node>> pool_;

  public:
    explicit generator(const size_t seed) : rng_{seed} {}

    auto clear() -> void { pool_.clear(); }

    auto pick(const size_t count) -> size_t { return rng_() % count; }

    auto make_type(const int depth) -> node* {
        const size_t kind{depth >= 3 ? 0 : pick(3)};

        if (kind == 0) {
            return make_scalar();
        }

        if (kind == 1) {
            return make_array(make_type(depth + 1));
        }

        return make_struct(depth);
    }

    auto make_array(const node* elem) -> node* {
        node* n{add_node()};
        n->elem = elem;
        n->length = 1 + pick(4);
        n->alignment = elem->alignment;
        n->size_bytes = elem->size_bytes * n->length;

        return n;
    }

    auto make_access(const node* root_type) -> access {
        access result;
        const node* cur{root_type};

        for (size_t depth{}; depth < 12; ++depth) {
            if (cur->length > 0) {
                if (pick(5) == 0) {
                    break;
                }

                step_into_array(result, *cur);
                cur = cur->elem;
            } else if (not cur->fields.empty()) {
                if (pick(4) == 0) {
                    break;
                }

                const field& f{cur->fields.at(pick(cur->fields.size()))};

                result.path.add_field(f.offset);
                result.steps.push_back({
                    .kind{path_step::kind_type::field},
                    .offset{f.offset},
                });

                cur = f.type;
            } else {
                break;
            }
        }

        result.final_bytes = cur->size_bytes;

        return result;
    }

  private:
    auto add_node() -> node* {
        pool_.push_back(std::make_unique<node>());
        return pool_.back().get();
    }

    auto make_scalar() -> node* {
        constexpr std::array<size_t, 4> sizes{1, 2, 4, 8};

        node* n{add_node()};
        n->size_bytes = sizes.at(pick(sizes.size()));
        n->alignment = n->size_bytes;

        return n;
    }

    auto make_struct(const int depth) -> node* {
        node* n{add_node()};
        size_t offset{};

        for (size_t i{}, count{1 + pick(4)}; i < count; ++i) {
            const node* type{make_type(depth + 1)};

            // a field is sometimes an array
            if (pick(3) == 0) {
                type = make_array(type);
            }

            offset = (offset + type->alignment - 1) / type->alignment *
                     type->alignment;

            n->fields.push_back({.offset{offset}, .type{type}});
            offset += type->size_bytes;
            n->alignment = std::max(n->alignment, type->alignment);
        }

        n->size_bytes =
            (offset + n->alignment - 1) / n->alignment * n->alignment;

        return n;
    }

    auto step_into_array(access& result, const node& array) -> void {
        if (pick(2) == 0) {
            const size_t index{pick(array.length)};

            result.path.add_fixed_index(index);
            result.steps.push_back({
                .kind{path_step::kind_type::fixed_index},
                .offset{index},
                .elem_bytes{array.elem->size_bytes},
                .length{array.length},
            });

            return;
        }

        result.path.add_run_time_index();
        result.steps.push_back({
            .kind{path_step::kind_type::run_time_index},
            .elem_bytes{array.elem->size_bytes},
            .length{array.length},
        });
    }
};

// every byte offset the access can start at, for every value of its indexes
auto starts_of(const access& acc) -> std::vector<size_t> {
    std::vector<size_t> starts{0};

    for (const eval_step& step : acc.steps) {
        std::vector<size_t> next;

        for (const size_t start : starts) {
            if (step.kind == path_step::kind_type::field) {
                next.push_back(start + step.offset);
            } else if (step.kind == path_step::kind_type::fixed_index) {
                next.push_back(start + (step.offset * step.elem_bytes));
            } else {
                for (size_t k{}; k < step.length; ++k) {
                    next.push_back(start + (k * step.elem_bytes));
                }
            }
        }

        starts = std::move(next);
    }

    return starts;
}

auto can_be_same_bytes(const access& lhs, const access& rhs) -> bool {
    const std::vector<size_t> lhs_starts{starts_of(lhs)};
    const std::vector<size_t> rhs_starts{starts_of(rhs)};

    return std::ranges::any_of(lhs_starts, [&](const size_t x) -> bool {
        return std::ranges::any_of(rhs_starts, [&](const size_t y) -> bool {
            return x < y + rhs.final_bytes and y < x + lhs.final_bytes;
        });
    });
}

} // namespace

auto main(const int argc, char** const argv) -> int {
    const std::vector<std::string> args(argv + 1, argv + argc);

    const size_t iterations{args.size() > 0 ? std::stoull(args.at(0))
                                            : 1000000};
    const size_t seed{args.size() > 1 ? std::stoull(args.at(1)) : 1};

    generator gen{seed};
    size_t both_overlap{};
    size_t both_disjoint{};
    size_t unsound{};
    size_t imprecise{};

    for (size_t it{}; it < iterations; ++it) {
        node* type{gen.make_type(0)};

        // a variable that is an array of the type, or the type
        if (gen.pick(2) == 0 and type->length == 0) {
            type = gen.make_array(type);
        }

        const access lhs{gen.make_access(type)};
        const access rhs{gen.make_access(type)};

        const bool truth{can_be_same_bytes(lhs, rhs)};
        const bool answer{lhs.path.overlaps(rhs.path)};

        if (truth and not answer) {
            ++unsound;
            std::printf("unsound at iteration %zu\n", it);
        } else if (not truth and answer) {
            ++imprecise;
            std::printf("imprecise at iteration %zu\n", it);
        } else if (truth) {
            ++both_overlap;
        } else {
            ++both_disjoint;
        }

        gen.clear();
    }

    std::printf("iterations %zu: overlap %zu, disjoint %zu, unsound %zu, "
                "imprecise %zu\n",
                iterations, both_overlap, both_disjoint, unsound, imprecise);

    return (unsound + imprecise) == 0 ? 0 : 1;
}
