#include <bits/stdc++.h>
using namespace std;

#define FOOD_NUMBER 20   // Số nguồn thức ăn (tức số ong thợ)
#define D 4              // Số chiều nghiệm (có thể thay đổi)
#define LOWER -10        // Giới hạn dưới miền tìm kiếm
#define UPPER 10         // Giới hạn trên miền tìm kiếm
#define LIMIT 30         // Ngưỡng trial để chuyển thành ong do thám
#define MAXCYCLE 2000    // Số vòng lặp tối đa
const double C = 1.5;     // Hằng số điều khiển mức độ "hút về gbest" (psi ∈ [0, C])

// Cấu trúc lưu thông tin 1 nguồn thức ăn
struct FoodSource {
    vector<double> x;   // Vị trí nghiệm (vector D chiều)
    double f;           // Giá trị hàm mục tiêu tại nghiệm
    double fitness;     // Giá trị fitness được chuyển đổi từ f
    int trial;          // Số lần cải thiện thất bại liên tiếp

    FoodSource() {
        x.assign(D, 0.0);
        f = 0;
        fitness = 0;
        trial = 0;
    }
};

FoodSource foods[FOOD_NUMBER];  // Danh sách nguồn thức ăn
FoodSource bestSolution;        // Lưu nghiệm tốt nhất tìm được

/// Hàm mục tiêu ví dụ (có thể thay đổi tùy bài toán)
// Ở đây dùng Sphere function f(x) = sum(x_i^2)
double objectiveFunction(const vector<double>& x) {
    double sum = 0;
    for (double v : x) sum += v * v;
    return sum;
}

// Chuyển giá trị f → fitness
// Fitness càng lớn càng tốt
double calculateFitness(double f) {
    if (f >= 0) return 1.0 / (1.0 + f);  // Giá trị nhỏ → fitness lớn
    return 1.0 + fabs(f);
}

// KHỞI TẠO NGẪU NHIÊN QUẦN THỂ
void initialize() {
    for (int i = 0; i < FOOD_NUMBER; i++) {

        // Random mỗi chiều nghiệm trong [LOWER, UPPER]
        for (int j = 0; j < D; j++) {
            foods[i].x[j] = LOWER + (double)rand() / RAND_MAX * (UPPER - LOWER);
        }

        // Tính giá trị f và fitness ban đầu
        foods[i].f = objectiveFunction(foods[i].x);
        foods[i].fitness = calculateFitness(foods[i].f);
        foods[i].trial = 0;
    }

    // Cập nhật nghiệm tốt nhất ban đầu (scan toàn bộ quần thể)
    int bestIdx = 0;
    for (int i = 1; i < FOOD_NUMBER; i++) {
        if (foods[i].fitness > foods[bestIdx].fitness) bestIdx = i;
    }
    bestSolution = foods[bestIdx];
}

// PHA ONG THỢ (Employed Bee Phase)
// Mỗi ong thợ cải thiện nguồn thức ăn của chính nó
void employedBeePhase() {
    for (int i = 0; i < FOOD_NUMBER; i++) {

        FoodSource newFood = foods[i]; // Sao chép nghiệm hiện tại

        int j = rand() % D;            // Chọn ngẫu nhiên 1 chiều để thay đổi

        // Chọn 1 nguồn khác k ≠ i
        int k;
        do { k = rand() % FOOD_NUMBER; } while (k == i);

        // Hệ số thay đổi ∈ [-1,1]
        double phi = ((double)rand() / RAND_MAX) * 2.0 - 1.0;

        // Hệ số hướng về gbest ∈ [0, C]
        double psi = ((double)rand() / RAND_MAX) * C;

        // Công thức cập nhật nghiệm theo GABC:
        // v = x_i + phi*(x_i - x_k) + psi*(gbest - x_i)
        newFood.x[j] = foods[i].x[j]
                        + phi * (foods[i].x[j] - foods[k].x[j])
                        + psi * (bestSolution.x[j] - foods[i].x[j]);

        // Giữ trong miền tìm kiếm
        newFood.x[j] = max((double)LOWER, min((double)UPPER, newFood.x[j]));

        // Đánh giá nghiệm mới
        newFood.f = objectiveFunction(newFood.x);
        newFood.fitness = calculateFitness(newFood.f);

        // Nếu nghiệm mới tốt hơn → chấp nhận
        if (newFood.fitness > foods[i].fitness) {
            foods[i] = newFood;
            foods[i].trial = 0;
        } else {
            foods[i].trial++; // tăng số lần thất bại
        }
    }
}

// PHA ONG QUAN SÁT (Onlooker Bee Phase)
// Chọn nguồn theo roulette wheel
void onlookerBeePhase() {
    double sumFit = 0;
    for (int i = 0; i < FOOD_NUMBER; i++) sumFit += foods[i].fitness;

    int t = 0;
    while (t < FOOD_NUMBER) {

        // Chọn nguồn theo xác suất fitness
        double r = ((double)rand() / RAND_MAX) * sumFit;
        double accum = 0;
        int i;

        for (i = 0; i < FOOD_NUMBER; i++) {
            accum += foods[i].fitness;
            if (accum >= r) break;
        }
        if (i == FOOD_NUMBER) i = FOOD_NUMBER - 1; // safety

        // Tương tự pha ong thợ nhưng có psi hướng về gbest
        FoodSource newFood = foods[i];
        int j = rand() % D;

        int k;
        do { k = rand() % FOOD_NUMBER; } while (k == i);

        double phi = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        double psi = ((double)rand() / RAND_MAX) * C;

        newFood.x[j] = foods[i].x[j]
                        + phi * (foods[i].x[j] - foods[k].x[j])
                        + psi * (bestSolution.x[j] - foods[i].x[j]);

        newFood.x[j] = max((double)LOWER, min((double)UPPER, newFood.x[j]));

        newFood.f = objectiveFunction(newFood.x);
        newFood.fitness = calculateFitness(newFood.f);

        if (newFood.fitness > foods[i].fitness) {
            foods[i] = newFood;
            foods[i].trial = 0;
        } else {
            foods[i].trial++;
        }

        t++;
    }
}

// PHA ONG DO THÁM (Scout Bee Phase)
// Nếu nguồn quá lâu không cải thiện → thay mới
void scoutBeePhase() {
    for (int i = 0; i < FOOD_NUMBER; i++) {
        if (foods[i].trial > LIMIT) {

            // Random lại hoàn toàn
            for (int j = 0; j < D; j++) {
                foods[i].x[j] = LOWER + (double)rand() / RAND_MAX * (UPPER - LOWER);
            }

            foods[i].f = objectiveFunction(foods[i].x);
            foods[i].fitness = calculateFitness(foods[i].f);
            foods[i].trial = 0;
        }
    }
}

// Lưu nghiệm tốt nhất
void memorizeBest() {
    for (int i = 0; i < FOOD_NUMBER; i++) {
        if (foods[i].fitness > bestSolution.fitness) {
            bestSolution = foods[i];
        }
    }
}

int main() {
    srand((unsigned)time(NULL));
    initialize(); // Khởi tạo quần thể ban đầu

    for (int cycle = 0; cycle < MAXCYCLE; cycle++) {
        employedBeePhase();
        onlookerBeePhase();
        scoutBeePhase();
        memorizeBest();
    }

    cout << "Best solution found:\n";
    cout << "x = ";
    for (double v : bestSolution.x) cout << v << " ";
    cout << "\nf(min) = " << bestSolution.f << endl;

    return 0;
}
