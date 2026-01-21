#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

struct DATA {
	double** x;
	double* y;
}data,test_data;

void RE_PRED(double* pred, double* W, struct DATA* D, int N_F, int N_S);
double MSE(double* P, struct DATA* D, int N_F, int N_S);
double SIGMOID(double linear_pred);
double BCE(double* P, struct DATA* D, int N_F, int N_S);

int main() {
	char line[1024];
	int i = 0;
	int j = 0;
	int k = 0;
	int n_features = 0;
	int n_samples = 0;
	char answer;
	int have_header = 2;
	int c;
	char* tok;
	int r;
	char* num_error;
	int model;

	double sum;
	double* average;
	double* standard;

	double* weight;
	double lr = 0.01;
	double* pred;
	double mse;
	double bce;

	int self_test = 1;
	char test_line[1024];
	int n_test_samples = 0;

	FILE* data_file; 
	FILE* test_file;
	data_file = fopen("train.csv", "r");

	if (data_file == NULL) {
		printf("파일 탐색 실패");
		return 0;
	}

	test_file = fopen("test.csv", "r");
	if (test_file == NULL) {
		self_test = 1;
	}
	else {
		self_test = 0;
	}

	// 헤더 여부
	do {
		printf("첫 행이 헤더인가요? y, n\n");
		scanf("%c", &answer);
		if (answer == 'y' || answer == 'Y') {
			have_header = 1;
			break;
		}
		else if (answer == 'n' || answer == 'N') {
			have_header = 0;
			break;
		}
		else {
			printf("다시 입력해주세요.\n");
		}
		while ((c = getchar()) != '\n' && c != EOF); // 버퍼 비우기
	} while (have_header == 2);



	// n_feature, n_sample, 결측치 처리
	fgets(line, sizeof(line), data_file);
	for (int f = 0; line[f] != '\0'; f++) {
		if (line[f] == ',') {
			n_features++;
		}
	}
	//printf("%d", n_features);

	if (!have_header) {
		rewind(data_file);
	}
	while (fgets(line, sizeof(line), data_file) != NULL) {
		// printf("%s", line);
		r = -1;
		n_samples++;

		tok = strtok(line, ",");
		while (tok != NULL) {
			r++;
			tok = strtok(NULL, ",");
		}
		
		if (r != n_features) {
			printf("결측치가 존재합니다. 프로그램 종료");
			break;
		}
	}
	rewind(data_file);

	//printf("%d\n", n_samples);
	//printf("%d\n", n_features);



	// 특성값 동적 할당 (열 별 파싱)
	data.x = (double**)malloc(sizeof(double*) * n_features);

	while (i < n_features) {
		data.x[i] = (double*)malloc(sizeof(double) * n_samples);
		i++;
	}

	// 실제값 동적 할당
	data.y = (double*)malloc(sizeof(double) * n_samples);




	// 각 데이터를 구조체에 저장
	if (have_header) { fgets(line, sizeof(line), data_file); }
	while (fgets(line, sizeof(line), data_file) != NULL) {
		tok = strtok(line, ",");
		for (i = 0; i < n_features && tok != NULL; i++) {
			data.x[i][j] = strtod(tok, &num_error);
			tok = strtok(NULL, ",");
			//printf("data.x[%d][%d] : %f\n", i, j, data.x[i][j]);
		}
		tok[strcspn(tok, "\r\n")] = '\0';	// 행이 바뀔 때 개행문자 제거
		data.y[j] = strtod(tok, &num_error);
		//printf("data.y[%d] : %f\n", j, data.y[j]);
		j++;
	}
	rewind(data_file);
	printf("\n");


	// 데이터 값 출력
	if (have_header == 1) {
		printf("%s", fgets(line, sizeof(line), data_file));
	}
	else {
		for (i = 0; i < n_features; i++) {
			printf("요인%d  ", i + 1);
		}
		printf("실제값\n");
	}
	for (i = 0; i < n_samples; i++) {
		for (j = 0; j < n_features; j++) {
			printf("%f  ", data.x[j][i]);
		}
		printf("%f\n", data.y[i]);
	}
	rewind(data_file);


	printf("\n\n");


	// 모델 선택
	while (1) {
		printf("\n학습모델을 선택해주세요. (숫자 입력) \n1. 선형회귀\n2. 로지스틱 회귀\n");
		scanf("%d", &model);
		if (model == 1) {
			model--;
			break;
		}
		else if (model == 2) {
			model--;
			break;
		}
		else {
			printf("다시 입력해주세요.\n\n");
			while ((c = getchar()) != '\n' && c != EOF);
			continue;
		}
	}






	//데이터 정규화
	if (model == 0) { // 선형회귀
		average = (double*)malloc(sizeof(double) * (n_features + 1));
		standard = (double*)malloc(sizeof(double) * (n_features + 1));
	}
	else { // 로지스틱 회귀
		average = (double*)malloc(sizeof(double) * (n_features));
		standard = (double*)malloc(sizeof(double) * (n_features));
	}


	for (i = 0; i < n_features; i++) {
		sum = 0;
		for (j = 0; j < n_samples; j++) {
			sum += data.x[i][j];
		}

		average[i] = sum / n_samples; // 평균
	}
	for (i = 0; i < n_features; i++) {
		sum = 0;
		for (j = 0; j < n_samples; j++) {
			sum += (data.x[i][j] - average[i]) * (data.x[i][j] - average[i]);
		}

		standard[i] = sqrt( sum / (n_samples - 1)); // 표준편차
	}

	if (model == 0) {
		sum = 0;
		for (i = 0; i < n_samples; i++) {
			sum += data.y[i];
		}
		average[n_features] = sum / n_samples; // 평균

		sum = 0;
		for (i = 0; i < n_samples; i++) {
			sum += (data.y[i] - average[n_features]) * (data.y[i] - average[n_features]);
		}
		standard[n_features] = sqrt(sum / (n_samples - 1)); // 표준편차
	}

	
	// 표준화
	for (i = 0; i < n_features; i++) {
		for (j = 0; j < n_samples; j++) {
			data.x[i][j] = (data.x[i][j] - average[i]) / standard[i];
			//printf("data.x[%d][%d] : %f\n", i, j, data.x[i][j]);
		}
	}
	if (model == 0) {
		for (i = 0; i < n_samples; i++) {
			data.y[i] = (data.y[i] - average[n_features]) / standard[n_features];
			//printf("data.y[%d] : %f\n", i, data.y[i]);
		}
	}
	
	/*
	// 표준화된 데이터 값 출력
	if (have_header == 1) {
		printf("%s", fgets(line, sizeof(line), data_file));
	}
	else {
		for (i = 0; i < n_features; i++) {
			printf("요인%d  ", i + 1);
		}
		printf("실제값\n");
	}
	for (i = 0; i < n_samples; i++) {
		for (j = 0; j < n_features; j++) {
			printf("%f  ",data.x[j][i]);
		}
		printf("%f\n", data.y[i]);
	}
	printf("\n");
	rewind(data_file);
	*/



	// 가중치와 예측값 할당
	weight = (double*)malloc(sizeof(double)*(n_features+1));
	pred = (double*)malloc(sizeof(double) * n_samples); // 각 데이터에 대한 예측값

	srand(time(NULL)); // 랜덤 시드 초기화

	for (i = 0; i < n_features + 1; i++) {
		weight[i] = ((double)rand() / RAND_MAX * 2.0 - 1.0) * 0.01; // 가중치 초기값 설정. (-0.01 ~ 0.01)
	}
	/*
	// f(x) = w0 + w1 * x1 + w2 * x2 ..
	for (i = 0; i < n_samples; i++) {
		sum = weight[0];
		for (j = 0; j < n_features; j++) {
			sum += weight[j + 1] * data.x[j][i];
		}

		pred[i] = sum;
		printf("%f\n", pred[i]);
	}
	*/
	RE_PRED(pred, weight, &data, n_features, n_samples); // pred 배열에 예측값 할당

	

	// 경사하강법 업데이트

	if (model == 0) { // 선형 회귀
		k = 0;
		do {
			mse = MSE(pred, &data, n_features, n_samples);
			printf("\n%d번째 업데이트\n", k);
			for (i = 0; i < n_features + 1; i++) {
				sum = 0;
				if (i == 0) {
					for (j = 0; j < n_samples; j++) {
						sum += data.y[j] - pred[j];
					}
				}
				else {
					for (j = 0; j < n_samples; j++) {
						sum += (data.y[j] - pred[j]) * data.x[i - 1][j];
					}
				}
				weight[i] += lr * (2 / (double)n_samples) * sum;
				printf("w'%d = %f  ", i, weight[i]);
			}
			RE_PRED(pred, weight, &data, n_features, n_samples);
			printf("\nMSE(전) = %f", mse);
			printf("\nMSE(후) = %f\n", MSE(pred, &data, n_features, n_samples));
			k++;
		} while (mse != MSE(pred, &data, n_features, n_samples) && k < 100000);



		// 표준화 되돌리기 (선형)
		sum = 0;
		for (i = 1; i < n_features + 1; i++) {
			weight[i] = weight[i] * standard[n_features] / standard[i - 1];
			sum += weight[i] * average[i - 1];
		}

		weight[0] = average[n_features] - sum;

		printf("\n\n\n\n[업데이트된 가중치]\n");
		for (i = 0; i < n_features + 1; i++) {
			printf("w%d = %f\n", i, weight[i]);
		}
		printf("\n\n\n\n[추정된 회귀모형]\ny = ");
		for (i = 0; i < n_features + 1; i++) {
			if (i != 0) {
				if (weight[i] > 0) {
					printf(" + %f*x%d", weight[i], i);
				}
				else if (weight[i] < 0) {
					printf(" - %f*x%d", -weight[i], i);
				}
			}
			else {
				printf("%f", weight[i]);
			}
			
		}
	}
	else { // 로지스틱 회귀
		k = 0;
		do {
			bce = BCE(pred, &data, n_features, n_samples);
			printf("\n%d번째 업데이트\n", k);
			for (i = 0; i < n_features + 1; i++) {
				sum = 0;
				if (i == 0) {
					for (j = 0; j < n_samples; j++) {
						sum += data.y[j] - SIGMOID(pred[j]);
					}
				}
				else {
					for (j = 0; j < n_samples; j++) {
						sum += (data.y[j] - SIGMOID(pred[j])) * data.x[i - 1][j];
					}
				}
				weight[i] += (lr/n_samples) * sum;
				printf("w'%d = %f  ", i, weight[i]);
			}
			RE_PRED(pred, weight, &data, n_features, n_samples);
			printf("\nBCE(전) = %f", bce);
			printf("\nBCE(후) = %f\n", BCE(pred, &data, n_features, n_samples));
			k++;
		} while (fabs (bce - BCE(pred, &data, n_features, n_samples)) > 1e-6 && k < 100000);


		// 표준화 되돌리기 (로지스틱)
		sum = 0;
		for (i = 1; i < n_features + 1; i++) {
			weight[i] = weight[i] / standard[i - 1];
			sum += weight[i] * average[i - 1];
		}

		weight[0] = weight[0] - sum;


		printf("\n\n\n\n[업데이트된 가중치]\n");
		for (i = 0; i < n_features + 1; i++) {
			printf("w%d = %f\n", i, weight[i]);
		}
		printf("\n\n\n\n[선형 판별 함수]\nz = ");
		for (i = 0; i < n_features + 1; i++) {
			if (i != 0) {
				if (weight[i] > 0) {
					printf(" + %f*x%d", weight[i], i);
				}
				else if (weight[i] < 0) {
					printf(" - %f*x%d", -weight[i], i);
				}
			}
			else {
				printf("%f", weight[i]);
			}
		}

	}

	printf("\n\n\n학습이 종료되었습니다.\n\n\n\n");



	// 테스트 해보기
	if (self_test == 0) { // test.csv 파일 존재
		while (fgets(test_line, sizeof(test_line), test_file) != NULL) {
			//printf("%s", test_line);
			r = 0;
			n_test_samples++;

			tok = strtok(test_line, ",");
			while (tok != NULL) {
				r++;
				tok = strtok(NULL, ",");
			}

			if (r != n_features) {
				printf("결측치가 존재합니다.\n");
				self_test = 1;
				goto end_if;
			}
		}
		rewind(test_file);


		// 동적 할당
		test_data.x = (double**)malloc(sizeof(double*) * n_features);
		i = 0;
		while (i < n_features) {
			test_data.x[i] = (double*)malloc(sizeof(double) * n_test_samples);
			i++;
		}

		test_data.y = (double*)malloc(sizeof(double) * n_test_samples);


		// 값 할당
		j = 0;
		while (fgets(test_line, sizeof(test_line), test_file) != NULL) {
			tok = strtok(test_line, ",");
			for (i = 0; i < n_features && tok != NULL; i++) {
				test_data.x[i][j] = strtod(tok, &num_error);
				tok = strtok(NULL, ",");
				//printf("test_data.x[%d][%d] : %f\n", i, j, test_data.x[i][j]);
			}
			j++;
		}

		// 예측값 출력

		if (model == 0) { // 선형 회귀
			printf("\n[선형 회귀 예측값]\n");
			if (have_header == 1) {
				printf("%s", fgets(line, sizeof(line), data_file));
			}
			else {
				for (i = 0; i < n_features; i++) {
					printf("요인%d,", i + 1);
				}
				printf("실제값\n");
			}
			for (i = 0; i < n_test_samples; i++) {
				sum = weight[0];
				for (j = 0; j < n_features; j++) {
					sum += weight[j + 1]*test_data.x[j][i];
					printf("%f  ", test_data.x[j][i]);
				}
				test_data.y[i] = sum;
				printf("=>  %f\n", test_data.y[i]);
			}
		}
		else { // 로지스틱 회귀
			printf("\n[로지스틱 회귀 예측값]\n");
			if (have_header == 1) {
				printf("%s", fgets(line, sizeof(line), data_file));
			}
			else {
				for (i = 0; i < n_features; i++) {
					printf("요인%d,", i + 1);
				}
				printf("실제값\n");
			}
			for (i = 0; i < n_test_samples; i++) {
				k = -1;
				sum = weight[0];
				for (j = 0; j < n_features; j++) {
					sum += weight[j + 1] * test_data.x[j][i];
					printf("%f  ", test_data.x[j][i]);
				}
				test_data.y[i] = SIGMOID(sum);
				if (test_data.y[i] < 0.5) {
					k = 0;
				}
				else {
					k = 1;
				}
				printf("=>  %d (%f)\n", k, test_data.y[i]);
			}
		}

	}
	
	rewind(data_file);
	end_if:
	while (1) {
		printf("\n\n테스트할 데이터를 직접 입력해주세요. (컴마로 구분, end 입력시 종료)\n");
		scanf("%s", test_line);

		
		if (strcmp(test_line, "end") == 0) {
			break;
		}
		
		// 예측값 계산
		sum = weight[0];
		tok = strtok(test_line, ",");
		for (i = 0; i < n_features && tok != NULL; i++) {
			sum += weight[i + 1] * strtod(tok, &num_error);
			tok = strtok(NULL, ",");
		}

		if (model == 0) { // 선형 회귀
			printf("\n[선형 회귀 예측값]\n");
			printf(" =>  %f  ", sum);
		}
		else { // 로지스틱 회귀
			k = -1;

			printf("\n[로지스틱 회귀 예측값]\n");
			if (SIGMOID(sum) < 0.5) {
				k = 0;
			}
			else {
				k = 1;
			}
			printf("=>  %d (%f)\n", k, SIGMOID(sum));

		}

	}












	// 동적 메모리 제거
	i = 0;
	while (i < n_features) {
		free(data.x[i]);
		i++;
	}
	free(data.x);
	free(data.y);
	
	free(average);
	free(standard);
	return 0;
}
// 예측값 계산
void RE_PRED(double* pred, double* W, struct DATA* D, int N_F, int N_S) {
	int i;
	int j;
	double sum;

	for (i = 0; i < N_S; i++) {
		sum = W[0];
		
		for (j = 0; j < N_F; j++) {
			sum += W[j + 1] * D->x[j][i];
		}

		pred[i] = sum;
		//printf("예측값 : %f  ", pred[i]);
	}
}

// 선형 회귀 손실함수 MSE
double MSE(double* P, struct DATA* D, int N_F, int N_S) {
	double sum = 0;
	int i;
	int j;
	
	for (i = 0; i < N_S; i++) {
		sum += (D->y[i] - P[i]) * (D->y[i] - P[i]);
	}
	
	return sum/N_S;
}

// 시그모이드 함수
double SIGMOID(double linear_pred) {
	return 1 / (1 + exp(-linear_pred));
}

// 로지스틱 회귀 손실함수 BCE
double BCE(double* P, struct DATA* D, int N_F, int N_S) {
	double sum = 0;
	int i;
	int j;

	for (i = 0; i < N_S; i++) {
		sum += D->y[i] * log(SIGMOID(P[i])) + (1 - D->y[i]) * log( 1 - SIGMOID(P[i]));
	}

	return -sum / N_S;
}
