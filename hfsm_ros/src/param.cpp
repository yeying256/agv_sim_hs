#include "hfsm/param.h"

namespace hfsm_ns
{
    void printf_yellow(const char *s)
    {
        printf("\033[0m\033[1;33m%s\033[0m\n", s);
    }

    void printf_green(const char *s)
    {
        printf("\033[0m\033[1;32m%s\033[0m\n", s);
    }

    std::unordered_map<int8_t, workbench> retUmap()
    {
        pose p1{2.2, 0.080258, 0.030675, {0.1, 0.1, 0.2}};
        pose p2{1.702234, 0.064573, 0.023148, {0.05, 0.05, 0.02}};
        pose p3{1.459606, 0.08444, 0.009566, {0.05, 0.05, 0.02}};

        pose p4{1.92759958, 0.6294623, -0.00596, {0.1, 0.1, 0.2}};
        pose p5{1.92759958, 0.6294623, -0.00596, {0.005, 0.005, 0.02}};
        pose p6{1.9218, -0.332654, -0.08207648, {0.005, 0.005, 0.02}};
        pose p7{2.90308, 0.633863, -0.01924, {0.1, 0.1, 0.2}};

        pose p8{1.3025910252302508, 0.010416443354787438, 0.006375339095745183,  {0.1, 0.1, 0.2}};
        pose p9{1.3025910252302508, 0.010416443354787438, 0.006375339095745183, {0.005, 0.005, 0.02}};
        pose p10{1.3025910252302508+0.1, 0.010416443354787438, 0.006375339095745183,  {0.005, 0.005, 0.02}};

        struct workbench w1{{p1, p2, p3}, 3, 1};
        struct workbench w2{{p4, p5, p6, p7}, 4, 5};
        struct workbench w3{{p8, p9, p10}, 3, 1};
        std::unordered_map<int8_t, workbench> tempUmap{
            {0, w1},
            {1, w2},
            {2, w3}};
        return tempUmap;
    }
}