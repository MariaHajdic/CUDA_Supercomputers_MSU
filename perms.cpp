#include <stdio.h>
#include <vector>

void print_arr(std::vector<int>& arr) {
	for (int i = 0; i < arr.size(); ++i) {
		printf("%d", arr[i]);
	}
	printf("\n");
}

int main() {
	const int n = 27;
	const int k = 3;
	std::vector<int> arr(n, 0);
	std::vector<int> pos(k);
	for (int i = 0; i < k; ++i) {
		arr[i] = 1;
		pos[i] = i;
	}

	int cur_pos = k - 1;
	print_arr(arr);
	int counter = 1;
	while (pos[0] < n-k) {
		if (pos[k-1] < n - 1) {
			arr[pos[k-1]] = 0;
			pos[k-1]++;
			arr[pos[k-1]] = 1;
		} else {
			int cur = k - 2;
			while (cur >= 0) {
				if (pos[cur] < n - (k-cur)) {
					arr[pos[cur]] = 0;
					pos[cur]++;
					arr[pos[cur]] = 1;
					for (int j = cur + 1; j < k; ++j) {
						pos[j] = pos[j - 1] + 1;
						arr[pos[j]] = 1;
					}
					for (int i = pos[k-1]+1; i < n; ++i) {
						arr[i] = 0;
					}
					break;
				}
				cur--;
			}
		}
		print_arr(arr);
		counter++;
	}

	printf("%d\n", counter);
	return 0;
}