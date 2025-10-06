#include <iostream>
#include <chrono>
#include <random>
std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
int cap_a, cap_b;
int scr_a, scr_b;
int win_a, win_b;
int isGameOver(int score1, int score2) {
    int maxScore, maxPlayer, minScore, minPlayer;
    if (score1 > score2) {
        maxScore = score1;
        maxPlayer = 1;
        minScore = score2;
        minPlayer = 2;
    } else {
        maxScore = score2;
        maxPlayer = 2;
        minScore = score1;
        minPlayer = 1;
    }
    if ((maxScore >= 11) and (maxScore - minScore >= 2)) {
        return maxPlayer;
    } else {
        return 0;
    }
}
int main() {
    printf("(cap_a, cap_b) = ");
    scanf("%d %d", &cap_a, &cap_b);
    printf("(scr_a, scr_b) = ");
    scanf("%d %d", &scr_a, &scr_b);
    int batch_size = 100000;
    for (int i = 1; i <= batch_size; i++) {
        int cur_a = scr_a, cur_b = scr_b;
        while (isGameOver(cur_a, cur_b) == false) {
            int dice = gen() % (cap_a + cap_b) + 1;
            if (dice <= cap_a) {
                cur_a++;
            } else {
                cur_b++;
            }
        }
       if (isGameOver(cur_a, cur_b) == 1) {
            win_a++;
       } else {
            win_b++;
       }
    }
    printf("winning rate for player a: %.6lf\n", 1. * win_a / batch_size);
    printf("winning rate for player b: %.6lf\n", 1. * win_b / batch_size);
    return 0;
}