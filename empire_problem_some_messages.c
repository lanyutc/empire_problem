#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

enum city_limit {
	min_city_num    = 1,
	max_city_num    = 100,
	capitol         = 1,
	city_start_pos  = 2,
};

struct empire_info_t {
	int city_num;

	int *city_transfer_cost; /* The time required memory, 
                              * format:2-1 3-1 3-2 4-1...... */
	int *city_route;         /* City connection info, *	e.g. 3-1 
                              * city_route[CITY_POS(3)*2] = 3 city_route[CITY_POS(3)*2+1] = 1 */ 
};

/* global vars */
struct empire_info_t empire;

#define PAGE_SIZE			4096
#define NO_ROUTE			(-1)
#define PRINT_DEBUG_INFO 
		

/* calc max way by city num, e.g. 1->0, 2->1, 3->3......*/
#define TOTAL_CITY_WAYS(x)		((x)*(x-1)/2)
/* calc num of city #x's connection must be entered */
#define SINGLE_CITY_WAYS(x)		(x-1)
/* calc city #x's pos(x-1, x-2 should be CITY_POS(x)+1) in city_transfer_cost */
#define CITY_POS(x)				((x-1)*(x-2)/2)

static int str_split(const char *ifs, char *line, int *field)
{
	static const char default_ifs[256] = {[' '] = 1, ['\t'] = 1, ['\n'] = 1};

	if (ifs == 0) {
		ifs = default_ifs;
	}

	int i = 0;
	while (1) {
		while (ifs[(unsigned char)*line]) {
			line++;
		}
		if (*line == '\0') {
			break;
		}

		if (*line == 'x') {
			field[i++] = NO_ROUTE;
		} else {
			field[i++] = atoi(line);
		}

		while (*line && !ifs[(unsigned char)*line]) {
			line++;
		}

		if (!*line) {
			break;
		}
	}

	return i;
}

static void fill_city_connection_info(int cur_city, int pos) 
{
	int i = capitol, j = 0;
	for ( ; i < cur_city; i++, j+=2) {
		empire.city_route[pos+j] = cur_city;
		empire.city_route[pos+j+1] = i;
	}
}

/* calc from-to's pos in city_transfer_cost */
static int get_xy_pos(int from, int to)
{
	if (from == to) {
		return NO_ROUTE;
	}

	if (from < to) {
		return get_xy_pos(to, from);
	}

	if (from > empire.city_num || to > empire.city_num || from == 0 || to == 0) {
		return NO_ROUTE;
	}

	return (CITY_POS(from) + to - 1);
}

/* calc time cost form city #x to city #y */
static int get_xy_time(int from, int to)
{
	if (from == to) {
		return 0;
	}

	if (from < to) {
		return get_xy_time(to, from);
	}

	if (from > empire.city_num || to > empire.city_num || from == 0 || to == 0) {
		return NO_ROUTE;
	}

	return empire.city_transfer_cost[get_xy_pos(from, to)];
}

static void start_init(int start_city, int *start_cost)
{
	int i = capitol;
	for ( ;i <= empire.city_num; i++) {
		start_cost[i] = get_xy_time(start_city, i);
	}
}


static void read_city_num()
{
	bool read_city_num_succ = false;
	char input_buffer[PAGE_SIZE] = {0};
	while (!read_city_num_succ) {
		fgets(input_buffer, PAGE_SIZE, stdin);
		empire.city_num = atoi(input_buffer);
		if (empire.city_num >= min_city_num && empire.city_num <= max_city_num) {
			read_city_num_succ = true;
		} else {
			printf("City number is error, please re-enter(1-100):\n");
		}
	}
}

static void malloc_need_memory()
{
	empire.city_transfer_cost = (int *)calloc(TOTAL_CITY_WAYS(empire.city_num), sizeof(int));
	if (empire.city_transfer_cost == NULL) {
		printf("malloc city_transfer_cost failed\n");
		exit(EXIT_FAILURE);
	}
	empire.city_route = (int *)calloc(TOTAL_CITY_WAYS(empire.city_num), sizeof(int[2]));
	if (empire.city_route == NULL) {
		printf("malloc city_route failed\n");
		exit(EXIT_FAILURE);
	}
}

static void free_need_memory()
{
	if (empire.city_transfer_cost) {
		free(empire.city_transfer_cost);
		empire.city_transfer_cost = NULL;
	}
	if (empire.city_route) {
		free(empire.city_route);
		empire.city_route = NULL;
	}
}

static void read_city_infos()
{
	char input_buffer[PAGE_SIZE] = {0};
	int city_index = city_start_pos;
	while (city_index <= empire.city_num) {
		int read_cnt = 0;
		fgets(input_buffer, PAGE_SIZE, stdin);
		read_cnt = str_split(0, input_buffer, &empire.city_transfer_cost[CITY_POS(city_index)]);
		if (read_cnt != SINGLE_CITY_WAYS(city_index)) {
			printf("City %d must has %d ways, please re-enter:\n", 
						city_index, SINGLE_CITY_WAYS(city_index));
			continue;
		}
		fill_city_connection_info(city_index, CITY_POS(city_index)*2);
		city_index ++;
	}
}

static void print_city_infos()
{
	int i = city_start_pos;
	while (i <= empire.city_num) {
		int  j = CITY_POS(i);
		for ( ; j < TOTAL_CITY_WAYS(i); j++) { 
			printf("%d->%d cost:%d ", empire.city_route[2*j],
					empire.city_route[2*j+1],empire.city_transfer_cost[j]);
		}
		printf("\n");
		i++;
	}
}

static void print_result(int start_city, int *start_cost)
{
	int least_spread_cost = 0;
	int i = capitol;
	bool no_route_city = false;
	for ( ; i <= empire.city_num; i++) {
		if (start_city == i) {
			continue;
		}

		if (start_cost[i] <= 0) {
			printf("least cost(%d->%d)=-1, there is no route from %d to %d\n", 
					start_city, i, start_city, i);
			no_route_city = true;
		} else {
			least_spread_cost = start_cost[i] > least_spread_cost ? start_cost[i]:least_spread_cost;
			printf("least cost(%d->%d)=%d\n", start_city, i, start_cost[i]);
		}
	}

	printf("%sresult:%d\n", no_route_city ? "inaccurate ":"", least_spread_cost);
}

void calc_least_cost(int start_city)
{
	int city_index = capitol, optimal_city = 0;
	int tmp_cost = 0;
	int start_cost[max_city_num+1] = {0};
	bool mark[max_city_num+1]      = {false};

	start_init(start_city, start_cost);
	mark[start_city] = true;

	/* Main loop, find least cost from current city to next */
	while (city_index <= empire.city_num) {
		int cur_cost = INT_MAX;
		int i = capitol;
		/* Find the next optimal city */
		for ( ; i <= empire.city_num; i++) {
			if (!mark[i] && start_cost[i] < cur_cost && start_cost[i] > 0) {
				cur_cost = start_cost[i]; /* update cost from start city to current one*/
				optimal_city = i;
			}
		}
		mark[optimal_city] = true;

		/* Loop linked city, 
		 * compare cost(start_city,optimal_city) with cost(optimal_city,linked_city)*/
		for (i = capitol; i <= empire.city_num; i++) {
			tmp_cost = get_xy_time(optimal_city, i);
			if (tmp_cost == NO_ROUTE) {
				continue;
			}
			/* If linked city is not yet marked, record the optimal path */
			if (!mark[i]) {
				tmp_cost += cur_cost;
				start_cost[i] = start_cost[i] <= 0 ? tmp_cost :
					(start_cost[i] < tmp_cost ? start_cost[i] : tmp_cost);
			}
		}

		city_index ++;
	}

	print_result(start_city, start_cost);
}

int main(int argc, char **argv)
{
	read_city_num();
	malloc_need_memory();
	read_city_infos();
/*	check_city_reachable();*/
#ifdef PRINT_DEBUG_INFO 
	print_city_infos();
#endif
	calc_least_cost(capitol);
	free_need_memory();
	return 0;
}
