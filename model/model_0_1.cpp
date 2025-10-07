#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <utility>
#include <cmath>
#include <iomanip>

std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

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
    if ((maxScore >= 11) && (maxScore - minScore >= 2)) {
        return maxPlayer;
    } else {
        return 0;
    }
}

PDD winningRate(double pot1, double pot2, int scr1, int scr2) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    std::uniform_real_distribution<double> distribution(0.0, pot1 + pot2);
    for (int i = 1; i <= batch_size; i++) {
        int cur1 = scr1, cur2 = scr2;
        while (isGameOver(cur1, cur2) == false) {
            double dice = distribution(gen);
            if (dice <= pot1) {
                cur1++;
            } else {
                cur2++;
            }
        }
        if (isGameOver(cur1, cur2) == 1) {
            win1++;
        } else {
            win2++;
        }
    }
    return std::make_pair(1.0 * win1 / batch_size, 1.0 * win2 / batch_size);
}

const char PLAYER_A = 'H';
const char PLAYER_B = 'F';
const double alpha = 0.33;
const double pot1 = 0.45;
const double pot2 = 0.55;

const std::string get_full_score_seq() {
    std::vector<std::string> score_parts = {
        // "HFHHHHHHHHHFH"
        // "HHFFHFFFHHFHHHFFHFHH"
        // "FFFFFFHHFHFFFHF"
        // "HFHFFHHHFFHHFFFFFF"
        // "HHHHFFHHHFHHFHH"
        // "FHFFHFFFHHFHHHFFFF"
        "FFHHHHFFFFHHHFFFFF"
        // "FFFFFFFFFFF"
    };
    std::string full_seq;
    for (const auto& part : score_parts) {
        full_seq += part;
    }
    return full_seq;
}

int main() {
    std::string score_seq = get_full_score_seq();
    int scrA = 0, scrB = 0;
    std::vector<double> G_A, G_B;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tScore(A:B)\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\n";
    std::cout << "-------------------------------------------------------------------------------------------------------\n";

    double M_A = pot1, M_B = pot2;

    for (int i = 0; i < score_seq.size(); i++) {
        char current_winner = score_seq[i];
        double rtwp_win, rtwp_lose, L_i;

        rtwp_win = winningRate(M_A, M_B, scrA + 1, scrB).first;
        rtwp_lose = winningRate(M_A, M_B, scrA, scrB + 1).first;
        L_i = rtwp_win - rtwp_lose;
        std::cerr << std::fixed << std::setprecision(6) << rtwp_win << " " << rtwp_lose << '\n';

        double ga, gb;

        if (current_winner == PLAYER_A) {
            scrA++, ga = L_i, gb = 0;
        } else {
            scrB++, ga = 0, gb = -L_i;
        }
        G_A.push_back(ga);
        G_B.push_back(gb);

        double numerator_A = 0.0, numerator_B = 0.0, denominator = 0.0;
        int current_idx = i;
        for (int k = 0; k <= current_idx; k++) {
            double weight = pow(1 - alpha, current_idx - k);
            numerator_A += G_A[k] * weight;
            numerator_B += G_B[k] * weight;
            denominator += weight;
        }
        double M_A = numerator_A / denominator;
        double M_B = numerator_B / denominator;

        std::cout << i + 1 << "\t\t" << scrA << ":" << scrB << "\t\t"
                  << L_i << "\t" << ga << "\t" << gb << "\t"
                  << M_A << "\t" << M_B << "\n";
    }

    return 0;
}