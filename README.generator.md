# Procedurally generate Matrix Market

The `fast_matrix_market` write mechanism can write procedurally generated data.

To make this process simpler, the `generator.hpp` header includes a method that can generate a coordinate Matrix Market file
where each `row`, `column`, `value` triplet is individually generated using a *Callable*.

# Usage

```c++
#include <fast_matrix_market/app/generator.hpp>
```

Create a *Callable* with the signature:
```c++
void generate_tuple(int64_t coo_index, IT &row, IT &col, VT &value);
```
where:
* `coo_index` is an input parameter with the index of the tuple to be generated.
* `row`, `col`, `value` are the output parameters defining the generated tuple.
* `IT` is the integral type of the row and column indices, eg. `int64_t` or `int`.
* `VT` is the value type, eg. `double` or `float`.


Then call `fast_matrix_market::write_matrix_market_generated_triplet<IT, VT>` which takes the output stream,
the header, number of nonzeros `nnz`, and the callable.

The callable is called when a value of a tuple is needed, so eventually it will be called for every index in the half-open range [0, `nnz`).
The calls may be out of order and in parallel. The callable must be thread safe.

The Matrix Market `field` type is deduced from `VT`, or can be set to `pattern` in the header.


### Example: Generate an identity matrix

```c++
// #rows, #cols, and nnz
const int64_t eye_rank = 10;

fast_matrix_market::write_matrix_market_generated_triplet<int64_t, double>(
    output_stream, {eye_rank, eye_rank}, eye_rank,
    [](auto coo_index, auto& row, auto& col, auto& value) {
        row = coo_index;
        col = coo_index;
        value = 1;
    });
```

### Example: Generate a random matrix

Generate a 100-by-100 matrix with 1000 randomized elements.
```c++
void generate_random_tuple([[maybe_unused]] int64_t coo_index, int64_t &row, int64_t &col, double& value) {
    // The RNG is cheap to use but expensive to create and not thread safe.
    // Use thread_local to create one instance per thread.
    static thread_local std::mt19937 generator{std::random_device{}()};
    // distribution objects are effectively optimized away
    std::uniform_int_distribution<int64_t> index_distribution(0, 99);
    std::uniform_real_distribution<double> value_distribution(0, 1);

    row = index_distribution(generator);
    col = index_distribution(generator);
    value = value_distribution(generator);
}

fast_matrix_market::write_matrix_market_generated_triplet<int64_t, double>(
    output_stream, {100, 100}, 1000, generate_random_tuple);
```
