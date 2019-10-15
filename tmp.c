#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SUCCESS 0
#define ERROR 1
#define ERROR_NULLPTR 2

/*#define MAT 0       */
/*#define COL_VECTOR 1*/
/*#define ROW_VECTOR 2*/

typedef struct Mat {
    float* ptr;
    int height;
    int width;
    /*int mat_type;*/
} Mat;

Mat* transpose(Mat* src, Mat* dst) {
    if (!src || !dst) {
        return NULL;
    }
    if (!src->ptr || !dst->ptr) {
        return NULL;
    }
    if (src->height != dst->width || src->width != dst->height) {
        return NULL;
    }
    for (int i = 0; i < src->height; i++) {
        for (int j = 0; j < src->width; j++) {
            dst->ptr[j * dst->width + i] = src->ptr[i * src->width + j];
        }
    }
    return dst;
}

Mat* mat_init(Mat* mat_ptr, int height, int width) {
    mat_ptr->ptr = malloc(sizeof(float) * height * width);
    mat_ptr->height = height;
    mat_ptr->width = width;
    /*mat_ptr->mat_type = type;*/
    return mat_ptr;
}

void mat_deinit(Mat* mat_ptr) {
    free(mat_ptr->ptr);
}

void mat_pri(Mat* src) {
    if (!src) {
        fprintf(stderr, "nullptr in mat_pri()\n");
    }
    for (int i = 0; i < src->height; i++) {
        for (int j = 0; j < src->width; j++) {
            printf("%.6f ", src->ptr[i * src->width + j]);
        }
        puts("");
    }
    puts("");
}

Mat* mat_mul(Mat* A, Mat* B, Mat* C) {
    if (!A || !B || !C || !A->ptr || !B->ptr || !C->ptr) {
        return NULL;
    }
    if (A->width != B->height || C->height != A->height ||
        C->width != B->width) {
        return NULL;
    }
    for (int i = 0; i < C->height; i++) {
        for (int j = 0; j < C->width; j++) {
            float sum = 0;
            for (int k = 0; k < B->height; k++) {
                sum += A->ptr[i * A->width + k] * B->ptr[k * B->width + j];
            }
            C->ptr[i * C->width + j] = sum;
        }
    }
    return C;
}

float square(float x) {
    return x * x;
}

float row_sum(Mat* src, int row_index, int start, int end,
              float (*func)(float)) {
    float sum = 0;
    for (int i = start; i < end; i++) {
        if (func != NULL) {
            sum += func(src->ptr[row_index * src->height + i]);
        } else {
            sum += src->ptr[row_index * src->height + i];
        }
    }
    printf("%f\n", sum);
    return sum;
}

// src should be A^T A
Mat* cholesky_decomposition(Mat* src, Mat* dst) {
    if (!src || !dst || !src->ptr || !dst->ptr) {
        return NULL;
    }
    if (src->height != src->width || src->height != dst->height ||
        src->width != dst->width) {
        return NULL;
    }
    // i col, j row
    for (int i = 0; i < src->height; i++) {
        for (int j = 0; j < src->height; j++) {
            if (j < i) {
                dst->ptr[j * src->height + i] = 0;
                continue;
            }
            if (i == j) {
                dst->ptr[j * src->height + i] =
                        sqrt(src->ptr[j * src->height + i] -
                             row_sum(dst, i, 0, i, square));
            } else {
                float sum = 0;
                for (int k = 0; k < i; k++) {
                    sum += dst->ptr[i * src->height + k] *
                           dst->ptr[j * src->height + k];
                }
                printf("dd %f %f\n", src->ptr[j * src->height + i] - sum,
                       dst->ptr[i * src->height + i]);
                dst->ptr[j * src->height + i] =
                        (src->ptr[j * src->height + i] - sum) /
                        dst->ptr[i * src->height + i];
            }
        }
    }
    return dst;
}

// X is result
Mat* linear_equation_upper(Mat* A, Mat* X, Mat* B) {
    if (!A || !X || !B || !A->ptr || !X->ptr || !B->ptr) {
        return NULL;
    }
    for(int i = A->height - 1; i >= 0; i--){
        float sum = 0;
        for(int j = A->height - 1; j > i; j--){
            sum += A->ptr[i * A->height + j] * X->ptr[j];
        }
        X->ptr[i] = (B->ptr[i] - sum) / A->ptr[i * A->height + i];
    }
    return X;
}

Mat* linear_equation_lower(Mat* A, Mat* X, Mat* B){
    if (!A || !X || !B || !A->ptr || !X->ptr || !B->ptr) {
        return NULL;
    }
    for(int i = 0; i < A->height; i++){
        float sum = 0;
        for(int j = 0; j < i; j++){
            sum += A->ptr[i * A->height + j] * X->ptr[j];
        }
        X->ptr[i] = (B->ptr[i] - sum) / A->ptr[i * A->height + i];
    }
    return X;
}

int main() {
    srand(0);
    Mat a;
    mat_init(&a, 3, 3);
    Mat b;
    mat_init(&b, 3, 3);
    Mat c;
    mat_init(&c, 3, 3);
    Mat decom;
    mat_init(&decom, 3, 3);
    Mat B;
    mat_init(&B, 3, 1);
    Mat X;
    mat_init(&X, 3, 1);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i > j) {
                a.ptr[i * 3 + j] = 0;
            } else {
                a.ptr[i * 3 + j] = random() % 100 + 1;
            }
        }
        B.ptr[i] = random() % 100 + 1;
    }
    mat_pri(&a);
    mat_pri(transpose(&a, &b));
    mat_pri(mat_mul(&a, &b, &c));
    mat_pri(cholesky_decomposition(&c, &decom));
    mat_pri(mat_mul(&decom, transpose(&decom, &c), &b));
    mat_pri(&c);
    mat_pri(&B);
    mat_pri(linear_equation_upper(&c, &X, &B));
    mat_pri(mat_mul(&c, &X, &B));
    mat_pri(linear_equation_lower(&decom, &X, &B));
    mat_pri(&decom);
    mat_pri(&B);
    mat_pri(mat_mul(&decom, &X, &B));
    mat_deinit(&X);
    mat_deinit(&B);
    mat_deinit(&decom);
    mat_deinit(&c);
    mat_deinit(&b);
    mat_deinit(&a);
    return 0;
}
