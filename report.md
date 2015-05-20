# Filter lock과 semaphore lock의 성능 비교

실험 보고서

시스템 프로그래밍  
2015 봄 학기  
2009-11744 심규민

## 목적

이 실험은
"Peterson's algorithm"을 확장한 **filter lock**과
binary **semaphore lock**을 사용하는
각각의 경우에 대해,
multiprocessing 환경에서 여러 threads가
mutually exclusive하게 shared variable을
증가시키는 작업을 수행하는 데에 걸리는 시간을 측정하여,
두 lock의 성능을 비교하는 것이다.

## 방법

먼저, C 언어로 n threads에 대한 filter lock을 구현했다.
수업 자료의 내용을 참고하였고 필요한 부분에 memory fence를 넣어주었다.

Semaphore lock은 POSIX semaphore의 초기값을 1로 설정하여 사용하였다.

n개의 threads가 하나의 integer variable을 공유하게 하고
이 변수를 각 thread에서 1000만 번씩 increment 하도록 하였다.
이 때 lock을 사용하여 해당 변수를 한 번에 하나의 thread만
참조할 수 있도록 하였다.
이렇게 하면 해당 변수는 최종적으로 n억 번 증가하는 것이 보장된다.

Thread는 POSIX thread를 사용하였다.

프로그램이 사용할 lock의 종류(type)와 생성할 thread의 개수(n)를
command line arguments로 받을 수 있게 하였다.
type은 filter, sem 중 하나이며, n은 2, 4, 8 중 하나이다.
이들의 조합인 총 여섯 가지 경우에 대해 실험하였다.

Processor가 8개인 PC에서 실험하였다. (자세한 스펙은 부록 참조)
`/usr/bin/time`을 사용하여 수행 시간(wall clock)을 측정하였다.

## 결과

각각의 경우에 대하여 5번 시행하였으며,
단위는 초(s)이고 95% 신뢰도이다. (자세한 결과는 부록 참조)

| n | filter         | semaphore      |
|---|----------------|----------------|
| 2 |   4.78(+-2.54) |   4.38(+-0.32) |
| 4 |  35.33(+-2.70) |  17.21(+-1.24) |
| 8 | 207.50(+-3.26) |  30.78(+-1.80) |

* Threads가 두 개일 때에는 filter lock과 semaphore lock의 유의미한 성능 차이가 보이지 않았다.
* Threads가 네 개일 때에는 filter lock이 semaphore lock 보다 약 105% 정도 느렸다.
* Threads가 여덟 개일 때에는 filter lock이 semaphore lock에 비해 약 574%나 느렸다.
* 전체적으로 보았을 때 threads 개수가 많아질수록 filter lock이 semaphore lock 보다 느려짐을 알 수 있다.

## 부록

### CPU 스펙

```bash
$ cat /proc/cpuinfo
(생략)
vendor_id       : GenuineIntel
cpu family      : 6
model           : 13
model name      : QEMU Virtual CPU version (cpu64-rhel6)
(생략)
cpu MHz         : 2199.998
(생략)
```

### 실험 결과

```bash
$ make test
(생략)
type=filter n=2
cnt = 20000000
0:03.05
type=filter n=4
cnt = 40000000
0:37.58
type=filter n=8
cnt = 80000000
3:23.27
type=sem n=2
cnt = 20000000
0:04.05
type=sem n=4
cnt = 40000000
0:19.32
type=sem n=8
cnt = 80000000
0:30.40
```

```bash
$ make test
(생략)
type=filter n=2
cnt = 20000000
0:05.03
type=filter n=4
cnt = 40000000
0:36.58
type=filter n=8
cnt = 80000000
3:32.09
type=sem n=2
cnt = 20000000
0:04.18
type=sem n=4
cnt = 40000000
0:17.92
type=sem n=8
cnt = 80000000
0:28.85
```

```bash
$ make test
(생략)
type=filter n=2
cnt = 20000000
0:03.04
type=filter n=4
cnt = 40000000
0:35.78
type=filter n=8
cnt = 80000000
3:30.42
type=sem n=2
cnt = 20000000
0:04.14
type=sem n=4
cnt = 40000000
0:15.83
type=sem n=8
cnt = 80000000
0:34.21
```

```bash
$ make test
(생략)
type=filter n=2
cnt = 20000000
0:09.72
type=filter n=4
cnt = 40000000
0:36.77
type=filter n=8
cnt = 80000000
3:24.75
type=sem n=2
cnt = 20000000
0:04.62
type=sem n=4
cnt = 40000000
0:16.25
type=sem n=8
cnt = 80000000
0:29.70
```

```bash
$ make test
(생략)
type=filter n=2
cnt = 20000000
0:03.05
type=filter n=4
cnt = 40000000
0:29.94
type=filter n=8
cnt = 80000000
3:26.97
type=sem n=2
cnt = 20000000
0:04.91
type=sem n=4
cnt = 40000000
0:16.73
type=sem n=8
cnt = 80000000
0:30.72
```

### 코드

#### Makefile

```make
CC = gcc
CFLAGS = -Wall -pthread

lock: main.c filter.c
    $(CC) $(CFLAGS) -o run main.c filter.c

test:
    for type in "filter" "sem" ; do \
        for n in 2 4 8 ; do \
            /usr/bin/time -f "%E" ./run $type $n ; \
        done ; \

clean:
    rm -f run *.o
```

#### filter.h

```c
#ifndef SGM_FILTER_H
#define SGM_FILTER_H

#define FILTER_MAX_N 16

typedef struct {
    int n;
    int level[FILTER_MAX_N];
    int victim[FILTER_MAX_N];
} filter_t;

void filter_init(filter_t *filter, unsigned int n);
void filter_lock(filter_t *filter, unsigned int i);
void filter_unlock(filter_t *filter, unsigned int i);

#endif
```

#### filter.c

```c
include <assert.h>
#include "filter.h"

void filter_init(filter_t *filter, unsigned int n) {
    unsigned int i;
    assert(n <= FILTER_MAX_N);
    filter->n = n;
    for(i = 0; i < n; i++)
        filter->level[i] = 0;
}

void filter_lock(filter_t *filter, unsigned int i) {
    unsigned int l, k;
    for(l = 1; l < filter->n; l++) {
        filter->level[i] = l;
        filter->victim[l] = i;
        // mfence between storing level[i] and loading level[k]
        __sync_synchronize();
        while(filter->victim[l] == i) {
            int exists = 0;
            for(k = 0; k < filter->n; k++) {
                if(k != i && filter->level[k] >= l) {
                    exists = 1;
                    break;
                }
            }
            if(!exists) break;
        }
    }
}

void filter_unlock(filter_t *filter, unsigned int i) {
    filter->level[i] = 0;
}
```

#### main.c

```c
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include "filter.h"

#define NULL ((void *)0)
#define MAX_N 16
#define MAX_CNT 100000000

static char type;
static unsigned int n;

static pthread_t threads[MAX_N];

static int cnt;
static sem_t sem;
static filter_t filter;

// ./run [type=sem|filter] [n<=16]
void parse_arguments(int argc, char *argv[]) {
    assert(argc >= 3);
    assert(argv[1][0] == 's' || argv[1][0] == 'f');
    type = argv[1][0];
    n = (unsigned int)(atoi(argv[2]));
    assert(n <= MAX_N);
}

void *count(void *param) {
    unsigned int i = (unsigned int)(unsigned long long)param, c;
    for(c = 0; c < MAX_CNT; c++) {
        if(type == 's') sem_wait(&sem);
        else filter_lock(&filter, i);
        cnt++;
        if(type == 's') sem_post(&sem);
        else filter_unlock(&filter, i);
    }
    return NULL;
}

void start_threads() {
    unsigned int i;
    if(type == 's') sem_init(&sem, 0, 1);
    else filter_init(&filter, n);
    for(i = 0; i < n; i++)
        pthread_create(&threads[i], NULL, count, (void *)(unsigned long long)i);
}

void join_threads() {
    unsigned int i;
    for(i = 0; i < n; i++)
        pthread_join(threads[i], NULL);
}

int main(int argc, char *argv[]) {
    parse_arguments(argc, argv);
    start_threads();
    join_threads();
    assert(cnt == MAX_CNT * n);
    printf("cnt = %d\n", cnt);
    return 0;
}
```

(EOF)
