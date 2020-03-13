#include <stdio.h>
#include <vector>
#include <queue>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <ctime>

#define N 9 // Max num of terms for polynomial combinations.

/* Type alias for representing functions with 64-bit integers. */
using fn_type = uint64_t;

/* Struct storing a val and its string representation. */
struct conj {
    fn_type value;
    std::string str_value;
};

std::vector<fn_type> base_vars;
std::vector<conj> all_conjs;

struct polinome{
    bool conjs[N]; // Indicating active terms in the polynomial.
    size_t target; // Target function value for this polynomial.
    size_t last_conj; // Idx of the last conjunction term in use.
};

/* Computing a polynomial value - XORing active conjunction terms. */
fn_type compute_polinome(fn_type* cuda_all_conjs, polinome* p) {
    fn_type res = 0;
    for (size_t i = 0; i < N; ++i) {
        if (p->conjs[i]) {
            res ^= cuda_all_conjs[i];
        }
    }
    return res;
}

/* Moving to the next polynomial configuration. */
bool next(polinome* p) {
    if (p->last_conj + 1 == N) {
        return false; // Reached the last possible configuration.
    }
    p->conjs[++p->last_conj] = true;
    return true;
}

/* Reseting the last term in the polynomial. */
void reset(polinome* p) {
    p->conjs[p->last_conj] = false;
}

/* Counting the number of active terms in the polynomial. */
size_t length(const polinome* p) {
    size_t res = 0;
    for (size_t i = 0; i < N; ++i) {
        if (p->conjs[i]) ++res;
    }
    return res;
}

/* Printing the binary representation of a function with 'n' bits. */
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

/* Converting polynomial structure to a readable string format. */
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

/* Recursively finding polynomials matching the target value. */
int curLenImpl(int len, fn_type target, polinome *result, fn_type* cuda_all_conjs) {
    int pos[N];
    polinome p;
    for (int i = 0; i < N; ++i) {
        p.conjs[i] = false;
    }
    p.last_conj = 0;

    for (int i = 0; i < len; ++i) {
		pos[i] = i;
        p.conjs[i] = true;
	}

	int cur_pos = len - 1;

    if (compute_polinome(cuda_all_conjs, &p) == target) {
        p.target = target;
        result[target] = p;
        return true;
    }

	while (pos[0] < N-len) {
		if (pos[len-1] < N - 1) {
            p.conjs[pos[len-1]] = false;
			pos[len-1]++;
			p.conjs[pos[len-1]] = true;
		} else {
			int cur = len - 2;
			while (cur >= 0) {
				if (pos[cur] < N - (len-cur)) {
					p.conjs[pos[cur]] = false;
					pos[cur]++;
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
            result[target] = p;
            return true;
        }
	}
    return false;
}

int curLenImpl(int len, fn_type target, polinome p, polinome* result, fn_type* cuda_all_conjs) {
    if (len == 1) {
        for (int i = 0; i < (N-p.last_conj); ++i) {
            polinome cur_p = p;
            cur_p.conjs[cur_p.last_conj + i] = true;
            cur_p.last_conj = p.last_conj + i;
            if (compute_polinome(cuda_all_conjs, &cur_p) == target) {
                cur_p.target = target;
                result[target] = cur_p;
                return true;
            }
        }
        return false;
    }

    for (int i = 0; i < (N-p.last_conj); ++i) {
        polinome cur_p = p;
        cur_p.conjs[cur_p.last_conj + i] = true;
        cur_p.last_conj = p.last_conj + i;
        if (curLenImpl(len - 1, target, cur_p, result, cuda_all_conjs)) {
            return true;
        }
    }
    return false;
}

/* Finding polynomials of different lengths that match the target value. */
void BFS(fn_type target, polinome p, polinome* result, fn_type* cuda_all_conjs) {
    for (int i = 1; i < N; ++i) {
        if (curLenImpl(i, target, p, result, cuda_all_conjs)) {
            return;
        }
    }
}

//void BFS(const fn_type offset, polinome* result, fn_type* cuda_all_conjs) {
    // fn_type target = offset;

    // const size_t polinome_size = N * 2048;
    // polinome *queue = (polinome*) malloc(N * polinome_size * sizeof(polinome));
    // size_t queue_idx = 0;

    // for (size_t i = 0; i < N; ++i) {
    //     polinome p;
    //     p.last_conj = i;
    //     for (int j = 0; j < N; ++j) {
    //         p.conjs[j] = false;
    //     }
    //     p.conjs[i] = true;

    //     queue[queue_idx++] = p;
    // }

    // size_t i = 0;
    // while (true) {
    //     //printf("queue_idx_size begin_idx %zu %zu\n", queue_idx, i);
    //     auto cur_p = queue[i++];
    //     //printf("cur polinome %zu\n", cur_p.last_conj);

    //     if (compute_polinome(cuda_all_conjs, &cur_p) == target) {
    //         result[target] = cur_p;
    //         printf("Finished %llu\n", target);
    //         return;
    //     }

    //     auto pol_it = cur_p;
    //     while (next(&pol_it)) {
    //         queue[queue_idx++] = pol_it;
    //         if (queue_idx >= polinome_size) {
    //             queue_idx = 0;
    //         }
    //         reset(&pol_it);
    //     }

    //     if (i > queue_idx) {
    //         i = 0;
    //     }
    // }
//}

/* Representation of a variable with 3 states. */
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

/* Helper method to generate all-ones bit patterns for a given size. */
fn_type one(const unsigned char n) {
    return (fn_type(1) << (fn_type(1) << n)) - 1;
}

/* Representing elementary conjunctions and handling state transitions. */
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

/* Initialising elementary conjunctions based on input of size n. */
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

    fn_type *cac;
    cac = (fn_type*)malloc(N*sizeof(fn_type));
    memcpy(cac, cuda_all_conjs, N*sizeof(fn_type));

    clock_t begin = clock();
    polinome *res;

    const fn_type num_functions = fn_type(1) << (fn_type(1) << n);

    res = (polinome*)malloc(num_functions*sizeof(polinome));
    // printf("Start cuda calculations\n");
    for (int i = 1; i < num_functions; ++i) {
        polinome p;
        p.last_conj = 0;
        for (int i = 0; i < N; ++i) {
            p.conjs[i] = false;
        }
        BFS(i, p, res, cac);
    }

    polinome result[num_functions];
    memcpy(result, res, num_functions*sizeof(polinome));
    for (const auto& p : result) {
        printf(" %s\n", to_string(&p).c_str());
    }

    printf("Stoped cuda calculations\n");
    //polinome result[num_functions];
    //memcpy(result, res, num_functions*sizeof(polinome));
    free(res);
    free(cac);

    std::vector<polinome> results;
    size_t max = 0;
    for (const auto& p: result) {
        const auto l = length(&p);
        if (l > max) {
            results.clear();
            max = l;
        }
        if (l == max) {
            results.push_back(p);
        }
    }


    printf("Max polinome size equal: %zu\n", max);
    for (const auto& p : results) {
        print_binary(p.target, n);
        printf(" %s\n", to_string(&p).c_str());
    }
    printf("Total: %lu\n", results.size());


    // 
    // std::vector<polinome> results;
    // size_t max = 0;
    // 
    // for (fn_type i = 1; i < num_functions; i += 256) {
    //     thrust::host_vector<polinome> res;
    //     BFS(i, &res);


    //     printf("Done %f %%\n", float(i)/num_functions*100);
    // }

    // clock_t end = clock();
    // double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    // printf("Elapsed time: %f\n", elapsed_secs);


    return 0;
}