#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TARGET_SIZE 128
#define STRING_LEN_MAX 128
#define CHECK_FAULT 5
#define VALID 1
#define INVALID 0

#define SUCCESS 0
#define ERROR 1
#define ERROR_NULLPTR 2
#define float double

#define BP 0
#define TR 1

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Targets {
    int* ptr;
    int size;
    int len;
} Targets;

typedef struct Bp {
    int flag;
    float time;
    int Id;
    float x, y, z;
    float range;
    int target_id;
    float mark_time;
} Bp;

typedef struct Tp {
    int flag;
    float time;
    int Id;
    int target_Id;
    float range;
    int is_valid;
} Tp;

typedef union Message {
    Bp bp;
    Tp tp;
} Message;

/*#define MAT 0       */
/*#define COL_VECTOR 1*/
/*#define ROW_VECTOR 2*/

typedef struct Mat {
    float* ptr;
    int height;
    int width;
    /*int mat_type;*/
} Mat;

float square(float x) {
    return x * x;
}

float Bp_square_pos(Bp* ptr) {
    return square(ptr->x) + square(ptr->y) + square(ptr->z);
}

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
    /*printf("%f\n", sum);*/
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
    for (int i = A->height - 1; i >= 0; i--) {
        float sum = 0;
        for (int j = A->height - 1; j > i; j--) {
            sum += A->ptr[i * A->height + j] * X->ptr[j];
        }
        X->ptr[i] = (B->ptr[i] - sum) / A->ptr[i * A->height + i];
    }
    return X;
}

Mat* linear_equation_lower(Mat* A, Mat* X, Mat* B) {
    if (!A || !X || !B || !A->ptr || !X->ptr || !B->ptr) {
        return NULL;
    }
    for (int i = 0; i < A->height; i++) {
        float sum = 0;
        for (int j = 0; j < i; j++) {
            sum += A->ptr[i * A->height + j] * X->ptr[j];
        }
        X->ptr[i] = (B->ptr[i] - sum) / A->ptr[i * A->height + i];
    }
    return X;
}

unsigned char calculate_checksum(const char* ptr) {
    unsigned char sum = 0;
    while (*ptr != '$') {
        ptr++;
    }
    ptr++;
    while (*ptr != '*') {
        sum += *ptr++;
    }
    return sum;
}

void get_result(Message* message, int val, int l, int r, float time,
                float base_time) {
    int cnt = 0;
    for (int i = l; i < r; i++) {
        if (message[i].bp.flag == TR) {
            continue;
        }
        if (message[i].bp.target_id == val &&
            message[i].bp.mark_time >= base_time) {
            cnt++;
        }
    }
    printf("%d\n", cnt);
    Mat A;
    Mat B;
    Mat AT;
    Mat ATA;
    Mat ATB;
    Mat L;
    Mat LT;
    Mat Y, X;
    mat_init(&A, cnt - 1, 3);
    mat_init(&B, cnt - 1, 1);
    mat_init(&AT, 3, cnt - 1);
    /*mat_init(&AT, cnt - 1, 3);*/
    mat_init(&ATA, 3, 3);
    mat_init(&ATB, 3, 1);
    mat_init(&L, 3, 3);
    mat_init(&LT, 3, 3);
    mat_init(&Y, 3, 1);
    mat_init(&X, 3, 1);
    Mat LLT;
    mat_init(&LLT, 3, 3);
    int begin_index = -1;
    int row = 0;
    for (int i = l; i < r; i++) {
        /*printf("%d %d\n", message[i].bp.target_id, val);*/
        if (message[i].bp.flag == TR) {
            continue;
        }
        if (begin_index == -1 && message[i].bp.target_id == val &&
            message[i].bp.mark_time >= base_time) {
            begin_index = i;
            /*printf("range = %f\n", message[i].bp.range);*/
        } else if (message[i].bp.target_id == val &&
                   message[i].bp.mark_time >= base_time) {
            /*printf("range = %f\n", message[i].bp.range);                  */
            /*printf("%d\n", i);                                            */
            /*printf("%f %f\n", message[i].bp.x, message[begin_index].bp.x);*/
            /*printf("%f %f\n", message[i].bp.y, message[begin_index].bp.y);*/
            /*printf("%f %f\n", message[i].bp.z, message[begin_index].bp.z);*/
            A.ptr[row * 3 + 0] =
                    2 * (message[i].bp.x - message[begin_index].bp.x);
            A.ptr[row * 3 + 1] =
                    2 * (message[i].bp.y - message[begin_index].bp.y);
            A.ptr[row * 3 + 2] =
                    2 * (message[i].bp.z - message[begin_index].bp.z);
            B.ptr[row] = square(message[begin_index].bp.range) -
                         square(message[i].bp.range) +
                         Bp_square_pos(&message[i].bp) -
                         Bp_square_pos(&message[begin_index].bp);
            row++;
        }
    }
    mat_pri(&A);
    mat_pri(&B);
    transpose(&A, &AT);
    mat_mul(&AT, &A, &ATA);
    mat_mul(&AT, &B, &ATB);
    cholesky_decomposition(&ATA, &L);
    transpose(&L, &LT);
    linear_equation_lower(&L, &Y, &ATB);
    linear_equation_upper(&LT, &X, &Y);
    mat_pri(&AT);
    mat_pri(&ATA);
    mat_pri(mat_mul(&L, &LT, &LLT));
    mat_pri(&L);
    mat_pri(&LT);
    mat_pri(&X);
    char buf[STRING_LEN_MAX];
    sprintf(buf, "$TP,%.2lf,%d,%.3lf,%.3lf,%.3lf*", time, val, X.ptr[0], X.ptr[1],
            X.ptr[2]);
    printf("%s", buf);
    printf("%hhu\n", calculate_checksum(buf));
    mat_deinit(&X);
    mat_deinit(&Y);
    mat_deinit(&LT);
    mat_deinit(&L);
    mat_deinit(&ATB);
    mat_deinit(&ATA);
    mat_deinit(&AT);
    mat_deinit(&B);
    mat_deinit(&A);
}

void calculate_result(Message* message, int val, int l, int r, float base_time,
                      float time_pri) {
    /*printf("%d %d %f\n", l, r, base_time);*/
    float time = 0;
    int cnt = 0;
    for (int i = l; i < r; i++) {
        /*printf("%d %d\n", message[i].bp.flag, message[i].tp.target_Id);*/
        if (message[i].bp.flag == TR && message[i].tp.target_Id == val) {
            time = MAX(time, message[i].tp.time);
            for (int j = r - 1; j >= 0; j--) {
                if (message[j].bp.flag == BP &&
                    message[j].bp.Id == message[i].tp.Id) {
                    message[j].bp.range = message[i].tp.range;
                    message[j].bp.target_id = val;
                    time = MAX(time, message[j].bp.time);
                    message[j].bp.mark_time = time;
                    break;
                }
            }
            cnt++;
        }
        /*if (message[i].tp.is_valid && message[i].tp.target_Id == val) { */
        /*    if (base_time <= message[i].tp.time && */
        /*        message[i].tp.time < base_time + 1) { */
        /*        time = time > message[i].tp.time ? time : message[i].tp.time;
         */
        /*        message[message[i].tp.Id].bp.range = message[i].tp.range; */
        /*        message[message[i].tp.Id].bp.target_id = val; */
        /*        message[message[i].tp.Id].bp.time = MAX( */
        /*                message[message[i].tp.Id].bp.time,
         * message[i].tp.time);*/
        /*        cnt++; */
        /*    } */
        /*} */
    }
    /*printf("cnt = %d\n", cnt);*/
    if (cnt >= 4) {
        get_result(message, val, 0, r, time_pri, base_time);
    }
    return;
}

void targets_init(Targets* targets) {
    targets->ptr = malloc(TARGET_SIZE * sizeof(int));
    targets->len = TARGET_SIZE;
    targets->size = 0;
}

void targets_insert(Targets* targets, int val) {
    if (targets->size < targets->len) {
        targets->ptr[targets->size++] = val;
    } else {
        int* tmp_ptr = malloc((targets->len << 1) * sizeof(int));
        targets->len <<= 1;
        memcpy(tmp_ptr, targets->ptr, targets->size);
        free(targets->ptr);
        targets->ptr = tmp_ptr;
        targets_insert(targets, val);
    }
}

void targets_deinit(Targets* targets) {
    free(targets->ptr);
}

int check_input(const char* ptr) {
    unsigned char sum = 0;
    unsigned char checksum = 0;
    while (*ptr != '$') {
        ptr++;
    }
    ptr++;
    while (*ptr != '*') {
        sum += *ptr++;
    }
    checksum = atoi(++ptr);
    return checksum == sum;
}

void read_BP_line(const char* ptr, Message* message, int index) {
    Bp tmp_bp;
    sscanf(ptr, ",%lf,%d,%lf,%lf,%lf*", &tmp_bp.time, &tmp_bp.Id, &tmp_bp.x,
           &tmp_bp.y, &tmp_bp.z);
    message[index].bp = tmp_bp;
    message[index].bp.flag = BP;
    message[index].bp.target_id = -1;
    message[index].bp.mark_time = -1;
}

void read_TP_line(const char* ptr, Message* message, int index) {
    Tp tmp_tp;
    sscanf(ptr, ",%lf,%d,%d,%lf*", &tmp_tp.time, &tmp_tp.Id, &tmp_tp.target_Id,
           &tmp_tp.range);
    /*printf("%d\n", index);*/
    message[index].tp = tmp_tp;
    message[index].tp.flag = TR;
    message[index].tp.is_valid = VALID;
    /*message[tmp_tp.Id].bp.time = message[tmp_tp.Id].bp.time > tmp_tp.time?
     * message[tmp_tp.Id].bp.time: tmp_tp.time;*/
}

float get_time(Targets* targets, Message* message, int l, int r) {
    for (int i = r - 1; i >= l; i--) {
        if (message[i].bp.flag == BP) {
            return message[i].bp.time;
        }
        for (int j = 0; j < targets->size; j++) {
            if (message[i].tp.target_Id == targets->ptr[j]) {
                return message[i].tp.time;
            }
        }
    }
    return 0;
}

int read_line(int check, Message* message, int* index, int* read) {
    char input[STRING_LEN_MAX];
    char type[3];
    type[2] = 0;
    if (scanf("%s", input) == EOF) {
        return EOF;
    }
    (*read)++;
    /*printf("%s\n", input);*/
    type[0] = input[1];
    type[1] = input[2];
    if (!check_input(input)) {
        if (check) {
            fprintf(stderr, "Error: Checksum for message '%s' failed.\n",
                    input);
        }
        return 1;
    }
    if (!strcmp(type, "BP")) {
        read_BP_line(input + 3, message, *index);
    } else if (!strcmp(type, "TR")) {
        read_TP_line(input + 3, message, *index);
    }
    (*index)++;
    return 0;
}

int main(int argc, char* argv[]) {
    const char* optstring = "t::b::cadefghijklmnopqrsuvwxyz";
    int opt;
    int b = 4, t;
    int max_t = 0;
    int b_is_set = 0;
    int enable_checksum = 0;
    Targets targets;
    targets_init(&targets);
    Message* message;
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        /*printf("opt = %c\n", opt);      */
        /*printf("optarg = %s\n", optarg);*/
        /*printf("optind = %d\n", optind);*/

        switch (opt) {
            case 'b':
                if (b_is_set == 0) {
                    b_is_set = 1;
                } else {
                    fprintf(stderr, "Error: -b was repeated.\n");
                    return 1;
                }
                b = atoi(optarg);
                if (b < 3) {
                    fprintf(stderr,
                            "Error: less than 4 buoys is insufficient to "
                            "localise a target.\n");
                    return 3;
                }
                break;
            case 't':
                t = atoi(optarg);
                if (t <= b) {
                    fprintf(stderr, "Error: Target ID within buoy ID range.\n");
                    return 4;
                }
                max_t = t > max_t ? t : max_t;
                targets_insert(&targets, t);
                break;
            case 'c':
                enable_checksum = 1;
                break;
            default:
                fprintf(stderr, "Error: Unknown argument '-%c' supplied.\n",
                        opt);
                return 2;
        }
    }
    message = malloc(sizeof(Message) * 256);
    int index = 0;
    int read = 0;
    float base_time = 0;
    int len = 0;
    int ret;
    while ((ret = read_line(enable_checksum, message, &index, &read)) != EOF) {
        if (!ret) {
            len++;
        }
    }
    int l = 0, r = 0;
    for (int i = 0; i < len && read; i++) {
        /*printf("%d\n", message[i].bp.flag);*/
        if (message[i].bp.flag == TR) {
            /*printf("TR \n");*/
            if (message[i].tp.time >= base_time + 1) {
                for (int j = 0; j < targets.size; j++) {
                    /*printf("im in \n");*/
                    calculate_result(message, targets.ptr[j], l, r, base_time,
                                     get_time(&targets, message, l, r));
                }
                l = r;
                while (base_time + 1 <= message[i].tp.time) {
                    base_time++;
                }
            }
            r++;

        } else {
            /*printf("BP \n");*/
            if (message[i].bp.time >= base_time + 1) {
                for (int j = 0; j < targets.size; j++) {
                    /*printf("im in \n");*/
                    calculate_result(message, targets.ptr[j], l, r, base_time,
                                     get_time(&targets, message, l, r));
                }
                l = r;
                while (base_time + 1 <= message[i].tp.time) {
                    base_time++;
                }
            }
            r++;
        }
    }
    if (l != r) {
        for (int j = 0; j < targets.size; j++) {
            calculate_result(message, targets.ptr[j], l, r, base_time,
                             get_time(&targets, message, l, r));
        }
    }
    /*printf("%p\n", &(&((Message*)(1))->bp)->flag);*/
    /*printf("%p\n", &(&((Message*)(1))->tp)->flag);*/
    /*if (read && current_time < base_time + 1) {                            */
    /*        printf("im in \n");                                            */
    /*        calculate_result(message, targets.ptr[i], b, 100, base_time);  */
    /*    }                                                                  */
    /*}                                                                      */
    /*[>for (int i = 0; i < b; i++) {                                      <]*/
    /*    printf("%f, %f, %f, %f\n", message[i].bp.time, message[i].bp.x,*/
    /*           message[i].bp.y, message[i].bp.z);                      */
    /*}                                                                  */
    return 0;
}
