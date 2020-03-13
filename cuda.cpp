#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <ctime>

#define N 81

// #define CUDA_CALL(x) do { \
//     cudaError_t err = (x); \
//     if (err != cudaSuccess) { \
//         printf("Error %s\n", cudaGetErrorString(err)); \
//     } \
// } while(0)

using fn_type = uint64_t;

struct conj {
    fn_type value;
    std::string str_value;
};

std::vector<fn_type> base_vars;
std::vector<conj> all_conjs;

struct polinome{
    size_t target;
    bool conjs[N];
};

fn_type compute_polinome(const fn_type* const cuda_all_conjs, const polinome* p) {
    fn_type res = 0;
    for (size_t i = 0; i < N; ++i) {
        if (p->conjs[i]) {
            res ^= cuda_all_conjs[i];
        }
    }
    return res;
}

size_t length(const polinome* p) {
    size_t res = 0;
    for (size_t i = 0; i < N; ++i) {
        if (p->conjs[i]) ++res;
    }
    return res;
}

void print_binary(fn_type f, unsigned char n) {
    auto len = 1 << n;
    std::string res;
    while (f) {
        if (f & 1)
            res += "1";
        else
            res += "0";
        f >>= 1;
    }
    for (int i = res.size(); i < len; ++i) {
        res += "0";
    }
    printf("%s", res.c_str());
}

const std::string to_string(const polinome* p) {
    std::string res;
    for (size_t i = 0; i < N; ++i) {
        if (p->conjs[i]) {
            res += all_conjs[i].str_value + "^";
        }
    }
    res.pop_back();
    return res;
}

bool curLenImpl(int len, fn_type target, fn_type fid, polinome *result, fn_type* cuda_all_conjs) {
    int pos[N];
    polinome p;
    for (int i = 0; i < N; ++i) {
        p.conjs[i] = false;
    }

    for (int i = 0; i < len; ++i) {
		pos[i] = i;
        p.conjs[i] = true;
	}

    if (compute_polinome(cuda_all_conjs, &p) == target) {
        p.target = target;
        result[fid] = p;
        return true;
    }

	while (pos[0] < N-len) {
		if (pos[len-1] < N - 1) {
            p.conjs[pos[len-1]] = false;
			++pos[len-1];
			p.conjs[pos[len-1]] = true;
		} else {
			int cur = len - 2;
			while (cur >= 0) {
				if (pos[cur] < N - (len-cur)) {
					p.conjs[pos[cur]] = false;
					++pos[cur];
					p.conjs[pos[cur]] = true;
					for (int j = cur + 1; j < len; ++j) {
						pos[j] = pos[j - 1] + 1;
						p.conjs[pos[j]] = true;
					}
					for (int i = pos[len-1]+1; i < N; ++i) {
						p.conjs[i] = false;
					}
					break;
				}
				cur--;
			}
		}

        if (compute_polinome(cuda_all_conjs, &p) == target) {
            p.target = target;
            result[fid] = p;
            return true;
        }
	}
    return false;
}

void BFS(size_t fiber_idx, fn_type offset, polinome* result, fn_type* cuda_all_conjs) {
    // size_t fiber_idx = blockIdx.x*blockDim.x + threadIdx.x;
    fn_type target = fiber_idx + offset;
    if (target == 0) {
        return;
    }
    for (int i = 1; i <= N; ++i) {
        if (curLenImpl(i, target, fiber_idx, result, cuda_all_conjs)) {
            return;
        }
    }
}

struct ternary_var {
    enum State {
        ZERO,
        ONE,
        NEGATIVE
    };

    void increase() {
        switch (state) {
        case ZERO:
            state = ONE;
            break;
        case ONE:
            state = NEGATIVE;
            break;
        case NEGATIVE:
            state = ZERO;
            break;
        }
    }

    State state = ZERO;
};

fn_type one(const unsigned char n) {
    return (fn_type(1) << (fn_type(1) << n)) - 1;
}

struct elem_conj {
    elem_conj(size_t n) : vars(n) {};

    void increase() {
        for (size_t i = 0; i < vars.size(); ++i) {
            vars[i].increase();
            if (vars[i].state != ternary_var::ZERO) {
                break;
            }
        }
    }

    std::string to_string() {
        std::string res;
        for (size_t i = 0; i < vars.size(); ++i) {
            switch (vars[i].state) {
            case ternary_var::ZERO:
                continue;
            case ternary_var::ONE:
                res += "x" + std::to_string(i + 1);
                break;
            case ternary_var::NEGATIVE:
                res += "!x" + std::to_string(i + 1);
                break;
            }
            res += "*";
        }
        res.pop_back();
        return res;
    }

    fn_type compute() {
        fn_type res = one(vars.size());
        for (size_t i = 0; i < vars.size(); ++i) {
            const auto& var = vars[i];

            switch (var.state) {
            case ternary_var::ZERO:
                continue;
            case ternary_var::ONE:
                res &= base_vars[i];
                break;
            case ternary_var::NEGATIVE:
                res &= ~base_vars[i];
                break;
            }
        }
        return res;
    }

    std::vector<ternary_var> vars;
};

std::vector<conj> init_elem_conjs(const unsigned char n) {
    base_vars = std::vector<fn_type>(n);
    fn_type twos = 1;

    for (char i = n - 1; i >= 0; --i) {
        fn_type period = ((1 << twos) - 1);
        period <<= twos;

        fn_type tmp = 0;
        for (size_t j = 0; j < (1 << n)/twos/2; ++j) {
            tmp <<= twos*2;
            tmp += period;
        }

        twos <<= 1;
        base_vars[i] = tmp;
    }

    unsigned char num_conjs = 1;
    for (unsigned char i = 0; i < n; ++i) {
        num_conjs *= 3;
    }

    std::vector<conj> el_conjs(num_conjs);
    elem_conj conj(n);
    el_conjs[0].value = one(n);
    el_conjs[0].str_value = "1";
    for (size_t i = 1; i < num_conjs; ++i) {
        conj.increase();
        el_conjs[i].str_value = conj.to_string();
        el_conjs[i].value = conj.compute();
    }

    // printf("nums %d\n", num_conjs);
    // for (size_t i = 0; i < num_conjs; ++i) {
    //     printf("%s\n",el_conjs[i].str_value.c_str());
    // }
    // printf("=====\n");
    return el_conjs;
}

int main() {
    unsigned char n = 0;
    printf("Enter n: ");
    if (scanf("%hhu", &n) != 1) {
        printf("Error reading input\n");
        return 1;
    }

    all_conjs = init_elem_conjs(n);
    fn_type cuda_all_conjs[N];
    for (size_t i = 0; i < all_conjs.size(); ++i) {
        cuda_all_conjs[i] = all_conjs[i].value;
    }

    const size_t batch_size = 2000;
    fn_type num_functions = fn_type(1) << (fn_type(1) << n);

    fn_type *cac = (fn_type*)malloc(N*sizeof(fn_type));
	memcpy(cac, cuda_all_conjs, N*sizeof(fn_type));

    clock_t begin = clock();
    size_t max = 0;
    std::vector<polinome> results;

    polinome *device_res;
    polinome result[batch_size];

    printf("Start cuda calculations\n");

    device_res = (polinome*) malloc(batch_size*sizeof(polinome));
    BFS(1692, 0, device_res, cac);
    memcpy(result, device_res, batch_size*sizeof(polinome));
    free(device_res);
    printf("%s\n",to_string(&result[1692]).c_str());

    // for (fn_type i = 0; i < num_functions; i += batch_size) {
	// 	device_res = (polinome*) malloc(batch_size*sizeof(polinome));
	// 	for (int j = 0; j < batch_size; ++j) {
    //     	BFS(j, i, device_res, cac);
	// 	}
	// 	memcpy(result, device_res, batch_size*sizeof(polinome));
    //     free(device_res);

    //     for (size_t j = 0; j < batch_size; ++j) {
    //         const auto p = result[j];
    //         const auto l = length(&p);
    //         if (l > max) {
    //             results.clear();
    //             max = l;
    //         }
    //         if (l == max) {
    //             results.push_back(p);
    //         }
    //     }
    //     printf("Done %f %%\n", float(i)/num_functions*100);
    // }

    free(cac);
    printf("Stoped cuda calculations\n");

    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    printf("Elapsed time: %f\n", elapsed_secs);
    printf("Max polinome size equal: %zu\n", max);
    for (const auto& p : results) {
        print_binary(p.target, n);
        printf(" %s\n", to_string(&p).c_str());
    }
	printf("Total: %zu\n", results.size());

    return 0;
}