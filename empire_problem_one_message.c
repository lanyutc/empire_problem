#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


enum city_limit {
	min_city_num   = 1,
	max_city_num   = 100,
	capitol		   = 1,
	city_start_pos = 2,
};

struct empire_info_t {
	int city_num;

	int *city_transfer_cost; /* The time required memory,
                              * format:2-1 3-1 3-2 4-1...... */
	int *city_route;         /* City connection info, e.g. 3-1 
                              * city_route[CITY_POS(3)*2] = 3 city_route[CITY_POS(3)*2+1] = 1 */ 
	int *best_way;           /* Record the city message has been passed */
	int *cur_city_min_cost;  /* Record minimum transfer cost from current city to other city */
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
/* the total passed city when message reaches the end, maybe actual value 2*x-2 */ 
#define MAX_WAY(x)				(2*x)

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

static int calc_next_city(int from, const int *city_route)
{
	return (city_route[0] == from) ? city_route[1] : city_route[0];
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

static int find_x_min_route(int from, bool *mark)
{
	if (from > empire.city_num) {
		return 0;
	}

	int i = capitol, m = 0, min_cost = 0;
	for ( ; i <= empire.city_num; i++) {
		int time = get_xy_time(from, i);
		int pos  = get_xy_pos(from, i);

		if (mark[i] == true) {
			continue;
		}

		if (time > 0 && m == 0) {
			empire.cur_city_min_cost[m++] = pos;
			min_cost = time;
			continue;
		}

		if (time > 0 && time == min_cost) {
			empire.cur_city_min_cost[m++] = pos;
		}

		if ((time > 0 && time < min_cost)) {
			empire.cur_city_min_cost[0] = pos;
			min_cost = time;
			m = 1;
		}
	}

	return m;
}

static int add_best_way(int cur_city)
{
	int i = 0;
	for ( ; i < MAX_WAY(empire.city_num); i++) {
		if (empire.best_way[i] > 0) {
			continue;
		}
		empire.best_way[i] = cur_city;
		return i;
	}
	return 0;
}

static bool check_spread_over(bool *mark)
{
	int i = capitol;
	for ( ; i <= empire.city_num; i++) {
		if (!mark[i]) {
			return false;
		}
	}
	return true;
}

void find_least_cost_path(int start_city)
{
	int cur_city = start_city;
	int total_cost = 0;
	int way_num = 0;
	bool mark[max_city_num] = {false};
	bool need_backtrack = false;
	mark[capitol] = true;
	empire.best_way[0] = capitol;

	printf("start from capitol #1\n");
	while (1) {
		int min_c = find_x_min_route(cur_city, mark); 
		/* Get the  minimum transfer cost of cur city
		 * if min_c <= 0, this city only connect with capitol */
		if (min_c > 0) {
			int cur_pos  = empire.cur_city_min_cost[0];
			cur_city = calc_next_city(cur_city, &empire.city_route[cur_pos*2]);

			/* check cost , maybe need backtrack */
			int i = way_num;
			int backtrack_cost = 0;
			int to_cur_cost = 0;
			while (i > 0) {
				to_cur_cost = get_xy_time(empire.best_way[i - 1], cur_city);
				backtrack_cost   += get_xy_time(empire.best_way[i], empire.best_way[i - 1]);
				/* we should backtrack when the cost from last city to next city less than current city cost */
				if (to_cur_cost > 0 && to_cur_cost + backtrack_cost < empire.city_transfer_cost[cur_pos]) {
					int backtrack_cnt = way_num - i; 
					int backtrack_pos = 1;
					/* Record the best way after backtrack 
					 * backtrack_cnt is the total backtrack city count for current city
					 * backtrack_pos is the begin pos of best way */
					while (backtrack_cnt >= 0) {
						add_best_way(empire.best_way[way_num - backtrack_pos]);
						printf("next pos city #%d\n", empire.best_way[way_num - backtrack_pos]);
						backtrack_cnt--;
						backtrack_pos++;
					}
					total_cost += to_cur_cost + backtrack_cost;
					way_num = add_best_way(cur_city);
					printf("next pos city #%d\n", cur_city);
					mark[cur_city] = true;
					need_backtrack = true;
					break;
				}
				i--;
			}

			if (!need_backtrack) {
				way_num = add_best_way(cur_city);
				mark[cur_city] = true;
				total_cost += empire.city_transfer_cost[cur_pos];
				printf("next pos city #%d\n", cur_city);
			}
		} else {
			/* Go back to capitol when there is no route*/
			cur_city = capitol;
			way_num = add_best_way(cur_city);
			printf("next pos city #%d\n", cur_city);
		}

		if (check_spread_over(mark)) {
			break;
		}
	}

	printf("result %d\n", total_cost);
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
	empire.cur_city_min_cost = (int *)calloc(empire.city_num, sizeof(int));
	if (empire.cur_city_min_cost == NULL) {
		printf("NULL\n");
		exit(EXIT_FAILURE);
	}
	empire.best_way = (int *)calloc(MAX_WAY(empire.city_num), sizeof(int));
	if (empire.best_way == NULL) {
		printf("malloc best_way failed\n");
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
	if (empire.cur_city_min_cost) {
		free(empire.cur_city_min_cost);
		empire.cur_city_min_cost = NULL;
	}
	if (empire.best_way) {
		free(empire.best_way);
		empire.best_way = NULL;
	}
}

static void read_city_infos()
{
	char input_buffer[PAGE_SIZE] = {0};
	int city_index = city_start_pos;
	while (city_index <= empire.city_num) {
		fgets(input_buffer, PAGE_SIZE, stdin);
		int read_cnt = str_split(0, input_buffer, &empire.city_transfer_cost[CITY_POS(city_index)]);
		if (read_cnt != SINGLE_CITY_WAYS(city_index)) {
			printf("City %d must has %d ways, please re-enter:\n", 
						city_index, SINGLE_CITY_WAYS(city_index));
			continue;
		}
		if (empire.city_transfer_cost[CITY_POS(city_index)] == NO_ROUTE) {
			printf("City %d is no way to capitol, please re-enter:\n", city_index);
			continue;
		}
		
		fill_city_connection_info(city_index, CITY_POS(city_index)*2);
		city_index ++;
	}
}

static void printf_city_infos()
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

int main(int argc, char **argv)
{
	read_city_num();
	malloc_need_memory();
	read_city_infos();
#ifdef PRINT_DEBUG_INFO 
	printf_city_infos();
#endif
	find_least_cost_path(capitol);
	free_need_memory();
	return 0;
}
