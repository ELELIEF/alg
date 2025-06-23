#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>

#define MAX_N 320000
#define MAX_W 1000000

typedef struct {
    int weight;
    double value;
    int index;
} Item;

typedef struct {
    int *selected;
    int count;
    double total_value;
    int total_weight;
} Result;

// 读取n=1000的物品数据
void load_items_from_csv(Item *items, int n, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("无法打开文件: %s\n", filename);
        exit(1);
    }
    char buf[128];
    fgets(buf, sizeof(buf), fp); // 跳过表头
    for (int i = 0; i < n; i++) {
        int idx, w;
        double v;
        fscanf(fp, "%d,%d,%lf", &idx, &w, &v);
        items[i].index = idx;
        items[i].weight = w;
        items[i].value = v;
        fgets(buf, sizeof(buf), fp); // 跳到下一行
    }
    fclose(fp);
}

// 随机生成物品
void generate_items(Item *items, int n) {
    for (int i = 0; i < n; i++) {
        items[i].weight = rand() % 100 + 1;
        items[i].value = (rand() % 901 + 100) + (rand() % 100) / 100.0;
        items[i].index = i + 1;
    }
}

// 蛮力法
void brute_force(Item *items, int n, int W, Result *res) {
    int total = 1 << n;
    double max_value = 0;
    int *best = (int *)malloc(n * sizeof(int));
    int *temp = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < total; i++) {
        int w = 0, cnt = 0;
        double v = 0;
        for (int j = 0; j < n; j++) {
            if (i & (1 << j)) {
                w += items[j].weight;
                v += items[j].value;
                temp[cnt++] = j;
            }
        }
        if (w <= W && v > max_value) {
            max_value = v;
            res->total_weight = w;
            res->count = cnt;
            for (int k = 0; k < cnt; k++) best[k] = temp[k];
        }
    }
    res->total_value = max_value;
    res->selected = best;
    free(temp);
}

// 动态规划法
void dp(Item *items, int n, int W, Result *res) {
    double *dp = (double *)calloc(W + 1, sizeof(double));
    if (!dp) {
        printf("内存分配失败，W=%d\n", W);
        exit(1);
    }
    int *pre = (int *)calloc(W + 1, sizeof(int));
    if (!pre) {
        printf("内存分配失败，W=%d\n", W);
        free(dp);
        exit(1);
    }
    for (int i = 0; i < n; i++) {
        for (int w = W; w >= items[i].weight; w--) {
            if (dp[w] < dp[w - items[i].weight] + items[i].value) {
                dp[w] = dp[w - items[i].weight] + items[i].value;
                pre[w] = i + 1;
            }
        }
    }
    int w = W, cnt = 0, *sel = (int *)malloc(n * sizeof(int));
    double total_value = dp[W];
    int total_weight = 0;
    while (w > 0 && pre[w] && cnt < n) {
        int idx = pre[w] - 1;
        if (items[idx].weight <= 0) break; // 防止死循环
        sel[cnt++] = idx;
        w -= items[idx].weight;
        total_weight += items[idx].weight;
    }
    if (cnt == n && w > 0 && pre[w]) {
        printf("还原选择时越界，cnt=%d, n=%d\n", cnt, n);
    }
    res->selected = sel;
    res->count = cnt;
    res->total_value = total_value;
    res->total_weight = total_weight;
    free(dp);
    free(pre);
}

// 贪心法
int cmp(const void *a, const void *b) {
    Item *ia = (Item *)a, *ib = (Item *)b;
    double ra = ia->value / ia->weight, rb = ib->value / ib->weight;
    return (rb > ra) - (rb < ra);
}
void greedy(Item *items, int n, int W, Result *res) {
    Item *copy = (Item *)malloc(n * sizeof(Item));
    for (int i = 0; i < n; i++) copy[i] = items[i];
    qsort(copy, n, sizeof(Item), cmp);
    int w = 0, cnt = 0, *sel = (int *)malloc(n * sizeof(int));
    double v = 0;
    for (int i = 0; i < n; i++) {
        if (w + copy[i].weight <= W) {
            w += copy[i].weight;
            v += copy[i].value;
            sel[cnt++] = copy[i].index - 1;
        }
    }
    res->selected = sel;
    res->count = cnt;
    res->total_value = v;
    res->total_weight = w;
    free(copy);
}

// 回溯法
void backtrack(Item *items, int n, int W, int idx, int cur_w, double cur_v, int *cur_sel, int cur_cnt, Result *res) {
    if (cur_w > W) return;
    if (cur_v > res->total_value) {
        res->total_value = cur_v;
        res->total_weight = cur_w;
        res->count = cur_cnt;
        for (int i = 0; i < cur_cnt; i++) res->selected[i] = cur_sel[i];
    }
    if (idx == n) return;
    cur_sel[cur_cnt] = idx;
    backtrack(items, n, W, idx + 1, cur_w + items[idx].weight, cur_v + items[idx].value, cur_sel, cur_cnt + 1, res);
    backtrack(items, n, W, idx + 1, cur_w, cur_v, cur_sel, cur_cnt, res);
}

// 输出结果
void print_result(const char *name, Result *res, Item *items) {
    printf("【%s】\n", name);
    printf("总价值: %.2lf, 总重量: %d\n", res->total_value, res->total_weight);
    printf("选择物品数量: %d\n", res->count);
    printf("\n");
}

int main() {
    // 你可以修改下面的n和W进行多组实验
    int n_list[] = {15, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 20000, 40000, 80000, 160000, 320000};
    int w_list[] = {10000, 100000, 1000000};
    int n_case = sizeof(n_list) / sizeof(n_list[0]);
    int w_case = sizeof(w_list) / sizeof(w_list[0]);

    for (int wi = 0; wi < w_case; wi++) {
        int W = w_list[wi];
        printf("==== 背包容量: %d ====\n", W);
        for (int ni = 0; ni < n_case; ni++) {
            int n = n_list[ni];
            if (n > MAX_N) continue;
            printf("---- 物品数量: %d ----\n", n);

            Item *items = (Item *)malloc(n * sizeof(Item));
            if (n == 1000) {
                load_items_from_csv(items, n, "items_1000.csv");
            } else {
                generate_items(items, n);
            }

            Result res;
            LARGE_INTEGER freq, start, end;
            QueryPerformanceFrequency(&freq);

            // 小规模：四种算法
            if (n <= 20) {
                // 蛮力法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                QueryPerformanceCounter(&start);
                brute_force(items, n, W, &res);
                QueryPerformanceCounter(&end);
                print_result("蛮力法", &res, items);
                printf("蛮力法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);

                // 动态规划法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                QueryPerformanceCounter(&start);
                dp(items, n, W, &res);
                QueryPerformanceCounter(&end);
                print_result("动态规划法", &res, items);
                printf("动态规划法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);

                // 贪心法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                QueryPerformanceCounter(&start);
                greedy(items, n, W, &res);
                QueryPerformanceCounter(&end);
                print_result("贪心法", &res, items);
                printf("贪心法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);

                // 回溯法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                int *cur_sel = (int *)malloc(n * sizeof(int));
                QueryPerformanceCounter(&start);
                backtrack(items, n, W, 0, 0, 0, cur_sel, 0, &res);
                QueryPerformanceCounter(&end);
                print_result("回溯法", &res, items);
                printf("回溯法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);
                free(cur_sel);
            } else {
                // 只运行DP和贪心法
                // 动态规划法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                QueryPerformanceCounter(&start);
                dp(items, n, W, &res);
                QueryPerformanceCounter(&end);
                print_result("动态规划法", &res, items);
                printf("动态规划法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);

                // 贪心法
                res.selected = (int *)malloc(n * sizeof(int));
                res.total_value = 0;
                QueryPerformanceCounter(&start);
                greedy(items, n, W, &res);
                QueryPerformanceCounter(&end);
                print_result("贪心法", &res, items);
                printf("贪心法用时：%.3lf ms\n", (double)(end.QuadPart - start.QuadPart) * 1000 / freq.QuadPart);
                free(res.selected);
            }
            free(items);
            printf("\n");
        }
    }
    return 0;
}