#include <bits/stdc++.h>
using namespace std;

#define FOOD_NUMBER 20   // Số nguồn thức ăn (tức số ong thợ)
#define D 4              // Số chiều nghiệm (có thể thay đổi)
#define LOWER -10        // Giới hạn dưới miền tìm kiếm
#define UPPER 10         // Giới hạn trên miền tìm kiếm
#define LIMIT 30         // Ngưỡng trial để chuyển thành ong do thám
#define MAXCYCLE 2000    // Số vòng lặp tối đa

const double BETA = 1.0;   // Tham số lượng tử β (điều chỉnh mức nhảy lượng tử)

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

// Sinh số ngẫu nhiên đồng nhất u trong (0,1] an toàn (tránh 0)
inline double rand01_nonzero() {
    return ( (double)rand() + 1.0 ) / ( (double)RAND_MAX + 1.0 );
}

// Cập nhật lượng tử cho một nghiệm: tạo nghiệm mới v từ x_i và gbest
// Công thức (mẫu): v_j = best_j + sign * beta * |x_j - best_j| * ln(1/u)
FoodSource quantumUpdate(const FoodSource &xi, const FoodSource &gbest) {
    FoodSource v = xi; // sao chép
    for (int j = 0; j < D; ++j) {
        double u = rand01_nonzero();           // u in (0,1]
        double logTerm = -log(u);              // ln(1/u) = -ln(u), >0
        int sign = (rand() % 2 == 0) ? 1 : -1; // ngẫu nhiên +/-
        double diff = fabs(xi.x[j] - gbest.x[j]);
        double delta = sign * BETA * diff * logTerm;
        v.x[j] = gbest.x[j] + delta;

        // Giữ trong miền tìm kiếm
        if (v.x[j] < LOWER) v.x[j] = LOWER;
        if (v.x[j] > UPPER) v.x[j] = UPPER;
    }
    return v;
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

// PHA ONG THỢ (Employed Bee Phase) — dùng cập nhật lượng tử
void employedBeePhase() {
    for (int i = 0; i < FOOD_NUMBER; i++) {

        // Tạo nghiệm lượng tử mới v từ xi hướng về best
        FoodSource newFood = quantumUpdate(foods[i], bestSolution);

        // Đánh giá nghiệm mới
        newFood.f = objectiveFunction(newFood.x);
        newFood.fitness = calculateFitness(newFood.f);

        // Greedy selection
        if (newFood.fitness > foods[i].fitness) {
            foods[i] = newFood;
            foods[i].trial = 0;
        } else {
            foods[i].trial++;
        }
    }
}

// PHA ONG QUAN SÁT (Onlooker Bee Phase) — chọn theo roulette, dùng Q-update
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

        // Tạo nghiệm lượng tử mới từ nguồn i
        FoodSource newFood = quantumUpdate(foods[i], bestSolution);

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
// Nếu nguồn quá lâu không cải thiện → thay mới (giữ nguyên như ABC)
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
